#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
APP_NAME="Partiels"

APP_VERSION=$(defaults read $REPO_PATH/build/Partiels.app/Contents/Info.plist CFBundleShortVersionString)

test -f "$REPO_PATH/build/$APP_NAME-v$APP_VERSION.dmg" && rm "$REPO_PATH/build/$APP_NAME-v$APP_VERSION.dmg"

echo '\033[0;34m' "Creating apple disk image..."
echo '\033[0m'
cd $REPO_PATH/BinaryData/Resource
appdmg macos-dmg-config.json "$REPO_PATH/build/$APP_NAME-v$APP_VERSION.dmg"

xcrun rez -append "$REPO_PATH/BinaryData/Resource/macos-dmg-icon.rsrc" -o "$REPO_PATH/build/$APP_NAME-v$APP_VERSION.dmg"
xcrun setFile -a C "$REPO_PATH/build/$APP_NAME-v$APP_VERSION.dmg"
