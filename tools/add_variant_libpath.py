"""
Pre-build script: adds the board variant directory to the linker search path.

The nordicnrf52/adafruit.py framework builder adds only the BSP's linker/
directory to LIBPATH.  When the custom linker script (e.g. nrf52840_s140_v7.ld)
lives in the project's boards/<variant>/ directory, the linker fails with
"cannot open linker script file ... No such file or directory".

Adding the variant directory here ensures the linker can find it regardless of
whether it also exists in the BSP.
"""
Import("env")  # noqa: F821  (SCons global)

import os

variants_dir = env.GetProjectOption("board_build.variants_dir", default="boards")
board        = env.BoardConfig()
variant      = board.get("build.variant", "")

if variant:
    variant_ld_dir = os.path.join(
        env.subst("$PROJECT_DIR"), variants_dir, variant
    )
    if os.path.isdir(variant_ld_dir):
        env.Append(LIBPATH=[variant_ld_dir])
