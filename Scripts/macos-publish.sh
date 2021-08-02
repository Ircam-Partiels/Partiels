#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
APP_NAME="Partiels"

mkdir -p $REPO_PATH/build
cd $REPO_PATH/build

APP_VERSION=$(defaults read $REPO_PATH/build/Partiels.app/Contents/Info.plist CFBundleShortVersionString)
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
zip -r "$APP_NAME-v$APP_FULL_VERSION-MacOS.zip" "$APP_NAME-v$APP_VERSION.dmg"

echo '\033[0;34m' "Downloading debian artifact..."
echo '\033[0m'
curl --output $APP_NAME-v$APP_FULL_VERSION-Debian-10.9.zip --header "PRIVATE-TOKEN: $PRIVATE_TOKEN" "https://forge-2.ircam.fr/api/v4/projects/567/jobs/artifacts/$APP_FULL_VERSION/download?job=Build::Debian"

echo '\033[0;34m' "Downloading ubuntu artifact..."
echo '\033[0m'
curl --output $APP_NAME-v$APP_FULL_VERSION-Ubuntu-20.04.zip --header "PRIVATE-TOKEN: $PRIVATE_TOKEN" "https://forge-2.ircam.fr/api/v4/projects/567/jobs/artifacts/$APP_FULL_VERSION/download?job=Build::Ubuntu"

echo '\033[0;34m' "Downloading windows artifact..."
echo '\033[0m'
curl --output $APP_NAME-v$APP_FULL_VERSION-Windows-10.zip --header "PRIVATE-TOKEN: $PRIVATE_TOKEN" "https://forge-2.ircam.fr/api/v4/projects/567/jobs/artifacts/$APP_FULL_VERSION/download?job=Build::Windows"

echo '\033[0;34m' "Installing zip files..."
echo '\033[0m'
cp "$APP_NAME-v$APP_FULL_VERSION-Debian-10.9.zip" /Users/guillot/Nextcloud/Partiels/Temp
cp "$APP_NAME-v$APP_FULL_VERSION-Ubuntu-20.04.zip" /Users/guillot/Nextcloud/Partiels/Temp
cp "$APP_NAME-v$APP_FULL_VERSION-Windows-10.zip" /Users/guillot/Nextcloud/Partiels/Temp
cp "$APP_NAME-v$APP_FULL_VERSION-MacOS.zip" /Users/guillot/Nextcloud/Partiels/Temp

echo '\033[0;34m' "done"
echo '\033[0m'
