#!/bin/sh

APP_NAME="Partiels"
PROJECT_URL="https://forge-2.ircam.fr/api/v4/projects/567/jobs/artifacts"

THIS_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$THIS_PATH/.."
PUBLISH_PATH="$REPO_PATH/package"

while [[ $# -gt 0 ]]; do
  case $1 in
    --tag)
      APP_VERSION="$2"
      shift
      shift
      ;;
    --token)
      GIT_TOKEN="$2"
      shift
      shift
      ;;
    -u|--user)
      NC_USER="$2"
      shift
      shift
      ;;
    -p|--password)
      NC_PASSWORD="$2"
      shift
      shift
      ;;
    --ci)
      BUILD_CI=true
      shift # past argument
      ;;
    -*|--*)
      echo "Unknown option $1"
      exit 1
      ;;
    *)
      shift # past argument
      ;;
  esac
done

if [ -z $BUILD_CI ]; then
  if [ -z $GIT_TOKEN ]; then
    echo '\033[0;34m' "Search for a local token..."
    echo '\033[0m'
    GIT_TOKEN=$(security find-generic-password -s 'Forge2 Ircam Token' -w)
  fi

  if [ -z $GIT_TOKEN ]; then
    echo '\033[0;31m' "Error: no private token defined"
    exit -1
  fi

  mkdir -p $PUBLISH_PATH
  cd $PUBLISH_PATH
  rm -rf *

  echo '\033[0;34m' "Generating ChangeLog.txt..."
  echo '\033[0m'
  echo "$APP_VERSION\n" > ./ChangeLog.txt
  cat "$REPO_PATH/BinaryData/Resource/ChangeLog.txt" >> ./ChangeLog.txt

  echo '\033[0;34m' "Preparing " $APP_NAME-v$APP_VERSION "..."
  echo '\033[0m'

  echo '\033[0;34m' "Downloading MacOS artifact..."
  echo '\033[0m'
  curl --output "$APP_NAME-v$APP_VERSION-MacOS.zip" --header "PRIVATE-TOKEN: $GIT_TOKEN" "$PROJECT_URL/$APP_VERSION/download?job=Build::MacOS"

  echo '\033[0;34m' "Downloading Linux artifact..."
  echo '\033[0m'
  curl --output "$APP_NAME-v$APP_VERSION-Linux.zip" --header "PRIVATE-TOKEN: $GIT_TOKEN" "$PROJECT_URL/$APP_VERSION/download?job=Build::Linux"

  echo '\033[0;34m' "Downloading Windows artifact..."
  echo '\033[0m'
  curl --output "$APP_NAME-v$APP_VERSION-Windows.zip" --header "PRIVATE-TOKEN: $GIT_TOKEN" "$PROJECT_URL/$APP_VERSION/download?job=Build::Windows"

  echo '\033[0;34m' "Downloading documentation artifact..."
  echo '\033[0m'
  curl --output "$APP_NAME-v$APP_VERSION-Manual.zip" --header "PRIVATE-TOKEN: $GIT_TOKEN" "$PROJECT_URL/$APP_VERSION/download?job=Doc"
  unzip  $APP_NAME-v$APP_VERSION-Manual.zip
  mv $APP_NAME-Manual.pdf "$APP_NAME-v$APP_VERSION-Manual.pdf"
  rm $APP_NAME-v$APP_VERSION-Manual.zip

  echo '\033[0;34m' "Publishing pakages..."
  echo '\033[0m'
  cp -r $PUBLISH_PATH/ $HOME/Nextcloud/Partiels/Temp/$APP_NAME-v$APP_VERSION/
else
  if [ -z $NC_USER ]; then
    echo '\033[0;31m' "Error: no private token defined"
    exit -1
  fi

  if [ -z $NC_PASSWORD ]; then
    echo '\033[0;31m' "Error: no private token defined"
    exit -1
  fi

  echo '\033[0;34m' "Generating ChangeLog.txt..."
  echo '\033[0m'
  echo "$APP_VERSION\n" > ./ChangeLog.txt
  cat "$REPO_PATH/BinaryData/Resource/ChangeLog.txt" >> ./ChangeLog.txt

  echo '\033[0;34m' "Generating pakages..."
  echo '\033[0m'
  zip -r "$APP_NAME-v$APP_VERSION-MacOS.zip" "Partiels.dmg"
  zip -r "$APP_NAME-v$APP_VERSION-Windows.zip" "Partiels-install.exe"
  zip -r "$APP_NAME-v$APP_VERSION-Linux.zip" "Partiels"``

  echo '\033[0;34m' "Publishing pakages..."
  echo '\033[0m'
  NC_PATH=https://nubo.ircam.fr/remote.php/dav/files/b936dfb8-439c-1038-9400-49253cd64ad0/Partiels/Temp/$APP_NAME-v$APP_VERSION
  curl -u $NC_USER:$NC_PASSWORD -X MKCOL $NC_PATH
  curl -u $NC_USER:$NC_PASSWORD -T "$APP_NAME-v$APP_VERSION-MacOS.zip" $NC_PATH/$APP_NAME-v$APP_VERSION-MacOS.zip
  curl -u $NC_USER:$NC_PASSWORD -T "$APP_NAME-v$APP_VERSION-Windows.zip" $NC_PATH/$APP_NAME-v$APP_VERSION-Windows.zip
  curl -u $NC_USER:$NC_PASSWORD -T "$APP_NAME-v$APP_VERSION-Linux.zip" $NC_PATH/$APP_NAME-v$APP_VERSION-Linux.zip
  curl -u $NC_USER:$NC_PASSWORD -T "Partiels-Manual.pdf" $NC_PATH/$APP_NAME-v$APP_VERSION-Manual.pdf
  curl -u $NC_USER:$NC_PASSWORD -T "ChangeLog.txt" $NC_PATH/ChangeLog.txt
fi
