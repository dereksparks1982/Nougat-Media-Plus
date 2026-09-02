#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <xcb/xcb.h>

typedef void GtkWidget;
typedef unsigned long GdkXID;
typedef int gboolean;
typedef unsigned int guint32;
typedef double gdouble;

typedef struct VkInstance_T* VkInstance;
typedef int32_t VkResult;
typedef uint64_t VkSurfaceKHR;
typedef uint32_t VkXcbSurfaceCreateFlagsKHR;
typedef int32_t VkStructureType;
typedef void VkAllocationCallbacks;
typedef void (*PFN_vkVoidFunction)(void);
struct VkXcbSurfaceCreateInfoKHR_Nougat;
typedef PFN_vkVoidFunction (*PFN_vkGetInstanceProcAddr)(VkInstance, const char*);
typedef VkResult (*PFN_vkCreateXcbSurfaceKHR)(
    VkInstance,
    const struct VkXcbSurfaceCreateInfoKHR_Nougat*,
    const VkAllocationCallbacks*,
    VkSurfaceKHR*);

struct VkXcbSurfaceCreateInfoKHR_Nougat {
  VkStructureType sType;
  const void* pNext;
  VkXcbSurfaceCreateFlagsKHR flags;
  xcb_connection_t* connection;
  xcb_window_t window;
};

static void* (*real_dlsym_fn(void))(void*, const char*) {
  static void* (*fn)(void*, const char*) = NULL;
  if (!fn) {
    fn = (void* (*)(void*, const char*))dlvsym(RTLD_NEXT, "dlsym", "GLIBC_2.2.5");
  }
  return fn;
}

static void* next_symbol(const char* name) {
  void* (*fn)(void*, const char*) = real_dlsym_fn();
  return fn ? fn(RTLD_NEXT, name) : NULL;
}

static unsigned long target_xid(void) {
  static int done = 0;
  static unsigned long value = 0;
  if (!done) {
    done = 1;
    const char* s = getenv("NOUGAT_XENIA_DIRECT_XID");
    if (s && *s) {
      errno = 0;
      char* end = NULL;
      unsigned long parsed = strtoul(s, &end, 0);
      if (!errno && end != s && *end == '\0' && parsed != 0) value = parsed;
    }
  }
  return value;
}


static int address_is_xenia(void* caller) {
  Dl_info info;
  memset(&info, 0, sizeof(info));
  if (!caller || !dladdr(caller, &info) || !info.dli_fname) return 0;
  return strstr(info.dli_fname, "xenia") != NULL ||
         strstr(info.dli_fname, "Xenia") != NULL;
}

static int debug_enabled(void) {
  const char* v = getenv("NOUGAT_XENIA_BRIDGE_DEBUG");
  return v && *v && strcmp(v, "0") != 0;
}

static PFN_vkGetInstanceProcAddr real_vk_get_instance_proc_addr = NULL;
static PFN_vkCreateXcbSurfaceKHR real_vk_create_xcb_surface = NULL;

static VkResult nougat_vkCreateXcbSurfaceKHR(
    VkInstance instance,
    const struct VkXcbSurfaceCreateInfoKHR_Nougat* create_info,
    const VkAllocationCallbacks* allocator,
    VkSurfaceKHR* surface) {
  if (!real_vk_create_xcb_surface && real_vk_get_instance_proc_addr) {
    real_vk_create_xcb_surface = (PFN_vkCreateXcbSurfaceKHR)
        real_vk_get_instance_proc_addr(instance, "vkCreateXcbSurfaceKHR");
  }
  if (!real_vk_create_xcb_surface || !create_info) return (VkResult)-3;

  struct VkXcbSurfaceCreateInfoKHR_Nougat redirected = *create_info;
  const unsigned long xid = target_xid();
  if (xid) redirected.window = (xcb_window_t)xid;

  if (debug_enabled()) {
    fprintf(stderr,
            "Nougat Xenia bridge: vkCreateXcbSurfaceKHR window 0x%x -> 0x%x\n",
            (unsigned int)create_info->window,
            (unsigned int)redirected.window);
  }
  return real_vk_create_xcb_surface(instance, &redirected, allocator, surface);
}

static PFN_vkVoidFunction nougat_vkGetInstanceProcAddr(VkInstance instance,
                                                        const char* name) {
  if (!real_vk_get_instance_proc_addr) return NULL;
  if (name && strcmp(name, "vkCreateXcbSurfaceKHR") == 0 && target_xid()) {
    PFN_vkVoidFunction real =
        real_vk_get_instance_proc_addr(instance, "vkCreateXcbSurfaceKHR");
    real_vk_create_xcb_surface = (PFN_vkCreateXcbSurfaceKHR)real;
    if (debug_enabled()) {
      fprintf(stderr,
              "Nougat Xenia bridge: intercepting Vulkan XCB surface creation\n");
    }
    return (PFN_vkVoidFunction)nougat_vkCreateXcbSurfaceKHR;
  }
  return real_vk_get_instance_proc_addr(instance, name);
}

/*
 * Edge explicitly dlopens libvulkan.so.1 and then dlsym()s
 * vkGetInstanceProcAddr from that handle. Interposing only the Vulkan symbol
 * itself is therefore insufficient. Interpose dlsym so the function-pointer
 * path Edge really uses passes through the Nougat surface redirect.
 */
void* dlsym(void* handle, const char* name) {
  void* (*real_fn)(void*, const char*) = real_dlsym_fn();
  if (!real_fn) return NULL;
  void* real = real_fn(handle, name);
  if (!target_xid()) return real;

  if (strcmp(name, "vkGetInstanceProcAddr") == 0 && real) {
    real_vk_get_instance_proc_addr = (PFN_vkGetInstanceProcAddr)real;
    if (debug_enabled()) {
      fprintf(stderr,
              "Nougat Xenia bridge: captured Edge vkGetInstanceProcAddr\n");
    }
    return (void*)nougat_vkGetInstanceProcAddr;
  }
  if (strcmp(name, "vkCreateXcbSurfaceKHR") == 0 && real) {
    real_vk_create_xcb_surface = (PFN_vkCreateXcbSurfaceKHR)real;
    return (void*)nougat_vkCreateXcbSurfaceKHR;
  }
  return real;
}

/* Keep the earlier GDK redirect as a secondary path. The Vulkan interception
 * above is now authoritative for actual presentation. */
GdkXID gdk_x11_window_get_xid(void* gdk_window) {
  typedef GdkXID (*Fn)(void*);
  static Fn real_fn = NULL;
  if (!real_fn) real_fn = (Fn)next_symbol("gdk_x11_window_get_xid");
  const unsigned long xid = target_xid();
  if (xid && address_is_xenia(__builtin_return_address(0))) return (GdkXID)xid;
  return real_fn ? real_fn(gdk_window) : 0;
}

static int widget_is_toplevel(GtkWidget* widget) {
  typedef gboolean (*Fn)(GtkWidget*);
  static Fn fn = NULL;
  if (!fn) fn = (Fn)next_symbol("gtk_widget_is_toplevel");
  return fn && widget && fn(widget) != 0;
}

static void make_toplevel_hidden(GtkWidget* widget) {
  if (!target_xid() || !widget || !widget_is_toplevel(widget)) return;

  typedef void (*BoolFn)(void*, gboolean);
  typedef void (*MoveFn)(void*, int, int);
  static BoolFn skip_taskbar = NULL;
  static BoolFn skip_pager = NULL;
  static MoveFn move_window = NULL;

  if (!skip_taskbar)
    skip_taskbar = (BoolFn)next_symbol("gtk_window_set_skip_taskbar_hint");
  if (!skip_pager)
    skip_pager = (BoolFn)next_symbol("gtk_window_set_skip_pager_hint");
  if (!move_window)
    move_window = (MoveFn)next_symbol("gtk_window_move");

  if (skip_taskbar) skip_taskbar(widget, 1);
  if (skip_pager) skip_pager(widget, 1);
  if (move_window) move_window(widget, -30000, -30000);

  typedef void* (*GetWindowFn)(GtkWidget*);
  typedef void* (*GetDisplayFn)(void*);
  typedef Display* (*GetXDisplayFn)(void*);
  typedef GdkXID (*RealXidFn)(void*);
  static GetWindowFn get_window = NULL;
  static GetDisplayFn get_display = NULL;
  static GetXDisplayFn get_xdisplay = NULL;
  static RealXidFn real_xid = NULL;
  if (!get_window) get_window = (GetWindowFn)next_symbol("gtk_widget_get_window");
  if (!get_display) get_display = (GetDisplayFn)next_symbol("gdk_window_get_display");
  if (!get_xdisplay) get_xdisplay = (GetXDisplayFn)next_symbol("gdk_x11_display_get_xdisplay");
  if (!real_xid) real_xid = (RealXidFn)next_symbol("gdk_x11_window_get_xid");
  if (get_window && get_display && get_xdisplay && real_xid) {
    void* gdk_window = get_window(widget);
    if (gdk_window) {
      void* gdk_display = get_display(gdk_window);
      Display* xdpy = gdk_display ? get_xdisplay(gdk_display) : NULL;
      Window xwin = (Window)real_xid(gdk_window);
      if (xdpy && xwin) {
        XMoveWindow(xdpy, xwin, -30000, -30000);
        XFlush(xdpy);
      }
    }
  }
}

void gtk_widget_show(GtkWidget* widget) {
  typedef void (*Fn)(GtkWidget*);
  static Fn real_fn = NULL;
  if (!real_fn) real_fn = (Fn)next_symbol("gtk_widget_show");
  if (real_fn) real_fn(widget);
  make_toplevel_hidden(widget);
}

void gtk_widget_show_all(GtkWidget* widget) {
  typedef void (*Fn)(GtkWidget*);
  static Fn real_fn = NULL;
  if (!real_fn) real_fn = (Fn)next_symbol("gtk_widget_show_all");
  if (real_fn) real_fn(widget);
  make_toplevel_hidden(widget);
}

void gtk_window_present(void* window) {
  typedef void (*Fn)(void*);
  static Fn real_fn = NULL;
  if (!real_fn) real_fn = (Fn)next_symbol("gtk_window_present");
  if (real_fn) real_fn(window);
  make_toplevel_hidden((GtkWidget*)window);
}

void gtk_window_present_with_time(void* window, guint32 timestamp) {
  typedef void (*Fn)(void*, guint32);
  static Fn real_fn = NULL;
  if (!real_fn) real_fn = (Fn)next_symbol("gtk_window_present_with_time");
  if (real_fn) real_fn(window, timestamp);
  make_toplevel_hidden((GtkWidget*)window);
}
