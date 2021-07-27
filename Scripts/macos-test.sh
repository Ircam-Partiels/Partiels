#!/bin/sh

ThisPath="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_PATH="$ThisPath/.."
BUILD_PATH="$REPO_PATH/build"
CI_PATH="$BUILD_PATH/PartielsCLI_artefacts/Debug/PartielsCLI"
TESTS_PATH="$REPO_PATH/BinaryData/Tests"

rm -rfv $BUILD_PATH/TestsOutput/*
$CI_PATH --input=$TESTS_PATH/Sound.wav --template=$TESTS_PATH/Template.ptldoc --output=$BUILD_PATH/TestsOutput/None/
$CI_PATH --input=$TESTS_PATH/Sound.wav --template=$TESTS_PATH/Template.ptldoc --output=$BUILD_PATH/TestsOutput/Xml/ --options=$TESTS_PATH/exportOptions.xml
$CI_PATH --input=$TESTS_PATH/Sound.wav --template=$TESTS_PATH/Template.ptldoc --output=$BUILD_PATH/TestsOutput/JPEG/ --format=jpeg --width=800 --height=600
$CI_PATH --input=$TESTS_PATH/Sound.wav --template=$TESTS_PATH/Template.ptldoc --output=$BUILD_PATH/TestsOutput/PNG/ --format=png --width=1200 --height=900 --group
$CI_PATH --input=$TESTS_PATH/Sound.wav --template=$TESTS_PATH/Template.ptldoc --output=$BUILD_PATH/TestsOutput/CSV/ --format=csv --nogrids --header --separator=,
$CI_PATH --input=$TESTS_PATH/Sound.wav --template=$TESTS_PATH/Template.ptldoc --output=$BUILD_PATH/TestsOutput/JSON/ --format=json
