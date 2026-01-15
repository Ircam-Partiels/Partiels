#pragma once

#include "AnlApplicationNeuralyzerModel.h"
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wshadow-field-in-constructor", "-Wimplicit-float-conversion", "-Wunused-function", "-Wzero-as-null-pointer-constant", "-Wfloat-equal", "-Wsign-conversion", "-Wdeprecated-copy-with-dtor", "-Wmissing-noreturn", "-Winconsistent-missing-destructor-override")
#include <chat.h>
#include <common.h>
#include <sampling.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE("-Wunused-parameter", "-Wunused-function", "-Wextra-semi", "-Wshadow", "-Wsign-conversion", "-Wshorten-64-to-32", "-Wimplicit-int-float-conversion", "-Wimplicit-int-conversion", "-Wconditional-uninitialized", "-Wcast-align")
#include <llama-cpp.h>
JUCE_END_IGNORE_WARNINGS_GCC_LIKE

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Neuralyzer
    {
        class Guard
        {
        public:
            Guard() = default;
            ~Guard() = default;

            juce::Result initializeModel(juce::File modelFile);
            void reset();
            bool isReady() const;

            juce::Result checkText(juce::String const& text);

            static juce::File getDefaultModelDirectory();
            static juce::URL getModelUrl();
            static juce::File getModelFile();
            static void downloadModelIfNecessary();

        private:
            mutable std::mutex mMutex;
            llama_model_ptr mModel;
            llama_context_ptr mContext;
            common_chat_templates_ptr mChatTemplates;
        };

    } // namespace Neuralyzer
} // namespace Application

ANALYSE_FILE_END
