#!/bin/sh

ThisPath="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

if [ -z $1 ];  then
	PACKAGE_PATH=$ThisPath/../build/Partiels
else
	PACKAGE_PATH=$1
fi

if [ -z $2 ];  then
  APP_PATH=$ThisPath/../build/Partiels_artefacts/Release
else
  APP_PATH=$2
fi

echo '\033[0;34m' "Prepare package in $PACKAGE_PATH..."
echo '\033[0m'

cd $APP_PATH
mkdir $PACKAGE_PATH
cp Partiels $PACKAGE_PATH
cp ../../../BinaryData/Icons/icon.png $PACKAGE_PATH
cp ../../../BinaryData/Resource/Partiels.desktop $PACKAGE_PATH
cp ../../../BinaryData/Resource/About.txt $PACKAGE_PATH
cp ../../../BinaryData/Resource/ChangeLog.txt $PACKAGE_PATH
cp ../../../BinaryData/Resource/linux-install.sh $PACKAGE_PATH/install.sh
cp ../../../BinaryData/Resource/linux-uninstall.sh $PACKAGE_PATH/uninstall.sh

echo '\033[0;34m' "Done"
echo '\033[0m'
