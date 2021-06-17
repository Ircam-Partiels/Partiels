#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
APP_NAME="Partiels"

APP_VERSION=$(defaults read $REPO_PATH/build/Partiels.app/Contents/Info.plist CFBundleShortVersionString)

cd $REPO_PATH/build

echo '\033[0;34m' "Creating apple disk image..."
echo '\033[0m'
mkdir -p $REPO_PATH/build/app
cp -r -f $REPO_PATH/build/Partiels.app $REPO_PATH/build/app/
test -f "$APP_NAME-v$APP_VERSION.dmg" && rm "$APP_NAME-v$APP_VERSION.dmg"

create-dmg \
	--volname "$APP_NAME-v$APP_VERSION" \
	--background "$REPO_PATH/BinaryData/Misc/macos-dmg-background.png" \
	--volicon "$REPO_PATH/BinaryData/Misc/macos-dmg-icon.icns" \
	--window-size 600 410 \
	--window-pos 200 120 \
	--icon-size 64 \
	--icon "Partiels.app" 166 94 \
  --app-drop-link 432 94 \
	--add-file "About.txt" "$REPO_PATH/BinaryData/Misc/About.txt" 392 280 \
	--add-file "ChangeLog.txt" "$REPO_PATH/BinaryData/Misc/ChangeLog.txt" 512 280 \
	--no-internet-enable \
	"$APP_NAME-v$APP_VERSION.dmg" \
	"app"

xcrun rez -append "$REPO_PATH/BinaryData/Misc/macos-dmg-icon.rsrc" -o "$APP_NAME-v$APP_VERSION.dmg"
xcrun setFile -a C "$APP_NAME-v$APP_VERSION.dmg"
