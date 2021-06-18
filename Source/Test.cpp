
#include "Application/AnlApplicationInstance.h"

extern "C" int main(int argc, char* argv[])
{
    juce::ignoreUnused(argc, argv);

    std::unique_ptr<juce::MessageManager> mm(juce::MessageManager::getInstance());
    juce::UnitTestRunner unitTestRunner;
    unitTestRunner.runAllTests();

    int failures = 0;
    for(int i = 0; i < unitTestRunner.getNumResults(); ++i)
    {
        if(auto* result = unitTestRunner.getResult(i))
        {
            failures += result->failures;
        }
    }
    return failures;
}
