#!/bin/sh

ThisPath="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
Uninstall=0
Help=0
UseHome=0
HasRootAccess=0 # 0 for no, 1 for root, 2 for sudo-available

while [ $# -gt 0 ]; do
    case "$1" in
        --home)
            UseHome=1
        ;;
        --uninstall)
            Uninstall=1
        ;;
         --help)
            echo "Usage: $0 with optional arguments"
            echo "  --uninstall (uninstall the Partiels application, its desktop integration and the usr local bin symlink)"
            echo "  --home (use "$HOME/opt/Partiels" instead of "opt/Partiels")"
            exit 0
        ;;
        *)
        break
        ;;
    esac
    shift
done

if [ "$UID" = "0" ] || [ "$(id -ur)" = "0" ]; then
    HasRootAccess=1
else
    which sudo 2>&1 1> /dev/null
    if [ "$?" = "0" ]; then 
        HasRootAccess=2; 
    fi
fi

if [ "$UseHome" = "1" ]; then
    if [ "$HasRootAccess" = "1" ]; then
        echo '\033[0;31m' "Running as root, if you wish to do a user-local install, please run this installer as a regular user."
        echo '\033[0m'
        exit -1
    fi
    InstallPath="$HOME/opt/Partiels"
else
    if [ "$HasRootAccess" = "0" ]; then
        echo '\033[0;31m' "Running as a regular user, if you wish to do a global install, please run this installer as a root (or install sudo)."
        echo '\033[0m'
        exit -1
    fi
    InstallPath="/opt/Partiels"
fi

if [ "$Uninstall" = "1" ]; then
    if [ "$HasRootAccess" = "0" ]; then
        echo '\033[0;34m'"Uninstalling Partiels from $InstallPath without desktop integration nor the usr local bin symlink"
        echo '\033[0m'
    else
        echo '\033[0;34m'"Uninstalling Partiels from $InstallPath, its desktop integration and the usr local bin symlink"
        echo '\033[0m'
    fi
    if [ "$HasRootAccess" = "1" ]; then
    	rm -rf $InstallPath
        rm -f /usr/bin/Partiels
        rm -f /usr/share/applications/Partiels.desktop
    elif [ "$HasRootAccess" = "2" ]; then
    	sudo rm -rf $InstallPath
        sudo rm -f /usr/bin/Partiels
        sudo rm -f /usr/share/applications/Partiels.desktop
    fi
else
    if [ "$HasRootAccess" = "0" ]; then
        echo '\033[0;34m'"Installing Partiels to $InstallPath without desktop integration nor usr local bin symlink"
        echo '\033[0m'
    else
        echo '\033[0;34m'"Installing Partiels to $InstallPath with desktop integration and usr local bin symlink"
        echo '\033[0m'
    fi
    if [ "$HasRootAccess" = "1" ]; then
    	mkdir -p $InstallPath
    	cp -r $ThisPath/Partiels $InstallPath/
    	cp -rf $ThisPath/PlugIns $InstallPath/
    	cp -rf $ThisPath/Templates $InstallPath/
    	cp -rf $ThisPath/Translations $InstallPath/
    	cp -rf $ThisPath/Scripts $InstallPath/
    	cp -f $ThisPath/icon.png $InstallPath/
        ln -sf $InstallPath/Partiels /usr/bin/Partiels
        cp -f $ThisPath/Partiels.desktop /usr/share/applications
    elif [ "$HasRootAccess" = "2" ]; then
    	sudo mkdir -p $InstallPath
    	sudo cp -r $ThisPath/Partiels $InstallPath/
    	sudo cp -rf $ThisPath/PlugIns $InstallPath/
    	sudo cp -rf $ThisPath/Templates $InstallPath/
    	sudo cp -rf $ThisPath/Translations $InstallPath/
    	sudo cp -rf $ThisPath/Scripts $InstallPath/
    	sudo cp -f $ThisPath/icon.png $InstallPath/
        sudo ln -sf $InstallPath/Partiels /usr/bin/Partiels
        sudo cp -f $ThisPath/Partiels.desktop /usr/share/applications
    fi
fi
