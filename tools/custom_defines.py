Import("env")
import os

# Optional per-build overrides, provisioned via plain shell environment
# variables (NOT the PLATFORMIO_BUILD_FLAGS system env var). This exists
# because PLATFORMIO_BUILD_FLAGS *replaces* an environment's entire
# `build_flags` option instead of merging with it (see PlatformIO's
# ProjectConfigBase.getraw() in platformio/project/config.py) -- so using it
# on an env that already sets build_flags (e.g. any of the *_encrypted envs,
# which rely on -DDUCK_CRYPTO_DEFAULT_ENABLED=1/-DMESH_GROUP_KEY_HEX=... from
# platformio.ini) silently discards those flags. Setting DUCK_ID and/or
# OPENDMS_STATIC_PUBLIC_KEY_HEX as plain env vars and letting this script
# Append() them as CPPDEFINES avoids that footgun entirely.
#
# Example:
#   DUCK_ID=ZAIHAN12 OPENDMS_STATIC_PUBLIC_KEY_HEX=<64 hex chars> \
#     EXAMPLE_DIR=Basic-Ducks/Heltec pio run -e local_heltec_wifi_lora_32_V3_encrypted -t upload
OVERRIDE_VARS = ("DUCK_ID", "OPENDMS_STATIC_PUBLIC_KEY_HEX", "MESH_GROUP_KEY_HEX")

defines = []
for name in OVERRIDE_VARS:
    value = os.getenv(name)
    if value:
        defines.append((name, env.StringifyMacro(value)))

if defines:
    env.Append(CPPDEFINES=defines)
    print("custom_defines.py: overriding {}".format(", ".join(name for name, _ in defines)))
