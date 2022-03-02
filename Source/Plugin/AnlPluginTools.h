#pragma once

#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace Plugin
{
    namespace Tools
    {
        std::unique_ptr<juce::Component> createProperty(Parameter const& parameter, std::function<void(Parameter const& parameter, float value)> applyChange);
    } // namespace Tools
} // namespace Plugin

ANALYSE_FILE_END
