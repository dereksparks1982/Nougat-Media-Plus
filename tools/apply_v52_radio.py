#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys

ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]).resolve()
PAYLOAD = Path(__file__).resolve().parents[1]


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
                if depth == 0: return i
        elif state == "line":
            if c == "\n": state = "code"
        elif state == "block":
            if c == "*" and n == "/": state = "code"; i += 2; continue
        else:
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
    need(brace >= 0, f"missing brace for {signature}")
    return start, matching_brace(text, brace) + 1


def replace_function(text: str, signature: str, replacement: str) -> str:
    start, end = function_span(text, signature)
    return text[:start] + replacement.rstrip() + text[end:]


def replace_range(text: str, start_marker: str, end_marker: str, replacement: str, label: str) -> str:
    start = text.find(start_marker)
    need(start >= 0, f"{label}: start marker not found")
    end = text.find(end_marker, start)
    need(end >= 0, f"{label}: end marker not found")
    return text[:start] + replacement.rstrip() + "\n\n" + text[end:]


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


def copy_payload(rel: str) -> None:
    src = PAYLOAD / rel
    dst = ROOT / rel
    need(src.is_file(), f"payload missing: {rel}")
    # The builder normally executes this patcher from the candidate package,
    # but keep manual/in-tree execution safe as well. shutil.copy2 raises
    # SameFileError when payload and destination resolve to the same file.
    if src.resolve() == dst.resolve():
        return
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def patch_cmake() -> None:
    path = ROOT / "CMakeLists.txt"
    text = path.read_text(encoding="utf-8")
    text = replace_once(text, "project(NougatMediaSuite VERSION 0.0.51 LANGUAGES CXX)",
                        "project(NougatMediaSuite VERSION 0.0.52 LANGUAGES CXX)", "CMake version")
    text = replace_once(text, "add_executable(Nougat_Media_Suite_v51\n    src/main.cpp",
                        "add_executable(Nougat_Media_Suite_v52\n    src/main.cpp\n    src/radio/radio_backend.cpp", "CMake target/sources")
    text = text.replace("target_compile_options(Nougat_Media_Suite_v51", "target_compile_options(Nougat_Media_Suite_v52")
    text = text.replace("target_link_libraries(Nougat_Media_Suite_v51", "target_link_libraries(Nougat_Media_Suite_v52")
    text = text.replace("target_compile_definitions(Nougat_Media_Suite_v51", "target_compile_definitions(Nougat_Media_Suite_v52")
    text = text.replace("target_include_directories(Nougat_Media_Suite_v51", "target_include_directories(Nougat_Media_Suite_v52")
    text = text.replace("target_link_options(Nougat_Media_Suite_v51", "target_link_options(Nougat_Media_Suite_v52")
    text = text.replace("set_target_properties(Nougat_Media_Suite_v51", "set_target_properties(Nougat_Media_Suite_v52")
    need("Nougat_Media_Suite_v51" not in text, "CMake still contains v51 target identity")
    path.write_text(text, encoding="utf-8")


def patch_main() -> None:
    path = ROOT / "src/main.cpp"
    text = path.read_text(encoding="utf-8")

    text = replace_once(text, '#include "world_tv/world_tv_service.hpp"',
                        '#include "world_tv/world_tv_service.hpp"\n#include "radio/radio_backend.hpp"', "Radio backend include")
    text = replace_once(text,
        'enum class RadioPanel { AM, FM, Shortwave, Weather, DAB, DRM, Internet, SDR, Favorites, Recordings };',
        'enum class RadioPanel { Local, Emergency, Weather, Satellite, Shortwave, Internet, Favorites, Recordings };',
        "Radio simple panels")

    old_members = '''    Rect radioAmBtn, radioFmBtn, radioShortwaveBtn, radioWeatherBtn, radioDabBtn;
    Rect radioDrmBtn, radioInternetBtn, radioSdrBtn, radioFavoritesBtn, radioRecordingsBtn, radioListBox;
    RadioPanel radioPanel = RadioPanel::FM;'''
    new_members = '''    Rect radioSimpleBtn, radioProBtn;
    Rect radioLocalBtn, radioEmergencyBtn, radioWeatherBtn, radioSatelliteBtn, radioShortwaveBtn, radioInternetBtn;
    Rect radioFavoritesBtn, radioRecordingsBtn, radioAntennaScanBtn;
    Rect radioFrequencyRect, radioTuneDownBtn, radioTuneUpBtn, radioListenBtn, radioStopBtn, radioScanBtn;
    Rect radioFavoriteBtn, radioRecordBtn, radioModeBtn, radioStepBtn, radioDeviceBtn;
    Rect radioGainDownBtn, radioGainUpBtn, radioSquelchDownBtn, radioSquelchUpBtn, radioTxTestBtn, radioListBox;
    RadioPanel radioPanel = RadioPanel::Local;
    reddmedia::RadioBackend radioBackend;
    bool radioProMode = false;
    bool radioFrequencyFocused = false;
    std::string radioFrequencyText = "100.100000";
    std::string radioInternetLabel;
    bool radioInternetPlaying = false;'''
    text = replace_once(text, old_members, new_members, "Radio state members")

    radio_layout = r'''        const Rect radioFrame = page_content_frame(ViewMode::Radio);
        const int radioGap = 6;
        const int left = radioFrame.x + 16;
        const int usable = std::max(260, radioFrame.w - 32);
        const int modeW = 92;
        radioSimpleBtn = {left, kPageControlY, modeW, kCompactButtonH};
        radioProBtn = {left + modeW + radioGap, kPageControlY, modeW, kCompactButtonH};
        const int presetY = kPageControlY + kCompactButtonH + 7;
        const int presetW = std::max(82, (usable - radioGap * 5) / 6);
        int rx = left;
        radioLocalBtn = {rx,presetY,presetW,kCompactButtonH}; rx += presetW + radioGap;
        radioEmergencyBtn = {rx,presetY,presetW,kCompactButtonH}; rx += presetW + radioGap;
        radioWeatherBtn = {rx,presetY,presetW,kCompactButtonH}; rx += presetW + radioGap;
        radioSatelliteBtn = {rx,presetY,presetW,kCompactButtonH}; rx += presetW + radioGap;
        radioShortwaveBtn = {rx,presetY,presetW,kCompactButtonH}; rx += presetW + radioGap;
        radioInternetBtn = {rx,presetY,presetW,kCompactButtonH};
        const int secondaryY = presetY + kCompactButtonH + 7;
        radioFavoritesBtn = {left,secondaryY,112,kCompactButtonH};
        radioRecordingsBtn = {left + 118,secondaryY,112,kCompactButtonH};
        radioAntennaScanBtn = {left + 236,secondaryY,150,kCompactButtonH};
        radioListBox = {left, secondaryY + kCompactButtonH + 10,
                        usable, std::max(170, radioFrame.y + radioFrame.h - (secondaryY + kCompactButtonH + 22))};
        const int consoleX = radioListBox.x + 16;
        const int consoleY = radioListBox.y + 42;
        radioFrequencyRect = {consoleX,consoleY,std::min(260,std::max(180,radioListBox.w/3)),38};
        radioTuneDownBtn = {radioFrequencyRect.x + radioFrequencyRect.w + 8,consoleY,54,38};
        radioTuneUpBtn = {radioTuneDownBtn.x + 60,consoleY,54,38};
        radioListenBtn = {radioTuneUpBtn.x + 64,consoleY,88,38};
        radioStopBtn = {radioListenBtn.x + 94,consoleY,70,38};
        const int actionY = consoleY + 48;
        radioScanBtn = {consoleX,actionY,82,30};
        radioFavoriteBtn = {consoleX+88,actionY,92,30};
        radioRecordBtn = {consoleX+186,actionY,92,30};
        radioModeBtn = {consoleX+284,actionY,92,30};
        radioStepBtn = {consoleX+382,actionY,100,30};
        radioDeviceBtn = {consoleX+488,actionY,std::max(110,radioListBox.w-520),30};
        const int proY = actionY + 40;
        radioGainDownBtn = {consoleX,proY,74,28};
        radioGainUpBtn = {consoleX+80,proY,74,28};
        radioSquelchDownBtn = {consoleX+164,proY,84,28};
        radioSquelchUpBtn = {consoleX+254,proY,84,28};
        radioTxTestBtn = {consoleX+348,proY,128,28};'''
    text = replace_range(text,
                         '        const Rect radioFrame = page_content_frame(ViewMode::Radio);',
                         '        layout_button_row({&streamYoutubeTab',
                         radio_layout,
                         "Radio layout")

    radio_click = '''        if (topNavHit && radioTab.contains(x,y)) {
            if (currentView != ViewMode::Radio) switch_view(ViewMode::Radio);
            radioBackend.refresh();
            const auto radioState = radioBackend.snapshot();
            char radioFreq[64];
            snprintf(radioFreq, sizeof(radioFreq), "%.6f", radioState.frequency_hz / 1000000.0);
            radioFrequencyText = radioFreq;
            return;
        }'''
    text = replace_once(text,
        '''        if (topNavHit && radioTab.contains(x,y)) {
            if (currentView != ViewMode::Radio) switch_view(ViewMode::Radio);
            return;
        }''',
        radio_click,
        "Radio top-tab refresh")

    panel_name = r'''    static const char* radio_panel_name(RadioPanel panel) {
        switch (panel) {
            case RadioPanel::Local: return "Local Radio";
            case RadioPanel::Emergency: return "Local Emergency / Scanner";
            case RadioPanel::Weather: return "Weather";
            case RadioPanel::Satellite: return "ISS / Satellite";
            case RadioPanel::Shortwave: return "Shortwave / HF";
            case RadioPanel::Internet: return "Internet Radio";
            case RadioPanel::Favorites: return "Favorites";
            case RadioPanel::Recordings: return "Recordings";
        }
        return "Radio";
    }

    static std::string radio_frequency_label(double hz) {
        char out[64];
        if (hz >= 1000000.0) snprintf(out,sizeof(out),"%.6f MHz",hz/1000000.0);
        else if (hz >= 1000.0) snprintf(out,sizeof(out),"%.3f kHz",hz/1000.0);
        else snprintf(out,sizeof(out),"%.0f Hz",hz);
        return out;
    }

    void set_radio_preset(RadioPanel panel) {
        radioPanel = panel;
        switch (panel) {
            case RadioPanel::Local:
                radioBackend.set_frequency(100100000.0);
                radioBackend.set_modulation(reddmedia::RadioModulation::WFM);
                radioBackend.set_tuning_step(100000.0);
                break;
            case RadioPanel::Emergency:
                radioBackend.set_frequency(155000000.0);
                radioBackend.set_modulation(reddmedia::RadioModulation::NFM);
                radioBackend.set_tuning_step(12500.0);
                break;
            case RadioPanel::Weather:
                radioBackend.set_frequency(162550000.0);
                radioBackend.set_modulation(reddmedia::RadioModulation::NFM);
                radioBackend.set_tuning_step(25000.0);
                break;
            case RadioPanel::Satellite:
                radioBackend.set_frequency(145800000.0);
                radioBackend.set_modulation(reddmedia::RadioModulation::NFM);
                radioBackend.set_tuning_step(5000.0);
                break;
            case RadioPanel::Shortwave:
                radioBackend.set_frequency(9500000.0);
                radioBackend.set_modulation(reddmedia::RadioModulation::AM);
                radioBackend.set_tuning_step(5000.0);
                break;
            case RadioPanel::Internet:
            case RadioPanel::Favorites:
            case RadioPanel::Recordings:
                break;
        }
        const auto state=radioBackend.snapshot();
        char out[64]; snprintf(out,sizeof(out),"%.6f",state.frequency_hz/1000000.0);
        radioFrequencyText=out;
        radioFrequencyFocused=false;
    }

    bool play_radio_internet_station(const std::string& url, const std::string& label) {
        cancel_tv_autoplay();
        if (!inst || !api.media_new_location || url.empty()) return false;
        p2pStream.stop();
        cleanup_player();
        libvlc_media_t* media=api.media_new_location(inst,url.c_str());
        if (!media) return false;
        if (api.media_add_option) {
            api.media_add_option(media,":network-caching=1800");
            api.media_add_option(media,":http-reconnect=true");
        }
        mp=api.player_new_from_media(media);
        api.media_release(media);
        if (!mp) return false;
        if (api.play(mp)!=0) { cleanup_player(); return false; }
        radioInternetPlaying=true;
        radioInternetLabel=label;
        return true;
    }

    void play_default_internet_radio() {
        // Stable public Icecast endpoint. Internet Radio is one source inside
        // Radio, not the definition of the Radio feature.
        if (!play_radio_internet_station("https://ice1.somafm.com/groovesalad-128-mp3","SomaFM Groove Salad")) {
            radioInternetPlaying=false;
            radioInternetLabel="Internet station failed to start.";
        }
    }'''
    text = replace_function(text, '    static const char* radio_panel_name(RadioPanel panel)', panel_name)

    draw_radio = r'''    void draw_radio_screen(Drawable target) {
        const ViewPalette palette=palette_for(ViewMode::Radio);
        const Rect frame=page_content_frame(ViewMode::Radio);
        draw_quilted_background(target,frame,ViewMode::Radio);
        button_on_state(target,radioSimpleBtn,"RADIO",radioProMode?SheetControlState::Normal:SheetControlState::Hover);
        button_on_state(target,radioProBtn,"PRO",radioProMode?SheetControlState::Hover:SheetControlState::Normal);
        const auto preset=[&](const Rect& r,const char* label,RadioPanel panel) {
            button_on_state(target,r,label,radioPanel==panel?SheetControlState::Hover:SheetControlState::Normal);
        };
        preset(radioLocalBtn,"Local",RadioPanel::Local);
        preset(radioEmergencyBtn,"Emergency",RadioPanel::Emergency);
        preset(radioWeatherBtn,"Weather",RadioPanel::Weather);
        preset(radioSatelliteBtn,"ISS / Sat",RadioPanel::Satellite);
        preset(radioShortwaveBtn,"Shortwave",RadioPanel::Shortwave);
        preset(radioInternetBtn,"Internet",RadioPanel::Internet);
        button_on(target,radioFavoritesBtn,"Favorites");
        button_on(target,radioRecordingsBtn,"Recordings");
        button_on(target,radioAntennaScanBtn,"TV Antenna Scan");

        draw_primary_panel(target,radioListBox,palette);
        section_text(target,radioListBox.x+14,radioListBox.y+26,std::string("RADIO • ")+radio_panel_name(radioPanel),palette.text);
        const auto state=radioBackend.snapshot();
        std::string freq=radioFrequencyFocused?radioFrequencyText:radio_frequency_label(state.frequency_hz);
        draw_concept_field(target,radioFrequencyRect,palette.field,palette.border,radioFrequencyFocused);
        text(target,radioFrequencyRect.x+12,radioFrequencyRect.y+25,head_to_width(freq,radioFrequencyRect.w-20),palette.text);
        button_on(target,radioTuneDownBtn,"-");
        button_on(target,radioTuneUpBtn,"+");
        button_on_state(target,radioListenBtn,state.receiving?"LISTENING":"LISTEN",state.receiving?SheetControlState::Hover:SheetControlState::Normal);
        button_on(target,radioStopBtn,"STOP");
        button_on_state(target,radioScanBtn,state.scanning?"SCANNING":"SCAN",state.scanning?SheetControlState::Hover:SheetControlState::Normal);
        button_on(target,radioFavoriteBtn,"FAVORITE");
        button_on_state(target,radioRecordBtn,state.recording?"RECORDING":"RECORD",state.recording?SheetControlState::Hover:SheetControlState::Normal);
        button_on(target,radioModeBtn,reddmedia::RadioBackend::modulation_name(state.modulation));
        char stepLabel[64];
        if(state.tuning_step_hz>=1000.0) snprintf(stepLabel,sizeof(stepLabel),"STEP %.1fk",state.tuning_step_hz/1000.0);
        else snprintf(stepLabel,sizeof(stepLabel),"STEP %.0f",state.tuning_step_hz);
        button_on(target,radioStepBtn,stepLabel);
        std::string deviceLabel="DEVICE: none";
        if(state.selected_device>=0 && state.selected_device<(int)state.devices.size()) deviceLabel="DEVICE: "+state.devices[(size_t)state.selected_device].backend;
        button_on(target,radioDeviceBtn,head_to_width(deviceLabel,radioDeviceBtn.w-12));

        const int meterX=radioFrequencyRect.x;
        const int meterY=radioFrequencyRect.y+94;
        text(target,meterX,meterY,"SIGNAL",palette.text);
        const int bars=10;
        const int active=state.signal_percent<0?0:std::max(0,std::min(bars,(state.signal_percent+9)/10));
        for(int i=0;i<bars;++i) {
            Rect bar{meterX+58+i*18,meterY-12,13,12};
            fill(target,bar,i<active?palette.accent:palette.buttonDark);
            outline(target,bar,palette.border);
        }
        int infoY=meterY+22;
        std::string hardware=state.devices.empty()?"No RF receiver detected.":std::to_string(state.devices.size())+" RF/TV device(s) detected.";
        text(target,meterX,infoY,head_to_width(hardware,radioListBox.w-32),palette.text); infoY+=18;
        text(target,meterX,infoY,head_to_width(state.status,radioListBox.w-32),palette.muted); infoY+=20;

        if(radioPanel==RadioPanel::Satellite) {
            text(target,meterX,infoY,"ISS voice downlink preset: 145.800 MHz FM. Pro mode permits manual satellite frequencies.",palette.text); infoY+=18;
        } else if(radioPanel==RadioPanel::Emergency) {
            text(target,meterX,infoY,"Scans receive-only public-safety spectrum. Encrypted traffic is reported as unavailable, never bypassed.",palette.text); infoY+=18;
        } else if(radioPanel==RadioPanel::Internet) {
            text(target,meterX,infoY,radioInternetPlaying?("Playing: "+radioInternetLabel):"Press LISTEN for the Internet Radio preset.",palette.text); infoY+=18;
        } else if(radioPanel==RadioPanel::Favorites) {
            const auto favs=radioBackend.favorites();
            text(target,meterX,infoY,favs.empty()?"No radio favorites saved yet.":("Saved frequencies: "+std::to_string(favs.size())),palette.text); infoY+=18;
        } else if(radioPanel==RadioPanel::Recordings) {
            const auto recs=radioBackend.recordings();
            text(target,meterX,infoY,recs.empty()?"No radio recordings yet.":("Radio recordings: "+std::to_string(recs.size())),palette.text); infoY+=18;
        }

        if(radioProMode) {
            button_on(target,radioGainDownBtn,"GAIN -");
            button_on(target,radioGainUpBtn,"GAIN +");
            button_on(target,radioSquelchDownBtn,"SQL -");
            button_on(target,radioSquelchUpBtn,"SQL +");
            button_on(target,radioTxTestBtn,"TX CHAIN TEST");
            char pro[256];
            snprintf(pro,sizeof(pro),"PRO: gain %d%% • squelch %d • Soapy %s • RTL %s • OP25 %s • DAB %s • DRM %s • SatDump %s • TX hardware %s (RF TX OFF)",
                     state.gain_percent,state.squelch,state.soapy_available?"ready":"no",state.rtl_available?"ready":"no",
                     state.op25_available?"ready":"no",state.dab_decoder_available?"ready":"no",state.drm_decoder_available?"ready":"no",
                     state.satellite_decoder_available?"ready":"no",state.tx_hardware_available?"detected":"none");
            text(target,meterX,std::min(radioListBox.y+radioListBox.h-14,infoY+20),head_to_width(pro,radioListBox.w-32),palette.muted);
        } else {
            text(target,meterX,std::min(radioListBox.y+radioListBox.h-14,infoY+20),
                 "RADIO mode chooses sane defaults. Switch to PRO only when you want the full receiver controls.",palette.muted);
        }
    }'''
    text = replace_function(text, '    void draw_radio_screen(Drawable target)', draw_radio)

    handle_radio = r'''    void handle_radio_click(int x, int y) {
        if (radioSimpleBtn.contains(x,y)) { radioProMode=false; radioFrequencyFocused=false; redraw(); return; }
        if (radioProBtn.contains(x,y)) { radioProMode=true; radioFrequencyFocused=false; redraw(); return; }
        if (radioLocalBtn.contains(x,y)) { set_radio_preset(RadioPanel::Local); redraw(); return; }
        if (radioEmergencyBtn.contains(x,y)) { set_radio_preset(RadioPanel::Emergency); redraw(); return; }
        if (radioWeatherBtn.contains(x,y)) { set_radio_preset(RadioPanel::Weather); redraw(); return; }
        if (radioSatelliteBtn.contains(x,y)) { set_radio_preset(RadioPanel::Satellite); redraw(); return; }
        if (radioShortwaveBtn.contains(x,y)) { set_radio_preset(RadioPanel::Shortwave); redraw(); return; }
        if (radioInternetBtn.contains(x,y)) { set_radio_preset(RadioPanel::Internet); redraw(); return; }
        if (radioFavoritesBtn.contains(x,y)) { radioPanel=RadioPanel::Favorites; radioFrequencyFocused=false; redraw(); return; }
        if (radioRecordingsBtn.contains(x,y)) { radioPanel=RadioPanel::Recordings; radioFrequencyFocused=false; redraw(); return; }
        if (radioAntennaScanBtn.contains(x,y)) {
            radioBackend.stop_receive();
            if (radioInternetPlaying) { cleanup_player(); radioInternetPlaying=false; }
            switch_view(ViewMode::LiveTV);
            start_live_tv_scan();
            redraw();
            return;
        }
        if (radioFrequencyRect.contains(x,y)) {
            const auto state=radioBackend.snapshot();
            char out[64]; snprintf(out,sizeof(out),"%.6f",state.frequency_hz/1000000.0);
            radioFrequencyText=out; radioFrequencyFocused=true; redraw(); return;
        }
        radioFrequencyFocused=false;
        if (radioTuneDownBtn.contains(x,y)) { const auto state=radioBackend.snapshot(); radioBackend.step_frequency(-state.tuning_step_hz); redraw(); return; }
        if (radioTuneUpBtn.contains(x,y)) { const auto state=radioBackend.snapshot(); radioBackend.step_frequency(state.tuning_step_hz); redraw(); return; }
        if (radioListenBtn.contains(x,y)) {
            if (radioPanel==RadioPanel::Internet) { play_default_internet_radio(); redraw(); return; }
            if (radioInternetPlaying) { cleanup_player(); radioInternetPlaying=false; }
            std::string status; radioBackend.start_receive(status); redraw(); return;
        }
        if (radioStopBtn.contains(x,y)) {
            radioBackend.stop_receive();
            if (radioInternetPlaying) { cleanup_player(); radioInternetPlaying=false; radioInternetLabel.clear(); }
            redraw(); return;
        }
        if (radioScanBtn.contains(x,y)) {
            const auto state=radioBackend.snapshot();
            std::string status;
            if (state.scanning) radioBackend.cancel_scan();
            else if (radioPanel==RadioPanel::Local) radioBackend.start_scan(88000000.0,108000000.0,200000.0,status);
            else if (radioPanel==RadioPanel::Emergency) radioBackend.start_scan(150000000.0,174000000.0,12500.0,status);
            else if (radioPanel==RadioPanel::Weather) radioBackend.start_scan(162400000.0,162550000.0,25000.0,status);
            else if (radioPanel==RadioPanel::Satellite) radioBackend.start_scan(145750000.0,145850000.0,5000.0,status);
            else if (radioPanel==RadioPanel::Shortwave) radioBackend.start_scan(5900000.0,6200000.0,5000.0,status);
            else status="Select an RF mode before scanning.";
            redraw(); return;
        }
        if (radioFavoriteBtn.contains(x,y)) { std::string status; radioBackend.toggle_favorite(radio_panel_name(radioPanel),status); redraw(); return; }
        if (radioRecordBtn.contains(x,y)) { std::string status; radioBackend.toggle_recording(status); redraw(); return; }
        if (radioModeBtn.contains(x,y)) { radioBackend.cycle_modulation(1); redraw(); return; }
        if (radioStepBtn.contains(x,y)) { radioBackend.cycle_tuning_step(1); redraw(); return; }
        if (radioDeviceBtn.contains(x,y)) { radioBackend.cycle_device(1); redraw(); return; }
        if (radioProMode && radioGainDownBtn.contains(x,y)) { auto s=radioBackend.snapshot(); radioBackend.set_gain_percent(s.gain_percent-5); redraw(); return; }
        if (radioProMode && radioGainUpBtn.contains(x,y)) { auto s=radioBackend.snapshot(); radioBackend.set_gain_percent(s.gain_percent+5); redraw(); return; }
        if (radioProMode && radioSquelchDownBtn.contains(x,y)) { auto s=radioBackend.snapshot(); radioBackend.set_squelch(s.squelch-5); redraw(); return; }
        if (radioProMode && radioSquelchUpBtn.contains(x,y)) { auto s=radioBackend.snapshot(); radioBackend.set_squelch(s.squelch+5); redraw(); return; }
        if (radioProMode && radioTxTestBtn.contains(x,y)) { std::string status; radioBackend.tx_chain_self_test(status); redraw(); return; }
    }'''
    text = replace_function(text, '    void handle_radio_click(int x, int y)', handle_radio)

    key_anchor = '''                else if (e.type == KeyPress) {
                    KeySym ks = XLookupKeysym(&e.xkey, 0);
                    if (currentView == ViewMode::Nougat && nougatPanel == NougatPanel::P2P && p2pMagnetFocused) {'''
    key_replacement = '''                else if (e.type == KeyPress) {
                    KeySym ks = XLookupKeysym(&e.xkey, 0);
                    if (currentView == ViewMode::Radio && radioFrequencyFocused) {
                        if (ks == XK_Escape) { radioFrequencyFocused=false; redraw(); }
                        else if (ks == XK_Return || ks == XK_KP_Enter) {
                            try {
                                const double mhz=std::stod(radioFrequencyText);
                                radioBackend.set_frequency(mhz*1000000.0);
                            } catch (const std::exception&) {}
                            radioFrequencyFocused=false; redraw();
                        } else if (ks == XK_BackSpace) {
                            if (!radioFrequencyText.empty()) radioFrequencyText.pop_back();
                            redraw();
                        } else {
                            char buf[16] = {};
                            KeySym typed=0;
                            const int count=XLookupString(&e.xkey,buf,sizeof(buf)-1,&typed,nullptr);
                            for(int i=0;i<count;++i) {
                                const char c=buf[i];
                                if ((c>='0'&&c<='9') || (c=='.' && radioFrequencyText.find('.')==std::string::npos)) {
                                    if (radioFrequencyText.size()<18) radioFrequencyText.push_back(c);
                                }
                            }
                            redraw();
                        }
                    }
                    else if (currentView == ViewMode::Nougat && nougatPanel == NougatPanel::P2P && p2pMagnetFocused) {'''
    text = replace_once(text, key_anchor, key_replacement, "Radio frequency keyboard input")

    text = replace_once(text, 'const std::string versionLabel = "v0.0.51";',
                        'const std::string versionLabel = "v0.0.52";', "visible version")
    text = replace_once(text, 'input.app_version = "Nougat Media Suite v0.0.51";',
                        'input.app_version = "Nougat Media Suite v0.0.52";', "diagnostic version")
    cli_normal = 'printf("Nougat Media Suite v0.0.51\\n");'
    cli_continuation = 'printf("Nougat Media Suite v0.0.51\\\n");'
    if cli_normal in text:
        text = replace_once(text, cli_normal, 'printf("Nougat Media Suite v0.0.52\\n");', "CLI version")
    elif cli_continuation in text:
        text = replace_once(text, cli_continuation, 'printf("Nougat Media Suite v0.0.52\\n");', "CLI version continuation")
    else:
        need(False, "CLI version: v0.0.51 anchor not found")

    path.write_text(text, encoding="utf-8")


def patch_launchers() -> None:
    for rel in ("NougatMediaSuite.desktop", "com.elderredsoftworks.NougatMediaSuite.desktop"):
        path = ROOT / rel
        text = path.read_text(encoding="utf-8")
        text = replace_once(text, "Nougat_Media_Suite_v51", "Nougat_Media_Suite_v52", f"{rel} executable")
        path.write_text(text, encoding="utf-8")


def patch_docs() -> None:
    prepend_after_title(ROOT / "CHANGELOG.md", "## v0.0.52 candidate - Professional Radio Receiver", '''
- Focuses v0.0.52 exclusively on replacing the rejected Radio shell with a functional receiver/scanner architecture.
- Adds a simple RADIO view for ordinary listening and a PRO view for frequency, modulation, tuning step, gain, squelch, device selection, scanning, recording, spectrum-engine/runtime status and TX-chain testing.
- Adds truthful hardware discovery across Linux radio/DVB devices plus SoapySDR-class, RTL-SDR, HackRF, LimeSDR, UHD/USRP and Airspy tooling when present.
- Adds real asynchronous RTL-SDR receive/scanning paths, persistent radio Favorites/Recordings, weather/emergency/ISS/shortwave presets, Internet Radio playback through Nougat's existing libVLC engine, and a TV Antenna Scan bridge to the existing Live TV tuner path.
- Adds receive-only public-safety/P25 capability hooks through OP25 and preserves encrypted-system boundaries.
- Keeps RF transmit disabled by default while adding RX/TX hardware capability reporting and a non-radiating generated-IQ TX-chain self-test so the architecture is not receiver-only.
- Vendors/pins the owner-supplied radio open-source foundations and preserves their upstream licenses behind Nougat-owned boundaries.
- Moves every non-Radio rejected-v0.0.51 repair into the v0.0.53 carry-forward document. No other rejected subsystem is repaired in v0.0.52.
- Candidate remains uncommitted and untagged until owner testing and acceptance.
''')

    prepend_after_title(ROOT / "ROADMAP.md", "## v0.0.53 planned - Rejected v0.0.51 carry-forward and alerts", '''
The complete mandatory carry-forward list is recorded in `docs/builds/NOUGAT_MEDIA_SUITE_v0_0_53_CARRY_FORWARD.md`. v0.0.52 is Radio-only. v0.0.53 resumes the deferred File Splitter, HDHomeRun/full-scan, World TV, Games/emulator/artwork, navigation, identity, overlay/process, LAN and related repair work. AMBER Alerts and the broader official public-warning integration are also assigned to v0.0.53.
''')

    prepend_after_title(ROOT / "DEPENDENCIES.md", "## v0.0.52 Radio dependency boundary", '''
Nougat Radio uses a provider/worker architecture so RF hardware and specialized decoders remain replaceable. The v0.0.52 source-vendor tool pins SoapySDR, liquid-dsp, OP25, GNU Radio 4 core and KISS FFT snapshots corresponding to the owner-supplied projects. Specialized DAB/DAB+, DRM, satellite and trunked-radio workers remain separate upstream components when used. GPL components are not linked into Elderred Softworks LLC Original Materials; they remain separate executables/workers with their own upstream licenses and notices. The core v0.0.52 executable builds without requiring these optional runtimes so unsupported hardware is reported honestly instead of blocking Nougat startup.
''')

    notices = ROOT / "THIRD_PARTY_NOTICES.md"
    text = notices.read_text(encoding="utf-8")
    heading = "## Nougat Radio v0.0.52 upstream components"
    if heading not in text:
        text += '''\n\n## Nougat Radio v0.0.52 upstream components\n\nNougat Radio may use separately licensed upstream components including SoapySDR (Boost Software License 1.0), liquid-dsp (MIT), KISS FFT (BSD-style), GNU Radio 4 core (MIT), and OP25 (GPL family), plus specialized DAB/DAB+, DRM, satellite and trunked-radio workers when installed. Their original source trees and license files are preserved under `components/radio/upstream/`; those licenses govern those upstream components. Nothing in this notice relicenses upstream code under Nougat's PolyForm license.\n'''
        notices.write_text(text, encoding="utf-8")


def main() -> int:
    try:
        need((ROOT / "src/main.cpp").is_file(), f"Not a Nougat source tree: {ROOT}")
        for rel in ("src/radio/radio_backend.hpp", "src/radio/radio_backend.cpp",
                    "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_52_SCOPE.md",
                    "docs/builds/NOUGAT_MEDIA_SUITE_v0_0_53_CARRY_FORWARD.md",
                    "components/radio/README.md", "components/radio/UPSTREAM_COMPONENTS.json",
                    "tools/vendor_radio_sources_v52.py", "tools/test_v52_radio_static.py"):
            copy_payload(rel)
        patch_cmake()
        patch_main()
        patch_launchers()
        patch_docs()
        print("PASS: v0.0.52 Radio source patch applied.")
        return 0
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
