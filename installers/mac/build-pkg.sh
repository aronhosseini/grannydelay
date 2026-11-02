#!/bin/bash
# Granny Delay - sign plugins, then build GrannyDelay-macOS.zip / .pkg / .dmg inside <dist-dir>.
# <dist-dir> must contain GrannyDelay.vst3 (and GrannyDelay.component).
#
# Without secrets: plugins are ad-hoc signed (so Apple Silicon will load them).
# With APPLE_APP_IDENTITY / APPLE_INSTALLER_IDENTITY / APPLE_ID / APPLE_TEAM_ID / APPLE_APP_PASSWORD:
# plugins are Developer ID signed, the pkg is signed + notarized + stapled.
set -euo pipefail

DIST="${1:?usage: build-pkg.sh <dist-dir>}"
VERSION="${GD_VERSION:-1.0.0}"
APP_ID="${APPLE_APP_IDENTITY:-}"
INST_ID="${APPLE_INSTALLER_IDENTITY:-}"

cd "$DIST"

# ---- 1. sign the bundles
for bundle in GrannyDelay.vst3 GrannyDelay.component; do
    if [ -d "$bundle" ]; then
        xattr -cr "$bundle" || true
        if [ -n "$APP_ID" ]; then
            codesign --force --deep --options runtime --timestamp -s "$APP_ID" "$bundle"
        else
            codesign --force --deep -s - "$bundle"
        fi
        codesign --verify --deep "$bundle" && echo "signed: $bundle"
    fi
done

# ---- 2. zip (used by install.sh and the Homebrew cask)
rm -f GrannyDelay-macOS.zip
BUNDLES=(GrannyDelay.vst3)
[ -d GrannyDelay.component ] && BUNDLES+=(GrannyDelay.component)
zip -r -y -q GrannyDelay-macOS.zip "${BUNDLES[@]}"

# ---- 3. installer package (installs system-wide into /Library/Audio/Plug-Ins)
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

mkdir -p "$STAGE/root/Library/Audio/Plug-Ins/VST3" "$STAGE/scripts"
ditto GrannyDelay.vst3 "$STAGE/root/Library/Audio/Plug-Ins/VST3/GrannyDelay.vst3"
if [ -d GrannyDelay.component ]; then
    mkdir -p "$STAGE/root/Library/Audio/Plug-Ins/Components"
    ditto GrannyDelay.component "$STAGE/root/Library/Audio/Plug-Ins/Components/GrannyDelay.component"
fi

cat > "$STAGE/scripts/postinstall" <<'POST'
#!/bin/bash
# remove the quarantine flag from the installed plugins
for p in "/Library/Audio/Plug-Ins/VST3/GrannyDelay.vst3" "/Library/Audio/Plug-Ins/Components/GrannyDelay.component"; do
    [ -e "$p" ] && xattr -dr com.apple.quarantine "$p" 2>/dev/null
done
exit 0
POST
chmod +x "$STAGE/scripts/postinstall"

pkgbuild --root "$STAGE/root" --scripts "$STAGE/scripts" \
         --identifier com.aronh.grannydelay --version "$VERSION" \
         --install-location / "$STAGE/unsigned.pkg"

rm -f GrannyDelay-macOS.pkg
if [ -n "$INST_ID" ]; then
    productsign --sign "$INST_ID" --timestamp "$STAGE/unsigned.pkg" GrannyDelay-macOS.pkg
    if [ -n "${APPLE_ID:-}" ] && [ -n "${APPLE_TEAM_ID:-}" ] && [ -n "${APPLE_APP_PASSWORD:-}" ]; then
        xcrun notarytool submit GrannyDelay-macOS.pkg --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" \
                                --password "$APPLE_APP_PASSWORD" --wait
        xcrun stapler staple GrannyDelay-macOS.pkg
    fi
else
    cp "$STAGE/unsigned.pkg" GrannyDelay-macOS.pkg
fi

# ---- 4. disk image
mkdir -p "$STAGE/dmg"
cp GrannyDelay-macOS.pkg "$STAGE/dmg/Install Granny Delay.pkg"
cat > "$STAGE/dmg/READ ME FIRST.txt" <<'TXT'
Granny Delay - macOS install

1. Double-click "Install Granny Delay.pkg".

If macOS says it cannot verify the installer:
  - Open System Settings > Privacy & Security, scroll down and click "Open Anyway", or
  - run this in Terminal, then open the pkg again:
        xattr -dr com.apple.quarantine "/Volumes/Granny Delay/Install Granny Delay.pkg"

Easiest of all (no warning at all) - paste this into Terminal:
        curl -fsSL https://github.com/__REPO__/releases/latest/download/install.sh | bash
TXT
if [ -n "${GD_REPO:-}" ]; then sed -i '' "s|__REPO__|$GD_REPO|g" "$STAGE/dmg/READ ME FIRST.txt"; fi

rm -f GrannyDelay-macOS.dmg
hdiutil create GrannyDelay-macOS.dmg -volname "Granny Delay" -srcfolder "$STAGE/dmg" -ov -format UDZO

echo "Created: GrannyDelay-macOS.zip / .pkg / .dmg in $DIST"
