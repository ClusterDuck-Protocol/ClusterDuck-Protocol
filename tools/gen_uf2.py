"""
Post-build script: converts firmware.hex → firmware.uf2 for nRF52840 boards.

The UF2 family ID 0xADA52840 matches the Adafruit nRF52840 bootloader (used by
the Seeed Wio Tracker L1 Pro and other nRF52840 boards with Adafruit bootloader).

Drop the generated firmware.uf2 onto the USB drive that appears when you
double-tap the reset button to enter bootloader mode.
"""
Import("env")  # noqa: F821  (SCons global)

import os
import subprocess


def _gen_uf2(source, target, env):
    bsp_dir = env.PioPlatform().get_package_dir("framework-arduinoadafruitnrf52")
    if not bsp_dir:
        print("gen_uf2: framework-arduinoadafruitnrf52 not found, skipping UF2 generation")
        return

    uf2conv = os.path.join(bsp_dir, "tools", "uf2conv", "uf2conv.py")
    hex_path = os.path.join(env.subst("$BUILD_DIR"), "firmware.hex")
    uf2_path = os.path.join(env.subst("$BUILD_DIR"), "firmware.uf2")

    if not os.path.isfile(hex_path):
        print("gen_uf2: %s not found, skipping" % hex_path)
        return

    ret = subprocess.call([
        env.subst("$PYTHONEXE"),
        uf2conv,
        "--family", "0xADA52840",   # Adafruit nRF52840; change to 0x1b57745f if bootloader rejects
        "--output", uf2_path,
        hex_path,
    ])

    if ret == 0:
        size_kb = os.path.getsize(uf2_path) // 1024
        print("\n*** UF2 image ready (%d KB): %s" % (size_kb, uf2_path))
    else:
        print("gen_uf2: uf2conv.py returned error code %d" % ret)


env.AddPostAction("$BUILD_DIR/firmware.hex", _gen_uf2)
