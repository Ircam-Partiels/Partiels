#pragma once

#include "AnlBase.h"

ANALYSE_FILE_BEGIN

class ComponentSnapshot
: public juce::Component
, public juce::Timer
, public juce::ApplicationCommandTarget
{
public:
    // clang-format off
    enum CommandIDs : int
    {
          performSnapshot = 0x4001
    };
    // clang-format on

    ComponentSnapshot();
    ~ComponentSnapshot() override = default;

    virtual void takeSnapshot() = 0;

    // juce::Component
    void paintOverChildren(juce::Graphics& g) override;

    // juce::ApplicationCommandTarget
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(juce::ApplicationCommandTarget::InvocationInfo const& info) override;

protected:
    void takeSnapshot(juce::Component& component, juce::String const& name, juce::Colour const& backgroundColour);

private:
    // juce::Timer
    void timerCallback() override;

    float mAlpha = 0.0f;
    juce::Image mImage;
};

ANALYSE_FILE_END
