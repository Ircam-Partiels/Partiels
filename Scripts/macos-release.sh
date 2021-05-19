#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
APP_NAME="Partiels"
APPLE_ACCOUNT="pierre.guillot@ircam.fr"
APPLE_PASSWORD="Developer-altool"

mkdir -p $REPO_PATH/build
cd $REPO_PATH/build

echo '\033[0;34m' "Prepare project..."
echo '\033[0m'
cmake .. -G Xcode

echo '\033[0;34m' "Creating archive..."
echo '\033[0m'
xcodebuild archive -project "$APP_NAME.xcodeproj" -quiet -configuration "Release" -scheme "ALL_BUILD" -archivePath "$APP_NAME.xcarchive"

echo '\033[0;34m' "Exporting archive..."
echo '\033[0m'
xcodebuild -exportArchive -archivePath "$APP_NAME.xcarchive" -exportPath "." -exportOptionsPlist "$ThisPath/macos-export-archive.plist"

echo '\033[0;34m' "Uploading archive..."
echo '\033[0m'
ditto -c -k --keepParent "$APP_NAME.app" "$APP_NAME.zip"
xc_out=$(xcrun altool --notarize-app --primary-bundle-id "fr.ircam.dev.partiels" --username "$APPLE_ACCOUNT" --password "@keychain:$APPLE_PASSWORD" --file "$APP_NAME.zip" 2>&1)
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

if [[ $request_status != "success" ]]; then
		echo '\033[0;31m' "Error: could not notarize $APP_NAME.zip."
		echo '\033[0m'
    exit 1
fi

APP_VERSION=$(defaults read $REPO_PATH/build/Partiels.app/Contents/Info.plist CFBundleShortVersionString)
$ThisPath/macos-package.sh

echo '\033[0;34m' "Compressing apple image disk..."
echo '\033[0m'
zip -r "$APP_NAME-v$APP_VERSION-MacOS-x86_64.zip" "$APP_NAME-v$APP_VERSION.dmg"

echo '\033[0;34m' "Installing zip file..."
echo '\033[0m'
cp "$APP_NAME-v$APP_VERSION-MacOS-x86_64.zip" /Users/guillot/Nextcloud/Partiels/Temp

echo '\033[0;34m' "done"
echo '\033[0m'
