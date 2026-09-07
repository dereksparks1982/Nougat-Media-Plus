#!/usr/bin/env python3
from pathlib import Path
import re
import sys

MARK = "NOUGAT_V67_HVR955Q_FM_END_TO_END_FIX12"


def die(msg):
    raise SystemExit("FAIL: " + msg)


def load(path):
    if not path.is_file():
        die(f"missing kernel source file: {path}")
    return path.read_text()


def function_span(text, signature):
    start = text.find(signature)
    if start < 0:
        die(f"function not found: {signature}")
    brace = text.find("{", start)
    if brace < 0:
        die(f"function opening brace not found: {signature}")
    depth = 0
    for i in range(brace, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return start, i + 1
    die(f"function closing brace not found: {signature}")


def patch_cards(path):
    text = load(path)
    if MARK + "_RADIO_NODE" in text:
        return

    start = text.find("[CX231XX_BOARD_HAUPPAUGE_955Q] = {")
    if start < 0:
        die("HVR-955Q board definition not found")

    rest = text[start + 1:]
    nxt = re.search(r'\n\s*\[CX231XX_BOARD_[A-Z0-9_]+\]\s*=\s*\{', rest)
    if not nxt:
        die("board following HVR-955Q not found")
    end = start + 1 + nxt.start()
    block = text[start:end]

    if re.search(r'(?m)^\s*\.radio\s*=', block):
        return

    closes = list(re.finditer(r'(?m)^\s*\},\s*$', block))
    if not closes:
        die("HVR-955Q outer board close not found")
    m = closes[-1]
    insert = (
        "\t/* " + MARK + "_RADIO_NODE */\n"
        "\t.radio = {\n"
        "\t\t.type = CX231XX_RADIO,\n"
        "\t},\n"
    )
    block = block[:m.start()] + insert + block[m.start():]
    path.write_text(text[:start] + block + text[end:])


def patch_video(path):
    text = load(path)

    if MARK + "_RADIO_CAPS" not in text:
        s, e = function_span(text, "static int radio_g_tuner(")
        func = text[s:e]
        old_name = 'strscpy(t->name, "Radio", sizeof(t->name));'
        if old_name not in func:
            die("radio_g_tuner name anchor not found")
        func = func.replace(old_name, 'strscpy(t->name, "FM Radio", sizeof(t->name));', 1)
        call = "call_all(dev, tuner, g_tuner, t);"
        if call not in func:
            die("radio_g_tuner subdevice call anchor not found")
        caps = (
            call + "\n\n"
            "\t/* " + MARK + "_RADIO_CAPS */\n"
            "\tt->type = V4L2_TUNER_RADIO;\n"
            "\tt->capability |= V4L2_TUNER_CAP_LOW | V4L2_TUNER_CAP_STEREO;\n"
            "\tt->rangelow = 1400000;   /* 87.5 MHz / 62.5 Hz */\n"
            "\tt->rangehigh = 1728000;  /* 108.0 MHz / 62.5 Hz */\n"
            "\tt->audmode = V4L2_TUNER_MODE_STEREO;"
        )
        func = func.replace(call, caps, 1)
        text = text[:s] + func + text[e:]

    if MARK + "_RADIO_OPEN" not in text:
        pattern = re.compile(
            r'(?ms)^(\s*)if \(vdev->vfl_type == VFL_TYPE_RADIO\) \{\n'
            r'\1\tcx231xx_videodbg\("video_open: setting radio device\\n"\);\n'
            r'\s*\n'
            r'(?:\1\t/\* cx231xx_start_radio\(dev\); \*/\n\s*\n)?'
            r'\1\tcall_all\(dev, tuner, s_radio\);\n'
            r'\1\}'
        )
        matches = list(pattern.finditer(text))
        if len(matches) != 1:
            die(f"radio open path: expected one anchor, found {len(matches)}")
        indent = matches[0].group(1)
        replacement = (
            indent + 'if (vdev->vfl_type == VFL_TYPE_RADIO) {\n'
            + indent + '\tcx231xx_videodbg("video_open: setting radio device\\n");\n\n'
            + indent + '\t/* ' + MARK + '_RADIO_OPEN */\n'
            + indent + '\tdev->active_mode = V4L2_TUNER_RADIO;\n'
            + indent + '\tcall_all(dev, tuner, s_radio);\n'
            + indent + '\tcx231xx_set_audio_decoder_input(dev, AUDIO_INPUT_TUNER_FM);\n'
            + indent + '}'
        )
        text = text[:matches[0].start()] + replacement + text[matches[0].end():]

    if MARK + "_TV_RESTORE" not in text:
        s, e = function_span(text, "int cx231xx_s_input(")
        func = text[s:e]
        anchor = ("\tif (INPUT(i)->type == CX231XX_VMUX_TELEVISION ||\n"
                  "\t    INPUT(i)->type == CX231XX_VMUX_CABLE) {")
        if anchor not in func:
            die("analog TV restore anchor not found")
        func = func.replace(
            anchor,
            anchor + "\n\t\t/* " + MARK + "_TV_RESTORE */\n"
                     "\t\tdev->active_mode = V4L2_TUNER_ANALOG_TV;",
            1,
        )
        text = text[:s] + func + text[e:]

    if MARK + "_FREQUENCY_MODE" not in text:
        s, e = function_span(text, "int cx231xx_s_frequency(")
        func = text[s:e]
        tuner_check = "\tif (0 != f->tuner)\n\t\treturn -EINVAL;\n"
        if tuner_check not in func:
            die("frequency tuner index anchor not found")
        func = func.replace(
            tuner_check,
            tuner_check
            + "\n\t/* " + MARK + "_FREQUENCY_MODE */\n"
            + "\tif (f->type == V4L2_TUNER_RADIO)\n"
            + "\t\tdev->active_mode = V4L2_TUNER_RADIO;\n"
            + "\telse\n"
            + "\t\tdev->active_mode = V4L2_TUNER_ANALOG_TV;\n",
            1,
        )

        if_anchor = ("\tif (need_if_freq || dev->tuner_type == TUNER_NXP_TDA18271) {\n"
                     "\t\tif (dev->norm & (V4L2_STD_MN | V4L2_STD_NTSC_443))")
        if if_anchor not in func:
            die("frequency IF selection anchor not found")
        func = func.replace(
            if_anchor,
            "\tif (need_if_freq || dev->tuner_type == TUNER_NXP_TDA18271) {\n"
            "\t\tif (f->type == V4L2_TUNER_RADIO)\n"
            "\t\t\tif_frequency = 6600000; /* HVR-9xx FM IF */\n"
            "\t\telse if (dev->norm & (V4L2_STD_MN | V4L2_STD_NTSC_443))",
            1,
        )

        lowif = "cx231xx_set_Colibri_For_LowIF(dev, if_frequency, 1, 1);"
        if lowif not in func:
            die("Colibri LowIF call anchor not found")
        func = func.replace(
            lowif,
            "cx231xx_set_Colibri_For_LowIF(\n"
            "\t\t\tdev, if_frequency, 1,\n"
            "\t\t\tf->type == V4L2_TUNER_RADIO ? 0 : 1); /* " + MARK + "_FM_DIF */",
            1,
        )
        text = text[:s] + func + text[e:]

    path.write_text(text)


def patch_dvb(path):
    text = load(path)
    if MARK + "_DVB_RADIO_MODE" in text:
        return
    s, e = function_span(text, "int cx231xx_set_analog_freq(")
    func = text[s:e]
    pattern = re.compile(
        r'(?m)^(\s*)params\.mode\s*=\s*0;\s*/\*\s*0-\s*Air;\s*1\s*-\s*cable\s*\*/\s*$'
    )
    matches = list(pattern.finditer(func))
    if len(matches) != 1:
        die(f"cx231xx-dvb radio mode: expected one anchor, found {len(matches)}")
    m = matches[0]
    replacement = (
        m.group(1)
        + "params.mode = dev->active_mode == V4L2_TUNER_RADIO "
          "? V4L2_TUNER_RADIO : dev->std_mode;"
        + "\t/* " + MARK + "_DVB_RADIO_MODE */"
    )
    func = func[:m.start()] + replacement + func[m.end():]
    path.write_text(text[:s] + func + text[e:])


def patch_avcore(path):
    text = load(path)
    if MARK + "_FM_AUDIO" in text:
        return

    # AUDIO_INPUT_TUNER_FM is handled in the decoder function, not in
    # cx231xx_set_audio_input(), which only maps a board input index to a
    # decoder audio source.
    s, e = function_span(text, "int cx231xx_set_audio_decoder_input(")
    func = text[s:e]

    # Remove the existing unfinished FM case wherever it lives. Do not depend
    # on its comments or statements. The case body ends at the next top-level
    # switch label.
    fm_case = re.compile(
        r'(?ms)^(?P<indent>[ \t]*)case AUDIO_INPUT_TUNER_FM:\s*\n'
        r'.*?'
        r'(?=^[ \t]*(?:case [A-Z0-9_]+:|default:))'
    )
    fm_matches = list(fm_case.finditer(func))
    if len(fm_matches) > 1:
        die(
            "cx231xx FM decoder path: expected at most one FM case, "
            f"found {len(fm_matches)}"
        )
    if len(fm_matches) == 1:
        fm = fm_matches[0]
        func = func[:fm.start()] + func[fm.end():]

    # Attach FM directly to the established tuner-TV SIF path by placing the
    # FM label immediately before the TV case label.
    tv_case = re.compile(
        r'(?m)^(?P<indent>[ \t]*)case AUDIO_INPUT_TUNER_TV:\s*$'
    )
    tv_matches = list(tv_case.finditer(func))
    if len(tv_matches) != 1:
        die(
            "cx231xx tuner decoder TV path: expected one TV case, "
            f"found {len(tv_matches)}"
        )

    tv = tv_matches[0]
    indent = tv.group("indent")
    insertion = (
        indent + "case AUDIO_INPUT_TUNER_FM:\n"
        + indent + "\t/* " + MARK
        + "_FM_AUDIO: share the established tuner SIF decoder path. */\n"
    )
    func = func[:tv.start()] + insertion + func[tv.start():]

    path.write_text(text[:s] + func + text[e:])


def patch_si2157(path):
    text = load(path)
    if MARK + "_SI2157_FM" in text:
        return
    s, e = function_span(text, "static int si2157_set_analog_params(")
    func = text[s:e]

    decl = "\tu8 invert_analog = 1; /* analog tuner spectrum; 0=normal, 1=inverted */\n"
    if decl not in func:
        die("Si2157 declaration anchor not found")
    func = func.replace(
        decl,
        decl + "\tbool fm_mode = false; /* " + MARK + "_SI2157_FM */\n",
        1,
    )

    active = "\tif (!dev->active) {\n\t\tret = -EAGAIN;\n\t\tgoto err;\n\t}\n"
    if active not in func:
        die("Si2157 active-state anchor not found")
    func = func.replace(
        active,
        active + "\tfm_mode = params->mode == V4L2_TUNER_RADIO;\n",
        1,
    )

    reject = re.compile(
        r'(?ms)^\tif \(params->mode == V4L2_TUNER_RADIO\) \{\n'
        r'\t/\*\n.*?'
        r'^\t\tgoto err;\n'
        r'^\t\}\n'
    )
    matches = list(reject.finditer(func))
    if len(matches) != 1:
        die(f"Si2157 upstream FM rejection: expected one anchor, found {len(matches)}")
    func = (
        func[:matches[0].start()]
        + "\t/* " + MARK + "_SI2157_FM: enable unfinished HVR FM path. */\n"
        + func[matches[0].end():]
    )

    selector = ("\t/* if_frequency values based on tda187271C2 */\n"
                "\tif (params->std & (V4L2_STD_B | V4L2_STD_GH)) {")
    if selector not in func:
        die("Si2157 standard selector anchor not found")
    func = func.replace(
        selector,
        "\t/* if_frequency values based on tda187271C2 */\n"
        "\tif (fm_mode) {\n"
        "\t\tstd = \"fm\";\n"
        "\t\tbandwidth = 1700000;\n"
        "\t\tif_frequency = 6600000;\n"
        "\t\tsystem = 0;\n"
        "\t} else if (params->std & (V4L2_STD_B | V4L2_STD_GH)) {",
        1,
    )

    rf_center = "freq = freq - 1250000 + (bandwidth / 2);"
    if rf_center not in func:
        die("Si2157 RF center anchor not found")
    func = func.replace(
        rf_center,
        "if (!fm_mode)\n\t\tfreq = freq - 1250000 + (bandwidth / 2);",
        1,
    )

    if_center = "if_frequency = if_frequency + 1250000 - (bandwidth / 2);"
    if if_center not in func:
        die("Si2157 IF center anchor not found")
    func = func.replace(
        if_center,
        "if (!fm_mode)\n\t\tif_frequency = if_frequency + 1250000 - (bandwidth / 2);",
        1,
    )

    path.write_text(text[:s] + func + text[e:])


def main():
    if len(sys.argv) != 2:
        die("usage: patch_kernel_source_fix12.py <exact-source-root>")
    root = Path(sys.argv[1]).resolve()
    patch_cards(root / "drivers/media/usb/cx231xx/cx231xx-cards.c")
    patch_video(root / "drivers/media/usb/cx231xx/cx231xx-video.c")
    patch_dvb(root / "drivers/media/usb/cx231xx/cx231xx-dvb.c")
    patch_avcore(root / "drivers/media/usb/cx231xx/cx231xx-avcore.c")
    patch_si2157(root / "drivers/media/tuners/si2157.c")

    header = root / "drivers/media/usb/cx231xx/cx231xx.h"
    if MARK in load(header):
        die("public cx231xx header unexpectedly modified")

    print("PASS: HVR-955Q FM end-to-end kernel patch applied")
    print("PASS: HVR board registers V4L2 radio node")
    print("PASS: cx231xx uses explicit radio active mode + 6.6 MHz FM IF")
    print("PASS: cx231xx FM audio uses tuner SIF pipeline")
    print("PASS: cx231xx-dvb passes radio mode to Si2157")
    print("PASS: Si2157 FM rejection removed")
    print("PASS: analog-TV restoration preserved")


if __name__ == "__main__":
    main()
