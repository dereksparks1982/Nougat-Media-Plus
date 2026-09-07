#!/usr/bin/env python3
from pathlib import Path
import re
import sys

MARK = "NOUGAT_V67_HVR955Q_FM_TWO_MODULE"

def die(msg):
    raise SystemExit("FAIL: " + msg)

def read(path):
    if not path.is_file():
        die(f"missing kernel source file: {path}")
    return path.read_text()

def replace_once(text, old, new, label):
    if new in text:
        return text
    count = text.count(old)
    if count != 1:
        die(f"{label}: expected one anchor, found {count}")
    return text.replace(old, new, 1)

def patch_cards(path):
    text = read(path)
    if MARK + "_RADIO_NODE" in text:
        return

    start = text.find("[CX231XX_BOARD_HAUPPAUGE_955Q] = {")
    end = text.find("[CX231XX_BOARD_TERRATEC_GRABBY]", start)
    if start < 0 or end < 0:
        die("could not isolate HVR-955Q board definition")

    block = text[start:end]
    if ".radio =" in block:
        die("HVR-955Q already has a radio definition in the clean source")

    anchor = "\t\t} },\n\t},\n"
    if block.count(anchor) != 1:
        die("HVR-955Q board closing anchor changed")

    addition = (
        "\t\t} },\n"
        "\t\t/* " + MARK + "_RADIO_NODE */\n"
        "\t\t.radio = {\n"
        "\t\t\t.type = CX231XX_RADIO,\n"
        "\t\t},\n"
        "\t},\n"
    )
    block = block.replace(anchor, addition, 1)
    path.write_text(text[:start] + block + text[end:])

def patch_video(path):
    text = read(path)

    old_tuner = '''static int radio_g_tuner(struct file *file, void *priv, struct v4l2_tuner *t)
{
\tstruct cx231xx *dev = video_drvdata(file);

\tif (t->index)
\t\treturn -EINVAL;

\tstrscpy(t->name, "Radio", sizeof(t->name));

\tcall_all(dev, tuner, g_tuner, t);

\treturn 0;
}'''
    new_tuner = '''static int radio_g_tuner(struct file *file, void *priv, struct v4l2_tuner *t)
{
\tstruct cx231xx *dev = video_drvdata(file);

\tif (t->index)
\t\treturn -EINVAL;

\tcall_all(dev, tuner, g_tuner, t);

\t/* __MARK___RADIO_CAPS */
\tstrscpy(t->name, "FM Radio", sizeof(t->name));
\tt->type = V4L2_TUNER_RADIO;
\tt->capability |= V4L2_TUNER_CAP_LOW | V4L2_TUNER_CAP_STEREO;
\tt->rangelow = 1400000;
\tt->rangehigh = 1728000;
\tt->audmode = V4L2_TUNER_MODE_STEREO;

\treturn 0;
}'''.replace("__MARK__", MARK)
    text = replace_once(text, old_tuner, new_tuner, "radio tuner capabilities")

    old_special = '''\tcase CX231XX_BOARD_EVROMEDIA_FULL_HYBRID_FULLHD:
\t\tif (dev->cx231xx_set_analog_freq)
\t\t\tdev->cx231xx_set_analog_freq(dev, f->frequency);
\t\tdev->ctl_freq = f->frequency;
\t\tneed_if_freq = 1;
\t\tbreak;'''
    new_special = '''\tcase CX231XX_BOARD_EVROMEDIA_FULL_HYBRID_FULLHD:
\t\t/*
\t\t * __MARK__: preserve the stock cx231xx-dvb callback ABI.
\t\t * A zero TV norm marks the HVR FM path for the patched Si2157.
\t\t */
\t\tif (f->type == V4L2_TUNER_RADIO)
\t\t\tdev->norm = 0;
\t\telse if (!dev->norm)
\t\t\tdev->norm = dev->board.norm;

\t\tif (dev->cx231xx_set_analog_freq)
\t\t\tdev->cx231xx_set_analog_freq(dev, f->frequency);

\t\tif (f->type == V4L2_TUNER_RADIO)
\t\t\tcall_all(dev, tuner, s_frequency, f);

\t\tdev->ctl_freq = f->frequency;
\t\tneed_if_freq = 1;
\t\tbreak;'''.replace("__MARK__", MARK)
    text = replace_once(text, old_special, new_special, "HVR radio frequency dispatch")

    old_if = '''\tif (need_if_freq || dev->tuner_type == TUNER_NXP_TDA18271) {
\t\tif (dev->norm & (V4L2_STD_MN | V4L2_STD_NTSC_443))
\t\t\tif_frequency = 5400000;  /*5.4MHz\t*/'''
    new_if = '''\tif (need_if_freq || dev->tuner_type == TUNER_NXP_TDA18271) {
\t\tif (f->type == V4L2_TUNER_RADIO)
\t\t\tif_frequency = 6600000;  /* __MARK__: HVR-9xx FM IF */
\t\telse if (dev->norm & (V4L2_STD_MN | V4L2_STD_NTSC_443))
\t\t\tif_frequency = 5400000;  /*5.4MHz\t*/'''.replace("__MARK__", MARK)
    text = replace_once(text, old_if, new_if, "6.6 MHz FM IF")

    old_open = '''\tif (vdev->vfl_type == VFL_TYPE_RADIO) {
\t\tcx231xx_videodbg("video_open: setting radio device\\n");

\t\t/* cx231xx_start_radio(dev); */

\t\tcall_all(dev, tuner, s_radio);
\t}'''
    new_open = '''\tif (vdev->vfl_type == VFL_TYPE_RADIO) {
\t\tcx231xx_videodbg("video_open: setting radio device\\n");

\t\t/* __MARK___RADIO_OPEN */
\t\tdev->norm = 0;
\t\tcx231xx_set_audio_input(dev, AUDIO_INPUT_TUNER_TV);
\t\tcall_all(dev, tuner, s_radio);
\t}'''.replace("__MARK__", MARK)
    text = replace_once(text, old_open, new_open, "FM startup/audio routing")

    path.write_text(text)

def patch_si2157(path):
    text = read(path)
    if MARK + "_SI2157_FM" in text:
        return

    fn_start = text.find("static int si2157_set_analog_params(")
    if fn_start < 0:
        die("Si2157 analog tuning function not found")

    fn_end = text.find("\nstatic ", fn_start + 1)
    if fn_end < 0:
        die("Si2157 analog tuning function end not found")

    prefix = text[:fn_start]
    func = text[fn_start:fn_end]
    suffix = text[fn_end:]

    decl_re = re.compile(
        r'(\tu8 color = 0;\s*/\* 0=NTSC/PAL, 0x10=SECAM \*/\n'
        r'\tu8 invert_analog = 1; /\* analog tuner spectrum; 0=normal, 1=inverted \*/\n)'
    )
    decl_match = decl_re.search(func)
    if not decl_match:
        die("FM mode declaration anchor not found in Si2157 analog function")
    decl_new = (
        decl_match.group(1)
        + "\tbool fm_mode = false; /* "
        + MARK
        + "_SI2157_FM */\n"
    )
    func = func[:decl_match.start()] + decl_new + func[decl_match.end():]

    active_anchor = '''\tif (!dev->active) {
\t\tret = -EAGAIN;
\t\tgoto err;
\t}
'''
    active_new = '''\tif (!dev->active) {
\t\tret = -EAGAIN;
\t\tgoto err;
\t}

\t/*
\t * __MARK__: the distro cx231xx-dvb callback uses mode 0. For HVR FM,
\t * the bridge sets std=0 and passes radio frequency in 62.5 Hz units.
\t */
\tfm_mode = params->mode == V4L2_TUNER_RADIO ||
\t\t(!params->std &&
\t\t params->frequency >= 1400000 &&
\t\t params->frequency <= 1728000);
'''.replace("__MARK__", MARK)
    func = replace_once(func, active_anchor, active_new, "FM mode inference")

    reject = re.compile(
        r'\tif \(params->mode == V4L2_TUNER_RADIO\) \{\n'
        r'\t/\*\n.*?\t\tgoto err;\n\t\}\n',
        re.S,
    )
    matches = list(reject.finditer(func))
    if len(matches) != 1:
        die(
            "Si2157 upstream FM rejection block: expected one anchor "
            f"inside analog function, found {len(matches)}"
        )
    m = matches[0]
    func = (
        func[:m.start()]
        + "\t/* " + MARK + "_SI2157_FM: HVR FM is enabled here. */\n"
        + func[m.end():]
    )

    selector = '''\t/* if_frequency values based on tda187271C2 */
\tif (params->std & (V4L2_STD_B | V4L2_STD_GH)) {'''
    fm_branch = '''\t/* if_frequency values based on tda187271C2 */
\tif (fm_mode) {
\t\tstd = "fm";
\t\tbandwidth = 1700000;
\t\tif_frequency = 6600000;
\t\tsystem = 0;
\t} else if (params->std & (V4L2_STD_B | V4L2_STD_GH)) {'''
    func = replace_once(func, selector, fm_branch, "Si2157 FM parameters")

    path.write_text(prefix + func + suffix)

def main():
    if len(sys.argv) != 2:
        die("usage: patch_kernel_source_v3.py <clean-exact-source-root>")

    root = Path(sys.argv[1]).resolve()

    patch_cards(root / "drivers/media/usb/cx231xx/cx231xx-cards.c")
    patch_video(root / "drivers/media/usb/cx231xx/cx231xx-video.c")
    patch_si2157(root / "drivers/media/tuners/si2157.c")

    header = root / "drivers/media/usb/cx231xx/cx231xx.h"
    dvb = root / "drivers/media/usb/cx231xx/cx231xx-dvb.c"

    if MARK in header.read_text():
        die("ABI guard failed: cx231xx.h was modified")
    if MARK in dvb.read_text():
        die("ABI guard failed: cx231xx-dvb.c was modified")

    print("PASS: two-module HVR-955Q FM patch applied")
    print("PASS: cx231xx.h untouched")
    print("PASS: cx231xx-dvb.c untouched")
    print("PASS: custom modules required: cx231xx + si2157 only")

if __name__ == "__main__":
    main()
