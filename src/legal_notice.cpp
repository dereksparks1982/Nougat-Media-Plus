#include "legal_notice.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

namespace {
constexpr const char* kVersion = "2026-10-30-v1";
constexpr int kWidth = 820;
constexpr int kHeight = 490;

std::filesystem::path receipt_path() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) return std::filesystem::path(xdg) / "nougat-media-plus" / "legal-notice.accepted";
    const char* home = std::getenv("HOME");
    if (home && *home) return std::filesystem::path(home) / ".config" / "nougat-media-plus" / "legal-notice.accepted";
    return {};
}

bool already_accepted() {
    const auto path = receipt_path();
    if (path.empty()) return false;
    std::ifstream input(path);
    std::string version;
    return static_cast<bool>(std::getline(input, version)) && version == kVersion;
}

bool save_acceptance() {
    const auto path = receipt_path();
    if (path.empty()) return false;
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) return false;
    std::ofstream output(path, std::ios::trunc);
    if (!output) return false;
    output << kVersion << '\n';
    output.flush();
    return static_cast<bool>(output);
}

unsigned long color(Display* display, int screen, unsigned short r, unsigned short g, unsigned short b) {
    XColor xcolor{};
    xcolor.red = r;
    xcolor.green = g;
    xcolor.blue = b;
    xcolor.flags = DoRed | DoGreen | DoBlue;
    if (XAllocColor(display, DefaultColormap(display, screen), &xcolor)) return xcolor.pixel;
    return WhitePixel(display, screen);
}

void line(Display* display, Window window, GC gc, unsigned long fg, int x, int y, const char* content) {
    XSetForeground(display, gc, fg);
    XDrawString(display, window, gc, x, y, content, static_cast<int>(std::strlen(content)));
}

void fill(Display* display, Window window, GC gc, unsigned long fg, int x, int y, int w, int h) {
    XSetForeground(display, gc, fg);
    XFillRectangle(display, window, gc, x, y, static_cast<unsigned>(w), static_cast<unsigned>(h));
}
}

bool nougat_show_first_run_legal_notice() {
    if (already_accepted()) return true;
    Display* display = XOpenDisplay(nullptr);
    if (!display) return false;
    const int screen = DefaultScreen(display);
    const auto background = color(display, screen, 0x0707, 0x1717, 0x1313);
    const auto panel = color(display, screen, 0x1111, 0x2929, 0x2222);
    const auto accent = color(display, screen, 0x7c7c, 0xcccc, 0x9999);
    const auto gold = color(display, screen, 0xeeee, 0xcccc, 0x8888);
    const auto white = color(display, screen, 0xeeee, 0xeeee, 0xeeee);
    const auto muted = color(display, screen, 0xaaaa, 0xbbbb, 0xaaaa);

    Window window = XCreateSimpleWindow(display, RootWindow(display, screen),
                                         130, 105, kWidth, kHeight, 0, accent, background);
    XStoreName(display, window, "Nougat Media Plus - First Run Legal Notice");
    XSelectInput(display, window, ExposureMask | ButtonPressMask | KeyPressMask | StructureNotifyMask);
    Atom close_message = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &close_message, 1);
    GC gc = XCreateGC(display, window, 0, nullptr);
    XFontStruct* font = XLoadQueryFont(display, "9x15");
    if (!font) font = XLoadQueryFont(display, "fixed");
    if (font) XSetFont(display, gc, font->fid);
    XMapRaised(display, window);
    XSetInputFocus(display, window, RevertToParent, CurrentTime);

    bool checked = false;
    bool storage_failed = false;
    bool accepted = false;
    bool done = false;
    const char* rows[] = {
        "Nougat Media Plus is a general-purpose media and technology application.",
        "Features may play, download, transmit, stream, or share content.",
        "You are responsible for the content you access or distribute",
        "and for complying with applicable copyright, privacy, licensing,",
        "communications, and other laws. Obtain the permissions you need.",
        "Elderred Softworks LLC does not authorize or endorse illegal use.",
        "To the extent permitted by law, we disclaim responsibility for",
        "unlawful or unauthorized user actions. Legal exceptions still apply.",
        "Read LEGAL_NOTICE.md, LICENSE, and THIRD_PARTY_NOTICES.md.",
        "No further notice is issued after acceptance of this version."
    };
    auto paint = [&] {
        fill(display, window, gc, background, 0, 0, kWidth, kHeight);
        fill(display, window, gc, panel, 18, 18, kWidth - 36, kHeight - 36);
        line(display, window, gc, gold, 40, 56, "NOUGAT MEDIA PLUS | FIRST-RUN LEGAL NOTICE");
        line(display, window, gc, muted, 40, 86, "Please read carefully. Acceptance is required to continue.");
        for (int i = 0; i < 10; ++i) line(display, window, gc, white, 40, 116 + i * 24, rows[i]);
        fill(display, window, gc, checked ? accent : muted, 40, 369, 19, 19);
        fill(display, window, gc, panel, 43, 372, 13, 13);
        if (checked) line(display, window, gc, accent, 45, 384, "X");
        line(display, window, gc, white, 71, 383, "I have read this notice and agree to use Nougat lawfully.");
        fill(display, window, gc, checked ? accent : muted, 510, 414, 264, 36);
        line(display, window, gc, background, 540, 438, "I UNDERSTAND & AGREE");
        line(display, window, gc, muted, 40, 438, "Cancel / Exit (Esc)");
        if (storage_failed) line(display, window, gc, gold, 40, 467, "Could not save acceptance. Check configuration permissions.");
        XFlush(display);
    };

    paint();
    while (!done) {
        XEvent event{};
        XNextEvent(display, &event);
        if (event.type == Expose && event.xexpose.count == 0) paint();
        if (event.type == ClientMessage && static_cast<Atom>(event.xclient.data.l[0]) == close_message) done = true;
        if (event.type == ButtonPress) {
            const int x = event.xbutton.x;
            const int y = event.xbutton.y;
            if (x >= 35 && x < 765 && y >= 358 && y < 397) { checked = !checked; storage_failed = false; paint(); }
            else if (x >= 510 && x < 774 && y >= 414 && y < 452 && checked) {
                accepted = save_acceptance();
                storage_failed = !accepted;
                done = accepted;
                if (!done) paint();
            } else if (x >= 38 && x <= 250 && y >= 411 && y <= 455) done = true;
        }
        if (event.type == KeyPress) {
            const KeySym key = XLookupKeysym(&event.xkey, 0);
            if (key == XK_Escape) done = true;
            if (key == XK_space) { checked = !checked; storage_failed = false; paint(); }
            if ((key == XK_Return || key == XK_KP_Enter) && checked) {
                accepted = save_acceptance();
                storage_failed = !accepted;
                done = accepted;
                if (!done) paint();
            }
        }
    }
    if (font) XFreeFont(display, font);
    XFreeGC(display, gc);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return accepted;
}
