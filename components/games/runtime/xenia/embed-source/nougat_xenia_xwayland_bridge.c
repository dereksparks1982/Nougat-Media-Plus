#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stop_requested = 0;
static void on_signal(int sig) { (void)sig; stop_requested = 1; }

static void sleep_ms(int ms) {
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (long)(ms % 1000) * 1000000L;
  nanosleep(&ts, NULL);
}

static long long now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static int get_parent(Display *d, Window w, Window *parent_out, Window *root_out) {
  Window root = 0, parent = 0, *children = NULL;
  unsigned int count = 0;
  Status ok = XQueryTree(d, w, &root, &parent, &children, &count);
  if (children) XFree(children);
  if (!ok) return 0;
  if (parent_out) *parent_out = parent;
  if (root_out) *root_out = root;
  return 1;
}

static void set_identity(Display *d, Window w, pid_t pid) {
  XStoreName(d, w, "xenia_canary");
  XClassHint hint;
  hint.res_name = (char *)"xenia_canary";
  hint.res_class = (char *)"xenia_canary";
  XSetClassHint(d, w, &hint);

  Atom pid_atom = XInternAtom(d, "_NET_WM_PID", False);
  unsigned long value = (unsigned long)pid;
  XChangeProperty(d, w, pid_atom, XA_CARDINAL, 32, PropModeReplace,
                  (unsigned char *)&value, 1);
}

static int set_env(const char *key, const char *value) {
  if (!key || !*key || !value) return 0;
  return setenv(key, value, 1);
}

int main(int argc, char **argv) {
  const char *hook = NULL;
  const char *edge = NULL;
  int edge_index = -1;
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--hook") == 0 && i + 1 < argc) {
      hook = argv[++i];
    } else if (strcmp(argv[i], "--edge") == 0 && i + 1 < argc) {
      edge = argv[++i];
      edge_index = i;
      break;
    }
  }
  if (!edge || edge_index < 0) {
    fprintf(stderr, "Nougat Xenia bridge usage: %s --hook /path/to/hook.so --edge /path/to/xenia [xenia args...]\n", argv[0]);
    return 2;
  }
  signal(SIGTERM, on_signal);
  signal(SIGINT, on_signal);

  Display *d = XOpenDisplay(NULL);
  if (!d) {
    fprintf(stderr, "Nougat Xbox runtime error: X11/XWayland display unavailable.\n");
    return 3;
  }

  Window root = DefaultRootWindow(d);
  XSetWindowAttributes attrs;
  memset(&attrs, 0, sizeof(attrs));
  attrs.background_pixel = BlackPixel(d, DefaultScreen(d));
  attrs.override_redirect = True;
  attrs.event_mask = StructureNotifyMask | FocusChangeMask | KeyPressMask | KeyReleaseMask |
                     ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

  Window proxy = XCreateWindow(d, root, -30000, -30000, 640, 360, 0,
                               CopyFromParent, InputOutput, CopyFromParent,
                               CWBackPixel | CWOverrideRedirect | CWEventMask, &attrs);
  if (!proxy) {
    XCloseDisplay(d);
    return 4;
  }
  set_identity(d, proxy, getpid());
  XMapWindow(d, proxy);
  XFlush(d);

  // Nougat v53 polls every 20 ms. Because this is override-redirect it is not
  // owned by Mutter/GNOME and can be safely reparented by Nougat even on an
  // XWayland desktop.
  Window parent = root;
  long long deadline = now_ms() + 10000;
  while (!stop_requested && now_ms() < deadline) {
    Window p = 0, r = 0;
    if (!get_parent(d, proxy, &p, &r)) break;
    parent = p;
    if (p && p != r) break;
    sleep_ms(5);
  }

  if (stop_requested || !parent || parent == root) {
    fprintf(stderr, "Nougat Xbox runtime error: Nougat did not adopt the Xenia render surface.\n");
    XDestroyWindow(d, proxy);
    XCloseDisplay(d);
    return 5;
  }

  char xid[64];
  snprintf(xid, sizeof(xid), "%lu", (unsigned long)proxy);

  // LD_PRELOAD treats both spaces and colons as separators. Nougat's project
  // directory intentionally contains spaces, so loading the hook directly from
  // its project path would silently split the name and fail. Publish a private
  // no-space symlink for the lifetime of this Edge process instead.
  char hook_link[256] = {0};
  if (hook && *hook) {
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (!runtime_dir || !*runtime_dir || strchr(runtime_dir, ' ') || strchr(runtime_dir, ':') ||
        access(runtime_dir, W_OK | X_OK) != 0)
      runtime_dir = "/tmp";
    snprintf(hook_link, sizeof(hook_link), "%s/nougat-xenia-hook-%d.so",
             runtime_dir, (int)getpid());
    unlink(hook_link);
    if (symlink(hook, hook_link) != 0) {
      fprintf(stderr, "Nougat Xbox runtime error: could not publish preload hook: %s\n",
              strerror(errno));
      XDestroyWindow(d, proxy);
      XCloseDisplay(d);
      return 6;
    }
  }

  pid_t edge_pid = fork();
  if (edge_pid < 0) {
    if (*hook_link) unlink(hook_link);
    XDestroyWindow(d, proxy);
    XCloseDisplay(d);
    return 7;
  }
  if (edge_pid == 0) {
    set_env("NOUGAT_XENIA_DIRECT_XID", xid);
    set_env("GDK_BACKEND", "x11");
    set_env("APPIMAGE_EXTRACT_AND_RUN", "1");
    if (*hook_link) set_env("LD_PRELOAD", hook_link);
    else unsetenv("LD_PRELOAD");

    // Preserve the Edge executable as argv[0] and all arguments following it.
    char **edge_argv = &argv[edge_index];
    execv(edge, edge_argv);
    _exit(127);
  }

  int status = 0;
  while (!stop_requested) {
    pid_t rc = waitpid(edge_pid, &status, WNOHANG);
    if (rc == edge_pid) break;
    if (rc < 0 && errno == ECHILD) break;
    sleep_ms(25);
  }

  if (stop_requested) {
    kill(edge_pid, SIGTERM);
    for (int i = 0; i < 20; ++i) {
      pid_t rc = waitpid(edge_pid, &status, WNOHANG);
      if (rc == edge_pid) break;
      sleep_ms(25);
    }
    if (kill(edge_pid, 0) == 0 || errno == EPERM) kill(edge_pid, SIGKILL);
    waitpid(edge_pid, &status, WNOHANG);
  }

  if (*hook_link) unlink(hook_link);
  XDestroyWindow(d, proxy);
  XCloseDisplay(d);

  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
  return 0;
}
