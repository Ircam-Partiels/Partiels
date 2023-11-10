#!/bin/sh

APP_NAME="Partiels"

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
BUILD_PATH="$REPO_PATH/release"
APP_VERSION=$(defaults read $BUILD_PATH/Partiels.app/Contents/Info.plist CFBundleShortVersionString)

cd $BUILD_PATH

PROJECT_URL="https://forge-2.ircam.fr/api/v4/projects/567/jobs/artifacts"
PRIVATE_TOKEN=$(security find-generic-password -s 'Forge2 Ircam Token' -w)

if [ -z $PRIVATE_TOKEN == ""]; then
  echo '\033[0;31m' "Error: no private token defined"
  exit -1
fi
if [ -z $1 ]; then
  APP_FULL_VERSION=$APP_VERSION
else
  APP_FULL_VERSION=$APP_VERSION-$1
fi

echo '\033[0;34m' "Preparing " $APP_NAME-v$APP_FULL_VERSION "..."
echo '\033[0m'

echo '\033[0;34m' "Compressing apple image disk..."
echo '\033[0m'
test -f "$APP_NAME-v$APP_FULL_VERSION-MacOS-64bit-Universal.zip" && rm "$APP_NAME-v$APP_FULL_VERSION-MacOS-64bit-Universal.zip"
zip -r "$APP_NAME-v$APP_FULL_VERSION-MacOS-64bit-Universal.zip" "$APP_NAME-v$APP_VERSION.dmg"

echo '\033[0;34m' "Downloading Linux artifact..."
echo '\033[0m'
test -f "$APP_NAME-v$APP_FULL_VERSION-Linux-64bit.zip" && rm "$APP_NAME-v$APP_FULL_VERSION-Linux-64bit.zip"
curl --output $APP_NAME-v$APP_FULL_VERSION-Linux-64bit.zip --header "PRIVATE-TOKEN: $PRIVATE_TOKEN" "$PROJECT_URL/$APP_FULL_VERSION/download?job=Build::Linux"

echo '\033[0;34m' "Downloading windows artifact..."
echo '\033[0m'
test -f "$APP_NAME-v$APP_FULL_VERSION-Windows-64bit.zip" && rm "$APP_NAME-v$APP_FULL_VERSION-Windows-64bit.zip"
curl --output $APP_NAME-v$APP_FULL_VERSION-Windows-64bit.zip --header "PRIVATE-TOKEN: $PRIVATE_TOKEN" "$PROJECT_URL/$APP_FULL_VERSION/download?job=Build::Windows"

echo '\033[0;34m' "Downloading documentation artifact..."
echo '\033[0m'
test -f "$APP_NAME-v$APP_FULL_VERSION-Manual.zip" && rm "$APP_NAME-v$APP_FULL_VERSION-Manual.zip"
test -f "$APP_NAME-v$APP_FULL_VERSION-Manual.pdf" && rm "$APP_NAME-v$APP_FULL_VERSION-Manual.pdf"
curl --output $APP_NAME-v$APP_FULL_VERSION-Manual.zip --header "PRIVATE-TOKEN: $PRIVATE_TOKEN" "$PROJECT_URL/$APP_FULL_VERSION/download?job=Doc"
unzip  $APP_NAME-v$APP_FULL_VERSION-Manual.zip
mv $APP_NAME-Manual.pdf "$APP_NAME-v$APP_FULL_VERSION-Manual.pdf"

echo '\033[0;34m' "Installing zip files..."
echo '\033[0m'
cp "$APP_NAME-v$APP_FULL_VERSION-Linux-64bit.zip" $HOME/Nextcloud/Partiels/Temp
cp "$APP_NAME-v$APP_FULL_VERSION-Windows-64bit.zip" $HOME/Nextcloud/Partiels/Temp
cp "$APP_NAME-v$APP_FULL_VERSION-MacOS-64bit-Universal.zip" $HOME/Nextcloud/Partiels/Temp
cp "$APP_NAME-v$APP_FULL_VERSION-Manual.pdf" $HOME/Nextcloud/Partiels/Temp
cp "$BUILD_PATH/ChangeLog.txt" $HOME/Nextcloud/Partiels/Temp

echo '\033[0;34m' "done"
echo '\033[0m'
