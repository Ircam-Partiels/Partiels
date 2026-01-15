#pragma once

#include "../Misc/AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        //! @brief Displays the answers suggested by the model to its question (see the MCP tool elicit)
        //! @details The answers are displayed as buttons whose height depends on the length of the text, and the
        //! buttons can be scrolled if they cannot all be displayed.
        class ChatElicitation
        : public juce::Component
        {
        public:
            ChatElicitation();
            ~ChatElicitation() override;

            //! @brief Sets the suggested answers (it replaces the buttons)
            //! @param options The suggested answers
            void setOptions(juce::StringArray const& options);

            //! @brief Gets the height required to display all the buttons without scrolling
            //! @param width The width of the component
            int getOptimalHeight(int width) const;

            //! @brief The callback called when an answer is clicked
            //! @details The buttons must not be replaced in the callback.
            std::function<void(juce::String const& answer)> onOptionClicked = nullptr;

            // juce::Component
            void resized() override;

        private:
            class OptionButton;

            //! @brief Lays out the buttons in the content for a width and returns the height of the content
            int layoutButtons(int width, bool apply) const;

            juce::Component mContent;
            juce::Viewport mViewport;
            std::vector<std::unique_ptr<OptionButton>> mButtons;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChatElicitation)
        };
    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
