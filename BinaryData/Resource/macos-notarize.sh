#!/bin/sh
# Notarizes $1 and staples $2 (defaults to $1) using the "notary-installer" keychain profile.
# Stapling requires the destination to be a bundle/pkg/dmg, not a zip archive.
set -e

FILE_PATH="$1"
STAPLE_PATH="${2:-$1}"
LOG_FILE="$(dirname "$FILE_PATH")/notarize.log"

xcrun notarytool submit "$FILE_PATH" --keychain-profile "notary-installer" --wait > "$LOG_FILE" 2>&1
cat "$LOG_FILE"
notaryid=$(awk '/^  id:/{print $2; exit}' "$LOG_FILE")
xcrun notarytool log "$notaryid" --keychain-profile "notary-installer"
xcrun stapler staple "$STAPLE_PATH"
xcrun stapler validate "$STAPLE_PATH"
