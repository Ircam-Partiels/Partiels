#!/bin/sh

APP_NAME="Partiels"
THIS_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$THIS_PATH/.."

POSITIONAL_ARGS=()

while [[ $# -gt 0 ]]; do
  case $1 in
    -c|--config)
      PARTIELS_BUILD_CONFIG="$2"
      shift
      shift
      ;;
    -l|--location)
      PARTIELS_BUILD_PATH="$2"
      shift
      shift
      ;;
    -p|--profile)
      PARTIELS_KEYCHAIN_PROFILE="$2"
      shift
      shift
      ;;
    --ci)
      PARTIELS_BUILD_CI=true
      shift # past argument
      ;;
    -*|--*)
      echo "Unknown option $1"
      exit 1
      ;;
    *)
      POSITIONAL_ARGS+=("$1") # save positional arg
      shift # past argument
      ;;
  esac
done

if [ -z $PARTIELS_BUILD_CONFIG ]; then
  PARTIELS_BUILD_CONFIG="Release"
fi

if [ -z $PARTIELS_BUILD_CI ]; then
  PARTIELS_BUILD_CI=false
fi

if [ -z $PARTIELS_BUILD_PATH ]; then
  PARTIELS_BUILD_PATH="$REPO_PATH/build"
fi

if [ -z $PARTIELS_KEYCHAIN_PROFILE ]; then
  PARTIELS_KEYCHAIN_PROFILE="Application Partiels"
fi

echo '\033[0;34m' "Release Partiels with config ${PARTIELS_BUILD_CONFIG} and profile ${PARTIELS_KEYCHAIN_PROFILE} in ${PARTIELS_BUILD_PATH} with CI ${PARTIELS_BUILD_CI}"
echo '\033[0m'

echo '\033[0;34m' "Creating archive..."
echo '\033[0m'
if [[ $PARTIELS_BUILD_CI ]]; then
  if ! command -v xcbeautify &> /dev/null; then
     set -o pipefail && xcodebuild archive -project "$PARTIELS_BUILD_PATH/$APP_NAME.xcodeproj" -configuration $PARTIELS_BUILD_CONFIG -scheme "Partiels" -archivePath "$PARTIELS_BUILD_PATH/$APP_NAME.xcarchive" -destination platform=macOS
  else
     set -o pipefail && NSUnbufferedIO=YES xcodebuild archive -project "$PARTIELS_BUILD_PATH/$APP_NAME.xcodeproj" -configuration $PARTIELS_BUILD_CONFIG -scheme "Partiels" -archivePath "$PARTIELS_BUILD_PATH/$APP_NAME.xcarchive" -destination platform=macOS | xcbeautify
  fi
else
  if ! command -v xcbeautify &> /dev/null; then
    echo "Install xcbeautify \`brew install xcbeautify\` to generates clean xcodebuild output"
    xcodebuild archive -project "$PARTIELS_BUILD_PATH/$APP_NAME.xcodeproj" -configuration $PARTIELS_BUILD_CONFIG -scheme "Partiels" -archivePath "$PARTIELS_BUILD_PATH/$APP_NAME.xcarchive" -destination platform=macOS
  else
    xcodebuild archive -project "$PARTIELS_BUILD_PATH/$APP_NAME.xcodeproj" -configuration $PARTIELS_BUILD_CONFIG -scheme "Partiels" -archivePath "$PARTIELS_BUILD_PATH/$APP_NAME.xcarchive" -destination platform=macOS | xcbeautify
  fi
fi

echo '\033[0;34m' "Exporting archive..."
echo '\033[0m'
xcodebuild -exportArchive -archivePath "$PARTIELS_BUILD_PATH/$APP_NAME.xcarchive" -exportPath "$PARTIELS_BUILD_PATH" -exportOptionsPlist "$THIS_PATH/macos-export-archive.plist" || { exit 1; }

echo '\033[0;34m' "Uploading archive..."
echo '\033[0m'
ditto -c -k --keepParent "$PARTIELS_BUILD_PATH/$APP_NAME.app" "$PARTIELS_BUILD_PATH/$APP_NAME.zip"
xcrun notarytool submit "$PARTIELS_BUILD_PATH/$APP_NAME.zip" --keychain-profile "$PARTIELS_KEYCHAIN_PROFILE" --wait > "$PARTIELS_BUILD_PATH/notarize.log" 2>&1
cat "$PARTIELS_BUILD_PATH/notarize.log"
notaryid=$(awk '/^  id:/{sub(/^  id:/, ""); print; exit}' "$PARTIELS_BUILD_PATH/notarize.log")
xcrun notarytool log $notaryid --keychain-profile "$PARTIELS_KEYCHAIN_PROFILE"

xcrun stapler staple "$PARTIELS_BUILD_PATH/$APP_NAME.app" || { exit 1; }
xcrun stapler validate "$PARTIELS_BUILD_PATH/$APP_NAME.app" || { exit 1; }

echo '\033[0;34m' "Creating apple disk image..."
echo '\033[0m'
PARTIELS_DMG_PATH="$PARTIELS_BUILD_PATH/$APP_NAME.dmg"

test -f "$PARTIELS_DMG_PATH" && rm "$PARTIELS_DMG_PATH"
cd $REPO_PATH/BinaryData/Resource
appdmg macos-dmg-config.json "$PARTIELS_DMG_PATH" || { exit 1; }

xcrun rez -append "$REPO_PATH/BinaryData/Resource/macos-dmg-icon.rsrc" -o "$PARTIELS_DMG_PATH"
xcrun setFile -a C "$PARTIELS_DMG_PATH"

echo '\033[0;34m' "Uploading apple disk image..."
echo '\033[0m'
xcrun notarytool submit "$PARTIELS_DMG_PATH" --keychain-profile "$PARTIELS_KEYCHAIN_PROFILE" --wait > "$PARTIELS_BUILD_PATH/notarize.log" 2>&1
cat "$PARTIELS_BUILD_PATH/notarize.log"
notaryid=$(awk '/^  id:/{sub(/^  id:/, ""); print; exit}' "$PARTIELS_BUILD_PATH/notarize.log")
xcrun notarytool log $notaryid --keychain-profile "$PARTIELS_KEYCHAIN_PROFILE"

xcrun stapler staple $PARTIELS_DMG_PATH  || { exit 1; }
xcrun stapler validate $PARTIELS_DMG_PATH  || { exit 1; }

echo '\033[0;34m' "done"
echo '\033[0m'
