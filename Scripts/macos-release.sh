#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
APP_NAME="Partiels"
APP_VERSION="Partiels"
APPLE_ACCOUNT="pierre.guillot@ircam.fr"
APPLE_PASSWORD="Developer-altool"

if [ -z $1 ];  then
	echo '\033[0;31m' "Error: No arguments provided. Expected build number as arg."
	echo '\033[0m'
	exit 1
else
	APP_VERSION=$1
	echo "Preparing $APP_NAME-$APP_VERSION";
fi

mkdir -p $REPO_PATH/build
cd $REPO_PATH/build

echo '\033[0;34m' "Prepare project..."
echo '\033[0m'
cmake .. -G Xcode

echo '\033[0;34m' "Creating archive..."
echo '\033[0m'
xcodebuild archive -project "$APP_NAME.xcodeproj" -quiet -configuration "Release" -scheme "$APP_NAME" -archivePath "$APP_PATH.xcarchive"

echo '\033[0;34m' "Exporting archive..."
echo '\033[0m'
xcodebuild -exportArchive -archivePath "$APP_NAME.xcarchive" -exportPath "." -exportOptionsPlist "$ThisPath/macos-export-archive.plist"

echo '\033[0;34m' "Uploading archive..."
echo '\033[0m'
ditto -c -k --keepParent "$APP_NAME.app" "$APP_NAME.zip"
xc_out=$(xcrun altool --notarize-app --primary-bundle-id "fr.ircam.dev.partiels" --username "$APPLE_ACCOUNT" --password "@keychain:$APPLE_PASSWORD" --file "$APP_NAME.zip" 2>&1)
MAX_UPLOAD_ATTEMPTS=30
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
    echo Status: "$request_status"
    count=$((count+1))
done

xcrun altool --notarization-info "$requestUUID" --username "$APPLE_ACCOUNT" --password "@keychain:$APPLE_PASSWORD"

if [[ $request_status != "success" ]]; then
		echo '\033[0;31m' "Error: could not notarize $APP_NAME.zip."
		echo '\033[0m'
    exit 1
fi

echo '\033[0;34m' "Creating apple disk image..."
echo '\033[0m'
test -f "$APP_NAME-$APP_VERSION.dmg" && rm "$APP_NAME-$APP_VERSION.dmg"

create-dmg \
	--volname "$APP_NAME-$APP_VERSION" \
	--background "$REPO_PATH/BinaryData/Misc/macos-dmg-background.png" \
	--volicon "$REPO_PATH/BinaryData/Misc/macos-dmg-icon.icns" \
	--window-size 600 380 \
	--window-pos 200 120 \
	--icon-size 64 \
	--icon "Partiels.app" 128 90 \
  --app-drop-link 432 90 \
	--add-file "About.txt" 	"$REPO_PATH/BinaryData/Misc/About.txt" 128 200 \
	"$APP_NAME-$APP_VERSION.dmg" \
	"app"

xcrun rez -append "$REPO_PATH/BinaryData/Misc/macos-dmg-icon.rsrc" -o "$APP_NAME-$APP_VERSION.dmg"
xcrun setFile -a C "$APP_NAME-$APP_VERSION.dmg"

echo '\033[0;34m' "Compressing apple image disk..."
echo '\033[0m'
zip -r "$APP_NAME-$APP_VERSION-MacOS-x86_64.zip" "$APP_NAME-$APP_VERSION.dmg"

echo '\033[0;34m' "Installing zip file..."
echo '\033[0m'
cp "$APP_NAME-$APP_VERSION-MacOS-x86_64.zip" /Users/guillot/Nextcloud/Partiels

echo '\033[0;34m' "done"
echo '\033[0m'
