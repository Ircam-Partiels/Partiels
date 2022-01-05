#!/bin/sh

APP_NAME="Partiels"

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
BUILD_PATH="$REPO_PATH/release"

APPLE_ACCOUNT="pierre.guillot@ircam.fr"
APPLE_PASSWORD="Developer-altool"

mkdir -p $BUILD_PATH
cd $BUILD_PATH
rm -rf *

echo '\033[0;34m' "Prepare project..."
echo '\033[0m'
cmake .. -G Xcode

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

xcrun stapler staple $APP_NAME.app
xcrun stapler validate $APP_NAME.app

$ThisPath/macos-package.sh

echo '\033[0;34m' "done"
echo '\033[0m'
