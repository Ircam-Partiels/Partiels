#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
APP_NAME="Partiels"

APPLE_ACCOUNT="pierre.guillot@ircam.fr"
APPLE_PASSWORD="Developer-altool"

APP_VERSION=$(defaults read $REPO_PATH/build/Partiels.app/Contents/Info.plist CFBundleShortVersionString)
DMG_PATH="$REPO_PATH/build/$APP_NAME-v$APP_VERSION.dmg"

test -f "$DMG_PATH" && rm "$DMG_PATH"

echo '\033[0;34m' "Creating apple disk image..."
echo '\033[0m'
cd $REPO_PATH/BinaryData/Resource
appdmg macos-dmg-config.json "$DMG_PATH"

xcrun rez -append "$REPO_PATH/BinaryData/Resource/macos-dmg-icon.rsrc" -o "$DMG_PATH"
xcrun setFile -a C "$DMG_PATH"

xc_out=$(xcrun altool --notarize-app --primary-bundle-id "fr.ircam.dev.partiels" --username "$APPLE_ACCOUNT" --password "@keychain:$APPLE_PASSWORD" --file "$DMG_PATH" 2>&1)
MAX_UPLOAD_ATTEMPTS=100
requestUUID=$(echo "$xc_out" | awk '/RequestUUID/ { print $NF; }')

if [[ $requestUUID == "" ]]; then
    echo "Failed to upload: $xc_out"
    exit 1
fi

echo '\033[0;34m' "Waiting for notarization..."
echo '\033[0m'
request_status="in progress"
count=0
while [[ "$count" -lt $MAX_UPLOAD_ATTEMPTS && ("$request_status" == "in progress" || "$request_status" == "") ]]; do
    sleep 10
    request_status=$(xcrun altool --notarization-info "$requestUUID" --username "$APPLE_ACCOUNT" --password "@keychain:$APPLE_PASSWORD" 2>&1 | awk -F ': ' '/Status:/ { print $2; }' )
    echo "$request_status"
    count=$((count+1))
done

xcrun altool --notarization-info "$requestUUID" --username "$APPLE_ACCOUNT" --password "@keychain:$APPLE_PASSWORD"

xcrun stapler staple $DMG_PATH
xcrun stapler validate $DMG_PATH
