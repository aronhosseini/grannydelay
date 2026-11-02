#!/bin/bash
# Granny Delay - macOS installer (no Gatekeeper prompts).
#
#   curl -fsSL https://github.com/__REPO__/releases/latest/download/install.sh | bash
#
# Options (append after "bash -s --"):  --system   install for all users (/Library, asks for your password)
#                                       --uninstall remove Granny Delay
#
# Files downloaded by curl are not quarantined, and the script also strips the flag explicitly,
# so macOS does not block the plugin.
set -euo pipefail

REPO="${GD_REPO:-__REPO__}"
URL="https://github.com/${REPO}/releases/latest/download/GrannyDelay-macOS.zip"

BASE="$HOME/Library/Audio/Plug-Ins"
SUDO=""
UNINSTALL=false
for arg in "$@"; do
    case "$arg" in
        --system)    BASE="/Library/Audio/Plug-Ins"; SUDO="sudo" ;;
        --uninstall) UNINSTALL=true ;;
        *) echo "Unknown option: $arg"; exit 1 ;;
    esac
done

VST3_DIR="$BASE/VST3"
AU_DIR="$BASE/Components"

if [ "$UNINSTALL" = true ]; then
    $SUDO rm -rf "$VST3_DIR/GrannyDelay.vst3" "$AU_DIR/GrannyDelay.component"
    echo "Granny Delay removed."
    exit 0
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "Downloading Granny Delay..."
curl -fL --progress-bar "$URL" -o "$TMP/GrannyDelay-macOS.zip"
ditto -x -k "$TMP/GrannyDelay-macOS.zip" "$TMP/unpacked"

install_bundle() {
    local src="$1" destdir="$2"
    [ -d "$src" ] || return 0
    $SUDO mkdir -p "$destdir"
    $SUDO rm -rf "$destdir/$(basename "$src")"
    $SUDO ditto "$src" "$destdir/$(basename "$src")"
    $SUDO xattr -dr com.apple.quarantine "$destdir/$(basename "$src")" 2>/dev/null || true
    $SUDO codesign --force --deep -s - "$destdir/$(basename "$src")" 2>/dev/null || true
    echo "Installed: $destdir/$(basename "$src")"
}

install_bundle "$TMP/unpacked/GrannyDelay.vst3"      "$VST3_DIR"
install_bundle "$TMP/unpacked/GrannyDelay.component" "$AU_DIR"

echo
echo "Done. Rescan plugins in your DAW (restart it if Granny Delay does not show up)."
