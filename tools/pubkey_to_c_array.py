"""
Converts an OpenDMS X25519 static public key (as printed/stored by the
meshbeacon Laravel repo, config/services.php `duck_crypto.public_key` /
DUCK_CRYPTO_PUBLIC_KEY -- base64) into either:
  - a C byte array initializer to paste into src/security/OpenDmsConfig.cpp,
    replacing the OPENDMS_STATIC_PUBLIC_KEY placeholder before flashing, or
  - a plain hex string for the OPENDMS_STATIC_PUBLIC_KEY_HEX build flag
    (see src/security/OpenDmsConfig.h), which OpenDmsConfig.cpp decodes at
    startup instead -- no pre-generation/pasting needed.

Usage:
    python tools/pubkey_to_c_array.py "<base64-or-hex-public-key>"
    python tools/pubkey_to_c_array.py --format hex "<base64-public-key>"

Accepts either base64 (default, matches OpenDMS's config value) or hex
(64 hex chars, matching what AT+OPENDMSKEY?/hexEncodeKey() prints over
serial) as input -- pass --hex if the input is already hex. Always
validates the decoded key is exactly 32 bytes (duckcrypto::PUBLIC_KEY_LENGTH)
before printing.

This key is not secret (only the matching private key, held by OpenDMS,
must stay confidential) -- see docs/crypto-design.tex, "Field Operator
Onboarding" -- so it's fine to compile into firmware/source control.
"""
import argparse
import base64
import sys

PUBLIC_KEY_LENGTH = 32


def decode_key(key_str, is_hex):
    key_str = key_str.strip()
    if is_hex:
        try:
            raw = bytes.fromhex(key_str)
        except ValueError as e:
            raise SystemExit("Invalid hex string: %s" % e)
    else:
        try:
            raw = base64.b64decode(key_str, validate=True)
        except Exception as e:
            raise SystemExit("Invalid base64 string: %s" % e)

    if len(raw) != PUBLIC_KEY_LENGTH:
        raise SystemExit(
            "Decoded key is %d bytes, expected %d (duckcrypto::PUBLIC_KEY_LENGTH)"
            % (len(raw), PUBLIC_KEY_LENGTH)
        )
    return raw


def to_c_array(raw, var_name="OPENDMS_STATIC_PUBLIC_KEY"):
    hex_bytes = ", ".join("0x%02x" % b for b in raw)
    return "uint8_t %s[duckcrypto::PUBLIC_KEY_LENGTH] = {%s};" % (var_name, hex_bytes)


def to_hex(raw):
    return raw.hex()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("key", help="the public key string (base64 by default)")
    parser.add_argument(
        "--hex", action="store_true",
        help="treat the input as 64 hex chars instead of base64",
    )
    parser.add_argument(
        "--format", choices=["carray", "hex"], default="carray",
        help="output a C array initializer (default) or a plain hex string "
             "for the OPENDMS_STATIC_PUBLIC_KEY_HEX build flag",
    )
    args = parser.parse_args()

    raw = decode_key(args.key, args.hex)
    print(to_hex(raw) if args.format == "hex" else to_c_array(raw))


if __name__ == "__main__":
    sys.exit(main())
