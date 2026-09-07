#!/usr/bin/env python3
from pathlib import Path
import re
import sys

def die(msg):
    raise SystemExit("FAIL: " + msg)

def read(path):
    if not path.exists():
        die(f"kernel source file missing: {path}")
    return path.read_text()

def write(path, text):
    path.write_text(text)

def replace_once(text, old, new, label):
    if new in text:
        return text
    count = text.count(old)
    if count != 1:
        die(f"{label}: expected exactly one source anchor, found {count}")
    return text.replace(old, new, 1)

def patch_cards(path):
    text = read(path)
    if "NOUGAT_V67_HVR955Q_RADIO_NODE" in text:
        return

    start = text.find("[CX231XX_BOARD_HAUPPAUGE_955Q] = {")
    end = text.find("[CX231XX_BOARD_TERRATEC_GRABBY]", start)
    if start < 0 or end < 0:
        die("could not isolate HVR-955Q board definition")

    block = text[start:end]
    if ".radio =" in block:
        die("HVR-955Q already has an unexpected radio definition; refusing blind edit")

    needle = "\t\t} },\n\t},\n"
    if needle not in block:
        die("HVR-955Q board closing anchor changed")

    replacement = '''\t\t} },
\t\t/* NOUGAT_V67_HVR955Q_RADIO_NODE
\t\t * HVR-955Q shares the RF tuner path with analog TV. Expose
\t\t * the existing cx231xx radio node so userspace can select FM.
\t\t */
\t\t.radio = {
\t\t\t.type = CX231XX_RADIO,
\t\t\t.vmux = CX231XX_VIN_3_1,
\t\t\t.amux = CX231XX_AMUX_VIDEO,
\t\t\t.gpio = NULL,
\t\t},
\t},
'''
    block = block.replace(needle, replacement, 1)
    write(path, text[:start] + block + text[end:])

def patch_header(path):
    text = read(path)
    text = replace_once(
        text,
        "int (*cx231xx_set_analog_freq) (struct cx231xx *dev, u32 freq);",
        "int (*cx231xx_set_analog_freq) (struct cx231xx *dev, u32 freq,\n"
        "\t\t\t\t\t enum v4l2_tuner_type type); /* NOUGAT_V67_FM */",
        "cx231xx callback signature",
    )
    text = replace_once(
        text,
        "int cx231xx_set_analog_freq(struct cx231xx *dev, u32 freq);",
        "int cx231xx_set_analog_freq(struct cx231xx *dev, u32 freq,\n"
        "\t\t\t       enum v4l2_tuner_type type); /* NOUGAT_V67_FM */",
        "cx231xx callback prototype",
    )
    write(path, text)

def patch_dvb(path):
    text = read(path)
    text = replace_once(
        text,
        "int cx231xx_set_analog_freq(struct cx231xx *dev, u32 freq)\n{",
        "int cx231xx_set_analog_freq(struct cx231xx *dev, u32 freq,\n"
        "\t\t\t       enum v4l2_tuner_type type)\n{ /* NOUGAT_V67_FM */",
        "cx231xx dvb analog function signature",
    )
    text = replace_once(
        text,
        "\t\t\tparams.mode = 0;\t/* 0- Air; 1 - cable */",
        "\t\t\tparams.mode = type; /* NOUGAT_V67_FM: preserve RADIO vs ANALOG_TV */",
        "cx231xx analog mode forwarding",
    )
    write(path, text)

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

\t/* NOUGAT_V67_HVR955Q_RADIO_CAPS
\t * cx25840 deliberately returns early from g_tuner while in radio
\t * mode. Seed the radio capabilities/range here so V4L2 userspace
\t * receives a truthful FM tuner interface.
\t */
\tstrscpy(t->name, "FM Radio", sizeof(t->name));
\tt->type = V4L2_TUNER_RADIO;
\tt->capability |= V4L2_TUNER_CAP_LOW | V4L2_TUNER_CAP_STEREO;
\tt->rangelow = 1400000;  /* 87.5 MHz in 62.5 Hz LOW units */
\tt->rangehigh = 1728000; /* 108.0 MHz in 62.5 Hz LOW units */
\tt->audmode = V4L2_TUNER_MODE_STEREO;

\tcall_all(dev, tuner, g_tuner, t);

\treturn 0;
}'''
    text = replace_once(text, old_tuner, new_tuner, "radio tuner capability block")

    old_special = '''\tcase CX231XX_BOARD_EVROMEDIA_FULL_HYBRID_FULLHD:
\t\tif (dev->cx231xx_set_analog_freq)
\t\t\tdev->cx231xx_set_analog_freq(dev, f->frequency);
\t\tdev->ctl_freq = f->frequency;
\t\tneed_if_freq = 1;
\t\tbreak;'''
    new_special = '''\tcase CX231XX_BOARD_EVROMEDIA_FULL_HYBRID_FULLHD:
\t\tif (dev->cx231xx_set_analog_freq)
\t\t\tdev->cx231xx_set_analog_freq(dev, f->frequency, f->type);
\t\t/*
\t\t * NOUGAT_V67_HVR955Q_FM: the Si2157 is attached through the DVB
\t\t * frontend, while cx25840 is a V4L2 subdevice. Notify cx25840
\t\t * after the tuner moves so its existing radio audio configuration
\t\t * is applied.
\t\t */
\t\tif (f->type == V4L2_TUNER_RADIO)
\t\t\tcall_all(dev, tuner, s_frequency, f);
\t\tdev->ctl_freq = f->frequency;
\t\tneed_if_freq = 1;
\t\tbreak;'''
    text = replace_once(text, old_special, new_special, "HVR analog frequency dispatch")

    old_if = '''\tif (need_if_freq || dev->tuner_type == TUNER_NXP_TDA18271) {
\t\tif (dev->norm & (V4L2_STD_MN | V4L2_STD_NTSC_443))
\t\t\tif_frequency = 5400000;  /*5.4MHz\t*/'''
    new_if = '''\tif (need_if_freq || dev->tuner_type == TUNER_NXP_TDA18271) {
\t\tif (f->type == V4L2_TUNER_RADIO)
\t\t\tif_frequency = 6600000;  /* NOUGAT_V67_FM: HVR-9xx FM IF */
\t\telse if (dev->norm & (V4L2_STD_MN | V4L2_STD_NTSC_443))
\t\t\tif_frequency = 5400000;  /*5.4MHz\t*/'''
    text = replace_once(text, old_if, new_if, "cx231xx FM IF")

    old_open = '''\tif (vdev->vfl_type == VFL_TYPE_RADIO) {
\t\tcx231xx_videodbg("video_open: setting radio device\\n");

\t\t/* cx231xx_start_radio(dev); */

\t\tcall_all(dev, tuner, s_radio);
\t}'''
    new_open = '''\tif (vdev->vfl_type == VFL_TYPE_RADIO) {
\t\tcx231xx_videodbg("video_open: setting radio device\\n");

\t\t/*
\t\t * NOUGAT_V67_HVR955Q_RADIO_START
\t\t * The historical cx231xx_start_radio() never existed as a completed
\t\t * implementation. Route the existing tuner/SIF audio path and put
\t\t * cx25840 into its already-implemented radio mode.
\t\t */
\t\tcx231xx_set_audio_input(dev, AUDIO_INPUT_TUNER_TV);
\t\tcall_all(dev, tuner, s_radio);
\t}'''
    text = replace_once(text, old_open, new_open, "cx231xx radio open path")

    write(path, text)

def patch_si2157(path):
    text = read(path)
    if "NOUGAT_V67_HVR9XX_FM_TUNE" in text:
        return

    pattern = re.compile(
        r'\tif \(params->mode == V4L2_TUNER_RADIO\) \{\n'
        r'\t/\*\n'
        r'.*?'
        r'\t\tgoto err;\n'
        r'\t\}\n',
        re.S,
    )
    match = pattern.search(text)
    if not match:
        die("Si2157 FM rejection block not found")

    fm = r'''\tif (params->mode == V4L2_TUNER_RADIO) {
\t\t/*
\t\t * NOUGAT_V67_HVR9XX_FM_TUNE
\t\t * Upstream left this path as a hard -EINVAL with the required
\t\t * HVR-9xx values written in a comment. Use those documented
\t\t * values without inventing AGC thresholds:
\t\t *   1.7 MHz narrowest practical FM bandwidth
\t\t *   6.6 MHz HVR-9xx / cx231xx analog IF
\t\t */
\t\ttmp_lval = params->frequency * 625LL;
\t\tdo_div(tmp_lval, 10);
\t\tfreq = (u32)tmp_lval;
\t\tif (freq < 1000000)
\t\t\tfreq *= 1000;

\t\tif (freq < 64000000 || freq > 120000000) {
\t\t\tret = -EINVAL;
\t\t\tgoto err;
\t\t}

\t\tstd = "fm";
\t\tbandwidth = 1700000;
\t\tif_frequency = 6600000;
\t\tdev->frequency = freq;

\t\tdev_dbg(&client->dev,
\t\t\t"FM mode std='%s' frequency=%u if=%u bandwidth=%u\\n",
\t\t\tstd, freq, if_frequency, bandwidth);

\t\tif (dev->part_id == SI2177) {
\t\t\tret = -EINVAL;
\t\t\tgoto err;
\t\t}

\t\tmemcpy(cmd.args, "\x14\x00\x11\x06\x00\x00", 6);
\t\tcmd.wlen = 6;
\t\tcmd.rlen = 4;
\t\tret = si2157_cmd_execute(client, &cmd);
\t\tif (ret)
\t\t\tgoto err;

\t\tmemcpy(cmd.args, "\x14\x00\x03\x06\x08\x02", 6);
\t\tcmd.args[4] = (dev->if_port == 1) ? 8 : 10;
\t\tcmd.args[5] = (dev->if_port == 1) ? 2 : 1;
\t\tcmd.wlen = 6;
\t\tcmd.rlen = 4;
\t\tret = si2157_cmd_execute(client, &cmd);
\t\tif (ret)
\t\t\tgoto err;

\t\tmemcpy(cmd.args, "\x14\x00\x0d\x06\x94\x64", 6);
\t\tcmd.wlen = 6;
\t\tcmd.rlen = 4;
\t\tret = si2157_cmd_execute(client, &cmd);
\t\tif (ret)
\t\t\tgoto err;

\t\tdev->if_frequency = if_frequency | 1;

\t\tmemcpy(cmd.args, "\x14\x00\x0c\x06", 4);
\t\tcmd.args[4] = (if_frequency / 1000) & 0xff;
\t\tcmd.args[5] = ((if_frequency / 1000) >> 8) & 0xff;
\t\tcmd.wlen = 6;
\t\tcmd.rlen = 4;
\t\tret = si2157_cmd_execute(client, &cmd);
\t\tif (ret)
\t\t\tgoto err;

\t\t/* Preserve upstream analog AGC until real RF measurements exist. */
\t\tmemcpy(cmd.args, "\x14\x00\x07\x06\x32\xc8", 6);
\t\tcmd.wlen = 6;
\t\tcmd.rlen = 4;
\t\tret = si2157_cmd_execute(client, &cmd);
\t\tif (ret)
\t\t\tgoto err;

\t\tmemcpy(cmd.args, "\x14\x00\x10\x06\xdc\x05", 6);
\t\tcmd.wlen = 6;
\t\tcmd.rlen = 4;
\t\tret = si2157_cmd_execute(client, &cmd);
\t\tif (ret)
\t\t\tgoto err;

\t\tmemcpy(cmd.args, "\x41\x01\x00\x00\x00\x00\x00\x00", 8);
\t\tcmd.args[4] = (freq >>  0) & 0xff;
\t\tcmd.args[5] = (freq >>  8) & 0xff;
\t\tcmd.args[6] = (freq >> 16) & 0xff;
\t\tcmd.args[7] = (freq >> 24) & 0xff;
\t\tcmd.wlen = 8;
\t\tcmd.rlen = 1;
\t\tret = si2157_cmd_execute(client, &cmd);
\t\tif (ret)
\t\t\tgoto err;

\t\tdev->bandwidth = bandwidth;
\t\tsi2157_tune_wait(client, 0);
\t\treturn 0;
\t}
'''
    fm = fm.replace(r"\\t", "\t")
    fm = fm.replace(r"\\\\n", r"\n")
    text = text[:match.start()] + fm + text[match.end():]
    write(path, text)

def main():
    if len(sys.argv) != 2:
        die("usage: patch_kernel_source.py <linux-source-root>")
    root = Path(sys.argv[1]).resolve()

    patch_cards(root / "drivers/media/usb/cx231xx/cx231xx-cards.c")
    patch_header(root / "drivers/media/usb/cx231xx/cx231xx.h")
    patch_dvb(root / "drivers/media/usb/cx231xx/cx231xx-dvb.c")
    patch_video(root / "drivers/media/usb/cx231xx/cx231xx-video.c")
    patch_si2157(root / "drivers/media/tuners/si2157.c")

    print("PASS: HVR-955Q FM kernel source patches applied")

if __name__ == "__main__":
    main()
