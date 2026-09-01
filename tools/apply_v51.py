#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]).resolve()


def need(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    need(count == 1, f"{label}: expected one exact anchor, found {count}")
    return text.replace(old, new, 1)


def matching_brace(text: str, open_at: int) -> int:
    need(open_at >= 0 and text[open_at] == "{", "invalid C++ opening brace")
    depth = 0
    state = "code"
    i = open_at
    while i < len(text):
        c = text[i]
        n = text[i + 1] if i + 1 < len(text) else ""
        if state == "code":
            if c == "/" and n == "/":
                state = "line"; i += 2; continue
            if c == "/" and n == "*":
                state = "block"; i += 2; continue
            if c == '"':
                state = "string"; i += 1; continue
            if c == "'":
                state = "char"; i += 1; continue
            if c == "{": depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    return i
        elif state == "line":
            if c == "\n": state = "code"
        elif state == "block":
            if c == "*" and n == "/": state = "code"; i += 2; continue
        elif state in ("string", "char"):
            quote = '"' if state == "string" else "'"
            if c == "\\": i += 2; continue
            if c == quote: state = "code"
        i += 1
    raise RuntimeError("unterminated C++ brace")


def function_span(text: str, signature: str) -> tuple[int, int]:
    start = text.find(signature)
    need(start >= 0, f"missing function: {signature}")
    need(text.find(signature, start + 1) < 0, f"ambiguous function: {signature}")
    brace = text.find("{", start + len(signature))
    need(brace >= 0, f"missing function brace: {signature}")
    end = matching_brace(text, brace)
    return start, end + 1


def replace_function(text: str, signature: str, replacement: str) -> str:
    start, end = function_span(text, signature)
    return text[:start] + replacement.rstrip() + text[end:]


def replace_block_in_function(text: str, signature: str, marker: str, replacement: str) -> str:
    fstart, fend = function_span(text, signature)
    pos = text.find(marker, fstart, fend)
    need(pos >= 0, f"missing block {marker!r} in {signature}")
    brace = text.find("{", pos, fend)
    need(brace >= 0, f"missing block brace for {marker!r}")
    end = matching_brace(text, brace) + 1
    return text[:pos] + replacement.rstrip() + text[end:]


def prepend_after_title(path: Path, heading: str, body: str) -> None:
    text = path.read_text(encoding="utf-8")
    if heading in text:
        return
    if text.startswith("# "):
        nl = text.find("\n")
        text = text[:nl + 1] + "\n" + heading + "\n\n" + body.strip() + "\n\n" + text[nl + 1:]
    else:
        text = heading + "\n\n" + body.strip() + "\n\n" + text
    path.write_text(text, encoding="utf-8")


def patch_main(path: Path) -> None:
    text = path.read_text(encoding="utf-8")

    # Version identity. Comments/history are intentionally not rewritten.
    text = replace_once(text, 'const std::string versionLabel = "v0.0.49";',
                        'const std::string versionLabel = "v0.0.51";', "visible header version")
    text = replace_once(text, 'input.app_version = "Nougat Media Suite v0.0.50";',
                        'input.app_version = "Nougat Media Suite v0.0.51";', "diagnostic version")
    text = replace_once(text, 'printf("Nougat Media Suite v0.0.50\\n");',
                        'printf("Nougat Media Suite v0.0.51\\n");', "CLI version")

    # New approved branding: exact raster lockup in the top bar and exact N crop
    # for _NET_WM_ICON through the replaced icon-data header.
    text = replace_function(text, '    void draw_suite_badge(Drawable target, int x0, int y0, unsigned char bgR, unsigned char bgG, unsigned char bgB)', r'''    void draw_suite_brand(Drawable target, int x0, int y0,
                          unsigned char bgR, unsigned char bgG, unsigned char bgB) {
        for (int y = 0; y < nougat_media_suite_icon::kTopBarHeight; ++y) {
            for (int x = 0; x < nougat_media_suite_icon::kTopBarWidth; ++x) {
                const std::uint32_t argb = nougat_media_suite_icon::kTopBarLockup[
                    y * nougat_media_suite_icon::kTopBarWidth + x];
                const unsigned char a = static_cast<unsigned char>((argb >> 24) & 0xffU);
                if (a == 0) continue;
                const unsigned char sr = static_cast<unsigned char>((argb >> 16) & 0xffU);
                const unsigned char sg = static_cast<unsigned char>((argb >> 8) & 0xffU);
                const unsigned char sb = static_cast<unsigned char>(argb & 0xffU);
                const unsigned char r = static_cast<unsigned char>((static_cast<unsigned>(sr) * a + static_cast<unsigned>(bgR) * (255U - a)) / 255U);
                const unsigned char g = static_cast<unsigned char>((static_cast<unsigned>(sg) * a + static_cast<unsigned>(bgG) * (255U - a)) / 255U);
                const unsigned char b = static_cast<unsigned char>((static_cast<unsigned>(sb) * a + static_cast<unsigned>(bgB) * (255U - a)) / 255U);
                XSetForeground(d, gc, visual_pixel(r, g, b));
                XDrawPoint(d, target, gc, x0 + x, y0 + y);
            }
        }
    }''')
    text = replace_once(text,
        '        return 28 + text_width("NOUGAT MEDIA SUITE") + 6;',
        '        return 4 + nougat_media_suite_icon::kTopBarWidth + 6;',
        "top navigation brand bound")
    old_header = '''        const int badgeY = (kTopBarH - nougat_media_suite_icon::kTopBar14Size) / 2;
        const int headerBaseline = kTopBarH / 2 + 5;
        draw_suite_badge(target, 8, badgeY, 227, 204, 172);
        text(target, 28, headerBaseline, "NOUGAT MEDIA SUITE", topText);'''
    new_header = '''        const int brandY = (kTopBarH - nougat_media_suite_icon::kTopBarHeight) / 2;
        const int headerBaseline = kTopBarH / 2 + 5;
        draw_suite_brand(target, 4, brandY, 227, 204, 172);'''
    text = replace_once(text, old_header, new_header, "approved header lockup")

    # v0.0.51 approved branding replaces the old square badge with the full
    # N + cursive Nougat Media Suite lockup.  Replace the old click shield
    # structurally so harmless whitespace changes cannot break the build.
    brand_decl = "        const Rect nougatBrandBadge"
    brand_start = text.find(brand_decl)
    need(brand_start >= 0, "approved brand lockup hitbox declaration not found")
    need(text.find(brand_decl, brand_start + 1) < 0, "approved brand lockup hitbox declaration is ambiguous")
    brand_open = text.find("{", brand_start + len(brand_decl))
    need(brand_open >= 0, "approved brand lockup hitbox opening brace not found")
    brand_close = matching_brace(text, brand_open)
    brand_end = brand_close + 1
    while brand_end < len(text) and text[brand_end] in " \t":
        brand_end += 1
    need(brand_end < len(text) and text[brand_end] == ";", "approved brand lockup hitbox terminator not found")
    brand_end += 1
    new_brand_hitbox = """        const Rect nougatBrandBadge{
            4,
            (kTopBarH - nougat_media_suite_icon::kTopBarHeight) / 2,
            nougat_media_suite_icon::kTopBarWidth,
            nougat_media_suite_icon::kTopBarHeight
        };"""
    text = text[:brand_start] + new_brand_hitbox + text[brand_end:]
    need("nougat_media_suite_icon::kTopBar14Size" not in text,
         "retired Nougat N header constant remains after approved branding patch")

    # Radio top-level view and Mulberry palette.
    text = replace_once(text,
        'enum class ViewMode { Home, VideoPlayer, Library, Discover, LiveTV, WorldTV, Nougat, Stream, Studio, Games, P2P, Debug };',
        'enum class ViewMode { Home, VideoPlayer, Library, Discover, LiveTV, WorldTV, Radio, Nougat, Stream, Studio, Games, P2P, Debug };',
        "Radio ViewMode")
    text = replace_once(text,
        'enum class GamesPanel { Library, Systems, Controllers, Settings };',
        'enum class GamesPanel { Library, Systems, Controllers, Settings };\n'
        'enum class RadioPanel { AM, FM, Shortwave, Weather, DAB, DRM, Internet, SDR, Favorites, Recordings };',
        "Radio panel enum")
    text = replace_once(text, '    Pixmap quiltTiles[12] = {};', '    Pixmap quiltTiles[13] = {};', "Radio quilt storage")
    text = replace_once(text,
        '    Rect homeTab, videoPlayerTab, libraryTab, discoverTab, liveTvTab, worldTvTab, nougatTab, ytdlpTab, studioTab, gamesTab, debugTab;\n'
        '    Rect studioSplitFileBtn, studioSplitFolderBtn, studioReassembleBtn, studioVerifyBtn;',
        '    Rect homeTab, videoPlayerTab, libraryTab, discoverTab, liveTvTab, worldTvTab, radioTab, nougatTab, ytdlpTab, studioTab, gamesTab, debugTab;\n'
        '    Rect studioSplitFileBtn, studioSplitFolderBtn, studioReassembleBtn, studioVerifyBtn;\n'
        '    Rect radioAmBtn, radioFmBtn, radioShortwaveBtn, radioWeatherBtn, radioDabBtn;\n'
        '    Rect radioDrmBtn, radioInternetBtn, radioSdrBtn, radioFavoritesBtn, radioRecordingsBtn, radioListBox;\n'
        '    RadioPanel radioPanel = RadioPanel::FM;',
        "Radio members")
    text = replace_once(text,
        '    std::vector<Rect> liveTvTunerHitboxes;',
        '    std::vector<LiveTvHitbox> liveTvTunerHitboxes;',
        "physical tuner hitbox indexing")
    text = replace_once(text,
        '            case ViewMode::WorldTV: return 11;\n            case ViewMode::Nougat: return 4;',
        '            case ViewMode::WorldTV: return 11;\n            case ViewMode::Radio: return 12;\n            case ViewMode::Nougat: return 4;',
        "Radio quilt index")
    text = replace_once(text,
        '            case ViewMode::WorldTV:     r=201; g=116; b=38;  blendPercent=56; break; // World TV orange\n'
        '            case ViewMode::Nougat:',
        '            case ViewMode::WorldTV:     r=201; g=116; b=38;  blendPercent=56; break; // World TV orange\n'
        '            case ViewMode::Radio:       r=112; g=38;  b=88;  blendPercent=58; break; // Mulberry radio\n'
        '            case ViewMode::Nougat:',
        "Radio quilt tint")

    # Insert palette after exact World TV palette return.
    world_palette = '''        if (view == ViewMode::WorldTV) return {
            rgb8(139,72,24), rgb8(169,91,30), rgb8(244,232,205),
            rgb8(96,48,16), cream, rgb8(239,211,183),
            rgb8(202,116,38), rgb8(184,98,30), rgb8(100,49,15),
            rgb8(232,154,78), cream, rgb8(218,127,43)};'''
    radio_palette = world_palette + '''
        if (view == ViewMode::Radio) return {
            rgb8(112,38,88), rgb8(143,52,111), rgb8(244,232,205),
            rgb8(78,25,61), cream, rgb8(232,199,222),
            rgb8(170,72,137), rgb8(151,60,119), rgb8(78,25,61),
            rgb8(199,111,167), cream, rgb8(222,151,70)};'''
    text = replace_once(text, world_palette, radio_palette, "Mulberry Radio palette")

    text = replace_once(text,
        '''        const ViewMode views[12] = {
            ViewMode::Home, ViewMode::VideoPlayer, ViewMode::Library, ViewMode::Discover,
            ViewMode::Nougat, ViewMode::Stream, ViewMode::P2P, ViewMode::Debug, ViewMode::LiveTV,
            ViewMode::Studio, ViewMode::Games, ViewMode::WorldTV
        };
        for (int i=0; i<12; ++i) quiltTiles[i] = create_quilt_tile(views[i]);''',
        '''        const ViewMode views[13] = {
            ViewMode::Home, ViewMode::VideoPlayer, ViewMode::Library, ViewMode::Discover,
            ViewMode::Nougat, ViewMode::Stream, ViewMode::P2P, ViewMode::Debug, ViewMode::LiveTV,
            ViewMode::Studio, ViewMode::Games, ViewMode::WorldTV, ViewMode::Radio
        };
        for (int i=0; i<13; ++i) quiltTiles[i] = create_quilt_tile(views[i]);''',
        "Radio quilt initialization")
    text = replace_once(text,
        '               view == ViewMode::LiveTV || view == ViewMode::WorldTV || view == ViewMode::Nougat ||',
        '               view == ViewMode::LiveTV || view == ViewMode::WorldTV || view == ViewMode::Radio || view == ViewMode::Nougat ||',
        "Radio page frame")
    text = replace_once(text,
        '''        if (currentView == ViewMode::Home || currentView == ViewMode::Library || currentView == ViewMode::Discover ||
            currentView == ViewMode::Nougat || currentView == ViewMode::Stream || currentView == ViewMode::Studio || currentView == ViewMode::Games || currentView == ViewMode::P2P ||
            currentView == ViewMode::Debug || currentView == ViewMode::LiveTV || currentView == ViewMode::WorldTV) {''',
        '''        if (currentView == ViewMode::Home || currentView == ViewMode::Library || currentView == ViewMode::Discover ||
            currentView == ViewMode::Radio || currentView == ViewMode::Nougat || currentView == ViewMode::Stream || currentView == ViewMode::Studio || currentView == ViewMode::Games || currentView == ViewMode::P2P ||
            currentView == ViewMode::Debug || currentView == ViewMode::LiveTV || currentView == ViewMode::WorldTV) {''',
        "Radio must unmap video child")
    # Top-level navigation has one source of truth.  The scroll extent is derived
    # from the actual tab list/geometry so adding Radio (or a later tab) cannot
    # leave an old count behind and strand the final System tab offscreen.
    text = replace_once(text,
        '        const int topControlCount = 11;',
        '''        Rect* const topTabs[] = {
            &homeTab, &videoPlayerTab, &libraryTab, &discoverTab, &liveTvTab, &worldTvTab,
            &radioTab, &nougatTab, &ytdlpTab, &studioTab, &gamesTab, &debugTab
        };
        const int topControlCount = static_cast<int>(sizeof(topTabs) / sizeof(topTabs[0]));''',
        "dynamic top navigation tab source")
    text = replace_once(text,
        '''        homeTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        videoPlayerTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        libraryTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        discoverTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        liveTvTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        worldTvTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        nougatTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        ytdlpTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        studioTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        gamesTab = {topX,1,kTopTabW,kTopTabH}; topX += topStep;
        debugTab = {topX,1,kTopTabW,kTopTabH};''',
        '''        for (Rect* tab : topTabs) {
            *tab = {topX,1,kTopTabW,kTopTabH};
            topX += topStep;
        }''',
        "dynamic top navigation layout")
    world_layout = '''        const Rect worldFrame = page_content_frame(ViewMode::WorldTV);
        worldTvPlayBtn = {worldFrame.x + 16, kPageControlY, kCompactButtonW, kCompactButtonH};
        worldTvOfficialBtn = {worldTvPlayBtn.x + kCompactButtonW + 6, kPageControlY, kCompactButtonW, kCompactButtonH};
        worldTvListBox = {worldFrame.x + 16, 88, std::max(180, worldFrame.w - 32), std::max(120, worldFrame.y + worldFrame.h - 100)};'''
    radio_layout = world_layout + '''
        const Rect radioFrame = page_content_frame(ViewMode::Radio);
        const int radioGap = 4;
        int radioX = radioFrame.x + 16;
        radioAmBtn = {radioX,kPageControlY,kCompactButtonW,kCompactButtonH}; radioX += kCompactButtonW + radioGap;
        radioFmBtn = {radioX,kPageControlY,kCompactButtonW,kCompactButtonH}; radioX += kCompactButtonW + radioGap;
        radioShortwaveBtn = {radioX,kPageControlY,kCompactButtonW,kCompactButtonH}; radioX += kCompactButtonW + radioGap;
        radioWeatherBtn = {radioX,kPageControlY,kCompactButtonW,kCompactButtonH}; radioX += kCompactButtonW + radioGap;
        radioDabBtn = {radioX,kPageControlY,kCompactButtonW,kCompactButtonH};
        radioX = radioFrame.x + 16;
        const int radioY2 = kPageControlY + kCompactButtonH + 6;
        radioDrmBtn = {radioX,radioY2,kCompactButtonW,kCompactButtonH}; radioX += kCompactButtonW + radioGap;
        radioInternetBtn = {radioX,radioY2,kCompactButtonW,kCompactButtonH}; radioX += kCompactButtonW + radioGap;
        radioSdrBtn = {radioX,radioY2,kCompactButtonW,kCompactButtonH}; radioX += kCompactButtonW + radioGap;
        radioFavoritesBtn = {radioX,radioY2,kCompactButtonW,kCompactButtonH}; radioX += kCompactButtonW + radioGap;
        radioRecordingsBtn = {radioX,radioY2,kCompactButtonW,kCompactButtonH};
        radioListBox = {radioFrame.x + 16, radioY2 + kCompactButtonH + 12,
                        std::max(180, radioFrame.w - 32),
                        std::max(120, radioFrame.y + radioFrame.h - (radioY2 + kCompactButtonH + 24))};'''
    text = replace_once(text, world_layout, radio_layout, "Radio page layout")

    text = replace_once(text,
        '        draw_tab(worldTvTab,"World TV",ViewMode::WorldTV);\n'
        '        draw_tab(nougatTab,"Search",ViewMode::Nougat);',
        '        draw_tab(worldTvTab,"World TV",ViewMode::WorldTV);\n'
        '        draw_tab(radioTab,"Radio",ViewMode::Radio);\n'
        '        draw_tab(nougatTab,"Search",ViewMode::Nougat);',
        "Radio top tab")
    text = replace_once(text,
        '            case ViewMode::WorldTV: tab = &worldTvTab; break;\n            case ViewMode::Nougat:',
        '            case ViewMode::WorldTV: tab = &worldTvTab; break;\n            case ViewMode::Radio: tab = &radioTab; break;\n            case ViewMode::Nougat:',
        "Radio active tab")
    text = replace_once(text,
        '&homeTab,&videoPlayerTab,&libraryTab,&discoverTab,&liveTvTab,&worldTvTab,&nougatTab,&ytdlpTab,&studioTab,&gamesTab,&debugTab,',
        '&homeTab,&videoPlayerTab,&libraryTab,&discoverTab,&liveTvTab,&worldTvTab,&radioTab,&nougatTab,&ytdlpTab,&studioTab,&gamesTab,&debugTab,',
        "Radio hover target")
    text = replace_function(text, '    void scroll_top_navigation(int delta)', r'''    int top_navigation_max_scroll() const {
        // The last rendered top-level tab is the authority.  Add the current
        // scroll offset back to recover its unscrolled right edge.
        const int contentRight = debugTab.x + debugTab.w + topNavScrollX;
        return std::max(0, contentRight - topNavClipRight);
    }

    void scroll_top_navigation(int delta) {
        const int maximum = top_navigation_max_scroll();
        topNavScrollX = std::max(0, std::min(topNavScrollX + delta, maximum));
        layout();
        redraw();
    }''')
    text = replace_once(text,
        '''        if (topNavHit && worldTvTab.contains(x,y)) {
            if (currentView != ViewMode::WorldTV) switch_view(ViewMode::WorldTV);
            return;
        }
        if (topNavHit && nougatTab.contains(x,y)) {''',
        '''        if (topNavHit && worldTvTab.contains(x,y)) {
            if (currentView != ViewMode::WorldTV) switch_view(ViewMode::WorldTV);
            return;
        }
        if (topNavHit && radioTab.contains(x,y)) {
            if (currentView != ViewMode::Radio) switch_view(ViewMode::Radio);
            return;
        }
        if (topNavHit && nougatTab.contains(x,y)) {''',
        "Radio navigation click")
    text = replace_once(text,
        '        case ViewMode::WorldTV: return "World TV";\n        case ViewMode::Nougat:',
        '        case ViewMode::WorldTV: return "World TV";\n        case ViewMode::Radio: return "Radio";\n        case ViewMode::Nougat:',
        "Radio current-view name")
    text = replace_once(text,
        '        if (currentView == ViewMode::WorldTV) draw_world_tv_screen(buffer);\n'
        '        if (currentView == ViewMode::Nougat) draw_nougat_screen(buffer);',
        '        if (currentView == ViewMode::WorldTV) draw_world_tv_screen(buffer);\n'
        '        if (currentView == ViewMode::Radio) draw_radio_screen(buffer);\n'
        '        if (currentView == ViewMode::Nougat) draw_nougat_screen(buffer);',
        "Radio redraw")
    text = replace_once(text,
        '''        if (currentView == ViewMode::WorldTV) {
            handle_world_tv_click(x,y,eventTime);
            return;
        }
        if (currentView == ViewMode::Games) {''',
        '''        if (currentView == ViewMode::WorldTV) {
            handle_world_tv_click(x,y,eventTime);
            return;
        }
        if (currentView == ViewMode::Radio) {
            handle_radio_click(x,y);
            return;
        }
        if (currentView == ViewMode::Games) {''',
        "Radio content click")

    radio_functions = r'''    static const char* radio_panel_name(RadioPanel panel) {
        switch (panel) {
            case RadioPanel::AM: return "AM";
            case RadioPanel::FM: return "FM";
            case RadioPanel::Shortwave: return "Shortwave";
            case RadioPanel::Weather: return "Weather";
            case RadioPanel::DAB: return "DAB / DAB+";
            case RadioPanel::DRM: return "DRM";
            case RadioPanel::Internet: return "Internet Radio";
            case RadioPanel::SDR: return "SDR";
            case RadioPanel::Favorites: return "Favorites";
            case RadioPanel::Recordings: return "Recordings";
        }
        return "Radio";
    }

    void draw_radio_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Radio);
        const Rect frame = page_content_frame(ViewMode::Radio);
        draw_quilted_background(target, frame, ViewMode::Radio);
        const auto drawRadio = [&](const Rect& r, const char* label, RadioPanel panel) {
            button_on_state(target, r, label,
                radioPanel == panel ? SheetControlState::Hover : SheetControlState::Normal);
        };
        drawRadio(radioAmBtn,"AM",RadioPanel::AM);
        drawRadio(radioFmBtn,"FM",RadioPanel::FM);
        drawRadio(radioShortwaveBtn,"Shortwave",RadioPanel::Shortwave);
        drawRadio(radioWeatherBtn,"Weather",RadioPanel::Weather);
        drawRadio(radioDabBtn,"DAB / DAB+",RadioPanel::DAB);
        drawRadio(radioDrmBtn,"DRM",RadioPanel::DRM);
        drawRadio(radioInternetBtn,"Internet Radio",RadioPanel::Internet);
        drawRadio(radioSdrBtn,"SDR",RadioPanel::SDR);
        drawRadio(radioFavoritesBtn,"Favorites",RadioPanel::Favorites);
        drawRadio(radioRecordingsBtn,"Recordings",RadioPanel::Recordings);
        draw_primary_panel(target, radioListBox, palette);
        section_text(target, radioListBox.x + 14, radioListBox.y + 28,
                     std::string("RADIO • ") + radio_panel_name(radioPanel), palette.text);
        text(target, radioListBox.x + 14, radioListBox.y + 56,
             "Nougat only enables radio modes backed by a detected compatible provider/device.", palette.muted);
        text(target, radioListBox.x + 14, radioListBox.y + 86,
             "Reception profile: Short Range • Normal • Long Range (hardware capability dependent).", palette.text);
        text(target, radioListBox.x + 14, radioListBox.y + 124,
             radioPanel == RadioPanel::Favorites ? "No saved radio favorites yet." :
             (radioPanel == RadioPanel::Recordings ? "No radio recordings yet." :
              "No compatible provider is active for this radio mode yet."), palette.muted);
    }

    void handle_radio_click(int x, int y) {
        if (radioAmBtn.contains(x,y)) radioPanel=RadioPanel::AM;
        else if (radioFmBtn.contains(x,y)) radioPanel=RadioPanel::FM;
        else if (radioShortwaveBtn.contains(x,y)) radioPanel=RadioPanel::Shortwave;
        else if (radioWeatherBtn.contains(x,y)) radioPanel=RadioPanel::Weather;
        else if (radioDabBtn.contains(x,y)) radioPanel=RadioPanel::DAB;
        else if (radioDrmBtn.contains(x,y)) radioPanel=RadioPanel::DRM;
        else if (radioInternetBtn.contains(x,y)) radioPanel=RadioPanel::Internet;
        else if (radioSdrBtn.contains(x,y)) radioPanel=RadioPanel::SDR;
        else if (radioFavoritesBtn.contains(x,y)) radioPanel=RadioPanel::Favorites;
        else if (radioRecordingsBtn.contains(x,y)) radioPanel=RadioPanel::Recordings;
        else return;
        redraw();
    }

'''
    marker = '    void draw_world_tv_screen(Drawable target)'
    need(text.count(marker) == 1, "World TV function insertion anchor mismatch")
    text = text.replace(marker, radio_functions + marker, 1)

    # File Splitter UI becomes one direct input workflow.
    text = replace_function(text, '    void handle_studio_click(int x, int y)', r'''    void handle_studio_click(int x, int y) {
        if (studioSplitFileBtn.contains(x,y)) { launch_studio_splitter_action("split"); return; }
        if (studioReassembleBtn.contains(x,y)) { launch_studio_splitter_action("reassemble"); return; }
        if (studioVerifyBtn.contains(x,y)) { launch_studio_splitter_action("verify"); return; }
    }''')
    text = replace_function(text, '    void draw_studio_screen(Drawable target)', r'''    void draw_studio_screen(Drawable target) {
        const ViewPalette palette = palette_for(ViewMode::Studio);
        draw_quilted_background(target, {0,kTopBarH,W,H-kTopBarH}, ViewMode::Studio);
        section_text(target, 28, 70, "STUDIO", palette.text);
        text(target, 28, 96, "Nougat creation, production, and media-processing workspace.", palette.muted);
        Rect panel{28,118,std::max(240,W-56),std::max(220,H-148)};
        draw_primary_panel(target,panel,palette);
        text(target,panel.x+16,panel.y+30,"File Splitter / Reassembler",palette.text);
        text(target,panel.x+16,panel.y+52,
             "Choose a normal folder, normal file, or existing ZIP. Nougat handles ZIP packaging and split pieces for you.",palette.muted);
        const int buttonY=panel.y+76;
        const int gap=10;
        const int buttonW=std::max(130,std::min(190,(panel.w-52)/3));
        studioSplitFileBtn={panel.x+16,buttonY,buttonW,32};
        studioSplitFolderBtn={0,0,0,0};
        studioReassembleBtn={studioSplitFileBtn.x+buttonW+gap,buttonY,buttonW,32};
        studioVerifyBtn={studioReassembleBtn.x+buttonW+gap,buttonY,buttonW,32};
        button_on(target,studioSplitFileBtn,"Split Folder / File / ZIP");
        button_on(target,studioReassembleBtn,"Reassemble");
        button_on(target,studioVerifyBtn,"Verify Parts");
        text(target,panel.x+16,buttonY+62,head_to_width(studioStatus,panel.w-32),palette.text);
        text(target,panel.x+16,buttonY+88,
             "You choose the output name and number of pieces. An optional maximum piece size drives mathematical minimum-piece suggestions.",palette.muted);
    }''')

    # Unified startup tuner discovery, including network tuners even with no cached DVB channels.
    text = replace_once(text, '''        // v0.0.42: silently discover an already-connected tuner at startup so
        // stale guide maintenance can happen without forcing a visit to Live TV.
        if (!liveTvChannels.empty()) {
            std::string tunerStatus;
            liveTvTuners = tunerBackend.detect(tunerStatus);
            if (!liveTvTuners.empty()) liveTvSelectedTuner = 0;
        }''',
        '''        // v0.0.51: provider-neutral startup discovery. LAN tuners are
        // discovered even when the legacy DVB channel cache is empty.
        refresh_live_tv_tuners(false);''',
        "provider-neutral startup tuner discovery")

    refresh = r'''    void refresh_live_tv_tuners(bool announce=true) {
        std::string dvbStatus;
        std::string hdhrStatus;
        liveTvTuners = tunerBackend.detect(dvbStatus);
        std::vector<reddmedia::TunerDevice> networkTuners = hdHomeRunProvider.detect(hdhrStatus);
        liveTvTuners.insert(liveTvTuners.end(), networkTuners.begin(), networkTuners.end());

        liveTvChannels = tunerBackend.load_channels();
        std::set<std::string> loadedDevices;
        for (const auto& tuner : networkTuners) {
            std::string deviceId;
            int tunerIndex=-1;
            if (!reddmedia::HdHomeRunProvider::decode_tuner_id(tuner,deviceId,tunerIndex)) continue;
            if (!loadedDevices.insert(deviceId).second) continue;
            std::vector<reddmedia::LiveTvChannel> lineup;
            std::string lineupStatus;
            if (hdHomeRunProvider.load_lineup(tuner,lineup,lineupStatus)) merge_live_tv_channels(lineup);
        }

        restore_live_tv_last_channel();
        if (liveTvTuners.empty()) liveTvSelectedTuner=-1;
        else if (liveTvSelectedTuner<0 || liveTvSelectedTuner>=static_cast<int>(liveTvTuners.size()) ||
                 !liveTvTuners[static_cast<std::size_t>(liveTvSelectedTuner)].readable) {
            liveTvSelectedTuner=-1;
            for (int i=0;i<static_cast<int>(liveTvTuners.size());++i) {
                if (liveTvTuners[static_cast<std::size_t>(i)].readable) { liveTvSelectedTuner=i; break; }
            }
        }

        if (liveTvTuners.empty()) {
            liveTvStatus="No compatible TV tuner detected.";
            if (announce) liveTvStatus += " Connect a supported USB/local tuner or HDHomeRun and press Detect Tuner again.";
            return;
        }
        std::set<std::string> hdhrDevices;
        int hdhrResources=0;
        int localResources=0;
        for (const auto& tuner : liveTvTuners) {
            if (reddmedia::HdHomeRunProvider::is_hdhomerun_tuner(tuner)) {
                std::string deviceId; int tunerIndex=-1;
                if (reddmedia::HdHomeRunProvider::decode_tuner_id(tuner,deviceId,tunerIndex)) {
                    hdhrDevices.insert(deviceId); ++hdhrResources;
                }
            } else ++localResources;
        }
        std::ostringstream status;
        bool wrote=false;
        if (!hdhrDevices.empty()) {
            status << "HDHomeRun: " << hdhrDevices.size() << " device" << (hdhrDevices.size()==1U?"":"s")
                   << " • " << hdhrResources << " tuner" << (hdhrResources==1?"":"s");
            wrote=true;
        }
        if (localResources>0) {
            if (wrote) status << " • ";
            status << localResources << " local/USB tuner" << (localResources==1?"":"s");
        }
        status << " available.";
        liveTvStatus=status.str();
    }'''
    text = replace_function(text, '    void refresh_live_tv_tuners(bool announce=true)', refresh)

    # One physical HDHomeRun device card, nested physical tuner resources.
    tuner_block = r'''        if (liveTvTunersMode) {
            const int top=liveTvListBox.y+66;
            if (liveTvTuners.empty()) {
                text(target,liveTvListBox.x+14,top+20,"No compatible TV tuner detected. Press Detect Tuner above.",palette.text);
                return;
            }
            std::set<std::string> renderedHdhrDevices;
            int y=top;
            for (std::size_t i=0;i<liveTvTuners.size();++i) {
                const auto& tuner=liveTvTuners[i];
                if (reddmedia::HdHomeRunProvider::is_hdhomerun_tuner(tuner)) {
                    std::string deviceId; int tunerIndex=-1;
                    if (!reddmedia::HdHomeRunProvider::decode_tuner_id(tuner,deviceId,tunerIndex)) continue;
                    if (!renderedHdhrDevices.insert(deviceId).second) continue;
                    std::vector<int> resources;
                    for (int j=0;j<static_cast<int>(liveTvTuners.size());++j) {
                        std::string candidate; int physical=-1;
                        if (reddmedia::HdHomeRunProvider::decode_tuner_id(liveTvTuners[static_cast<std::size_t>(j)],candidate,physical) && candidate==deviceId)
                            resources.push_back(j);
                    }
                    const int cardH=66+static_cast<int>(resources.size())*24;
                    Rect card{liveTvListBox.x+10,y,std::max(120,liveTvListBox.w-20),cardH};
                    const bool selected=std::find(resources.begin(),resources.end(),liveTvSelectedTuner)!=resources.end();
                    fill_round(target,card,7,selected?palette.selection:palette.panel);
                    outline_round(target,card,7,palette.border);
                    std::string title=tuner.name.empty()?"HDHomeRun":tuner.name;
                    const std::size_t suffix=title.find(" / Tuner ");
                    if (suffix!=std::string::npos) title.erase(suffix);
                    text(target,card.x+12,card.y+20,head_to_width(title,card.w-24),palette.text);
                    text(target,card.x+12,card.y+40,
                         head_to_width("Device ID: "+deviceId+" • Address: "+tuner.frontend_path+" • Tuners: "+std::to_string(resources.size()),card.w-24),palette.muted);
                    int lineY=card.y+62;
                    for (int resourceIndex : resources) {
                        const auto& resource=liveTvTuners[static_cast<std::size_t>(resourceIndex)];
                        reddmedia::HdHomeRunTunerStatus runtime; std::string probe;
                        const bool accessible=hdHomeRunProvider.probe_runtime_status(resource,runtime,probe);
                        std::string rid; int physical=-1;
                        reddmedia::HdHomeRunProvider::decode_tuner_id(resource,rid,physical);
                        std::string state=!accessible?"Not accessible":(runtime.busy?"In use":"Available");
                        if (accessible && !runtime.channel.empty() && runtime.channel!="none") state += " • "+runtime.channel;
                        text(target,card.x+22,lineY,"Tuner "+std::to_string(physical+1)+": "+state,
                             accessible?palette.text:rgb8(170,50,40));
                        lineY+=24;
                    }
                    liveTvTunerHitboxes.push_back({card,resources.empty()?static_cast<int>(i):resources.front()});
                    y+=cardH+8;
                } else {
                    Rect card{liveTvListBox.x+10,y,std::max(120,liveTvListBox.w-20),86};
                    fill_round(target,card,7,static_cast<int>(i)==liveTvSelectedTuner?palette.selection:palette.panel);
                    outline_round(target,card,7,palette.border);
                    text(target,card.x+12,card.y+20,head_to_width(tuner.name.empty()?tuner.id:tuner.name,card.w-24),palette.text);
                    text(target,card.x+12,card.y+42,head_to_width("Frontend: "+tuner.frontend_path,card.w-24),palette.muted);
                    text(target,card.x+12,card.y+64,tuner.readable?"Device access: Ready":"Device access: Not readable",
                         tuner.readable?palette.text:rgb8(170,50,40));
                    liveTvTunerHitboxes.push_back({card,static_cast<int>(i)});
                    y+=94;
                }
                if (y+86>liveTvListBox.y+liveTvListBox.h-8) break;
            }
            return;
        }'''
    text = replace_block_in_function(text, '    void draw_live_tv_screen(Drawable target)', '        if (liveTvTunersMode) {', tuner_block)

    tuner_click = r'''        if (liveTvTunersMode) {
            for (const auto& hit : liveTvTunerHitboxes) {
                if (!hit.rect.contains(x,y)) continue;
                const int index=hit.channel_index;
                if (index<0 || index>=static_cast<int>(liveTvTuners.size())) return;
                liveTvSelectedTuner=index;
                liveTvStatus="Selected tuner device: "+(liveTvTuners[static_cast<std::size_t>(index)].name.empty()?liveTvTuners[static_cast<std::size_t>(index)].id:liveTvTuners[static_cast<std::size_t>(index)].name)+".";
                redraw();
                return;
            }
            return;
        }'''
    text = replace_block_in_function(text, '    void handle_live_tv_click(int x,int y, Time eventTime)', '        if (liveTvTunersMode) {', tuner_click)

    # World TV probe/guide/artwork workers are self-contained.  On application
    # teardown they must not hold the GUI shutdown path for the full network
    # timeout; detach them instead of synchronously joining network workers.
    text = replace_once(text,
        '''        if (worldTvResolveWorker.joinable()) worldTvResolveWorker.join();
        if (worldTvArtworkWorker.joinable()) worldTvArtworkWorker.join();
        if (worldTvGuideWorker.joinable()) worldTvGuideWorker.join();''',
        '''        if (worldTvResolveWorker.joinable()) worldTvResolveWorker.detach();
        if (worldTvArtworkWorker.joinable()) worldTvArtworkWorker.detach();
        if (worldTvGuideWorker.joinable()) worldTvGuideWorker.detach();''',
        "World TV shutdown must not block on network workers")

    # World TV uses the same guide/timeline grammar as Live TV, with orange data/palette.
    world_guide = r'''    void draw_world_tv_screen(Drawable target) {
        const ViewPalette palette=palette_for(ViewMode::WorldTV);
        const Rect frame=page_content_frame(ViewMode::WorldTV);
        draw_quilted_background(target,frame,ViewMode::WorldTV);
        button_on(target,worldTvPlayBtn,"Watch Live");
        button_on(target,worldTvOfficialBtn,"Official Site");
        draw_primary_panel(target,worldTvListBox,palette);
        worldTvHitboxes.clear(); worldTvRowStationIndices.clear();
        section_text(target,worldTvListBox.x+12,worldTvListBox.y+24,"WORLD TV GUIDE",palette.text);
        text(target,worldTvListBox.x+12,worldTvListBox.y+48,head_to_width("Status: "+worldTvStatus,worldTvListBox.w-24),palette.text);

        const auto& stations=world_tv_catalog();
        const auto visibleStations=world_tv_visible_indices();
        if (visibleStations.empty()) {
            text(target,worldTvListBox.x+14,worldTvListBox.y+112,"Loading and verifying real station artwork...",palette.text);
            return;
        }
        if (std::find(visibleStations.begin(),visibleStations.end(),worldTvSelected)==visibleStations.end())
            worldTvSelected=visibleStations.front();

        const int top=worldTvListBox.y+66;
        const int left=worldTvListBox.x+8;
        const int channelW=210;
        const int headerH=34;
        const int rowH=62;
        const int gridX=left+channelW;
        const int gridW=std::max(120,worldTvListBox.w-channelW-16);
        const int slotCount=std::max(3,std::min(8,gridW/112));
        const int slotW=std::max(1,gridW/slotCount);
        const long long now=static_cast<long long>(std::time(nullptr));
        const long long base=(now/1800LL)*1800LL;
        const long long windowEnd=base+static_cast<long long>(slotCount)*1800LL;

        fill(target,{left,top,channelW,headerH},palette.button);
        outline(target,{left,top,channelW,headerH},palette.border);
        text(target,left+8,top+22,"CHANNEL",palette.buttonText);
        for (int slot=0;slot<slotCount;++slot) {
            Rect h{gridX+slot*slotW,top,slotW,headerH};
            fill(target,h,palette.button); outline(target,h,palette.border);
            text(target,h.x+6,h.y+22,head_to_width(live_tv_clock(base+slot*1800LL),h.w-10),palette.buttonText);
        }

        const int visible=std::max(1,(worldTvListBox.y+worldTvListBox.h-(top+headerH)-8)/rowH);
        const int maxScroll=std::max(0,static_cast<int>(visibleStations.size())-visible);
        worldTvScroll=std::max(0,std::min(worldTvScroll,maxScroll));
        for (int row=0;row<visible;++row) {
            const int position=worldTvScroll+row;
            if (position>=static_cast<int>(visibleStations.size())) break;
            const int index=visibleStations[static_cast<std::size_t>(position)];
            const auto& station=stations[static_cast<std::size_t>(index)];
            const int ry=top+headerH+row*rowH;
            Rect channel{left,ry,channelW,rowH};
            fill(target,channel,index==worldTvSelected?palette.selection:palette.panel);
            outline(target,channel,palette.border);
            const Rect logo{channel.x+5,channel.y+7,58,48};
            draw_world_tv_logo(target,logo,index);
            text(target,channel.x+68,channel.y+24,head_to_width(station.name,channel.w-74),palette.text);
            text(target,channel.x+68,channel.y+45,head_to_width(station.country+" • "+station.language,channel.w-74),palette.muted);
            worldTvHitboxes.push_back(channel); worldTvRowStationIndices.push_back(index);

            Rect rowBg{gridX,ry,gridW,rowH};
            fill(target,rowBg,palette.background); outline(target,rowBg,palette.border);
            bool drew=false;
            const auto gi=worldTvGuideCache.find(index);
            if (gi!=worldTvGuideCache.end() && gi->second.available) {
                const auto drawProgram=[&](long long start,long long end,const std::string& title,bool current) {
                    if (title.empty() || end<=start || end<=base || start>=windowEnd) return;
                    const long long a=std::max(base,start), b=std::min(windowEnd,end);
                    const int px=gridX+static_cast<int>((a-base)*gridW/(windowEnd-base));
                    const int pr=gridX+static_cast<int>((b-base)*gridW/(windowEnd-base));
                    Rect block{px+1,ry+2,std::max(34,pr-px-2),rowH-4};
                    fill(target,block,current?palette.selection:palette.button); outline(target,block,palette.border);
                    text(target,block.x+6,block.y+22,head_to_width(title,block.w-12),current?palette.text:palette.buttonText);
                    if (block.w>100) text(target,block.x+6,block.y+44,head_to_width(live_tv_clock(start)+"-"+live_tv_clock(end),block.w-12),current?palette.muted:palette.buttonText);
                    drew=true;
                };
                drawProgram(gi->second.current_start,gi->second.current_end,gi->second.current_title,true);
                drawProgram(gi->second.next_start,gi->second.next_end,gi->second.next_title,false);
            }
            if (!drew) text(target,gridX+8,ry+36,"Program guide unavailable",palette.muted);
        }
        if (base<=now && now<windowEnd) {
            const int nx=gridX+static_cast<int>((now-base)*gridW/(windowEnd-base));
            line(target,nx,top,nx,std::min(worldTvListBox.y+worldTvListBox.h-8,top+headerH+visible*rowH),rgb8(244,197,72));
        }
        draw_visible_vertical_scrollbar(target,worldTvListBox,worldTvScroll,static_cast<int>(visibleStations.size()),visible,palette);
    }'''
    text = replace_function(text, '    void draw_world_tv_screen(Drawable target)', world_guide)

    # System-wide transient overlay styling: channel/media identity, fullscreen transport,
    # and seek preview all get the same translucency policy and rounded clip mask.
    helper = r'''    void set_transient_opacity(Window window) {
        if (!window || !d) return;
        const Atom property=XInternAtom(d,"_NET_WM_WINDOW_OPACITY",False);
        const unsigned long opacity=0xD9000000UL;
        XChangeProperty(d,window,property,XA_CARDINAL,32,PropModeReplace,
                        reinterpret_cast<const unsigned char*>(&opacity),1);
    }

    void apply_rounded_transient_shape(Window window,int width,int height,int radius) {
        if (!window || !xShapeCombineMask || width<=0 || height<=0) return;
        constexpr int kShapeBounding=0;
        constexpr int kShapeSet=0;
        radius=std::max(0,std::min(radius,std::min(width,height)/2));
        Pixmap mask=XCreatePixmap(d,window,static_cast<unsigned>(width),static_cast<unsigned>(height),1);
        if (!mask) return;
        GC mgc=XCreateGC(d,mask,0,nullptr);
        if (!mgc) { XFreePixmap(d,mask); return; }
        XSetForeground(d,mgc,0); XFillRectangle(d,mask,mgc,0,0,width,height);
        XSetForeground(d,mgc,1);
        const int dia=radius*2;
        XFillRectangle(d,mask,mgc,radius,0,std::max(1,width-dia),height);
        XFillRectangle(d,mask,mgc,0,radius,width,std::max(1,height-dia));
        XFillArc(d,mask,mgc,0,0,dia,dia,90*64,90*64);
        XFillArc(d,mask,mgc,width-dia,0,dia,dia,0,90*64);
        XFillArc(d,mask,mgc,0,height-dia,dia,dia,180*64,90*64);
        XFillArc(d,mask,mgc,width-dia,height-dia,dia,dia,270*64,90*64);
        xShapeCombineMask(d,window,kShapeBounding,0,0,mask,kShapeSet);
        XFreeGC(d,mgc); XFreePixmap(d,mask);
    }

    void apply_transient_window_style(Window window,int width,int height,int radius=8) {
        set_transient_opacity(window);
        apply_rounded_transient_shape(window,width,height,radius);
    }

'''
    marker='    void apply_video_corner_shape() {'
    need(text.count(marker)==1,"transient style insertion anchor mismatch")
    text=text.replace(marker,helper+marker,1)
    text=replace_once(text,
        '''        xextHandle = dlopen("libXext.so.6", RTLD_NOW | RTLD_LOCAL);
        if (xextHandle) xShapeCombineMask = reinterpret_cast<XShapeCombineMaskFn>(dlsym(xextHandle, "XShapeCombineMask"));
        XMapWindow(d, video);''',
        '''        xextHandle = dlopen("libXext.so.6", RTLD_NOW | RTLD_LOCAL);
        if (xextHandle) xShapeCombineMask = reinterpret_cast<XShapeCombineMaskFn>(dlsym(xextHandle, "XShapeCombineMask"));
        apply_transient_window_style(videoActivityOverlayWindow,220,34,8);
        apply_transient_window_style(fullscreenTransportWindow,190,68,9);
        apply_transient_window_style(seekPreviewWindow,260,176,10);
        XMapWindow(d, video);''',
        "initial transient overlay style")
    text=replace_once(text,
        '''        XMoveResizeWindow(d,videoActivityOverlayWindow,18,18,
                          static_cast<unsigned>(overlayW),static_cast<unsigned>(overlayH));
        XMapRaised(d,videoActivityOverlayWindow);''',
        '''        XMoveResizeWindow(d,videoActivityOverlayWindow,18,18,
                          static_cast<unsigned>(overlayW),static_cast<unsigned>(overlayH));
        apply_transient_window_style(videoActivityOverlayWindow,overlayW,overlayH,8);
        XMapRaised(d,videoActivityOverlayWindow);''',
        "identity overlay rounded translucency")
    text=replace_once(text,
        '''        XMoveResizeWindow(d,fullscreenTransportWindow,overlayX,overlayY,
                          static_cast<unsigned>(overlayW),static_cast<unsigned>(overlayH));
        XMapRaised(d,fullscreenTransportWindow);''',
        '''        XMoveResizeWindow(d,fullscreenTransportWindow,overlayX,overlayY,
                          static_cast<unsigned>(overlayW),static_cast<unsigned>(overlayH));
        apply_transient_window_style(fullscreenTransportWindow,overlayW,overlayH,9);
        XMapRaised(d,fullscreenTransportWindow);''',
        "fullscreen overlay rounded translucency")

    # Retained navigation regression now proves the actual right-edge geometry
    # at several narrow widths.  It does not carry a tab count at all.
    old_nav_test = '''    if (argc > 1 && std::string(argv[1]) == "--v47-nav-self-test") {
        App app;
        app.W=1000;
        app.H=650;
        app.layout();
        app.topNavScrollX=app.clamp_button_scroll(100000,11,app.topNavViewportW);
        app.layout();
        const bool visible=app.debugTab.x>=app.topNavClipX &&
                           app.debugTab.x+app.debugTab.w<=app.topNavClipRight;
        if (!visible) {
            std::fprintf(stderr,"Nougat v0.0.47 top navigation FAIL: System tab remains clipped.\\n");
            return 1;
        }
        std::printf("Nougat v0.0.47 top navigation PASS: System tab fully reachable.\\n");
        return 0;
    }'''
    new_nav_test = '''    if (argc > 1 && std::string(argv[1]) == "--v47-nav-self-test") {
        App app;
        app.H=650;
        bool visible=true;
        for (const int width : {640, 828, 1000, 1280}) {
            app.W=width;
            app.topNavScrollX=0;
            app.layout();
            app.topNavScrollX=app.top_navigation_max_scroll();
            app.layout();
            visible = visible &&
                      app.debugTab.x>=app.topNavClipX &&
                      app.debugTab.x+app.debugTab.w<=app.topNavClipRight;
        }
        if (!visible) {
            std::fprintf(stderr,"Nougat v0.0.47 top navigation FAIL: final System tab remains clipped.\\n");
            return 1;
        }
        std::printf("Nougat v0.0.47 top navigation PASS: System tab fully reachable.\\n");
        return 0;
    }'''
    text = replace_once(text, old_nav_test, new_nav_test, "geometry-derived retained top-navigation self-test")

    path.write_text(text, encoding="utf-8")


def patch_hdhr(path: Path) -> None:
    text=path.read_text(encoding="utf-8")
    old='''    const CommandResult clear_result = config_command(shell_quote(device_id) + " set /tuner" +
        std::to_string(tuner_index) + "/channel none");
    (void)clear_result;
    if (cancelled) {
        status = "HDHomeRun channel scan cancelled.";
        return false;
    }
    if (code != 0) {
        status = "HDHomeRun channel scan failed on tuner " + std::to_string(tuner_index) + ".";
        return false;
    }

    for (auto& item : found) channels.push_back(std::move(item.second));
    std::sort(channels.begin(), channels.end(), [](const LiveTvChannel& a, const LiveTvChannel& b) { return a.id < b.id; });
    status = "HDHomeRun scan complete on tuner " + std::to_string(tuner_index) + ": " +
             std::to_string(channels.size()) + " channel(s) found.";
    return true;'''
    new='''    const CommandResult clear_result = config_command(shell_quote(device_id) + " set /tuner" +
        std::to_string(tuner_index) + "/channel none");
    if (cancelled) {
        status = "HDHomeRun channel scan cancelled.";
        return false;
    }

    const bool rf_traversal_complete = completed >= 35;
    if (code != 0 && !rf_traversal_complete) {
        status = "HDHomeRun RF scan stopped before full traversal on tuner " +
                 std::to_string(tuner_index) + " (" + std::to_string(completed) +
                 "/35 RF steps, helper exit " + std::to_string(code) + ").";
        return false;
    }

    for (auto& item : found) channels.push_back(std::move(item.second));
    std::sort(channels.begin(), channels.end(), [](const LiveTvChannel& a, const LiveTvChannel& b) {
        const auto numeric=[](const std::string& id) {
            const std::size_t dot=id.find('.');
            const int major=std::atoi(id.substr(0,dot).c_str());
            const int minor=dot==std::string::npos?0:std::atoi(id.substr(dot+1U).c_str());
            return major*1000+minor;
        };
        return numeric(a.id)<numeric(b.id);
    });
    status = "HDHomeRun RF scan complete: " + std::to_string(channels.size()) + " channel(s) found.";
    if (code != 0)
        status += " Full RF traversal completed; helper exit " + std::to_string(code) + " retained as diagnostic evidence.";
    if (clear_result.code != 0)
        status += " Tuner release needs attention: " + (clear_result.output.empty()?std::string("release command failed"):clear_result.output) + ".";
    else
        status += " Tuner released.";
    return true;'''
    text=replace_once(text,old,new,"HDHomeRun full scan truth")
    path.write_text(text,encoding="utf-8")


def patch_world_service(path: Path) -> None:
    text=path.read_text(encoding="utf-8")
    text=replace_once(text,
        'const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(32);',
        'const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(70);',
        "World TV outer resolver deadline")
    path.write_text(text,encoding="utf-8")


def patch_world_worker(path: Path) -> None:
    text=path.read_text(encoding="utf-8")
    text=replace_once(text,
        'UA = "Mozilla/5.0 (X11; Linux x86_64) NougatMediaSuite/0.0.47"',
        'UA = "Mozilla/5.0 (X11; Linux x86_64) NougatMediaSuite/0.0.51"',
        "World TV user agent version")
    replacement=r'''LAST_PROBE_REASON = ""

def ffprobe_candidate(url: str, referrer: str, user_agent: str,
                      require_audio: bool = False) -> bool:
    global LAST_PROBE_REASON
    LAST_PROBE_REASON = ""
    ffprobe = shutil.which("ffprobe")
    if not ffprobe:
        LAST_PROBE_REASON = "ffprobe is not installed"
        return False
    probe_args = [ffprobe, "-v", "error", "-rw_timeout", "7000000"]
    if user_agent:
        probe_args += ["-user_agent", user_agent]
    headers = f"Referer: {referrer}\\r\\n" if referrer else ""
    if headers:
        probe_args += ["-headers", headers]
    probe_args += ["-show_entries", "stream=codec_type,width,height,codec_name", "-of", "json", url]
    try:
        result = subprocess.run(probe_args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                text=True, timeout=8)
    except subprocess.TimeoutExpired:
        LAST_PROBE_REASON = "stream probe timed out"
        return False
    except OSError as exc:
        LAST_PROBE_REASON = f"stream probe could not start: {exc}"
        return False
    if result.returncode != 0:
        detail = clean((result.stderr or "").splitlines()[-1] if result.stderr else "")
        LAST_PROBE_REASON = "ffprobe rejected source" + (f": {detail}" if detail else "")
        return False
    try:
        payload = json.loads(result.stdout or "{}")
    except json.JSONDecodeError:
        LAST_PROBE_REASON = "ffprobe returned malformed stream metadata"
        return False
    has_video = any(isinstance(s,dict) and s.get("codec_type")=="video" and clean(s.get("codec_name")) for s in payload.get("streams",[]))
    has_audio = any(isinstance(s,dict) and s.get("codec_type")=="audio" and clean(s.get("codec_name")) for s in payload.get("streams",[]))
    if not has_video:
        LAST_PROBE_REASON = "source has no decodable video stream"
        return False
    if require_audio and not has_audio:
        LAST_PROBE_REASON = "source has video but no decodable audio stream"
        return False
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        LAST_PROBE_REASON = "verified by ffprobe; ffmpeg frame sample unavailable"
        return True
    frame_args=[ffmpeg,"-nostdin","-v","error","-rw_timeout","7000000"]
    if user_agent:
        frame_args += ["-user_agent",user_agent]
    if headers:
        frame_args += ["-headers",headers]
    frame_args += ["-i",url,"-t","3","-vf",
                   "fps=1,scale=32:18:force_original_aspect_ratio=decrease,pad=32:18:(ow-iw)/2:(oh-ih)/2,format=gray",
                   "-frames:v","3","-f","rawvideo","pipe:1"]
    try:
        sampled=subprocess.run(frame_args,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=10)
    except subprocess.TimeoutExpired:
        LAST_PROBE_REASON = "video frame sample timed out after stream metadata succeeded"
        return False
    except OSError as exc:
        LAST_PROBE_REASON = f"video frame sampler could not start: {exc}"
        return False
    frame_size=32*18
    data=sampled.stdout or b""
    if sampled.returncode != 0 or len(data) < frame_size:
        LAST_PROBE_REASON = "stream opened but no complete video frame arrived"
        return False
    frames=[data[o:o+frame_size] for o in range(0,min(len(data),frame_size*3),frame_size) if len(data[o:o+frame_size])==frame_size]
    for frame in frames:
        low=min(frame); high=max(frame); mean=sum(frame)/len(frame)
        if mean>=8.0 or high-low>=14:
            LAST_PROBE_REASON = "playable video verified"
            return True
    LAST_PROBE_REASON = "stream opened but sampled frames remained black/blank"
    return False
'''
    start=text.find('def ffprobe_candidate(')
    need(start>=0,"World TV ffprobe function missing")
    end=text.find('\ndef resolve_mode(',start)
    need(end>start,"World TV resolve function boundary missing")
    text=text[:start]+replacement+text[end:]
    text=replace_once(text,
        '    max_checks = 6 if require_audio else 3\n    deadline = time.monotonic() + (36.0 if require_audio else 24.0)',
        '    max_checks = 8 if require_audio else 6\n    deadline = time.monotonic() + (60.0 if require_audio else 48.0)',
        "World TV candidate/deadline tolerance")
    text=replace_once(text,
        '    emit(OK=0, ERROR="No playable non-YouTube direct source passed the World TV probe.")',
        '    emit(OK=0, ERROR=f"No playable direct source passed verification after {checked} checks from {len(candidates)} candidates. Last probe: {LAST_PROBE_REASON or \'no candidate completed\'}.")',
        "World TV failure evidence")
    path.write_text(text,encoding="utf-8")


def patch_lan() -> None:
    h=ROOT/'src/lan/lan_media_service.hpp'
    text=h.read_text(encoding='utf-8')
    text=replace_once(text,
        '    bool automatic_cloud_relay = false;\n    std::string discovery_name = "nougat.local";',
        '    bool automatic_cloud_relay = false;\n    bool pairing_required = true;\n    bool private_lan_only = true;\n    std::string discovery_name = "nougat.local";',
        'LAN security flags')
    h.write_text(text,encoding='utf-8')
    c=ROOT/'src/lan/lan_media_service.cpp'
    text=c.read_text(encoding='utf-8')
    old='''        {"catalog", "/nougat/v1/catalog", "Local Library catalog and collection metadata"},
        {"history", "/nougat/v1/history", "Local viewing history and resume state"},
        {"media", "/nougat/v1/media", "Direct byte-range delivery of locally owned media"},
        {"hls", "/nougat/v1/hls", "Reserved versioned HLS/transcoding surface"},
        {"pair", "/nougat/v1/pair", "Reserved local pairing/authentication surface"},
        {"web", "/", "Reserved local phone/tablet/laptop/TV browser UI"},'''
    new='''        {"health", "/nougat/v1/health", "LAN service health and version contract"},
        {"catalog", "/nougat/v1/catalog", "Local Library catalog and collection metadata"},
        {"history", "/nougat/v1/history", "Local viewing history and exact resume state"},
        {"artwork", "/nougat/v1/artwork", "Local poster, backdrop, channel and game artwork"},
        {"media", "/nougat/v1/media", "Direct byte-range delivery of locally owned media"},
        {"hls", "/nougat/v1/hls", "Versioned HLS/transcoding surface for browser-incompatible media"},
        {"livetv", "/nougat/v1/live-tv", "Local Live TV channel, guide and stream endpoint foundation"},
        {"devices", "/nougat/v1/devices", "Paired LAN device/session inventory"},
        {"session", "/nougat/v1/session", "Versioned playback/session state"},
        {"pair", "/nougat/v1/pair", "Local pairing/PIN authentication surface"},
        {"web", "/", "Local phone/tablet/laptop/TV browser UI surface"},'''
    text=replace_once(text,old,new,'LAN endpoint foundation')
    c.write_text(text,encoding='utf-8')


def patch_cmake_desktop() -> None:
    p=ROOT/'CMakeLists.txt'
    text=p.read_text(encoding='utf-8')
    text=replace_once(text,'project(NougatMediaSuite VERSION 0.0.50 LANGUAGES CXX)',
                      'project(NougatMediaSuite VERSION 0.0.51 LANGUAGES CXX)','CMake version')
    need('Nougat_Media_Suite_v50' in text,'CMake v50 target missing')
    text=text.replace('Nougat_Media_Suite_v50','Nougat_Media_Suite_v51')
    p.write_text(text,encoding='utf-8')
    for name in ('NougatMediaSuite.desktop','com.elderredsoftworks.NougatMediaSuite.desktop'):
        d=ROOT/name
        body=d.read_text(encoding='utf-8')
        body=replace_once(body,'Nougat_Media_Suite_v50','Nougat_Media_Suite_v51',name+' Exec')
        body=replace_once(body,'Icon=nougat-media-suite-concept-sheet-v24','Icon=nougat-media-suite-v51',name+' icon')
        d.write_text(body,encoding='utf-8')


def patch_docs() -> None:
    scope=ROOT/'docs/builds/NOUGAT_MEDIA_SUITE_v0_0_51_SCOPE.md'
    scope.parent.mkdir(parents=True,exist_ok=True)
    scope.write_text('''# Nougat Media Suite v0.0.51 Candidate Scope\n\nBase: accepted/published v0.0.50 commit `45f163752e5f1e0ed00f7d6d851bb6f6a5abf96e`.\n\n## Candidate work\n\n- Repairs the File Splitter around direct folder, normal file, and existing ZIP input; owner-selected output name; owner-selected piece count; mathematically derived minimum recommendation when an explicit maximum-piece setting requires it; `.zip.001` naming; SHA-256 verification; exact packaged reassembly.\n- Repairs HDHomeRun physical-device presentation so one FLEX-class unit is one device card with nested tuner resources, makes detection/status provider-neutral, and separates completed RF traversal from helper/cleanup status.\n- Rebuilds World TV around the Live-TV-style timeline guide geometry using World TV orange data/palette and improves direct-stream resolution tolerance/evidence; source verification stays off the X11 event thread and teardown does not synchronously wait on the long-running World TV network workers.\n- Makes translucent rounded treatment a system-wide policy for transient player overlays rather than a World-TV-only exception.\n- Adds the top-level Mulberry Radio foundation with AM, FM, Shortwave, Weather, DAB/DAB+, DRM, Internet Radio, SDR, Favorites, and Recordings divisions. Radio is an independent root view and does not map the Video Player/resume child merely by opening Radio. Hardware-dependent modes stay unavailable without a real supporting provider/device.\n- Advances the LAN Web Viewer versioned endpoint foundation for health, catalog, history, artwork, media, HLS, Live TV, devices/session, pairing and browser UI while preserving LAN-only/no-cloud defaults.\n- Replaces the old Nougat brand mark with the owner-approved v0.0.51 artwork: the exact N crop is the executable/window/launcher/sidebar icon, and the exact N + cursive `Nougat Media Suite` lockup is scaled into the top application bar.\n- Corrects all current executable/diagnostic/header identity to v0.0.51, repairs the N alpha silhouette so both bottom corners outside the rounded badge are transparent, and makes successful promotion replace v50 rather than leaving old and new root executables side by side.\n- Repairs the top-level horizontal navigation structurally: the tab list is one source of truth, the maximum scroll is derived from the rendered final System tab, and narrow-width regression checks require the complete last tab to be reachable.\n\n## Controller roadmap locked by owner\n\nNougat will gain a unified Controller & Remote Input Framework for the entire app. Controller setup belongs in the System tab. D-pad/left-stick navigation, A/Cross Select, B/Circle Back/Cancel, tab/page actions, subtitles/audio/player controls, remapping, dead zones, sensitivity and controller testing feed a common action abstraction. UI, Video Player, Games and Drone Flight are separate contexts. Drone Flight owns flight axes exclusively while armed so those inputs cannot simultaneously navigate Nougat.\n\nNo Git commit/tag/GitHub push is part of candidate construction. Owner testing and acceptance come first.\n''',encoding='utf-8')
    prepend_after_title(ROOT/'CHANGELOG.md','## v0.0.51 candidate','''
- Repairs v0.0.50 File Splitter, HDHomeRun grouping/full-scan status, World TV guide/reliability, version identity and transient overlay clipping/opacity.
- Keeps World TV source verification off the X11 event thread and prevents application teardown from blocking on long-running World TV resolver/guide/artwork network workers.
- Adds the Mulberry Radio top-level foundation as an independent root view that cannot expose the Video Player resume child simply by opening Radio.
- Repairs top-level navigation so its full horizontal scroll extent is derived from the actual rendered final System tab rather than a stale hard-coded count.
- Replaces the old N with the owner-approved new N everywhere, repairs the rounded icon alpha silhouette at both bottom corners, and uses the approved cursive brand lockup in the application header.
- Enforces replacement-style root promotion so v50 cannot remain beside v51 after a successful candidate apply, while preserving v50 in rollback evidence.
- Records the Nougat-wide controller framework roadmap with configuration under System and separate Drone Flight context.
- Candidate stays uncommitted/unpushed until owner acceptance.
''')
    prepend_after_title(ROOT/'ROADMAP.md','## v0.0.51 and forward: Controller, Radio, LAN Viewer, and Aerial Production','''
- **Unified Controller & Remote Input Framework:** app-wide controller navigation, not a drone-only input stack. Controller setup is in the **System tab**. Default vocabulary includes D-pad/left stick navigation, A/Cross Select, B/Circle Back/Cancel, tab/page movement, player seek/play, subtitles/audio and context actions, with remapping, dead zones, sensitivity and test input. Separate UI, Video Player, Games and **Drone Flight** contexts. Drone Flight receives exclusive flight-axis ownership while armed.
- **Radio:** top-level Mulberry Radio area with AM, FM, Shortwave, Weather, DAB/DAB+, DRM, Internet Radio, SDR, Favorites and Recordings. Capability-gated by actual hardware/providers. Reception controls roadmap includes Short Range, Normal and Long Range antenna modes where hardware supports or user metadata makes them meaningful.
- **Regional tuner profiles:** keep Live TV provider-neutral for the home HDHomeRun FLEX DUO and portable Hauppauge/other regional USB tuners, including future Turkey-compatible DVB hardware. Frequency plan and scan behavior belong to provider/region capability data, not WinTV-specific assumptions.
- **LAN Web Viewer:** versioned local catalog/history/artwork/media/Live-TV/device/session/pairing/health/browser contracts, LAN-only default, no cloud-login requirement, no automatic port forwarding, no external relay.
- **Studio Aerial Production:** DJI/open-autopilot ground-control integration, gamepad/PS5 DualSense control, repeatable cinematic flight/gimbal/camera paths, live video/telemetry and synchronized production metadata. Flight controls remain in Aerial Production; System owns controller configuration.
''')
    prepend_after_title(ROOT/'DEPENDENCIES.md','## v0.0.51 candidate dependency notes','''
- No new linked controller library is introduced in v0.0.51; controller support is roadmap architecture only in this candidate.
- Studio File Splitter uses Python 3 standard-library ZIP64/ZIP handling and Zenity for the current desktop chooser/dialog flow.
- HDHomeRun continues to use `hdhomerun_config` and `curl` behind Nougat's provider boundary.
- World TV continues to use `ffprobe`/`ffmpeg`, with longer bounded verification windows and evidence-bearing failure messages.
- The approved v0.0.51 N and brand lockup are bundled raster assets; no runtime font dependency is required.
''')


def main() -> int:
    try:
        patch_main(ROOT/'src/main.cpp')
        patch_hdhr(ROOT/'src/live_tv/hdhomerun_provider.cpp')
        patch_world_service(ROOT/'src/world_tv/world_tv_service.cpp')
        patch_world_worker(ROOT/'components/world_tv/nougat_world_tv_worker.py')
        patch_lan()
        patch_cmake_desktop()
        patch_docs()
        print('PASS: v0.0.51 source patch applied')
        return 0
    except Exception as exc:
        print('FAIL:',exc)
        return 1

if __name__=='__main__':
    raise SystemExit(main())
