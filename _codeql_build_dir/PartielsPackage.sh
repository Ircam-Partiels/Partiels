#!/bin/sh

PARTIELS_BINARY_PATH=$1
rm -rf /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels
mkdir -p /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels
cp $PARTIELS_BINARY_PATH/Partiels /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels
cp -r $PARTIELS_BINARY_PATH/PlugIns /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels/PlugIns
cp -r $PARTIELS_BINARY_PATH/Templates /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels/Templates
cp -r $PARTIELS_BINARY_PATH/Translations /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels/Translations
cp -r $PARTIELS_BINARY_PATH/Scripts /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels/Scripts
cp /home/runner/work/Partiels/Partiels/BinaryData/Resource/icon.png /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels
cp /home/runner/work/Partiels/Partiels/BinaryData/Resource/Partiels.desktop /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels
cp /home/runner/work/Partiels/Partiels/BinaryData/Resource/About.txt /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels
cp /home/runner/work/Partiels/Partiels/BinaryData/Resource/ChangeLog.txt /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels
cp /home/runner/work/Partiels/Partiels/BinaryData/Resource/linux-install.sh /home/runner/work/Partiels/Partiels/_codeql_build_dir/Partiels/Partiels-install.sh
cd /home/runner/work/Partiels/Partiels/_codeql_build_dir
tar zcvf Partiels-Linux.tar.gz Partiels
