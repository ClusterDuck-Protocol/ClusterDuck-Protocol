"""
Pre-build script: makes the Adafruit nRF52 core's SVC_Handler and
HardFault_Handler weak so example sketches (MamaDuck.ino, BLETest.ino) can
install their own strong overrides — FreeRTOS+SoftDevice SVC dispatch and a
fault-visible LED blink — without a linker error.

Without this, linking fails with:
  multiple definition of `SVC_Handler'
  multiple definition of `HardFault_Handler'
because the stock framework defines both as strong (non-weak) symbols in
freertos/portable/GCC/nrf52/port.c (vPortSVCHandler, macro-renamed to
SVC_Handler via FreeRTOSConfig.h) and cores/nRF5/utility/debug.cpp.

Idempotent: safe to run on every build; skips files that are already patched.
"""
Import("env")  # noqa: F821  (SCons global)

import os


def _patch(path, replacements, marker):
    if not os.path.isfile(path):
        print("[patch_nrf52_weak_handlers] WARNING: not found: {}".format(path))
        return
    with open(path, "r") as f:
        content = f.read()
    if marker in content:
        return  # already patched
    changed = False
    for old, new in replacements:
        if old in content:
            content = content.replace(old, new, 1)
            changed = True
        else:
            print("[patch_nrf52_weak_handlers] WARNING: pattern not found in {}: {!r}".format(path, old))
    if changed:
        with open(path, "w") as f:
            f.write(content)
        print("[patch_nrf52_weak_handlers] patched {}".format(path))


framework_dir = env.PioPlatform().get_package_dir("framework-arduinoadafruitnrf52")
if framework_dir:
    port_c = os.path.join(
        framework_dir, "cores", "nRF5", "freertos", "portable", "GCC", "nrf52", "port.c"
    )
    _patch(
        port_c,
        [
            (
                "void vPortSVCHandler( void ) __attribute__ (( naked ));",
                "void vPortSVCHandler( void ) __attribute__ (( naked, weak ));",
            ),
            (
                "void vPortSVCHandler( void )\n{",
                "__attribute__(( weak ))\nvoid vPortSVCHandler( void )\n{",
            ),
        ],
        marker="__attribute__(( weak ))\nvoid vPortSVCHandler",
    )

    debug_cpp = os.path.join(framework_dir, "cores", "nRF5", "utility", "debug.cpp")
    _patch(
        debug_cpp,
        [
            (
                "void HardFault_Handler(void)\n{",
                "__attribute__((weak))\nvoid HardFault_Handler(void)\n{",
            ),
        ],
        marker="__attribute__((weak))\nvoid HardFault_Handler",
    )
else:
    print("[patch_nrf52_weak_handlers] WARNING: framework-arduinoadafruitnrf52 package dir not found")
