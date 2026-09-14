#define _GNU_SOURCE
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "game_html.inc"

#define GAME_TITLE "Supply Line"
#define GAME_TOKEN "SupplyLine"

static long long mono_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
    return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

static int write_all(int fd, const void *buffer, size_t length) {
    const unsigned char *p = (const unsigned char *)buffer;
    while (length > 0U) {
        ssize_t n = write(fd, p, length);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (n == 0) return -1;
        p += (size_t)n;
        length -= (size_t)n;
    }
    return 0;
}

static void serve_client(int cfd) {
    char req[4096];
    ssize_t n = read(cfd, req, sizeof(req) - 1U);
    if (n <= 0) return;
    req[n] = '\0';

    const char *hdr =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "X-Content-Type-Options: nosniff\r\n\r\n";
    if (write_all(cfd, hdr, strlen(hdr)) < 0) return;
    (void)write_all(cfd, game_html, game_html_len);
}

static int path_exec(const char *path) {
    return path != NULL && access(path, X_OK) == 0;
}

static int resolve_command(const char *cmd, char *out, size_t out_size) {
    if (!cmd || !*cmd || !out || out_size == 0U) return 0;
    if (strchr(cmd, '/')) {
        if (!path_exec(cmd)) return 0;
        if (snprintf(out, out_size, "%s", cmd) >= (int)out_size) return 0;
        return 1;
    }
    const char *path = getenv("PATH");
    if (!path) return 0;
    char *copy = strdup(path);
    if (!copy) return 0;
    char *save = NULL;
    for (char *dir = strtok_r(copy, ":", &save); dir; dir = strtok_r(NULL, ":", &save)) {
        char candidate[4096];
        if (snprintf(candidate, sizeof(candidate), "%s/%s", dir, cmd) >= (int)sizeof(candidate)) continue;
        if (path_exec(candidate)) {
            int ok = snprintf(out, out_size, "%s", candidate) < (int)out_size;
            free(copy);
            return ok;
        }
    }
    free(copy);
    return 0;
}

static int pid_descends_from(pid_t pid, pid_t ancestor) {
    if (pid <= 1 || ancestor <= 1) return 0;
    pid_t current = pid;
    for (int depth = 0; depth < 48 && current > 1; ++depth) {
        if (current == ancestor) return 1;
        char path[64];
        if (snprintf(path, sizeof(path), "/proc/%ld/stat", (long)current) >= (int)sizeof(path)) return 0;
        FILE *fp = fopen(path, "r");
        if (!fp) return 0;
        char line[4096];
        if (!fgets(line, sizeof(line), fp)) {
            fclose(fp);
            return 0;
        }
        fclose(fp);
        char *right = strrchr(line, ')');
        if (!right || right[1] != ' ') return 0;
        char state = '\0';
        long parent = -1;
        if (sscanf(right + 2, "%c %ld", &state, &parent) != 2 || state == '\0' || parent <= 1) return 0;
        if ((pid_t)parent == current) return 0;
        current = (pid_t)parent;
    }
    return 0;
}

static pid_t window_pid(Display *dpy, Window w) {
    Atom atom = XInternAtom(dpy, "_NET_WM_PID", True);
    if (atom == None) return -1;
    Atom type = None;
    int format = 0;
    unsigned long count = 0, after = 0;
    unsigned char *bytes = NULL;
    pid_t result = -1;
    if (XGetWindowProperty(dpy, w, atom, 0, 1, False, XA_CARDINAL,
                           &type, &format, &count, &after, &bytes) == Success &&
        bytes && format == 32 && count >= 1U) {
        unsigned long raw = *(unsigned long *)bytes;
        if (raw > 0UL) result = (pid_t)raw;
    }
    if (bytes) XFree(bytes);
    return result;
}

static int identity_is_renderer(Display *dpy, Window w) {
    XClassHint hint;
    memset(&hint, 0, sizeof(hint));
    int match = 0;
    if (XGetClassHint(dpy, w, &hint)) {
        if ((hint.res_name && strstr(hint.res_name, "NougatArcadeRenderer")) ||
            (hint.res_class && strstr(hint.res_class, "NougatArcadeRenderer"))) {
            match = 1;
        }
        if (hint.res_name) XFree(hint.res_name);
        if (hint.res_class) XFree(hint.res_class);
    }
    return match;
}

static Window *client_windows(Display *dpy, Window root, unsigned long *count_out) {
    *count_out = 0U;
    Atom list = XInternAtom(dpy, "_NET_CLIENT_LIST", True);
    if (list == None) return NULL;
    Atom type = None;
    int format = 0;
    unsigned long count = 0, after = 0;
    unsigned char *bytes = NULL;
    if (XGetWindowProperty(dpy, root, list, 0, 8192, False, XA_WINDOW,
                           &type, &format, &count, &after, &bytes) != Success ||
        !bytes || format != 32) {
        if (bytes) XFree(bytes);
        return NULL;
    }
    Window *copy = calloc(count ? count : 1U, sizeof(Window));
    if (!copy) {
        XFree(bytes);
        return NULL;
    }
    Window *src = (Window *)bytes;
    for (unsigned long i = 0; i < count; ++i) copy[i] = src[i];
    XFree(bytes);
    *count_out = count;
    return copy;
}

static int in_snapshot(Window w, const Window *snapshot, unsigned long count) {
    for (unsigned long i = 0; i < count; ++i) if (snapshot[i] == w) return 1;
    return 0;
}

static Window find_browser_window(Display *dpy, Window root,
                                  const Window *snapshot, unsigned long snapshot_count,
                                  pid_t browser_pid) {
    unsigned long count = 0U;
    Window *windows = client_windows(dpy, root, &count);
    if (!windows) return 0;

    Window best = 0;
    unsigned long best_area = 0U;
    for (unsigned long i = 0; i < count; ++i) {
        Window w = windows[i];
        if (!w || in_snapshot(w, snapshot, snapshot_count)) continue;
        XWindowAttributes a;
        if (!XGetWindowAttributes(dpy, w, &a)) continue;
        if (a.map_state != IsViewable || a.width < 160 || a.height < 100) continue;

        pid_t owner = window_pid(dpy, w);
        int owned = owner > 1 && pid_descends_from(owner, browser_pid);
        int class_match = identity_is_renderer(dpy, w);
        if (!owned && !class_match) continue;

        unsigned long area = (unsigned long)a.width * (unsigned long)a.height;
        if (!best || class_match || area > best_area) {
            best = w;
            best_area = area;
            if (class_match) break;
        }
    }
    free(windows);
    return best;
}

static int make_profile(char *profile, size_t size, int snap_firefox) {
    const char *home = getenv("HOME");
    if (!home || !*home) home = "/tmp";
    if (snap_firefox) {
        char base[4096];
        if (snprintf(base, sizeof(base), "%s/snap/firefox/common", home) >= (int)sizeof(base)) return 0;
        if (mkdir(base, 0700) != 0 && errno != EEXIST) return 0;
        if (snprintf(profile, size, "%s/nougat-arcade-" GAME_TOKEN "-XXXXXX", base) >= (int)size) return 0;
    } else {
        char base[4096];
        if (snprintf(base, sizeof(base), "%s/.cache", home) >= (int)sizeof(base)) return 0;
        if (mkdir(base, 0700) != 0 && errno != EEXIST) return 0;
        if (snprintf(profile, size, "%s/nougat-arcade-" GAME_TOKEN "-XXXXXX", base) >= (int)size) return 0;
    }
    return mkdtemp(profile) != NULL;
}

static pid_t launch_browser(const char *url, char *selected, size_t selected_size) {
    static const char *chromium_family[] = {
        "google-chrome", "google-chrome-stable", "chromium", "chromium-browser",
        "brave-browser", "microsoft-edge", NULL
    };

    char binary[4096];
    for (int i = 0; chromium_family[i]; ++i) {
        if (!resolve_command(chromium_family[i], binary, sizeof(binary))) continue;
        char profile[4096];
        if (!make_profile(profile, sizeof(profile), 0)) continue;

        pid_t child = fork();
        if (child < 0) return -1;
        if (child == 0) {
            setenv("OZONE_PLATFORM", "x11", 1);
            setenv("GDK_BACKEND", "x11", 1);
            char app[512];
            char userdir[4608];
            if (snprintf(app, sizeof(app), "--app=%s", url) >= (int)sizeof(app)) _exit(120);
            if (snprintf(userdir, sizeof(userdir), "--user-data-dir=%s", profile) >= (int)sizeof(userdir)) _exit(121);
            execl(binary, binary,
                  app,
                  userdir,
                  "--class=NougatArcadeRenderer",
                  "--ozone-platform=x11",
                  "--disable-session-crashed-bubble",
                  "--no-first-run",
                  "--no-default-browser-check",
                  (char *)NULL);
            fprintf(stderr, "NOUGAT_ARCADE: exec failed: %s: %s\n", binary, strerror(errno));
            _exit(127);
        }
        (void)snprintf(selected, selected_size, "%s", binary);
        return child;
    }

    const char *firefox_candidates[] = {"/snap/bin/firefox", "firefox", "/usr/bin/firefox", NULL};
    for (int i = 0; firefox_candidates[i]; ++i) {
        if (!resolve_command(firefox_candidates[i], binary, sizeof(binary))) continue;
        int snap = strstr(binary, "/snap/") != NULL || strcmp(binary, "/snap/bin/firefox") == 0;
        char profile[4096];
        if (!make_profile(profile, sizeof(profile), snap)) continue;

        pid_t child = fork();
        if (child < 0) return -1;
        if (child == 0) {
            setenv("MOZ_ENABLE_WAYLAND", "0", 1);
            setenv("GDK_BACKEND", "x11", 1);
            setenv("OZONE_PLATFORM", "x11", 1);
            execl(binary, binary,
                  "--no-remote",
                  "--new-instance",
                  "--profile", profile,
                  "--class", "NougatArcadeRenderer",
                  "--kiosk",
                  "--new-window", url,
                  (char *)NULL);
            fprintf(stderr, "NOUGAT_ARCADE: exec failed: %s: %s\n", binary, strerror(errno));
            _exit(127);
        }
        (void)snprintf(selected, selected_size, "%s", binary);
        return child;
    }

    fprintf(stderr, "NOUGAT_ARCADE: no supported browser executable found\n");
    return -1;
}

static void set_private_window(Display *dpy, Window w) {
    Atom state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom skip_taskbar = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
    Atom skip_pager = XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False);
    Atom values[2] = {skip_taskbar, skip_pager};
    XChangeProperty(dpy, w, state, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)values, 2);
}

int main(void) {
    signal(SIGPIPE, SIG_IGN);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        fprintf(stderr, "NOUGAT_ARCADE: socket failed: %s\n", strerror(errno));
        return 1;
    }
    int one = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) != 0) {
        fprintf(stderr, "NOUGAT_ARCADE: setsockopt failed: %s\n", strerror(errno));
    }
    int flags = fcntl(s, F_GETFL, 0);
    if (flags >= 0) (void)fcntl(s, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in a;
    memset(&a, 0, sizeof(a));
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons(0);
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) < 0) {
        fprintf(stderr, "NOUGAT_ARCADE: bind failed: %s\n", strerror(errno));
        close(s);
        return 1;
    }
    socklen_t alen = sizeof(a);
    if (getsockname(s, (struct sockaddr *)&a, &alen) < 0 || listen(s, 8) < 0) {
        fprintf(stderr, "NOUGAT_ARCADE: HTTP setup failed: %s\n", strerror(errno));
        close(s);
        return 1;
    }

    char url[256];
    if (snprintf(url, sizeof(url), "http://127.0.0.1:%u/", (unsigned)ntohs(a.sin_port)) >= (int)sizeof(url)) {
        close(s);
        return 1;
    }
    fprintf(stderr, "NOUGAT_ARCADE: %s HTTP ready at %s\n", GAME_TITLE, url);

    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "NOUGAT_ARCADE: XOpenDisplay failed; DISPLAY=%s\n",
                getenv("DISPLAY") ? getenv("DISPLAY") : "(unset)");
        close(s);
        return 2;
    }

    Window root = DefaultRootWindow(dpy);
    Window parent = root;
    const char *embed_text = getenv("NOUGAT_EMBED_XID");
    if (embed_text && *embed_text) {
        char *end = NULL;
        unsigned long raw = strtoul(embed_text, &end, 0);
        if (end && *end == '\0' && raw != 0UL) {
            XWindowAttributes pa;
            if (XGetWindowAttributes(dpy, (Window)raw, &pa)) parent = (Window)raw;
        }
    }

    int width = 960;
    int height = 540;
    if (parent != root) {
        XWindowAttributes pa;
        if (XGetWindowAttributes(dpy, parent, &pa)) {
            width = pa.width > 1 ? pa.width : width;
            height = pa.height > 1 ? pa.height : height;
        }
    }

    Window host = XCreateSimpleWindow(dpy, parent, 0, 0,
                                      (unsigned)width, (unsigned)height,
                                      0, BlackPixel(dpy, DefaultScreen(dpy)),
                                      BlackPixel(dpy, DefaultScreen(dpy)));
    if (!host) {
        fprintf(stderr, "NOUGAT_ARCADE: failed to create native host window\n");
        XCloseDisplay(dpy);
        close(s);
        return 3;
    }

    XStoreName(dpy, host, "Nougat Arcade");
    XClassHint class_hint;
    class_hint.res_name = (char *)"NougatArcade";
    class_hint.res_class = (char *)"NougatArcade";
    XSetClassHint(dpy, host, &class_hint);

    Atom pid_atom = XInternAtom(dpy, "_NET_WM_PID", False);
    unsigned long self_pid = (unsigned long)getpid();
    XChangeProperty(dpy, host, pid_atom, XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&self_pid, 1);
    XSelectInput(dpy, host, StructureNotifyMask | SubstructureNotifyMask);
    XMapWindow(dpy, host);
    XFlush(dpy);
    fprintf(stderr, "NOUGAT_ARCADE: native host mapped xid=0x%lx parent=0x%lx pid=%ld\n",
            (unsigned long)host, (unsigned long)parent, (long)getpid());

    unsigned long snapshot_count = 0U;
    Window *snapshot = client_windows(dpy, root, &snapshot_count);

    char selected[4096] = {0};
    pid_t browser = launch_browser(url, selected, sizeof(selected));
    if (browser < 0) {
        fprintf(stderr, "NOUGAT_ARCADE: browser launch failed\n");
        if (snapshot) free(snapshot);
        XDestroyWindow(dpy, host);
        XCloseDisplay(dpy);
        close(s);
        return 4;
    }
    fprintf(stderr, "NOUGAT_ARCADE: browser selected=%s pid=%ld forced-x11=1\n",
            selected, (long)browser);

    Window renderer = 0;
    const long long deadline = mono_ms() + 30000LL;
    int browser_status = 0;

    while (mono_ms() < deadline && !renderer) {
        for (;;) {
            int c = accept(s, NULL, NULL);
            if (c < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) break;
                break;
            }
            serve_client(c);
            close(c);
        }

        pid_t wr = waitpid(browser, &browser_status, WNOHANG);
        if (wr == browser) {
            fprintf(stderr, "NOUGAT_ARCADE: browser exited before renderer appeared status=%d\n",
                    browser_status);
            break;
        }

        renderer = find_browser_window(dpy, root, snapshot, snapshot_count, browser);
        if (renderer) break;
        usleep(50000);
    }

    if (snapshot) free(snapshot);

    if (!renderer) {
        fprintf(stderr, "NOUGAT_ARCADE: no X11/XWayland renderer found within launch window\n");
        if (kill(browser, 0) == 0) kill(browser, SIGTERM);
        (void)waitpid(browser, NULL, 0);
        XDestroyWindow(dpy, host);
        XCloseDisplay(dpy);
        close(s);
        return 5;
    }

    XWindowAttributes ra;
    if (!XGetWindowAttributes(dpy, renderer, &ra)) {
        fprintf(stderr, "NOUGAT_ARCADE: renderer disappeared before containment\n");
        kill(browser, SIGTERM);
        (void)waitpid(browser, NULL, 0);
        XDestroyWindow(dpy, host);
        XCloseDisplay(dpy);
        close(s);
        return 6;
    }

    set_private_window(dpy, renderer);
    XUnmapWindow(dpy, renderer);
    XReparentWindow(dpy, renderer, host, 0, 0);
    XMoveResizeWindow(dpy, renderer, 0, 0, (unsigned)width, (unsigned)height);
    XMapWindow(dpy, renderer);
    XRaiseWindow(dpy, renderer);
    XSetInputFocus(dpy, renderer, RevertToParent, CurrentTime);
    XFlush(dpy);
    fprintf(stderr, "NOUGAT_ARCADE: renderer contained xid=0x%lx %dx%d\n",
            (unsigned long)renderer, width, height);

    int running = 1;
    while (running) {
        for (;;) {
            int c = accept(s, NULL, NULL);
            if (c < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) break;
                running = 0;
                break;
            }
            serve_client(c);
            close(c);
        }

        while (XPending(dpy) > 0) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            if (ev.type == ConfigureNotify && ev.xconfigure.window == host) {
                width = ev.xconfigure.width > 0 ? ev.xconfigure.width : 1;
                height = ev.xconfigure.height > 0 ? ev.xconfigure.height : 1;
                if (renderer) XMoveResizeWindow(dpy, renderer, 0, 0, (unsigned)width, (unsigned)height);
            } else if (ev.type == DestroyNotify &&
                       (ev.xdestroywindow.window == renderer || ev.xdestroywindow.window == host)) {
                running = 0;
            }
        }
        XFlush(dpy);

        pid_t wr = waitpid(browser, &browser_status, WNOHANG);
        if (wr == browser) {
            fprintf(stderr, "NOUGAT_ARCADE: browser renderer process exited status=%d\n",
                    browser_status);
            running = 0;
        }
        usleep(10000);
    }

    if (kill(browser, 0) == 0) kill(browser, SIGTERM);
    (void)waitpid(browser, NULL, 0);
    XDestroyWindow(dpy, host);
    XCloseDisplay(dpy);
    close(s);
    return 0;
}
