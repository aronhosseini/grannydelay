#!/bin/bash
# Usage: make-release-files.sh <github-owner/repo> <release-tag or empty> <dist-dir>
# Writes install.sh (with the repo filled in) and granny-delay.rb (Homebrew cask) into <dist-dir>.
set -euo pipefail

REPO="${1:?usage: make-release-files.sh <owner/repo> <tag> <dist-dir>}"
TAG="${2:-}"
DIST="${3:?dist dir}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

sed "s|__REPO__|$REPO|g" "$HERE/install.sh" > "$DIST/install.sh"
chmod +x "$DIST/install.sh"

if [ -n "$TAG" ]; then
    URL="https://github.com/$REPO/releases/download/$TAG/GrannyDelay-macOS.zip"
    VERSION="${TAG#v}"
else
    URL="https://github.com/$REPO/releases/latest/download/GrannyDelay-macOS.zip"
    VERSION="${GD_VERSION:-1.0.0}"
fi
SHA="$(shasum -a 256 "$DIST/GrannyDelay-macOS.zip" | awk '{print $1}')"

cat > "$DIST/granny-delay.rb" <<RUBY
cask "granny-delay" do
  version "$VERSION"
  sha256 "$SHA"

  url "$URL"
  name "Granny Delay"
  desc "Granular delay plugin by Aron H (VST3 / AU)"
  homepage "https://github.com/$REPO"

  vst3_plugin "GrannyDelay.vst3"
  audio_unit_plugin "GrannyDelay.component"

  postflight do
    [
      "#{Dir.home}/Library/Audio/Plug-Ins/VST3/GrannyDelay.vst3",
      "#{Dir.home}/Library/Audio/Plug-Ins/Components/GrannyDelay.component",
    ].each do |path|
      system_command "/usr/bin/xattr", args: ["-dr", "com.apple.quarantine", path]
    end
  end
end
RUBY

echo "Wrote $DIST/install.sh and $DIST/granny-delay.rb"
