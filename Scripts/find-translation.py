import os
import sys
import glob
import re

def add_translations_to_file(targetFile, entry):
    if(entry.find("\"") != 0):
        print("error: " + entry)
        return

    if(os.path.isfile(targetFile)):
        with open(targetFile) as file:
            if entry in file.read():
                return

    transFile = open(targetFile, "a")
    transFile.write(entry + " = " + '\n')
    transFile.close()

def find_translations_in_file(targetFile, file):
    text = open(file).read()

    endPattern = 0
    beginPattern = 0
    found = False
    while(beginPattern != -1):
        if(found == False):
            beginPattern = text.find("juce::translate(", endPattern)
            if(beginPattern != -1):
                beginPattern += len("juce::translate(")
                found = True
            else:
                found = False
        else:
            endPattern = text.find(")", beginPattern+1)
            if(endPattern != -1):
                add_translations_to_file(targetFile, text[beginPattern:endPattern])
                beginPattern = endPattern+1
                found = False
            else:
                add_translations_to_file(targetFile, text[beginPattern])
                beginPattern = -1
                found = False

def main(argv):
    partielsDir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
    sourcesDir = os.path.join(partielsDir, "Source")
    targetFile = os.path.join(partielsDir, "BinaryData/Translations/Fr.txt")

    for file in glob.glob(os.path.join(sourcesDir, "**/*.cpp"), recursive=True):
        find_translations_in_file(targetFile, file)

if __name__=="__main__":
    main(sys.argv[1:])
