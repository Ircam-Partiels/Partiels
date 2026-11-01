#include "MiscBasics.h"

MISC_FILE_BEGIN

class FormatUnitTest
: public juce::UnitTest
{
public:
    FormatUnitTest()
    : juce::UnitTest("Format", "Format")
    {
    }

    ~FormatUnitTest() override = default;

    void runTest() override
    {
        beginTest("secondsToClockString");
        {
            expectEquals(Format::secondsToClockString(0.0, false), juce::String("0:00:000"));
            expectEquals(Format::secondsToClockString(1.0, false), juce::String("0:01:000"));
            expectEquals(Format::secondsToClockString(72.25, false), juce::String("1:12:250"));
            expectEquals(Format::secondsToClockString(725.0, false), juce::String("12:05:000"));
            expectEquals(Format::secondsToClockString(3728.5, true), juce::String("1:02:08:500"));
            expectEquals(Format::secondsToClockString(128.0, true), juce::String("0:02:08:000"));
            expectEquals(Format::secondsToClockString(59.9996, false), juce::String("1:00:000"));
            expectEquals(Format::secondsToClockString(2.0000000001, false), juce::String("0:02:000"));
            expectEquals(Format::secondsToClockString(-1.5, false), juce::String("-0:01:500"));
            expectEquals(Format::secondsToClockString(-0.0001, false), juce::String("0:00:000"));
        }
    }
};

static FormatUnitTest formatUnitTest;

MISC_FILE_END
