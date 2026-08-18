"""
Post-build script: merges bootloader + partition table + boot_app0 + the
application image into a single flashable firmware.factory.bin for ESP32
boards.

PlatformIO/Arduino-ESP32 only ever writes firmware.bin (the standalone app
image) and feeds bootloader.bin/partitions.bin/boot_app0.bin to esptool as
separate `write_flash <offset> <file>` arguments at upload time (via
FLASH_EXTRA_IMAGES) -- it never merges them into one file on disk. This
script uses esptool's `merge_bin` command to produce that single "factory"
image so it can be written to a blank board in one shot, e.g.:
  esptool.py --chip esp32 write_flash 0x0 firmware.factory.bin
"""
Import("env")  # noqa: F821  (SCons global)

import os
import subprocess


def _gen_factory_bin(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    app_bin = os.path.join(build_dir, "firmware.bin")
    if not os.path.isfile(app_bin):
        print("gen_esp32_factory_bin: %s not found, skipping" % app_bin)
        return

    extra_images = env.get("FLASH_EXTRA_IMAGES", [])
    if not extra_images:
        print("gen_esp32_factory_bin: no FLASH_EXTRA_IMAGES (bootloader/partitions), skipping")
        return

    board = env.BoardConfig()
    mcu = board.get("build.mcu", "esp32")
    app_offset = env.subst("$ESP32_APP_OFFSET")
    factory_bin = os.path.join(build_dir, "firmware.factory.bin")

    esptool = os.path.join(env.PioPlatform().get_package_dir("tool-esptoolpy") or "", "esptool.py")
    merge_args = [
        env.subst("$PYTHONEXE"),
        esptool,
        "--chip", mcu,
        "merge_bin",
        "-o", factory_bin,
        "--flash_mode", env.subst("${__get_board_flash_mode(__env__)}"),
        "--flash_freq", env.subst("${__get_board_f_flash(__env__)}"),
        "--flash_size", board.get("upload.flash_size", "4MB"),
    ]
    for offset, image in extra_images:
        merge_args += [env.subst(offset), env.subst(image)]
    merge_args += [app_offset, app_bin]

    ret = subprocess.call(merge_args)
    if ret == 0:
        size_kb = os.path.getsize(factory_bin) // 1024
        print("\n*** Factory image ready (%d KB): %s" % (size_kb, factory_bin))
    else:
        print("gen_esp32_factory_bin: merge_bin returned error code %d" % ret)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", _gen_factory_bin)
