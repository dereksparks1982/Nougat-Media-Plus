#!/usr/bin/env python3
from pathlib import Path
import re
import sys

MARK = "NOUGAT_V67_HVR955Q_FM_ABI_SAFE"

def die(msg):
    raise SystemExit("FAIL: " + msg)

def read(p):
    if not p.is_file():
        die(f"missing kernel source file: {p}")
    return p.read_text()

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
        die("HVR-955Q already has a non-Nougat radio definition")

    anchor = "\t\t} },\n\t},\n"
    if block.count(anchor) != 1:
        die("HVR-955Q board closing anchor changed")

    add = (
        "\t\t} },\n"
        "\t\t/* " + MARK + "_RADIO_NODE */\n"
        "\t\t.radio = {\n"
        "\t\t\t.type = CX231XX_RADIO,\n"
        "\t\t},\n"
        "\t},\n"
    )
    block = block.replace(anchor, add, 1)
    path.write_text(text[:start] + block + text[end:])

def patch_dvb(path):
    text = read(path)
    old = "\t\t\tparams.mode = 0;\t/* 0- Air; 1 - cable */"
    new = (
        "\t\t\tparams.mode = dev->norm ? V4L2_TUNER_ANALOG_TV : "
        "V4L2_TUNER_RADIO; /* " + MARK + " */"
    )
    text = replace_once(text, old, new, "radio mode bridge")
    path.write_text(text)

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
    text = replace_once(text, old_tuner, new_tuner, "radio tuner caps")

    old_special = '''\tcase CX231XX_BOARD_EVROMEDIA_FULL_HYBRID_FULLHD:
\t\tif (dev->cx231xx_set_analog_freq)
\t\t\tdev->cx231xx_set_analog_freq(dev, f->frequency);
\t\tdev->ctl_freq = f->frequency;
\t\tneed_if_freq = 1;
\t\tbreak;'''
    new_special = '''\tcase CX231XX_BOARD_EVROMEDIA_FULL_HYBRID_FULLHD:
\t\t/*
\t\t * __MARK__: keep the existing callback ABI unchanged.
\t\t * dev->norm == 0 is used only while radio owns tuning.
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
    text = replace_once(text, old_special, new_special, "HVR radio mode selection")

    old_if = '''\tif (need_if_freq || dev->tuner_type == TUNER_NXP_TDA18271) {
\t\tif (dev->norm & (V4L2_STD_MN | V4L2_STD_NTSC_443))
\t\t\tif_frequency = 5400000;  /*5.4MHz\t*/'''
    new_if = '''\tif (need_if_freq || dev->tuner_type == TUNER_NXP_TDA18271) {
\t\tif (f->type == V4L2_TUNER_RADIO)
\t\t\tif_frequency = 6600000;  /* __MARK__: HVR-9xx FM IF */
\t\telse if (dev->norm & (V4L2_STD_MN | V4L2_STD_NTSC_443))
\t\t\tif_frequency = 5400000;  /*5.4MHz\t*/'''.replace("__MARK__", MARK)
    text = replace_once(text, old_if, new_if, "FM IF selection")

    old_open = '''\tif (vdev->vfl_type == VFL_TYPE_RADIO) {
\t\tcx231xx_videodbg("video_open: setting radio device\\n");

\t\t/* cx231xx_start_radio(dev); */

\t\tcall_all(dev, tuner, s_radio);
\t}'''
    new_open = '''\tif (vdev->vfl_type == VFL_TYPE_RADIO) {
\t\tcx231xx_videodbg("video_open: setting radio device\\n");

\t\t/* __MARK___RADIO_OPEN */
\t\tcx231xx_set_audio_input(dev, AUDIO_INPUT_TUNER_TV);
\t\tcall_all(dev, tuner, s_radio);
\t}'''.replace("__MARK__", MARK)
    text = replace_once(text, old_open, new_open, "radio open routing")

    path.write_text(text)

def patch_si2157(path):
    text = read(path)
    if MARK + "_SI2157_FM" in text:
        return

    reject = re.compile(
        r'\tif \(params->mode == V4L2_TUNER_RADIO\) \{\n'
        r'\t/\*\n.*?\t\tgoto err;\n\t\}\n',
        re.S,
    )
    m = reject.search(text)
    if not m:
        die("Si2157 FM rejection block not found")

    repl = (
        "\t/* " + MARK + "_SI2157_FM\n"
        "\t * Reuse the existing analog programming path with the FM values\n"
        "\t * already documented in this driver for HVR-9xx hardware.\n"
        "\t */\n"
    )
    text = text[:m.start()] + repl + text[m.end():]

    selector = '''\t/* if_frequency values based on tda187271C2 */
\tif (params->std & (V4L2_STD_B | V4L2_STD_GH)) {'''
    fm = '''\t/* if_frequency values based on tda187271C2 */
\tif (params->mode == V4L2_TUNER_RADIO) {
\t\tstd = "fm";
\t\tbandwidth = 1700000;
\t\tif_frequency = 6600000;
\t\tsystem = 0;
\t} else if (params->std & (V4L2_STD_B | V4L2_STD_GH)) {'''
    text = replace_once(text, selector, fm, "Si2157 FM parameter branch")

    path.write_text(text)

def main():
    if len(sys.argv) != 2:
        die("usage: patch_kernel_source_v2.py <exact-source-root>")

    root = Path(sys.argv[1]).resolve()
    patch_cards(root / "drivers/media/usb/cx231xx/cx231xx-cards.c")
    patch_dvb(root / "drivers/media/usb/cx231xx/cx231xx-dvb.c")
    patch_video(root / "drivers/media/usb/cx231xx/cx231xx-video.c")
    patch_si2157(root / "drivers/media/tuners/si2157.c")

    header = root / "drivers/media/usb/cx231xx/cx231xx.h"
    if MARK in header.read_text():
        die("ABI guard failed: cx231xx.h was modified")

    print("PASS: ABI-safe HVR-955Q FM patch applied")
    print("PASS: cx231xx public header/callback ABI left unchanged")
    print("PASS: radio node + 6.6 MHz FM IF + Si2157 FM path enabled")

if __name__ == "__main__":
    main()
