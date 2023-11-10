#!/bin/sh

APP_NAME="Partiels"

if [ -z $1 ]; then
	echo '\033[0;31m' "Error: Partiels build tag is undefined"
  exit -1
else
  PARTIELS_BUILD_TAG=$1
fi

if [ -z $2 ]; then
	echo '\033[0;31m' "Error: Partiels build ID is undefined"
  exit -1
else
  PARTIELS_BUILD_ID=$2
fi

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
BUILD_PATH="$REPO_PATH/release"

mkdir -p $BUILD_PATH
cd $BUILD_PATH
rm -rf *

echo '\033[0;34m' "Preparing project..."
echo '\033[0m'
cmake .. -G Xcode -DPARTIELS_BUILD_TAG=$PARTIELS_BUILD_TAG -DPARTIELS_BUILD_ID=$PARTIELS_BUILD_ID

echo '\033[0;34m' "Creating archive..."
echo '\033[0m'
if ! command -v xcbeautify &> /dev/null; then
  echo "Install xcbeautify \`brew install xcbeautify\` to generates clean xcodebuild output"
  xcodebuild archive -project "$APP_NAME.xcodeproj" -configuration "Release" -scheme "Partiels" -archivePath "$APP_NAME.xcarchive" -destination platform=macOS
else
  xcodebuild archive -project "$APP_NAME.xcodeproj" -configuration "Release" -scheme "Partiels" -archivePath "$APP_NAME.xcarchive" -destination platform=macOS | xcbeautify
fi

echo '\033[0;34m' "Exporting archive..."
echo '\033[0m'
xcodebuild -exportArchive -archivePath "$APP_NAME.xcarchive" -exportPath "." -exportOptionsPlist "$ThisPath/macos-export-archive.plist"

echo '\033[0;34m' "Uploading archive..."
echo '\033[0m'
ditto -c -k --keepParent "$APP_NAME.app" "$APP_NAME.zip"
xcrun notarytool submit "$APP_NAME.zip" --keychain-profile "Application Partiels" --wait

xcrun stapler staple $APP_NAME.app
xcrun stapler validate $APP_NAME.app

echo '\033[0;34m' "Creating apple disk image..."
echo '\033[0m'
APP_VERSION=$(defaults read $BUILD_PATH/Partiels.app/Contents/Info.plist CFBundleShortVersionString)
DMG_NAME="$BUILD_PATH/$APP_NAME-v$APP_VERSION"

test -f "$DMG_NAME.dmg" && rm "$DMG_NAME.dmg"
cd $REPO_PATH/BinaryData/Resource
appdmg macos-dmg-config.json "$DMG_NAME.dmg"

xcrun rez -append "$REPO_PATH/BinaryData/Resource/macos-dmg-icon.rsrc" -o "$DMG_NAME.dmg"
xcrun setFile -a C "$DMG_NAME.dmg"

echo '\033[0;34m' "Uploading apple disk image..."
echo '\033[0m'
xcrun notarytool submit "$DMG_NAME.dmg" --keychain-profile "Application Partiels" --wait

xcrun stapler staple $DMG_NAME.dmg
xcrun stapler validate $DMG_NAME.dmg

echo '\033[0;34m' "done"
echo '\033[0m'
