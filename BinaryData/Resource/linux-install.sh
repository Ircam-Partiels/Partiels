#!/bin/sh

ThisPath="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
mkdir -p /opt/Partiels
mkdir -p /opt/Partiels/PlugIns
cp -r $ThisPath/Partiels /opt/Partiels/
cp -rf $ThisPath/PlugIns /opt/Partiels/
cp -f $ThisPath/icon.png /opt/Partiels/
cp -f $ThisPath/uninstall.sh /opt/Partiels/
ln -sf /opt/Partiels/Partiels /usr/bin/Partiels
cp -f $ThisPath/Partiels.desktop /usr/share/applications
