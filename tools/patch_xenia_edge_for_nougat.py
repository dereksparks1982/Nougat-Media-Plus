#!/usr/bin/env python3
from pathlib import Path
import sys

src = Path(sys.argv[1]).resolve()
path = src / "src/xenia/ui/window_wx.cc"
if not path.is_file():
    raise SystemExit("STOP: Xenia Edge window_wx.cc was not found.")

text = path.read_text(encoding="utf-8")
marker = "NOUGAT_EDGE_EMBED_SOURCE_PATCH_V1"
if marker in text:
    print("PASS: Nougat Edge source patch is already present.")
    raise SystemExit(0)

def replace_once(old, new, label):
    global text
    count = text.count(old)
    if count != 1:
        raise SystemExit(
            f"STOP: expected exactly one {label} pattern, found {count}. "
            "No source file was written."
        )
    text = text.replace(old, new, 1)

replace_once(
'''#include <cstring>
#include <vector>
''',
'''#include <cstring>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <vector>
''',
"standard includes"
)

replace_once(
'''#elif XE_PLATFORM_LINUX
#include <gtk/gtk.h>
#include "xenia/ui/surface_gnulinux.h"
''',
'''#elif XE_PLATFORM_LINUX
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "xenia/ui/surface_gnulinux.h"
''',
"Linux includes"
)

replace_once(
'''namespace {
constexpr char kGeometryKeyX[] = "/window/x";
constexpr char kGeometryKeyY[] = "/window/y";
constexpr char kGeometryKeyMaximized[] = "/window/maximized";
}  // namespace
''',
'''namespace {
constexpr char kGeometryKeyX[] = "/window/x";
constexpr char kGeometryKeyY[] = "/window/y";
constexpr char kGeometryKeyMaximized[] = "/window/maximized";

// NOUGAT_EDGE_EMBED_SOURCE_PATCH_V1
#if XE_PLATFORM_LINUX
bool NougatEmbedMode() {
  const char* value = std::getenv("NOUGAT_EMBED_MODE");
  return value && value[0] == '1' && value[1] == '\\0';
}

bool X11ClassIsNougat(Display* display, ::Window window) {
  if (!display || !window) {
    return false;
  }

  XClassHint hint{};
  if (XGetClassHint(display, window, &hint) == 0) {
    return false;
  }

  bool match = false;
  if (hint.res_name &&
      std::string(hint.res_name) == "nougat-media-suite") {
    match = true;
  }
  if (hint.res_class &&
      std::string(hint.res_class) == "NougatMediaSuite") {
    match = true;
  }

  if (hint.res_name) {
    XFree(hint.res_name);
  }
  if (hint.res_class) {
    XFree(hint.res_class);
  }
  return match;
}

bool X11HasNougatAncestor(Display* display, ::Window window) {
  if (!display || !window) {
    return false;
  }

  ::Window current = window;
  for (int depth = 0; depth < 32 && current; ++depth) {
    ::Window root_return = 0;
    ::Window parent_return = 0;
    ::Window* children = nullptr;
    unsigned int child_count = 0;

    if (XQueryTree(display, current, &root_return, &parent_return,
                   &children, &child_count) == 0) {
      if (children) {
        XFree(children);
      }
      return false;
    }

    if (children) {
      XFree(children);
    }

    if (!parent_return || parent_return == current) {
      return false;
    }
    if (X11ClassIsNougat(display, parent_return)) {
      return true;
    }
    if (parent_return == root_return) {
      return false;
    }

    current = parent_return;
  }

  return false;
}

bool PrepareNougatX11Frame(wxFrame* frame, Display*& display_out,
                           ::Window& xid_out) {
  display_out = nullptr;
  xid_out = 0;

  if (!NougatEmbedMode() || !frame) {
    return true;
  }

  auto* widget = static_cast<GtkWidget*>(frame->GetHandle());
  if (!widget) {
    return false;
  }

  gtk_window_set_decorated(GTK_WINDOW(widget), FALSE);
  gtk_widget_realize(widget);

  GdkWindow* gdk_window = gtk_widget_get_window(widget);
  if (!gdk_window) {
    return false;
  }

  GdkDisplay* gdk_display = gdk_window_get_display(gdk_window);
  if (!gdk_display || !GDK_IS_X11_DISPLAY(gdk_display)) {
    return false;
  }

  Display* display = gdk_x11_display_get_xdisplay(gdk_display);
  ::Window xid = gdk_x11_window_get_xid(gdk_window);
  if (!display || !xid) {
    return false;
  }

  XClassHint class_hint{};
  class_hint.res_name = const_cast<char*>("xenia_canary");
  class_hint.res_class = const_cast<char*>("xenia_canary");
  XSetClassHint(display, xid, &class_hint);
  XStoreName(display, xid, "xenia_canary - Xenia Edge - Nougat embedded");
  XSync(display, False);

  display_out = display;
  xid_out = xid;
  return true;
}

bool WaitForNougatReparent(Display* display, ::Window xid,
                           int timeout_ms = 30000) {
  if (!NougatEmbedMode()) {
    return true;
  }
  if (!display || !xid) {
    return false;
  }

  const auto deadline =
      std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);

  while (std::chrono::steady_clock::now() < deadline) {
    XSync(display, False);
    if (X11HasNougatAncestor(display, xid)) {
      return true;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return false;
}
#endif
}  // namespace
''',
"Nougat Linux helper block"
)

replace_once(
'''  auto* main_menu = static_cast<WxMenuItem*>(GetMainMenu());
  if (main_menu && main_menu->GetMenuBar()) {
    frame_->SetMenuBar(main_menu->GetMenuBar());
  }

  frame_->SetClientSize(wxSize(physical_width, physical_height));
''',
'''  auto* main_menu = static_cast<WxMenuItem*>(GetMainMenu());
#if XE_PLATFORM_LINUX
  if (!NougatEmbedMode() && main_menu && main_menu->GetMenuBar()) {
    frame_->SetMenuBar(main_menu->GetMenuBar());
  }
#else
  if (main_menu && main_menu->GetMenuBar()) {
    frame_->SetMenuBar(main_menu->GetMenuBar());
  }
#endif

  frame_->SetClientSize(wxSize(physical_width, physical_height));
''',
"main menu setup"
)

replace_once(
'''  RestoreGeometryFromConfig();

  if (IsFullscreen()) {
    frame_->ShowFullScreen(true);
  }
''',
'''#if XE_PLATFORM_LINUX
  if (!NougatEmbedMode()) {
    RestoreGeometryFromConfig();
  }
  if (IsFullscreen() && !NougatEmbedMode()) {
    frame_->ShowFullScreen(true);
  }
#else
  RestoreGeometryFromConfig();
  if (IsFullscreen()) {
    frame_->ShowFullScreen(true);
  }
#endif
''',
"geometry/fullscreen startup"
)

replace_once(
'''  frame_->Show(true);
  render_target()->SetFocus();

  wxSize client_size = render_target()->GetClientSize();
''',
'''#if XE_PLATFORM_LINUX
  Display* nougat_x11_display = nullptr;
  ::Window nougat_x11_frame = 0;
  if (!PrepareNougatX11Frame(frame_, nougat_x11_display,
                             nougat_x11_frame)) {
    return false;
  }
#endif

  frame_->Show(true);

#if XE_PLATFORM_LINUX
  if (NougatEmbedMode() &&
      !WaitForNougatReparent(nougat_x11_display, nougat_x11_frame)) {
    frame_->Hide();
    return false;
  }
#endif

  render_target()->SetFocus();

  wxSize client_size = render_target()->GetClientSize();
''',
"show and pre-Vulkan wait"
)

replace_once(
'''void WxWindow::ApplyNewFullscreen() {
  if (!frame_) {
    return;
  }
''',
'''void WxWindow::ApplyNewFullscreen() {
  if (!frame_) {
    return;
  }
#if XE_PLATFORM_LINUX
  if (NougatEmbedMode()) {
    return;
  }
#endif
''',
"fullscreen runtime guard"
)

replace_once(
'''void WxWindow::ApplyNewMainMenu(MenuItem* old_main_menu) {
  if (!frame_) {
    return;
  }
''',
'''void WxWindow::ApplyNewMainMenu(MenuItem* old_main_menu) {
  if (!frame_) {
    return;
  }
#if XE_PLATFORM_LINUX
  if (NougatEmbedMode()) {
    frame_->SetMenuBar(nullptr);
    return;
  }
#endif
''',
"menu runtime guard"
)

path.write_text(text, encoding="utf-8")
print("PASS: Nougat source-level Xenia Edge embedding patch applied.")
