#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        namespace Ast
        {
            struct Section
            {
                juce::String content;
                std::vector<juce::String> hierarchy;
            };

            struct Document
            {
                juce::String document;
                std::vector<Section> sections;
            };

            Document parseMarkdown(juce::String const& content, juce::String const& name, bool includeImages = false);
        } // namespace Ast
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
