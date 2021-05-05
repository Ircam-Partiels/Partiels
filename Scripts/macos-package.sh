#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
APP_NAME="Partiels"

APP_VERSION=$(defaults read $REPO_PATH/build/Partiels.app/Contents/Info.plist CFBundleShortVersionString)

echo '\033[0;34m' "Creating apple disk image..."
echo '\033[0m'
cp -r -f $REPO_PATH/build/Partiels.app $REPO_PATH/build/app
test -f "$APP_NAME-v$APP_VERSION.dmg" && rm "$APP_NAME-v$APP_VERSION.dmg"

create-dmg \
	--volname "$APP_NAME-v$APP_VERSION" \
	--background "$REPO_PATH/BinaryData/Misc/macos-dmg-background.png" \
	--volicon "$REPO_PATH/BinaryData/Misc/macos-dmg-icon.icns" \
	--window-size 600 380 \
	--window-pos 200 120 \
	--icon-size 64 \
	--icon "Partiels.app" 128 90 \
  --app-drop-link 432 90 \
	--add-file "About.txt" 	"$REPO_PATH/BinaryData/Misc/About.txt" 128 200 \
	"$APP_NAME-v$APP_VERSION.dmg" \
	"app"

xcrun rez -append "$REPO_PATH/BinaryData/Misc/macos-dmg-icon.rsrc" -o "$APP_NAME-v$APP_VERSION.dmg"
xcrun setFile -a C "$APP_NAME-v$APP_VERSION.dmg"
