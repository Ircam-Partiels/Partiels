#pragma once

#include "MiscTransportModel.h"

MISC_FILE_BEGIN

namespace Transport
{
    namespace Action
    {
        class Restorer
        : public juce::UndoableAction
        {
        public:
            Restorer(std::function<Accessor&(void)> accessorGetter, double newPlayhead, juce::Range<double> const& newSelection);
            ~Restorer() override = default;

            // juce::UndoableAction
            bool perform() override;
            bool undo() override;

        protected:
            std::function<Accessor&(void)> const mAccessorGetter;
            double const mSavedPlayhead;
            juce::Range<double> const mSavedSelection;
            double const mNewPlayhead;
            juce::Range<double> const mNewSelection;
        };
    } // namespace Action
} // namespace Transport

MISC_FILE_END
