#pragma once

#include "AnlTrackResultModifier.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    namespace Result
    {
        class CellBase
        : public juce::Component
        {
        public:
            CellBase(Director& director, Zoom::Accessor& timeZoomAccessor, size_t channel, size_t index);
            ~CellBase() override = default;

            bool isValid() const;

        protected:
            void attachToListener();
            void detachFromListener();
            virtual void update() = 0;

            Director& mDirector;
            Accessor& mAccessor{mDirector.getAccessor()};
            Accessor::Listener mListener{typeid(*this).name()};
            Zoom::Accessor& mTimeZoomAccessor;
            Zoom::Accessor::Listener mTimeZoomListener{typeid(*this).name()};
            size_t const mChannel;
            size_t const mIndex;
            JUCE_LEAK_DETECTOR(CellBase)
        };

        class CellTime
        : public CellBase
        {
        public:
            CellTime(Director& director, Zoom::Accessor& timeZoomAccessor, size_t channel, size_t index);
            ~CellTime() override;

            // juce::Component
            void resized() override;
            void lookAndFeelChanged() override;

            // CellBase
            void update() override;

        private:
            HMSmsField mHMSmsField;
            double mCurrentTime;
            JUCE_LEAK_DETECTOR(CellTime)
        };

        class CellDuration
        : public CellBase
        {
        public:
            CellDuration(Director& director, Zoom::Accessor& timeZoomAccessor, size_t channel, size_t index);
            ~CellDuration() override;

            // juce::Component
            void resized() override;
            void lookAndFeelChanged() override;

            // CellBase
            void update() override;

        private:
            HMSmsField mHMSmsField;
            double mCurrentDuration;
            JUCE_LEAK_DETECTOR(CellDuration)
        };

        class CellValue
        : public CellBase
        {
        public:
            CellValue(Director& director, Zoom::Accessor& timeZoomAccessor, size_t channel, size_t index);
            ~CellValue() override;

            // juce::Component
            void resized() override;
            void lookAndFeelChanged() override;

            // CellBase
            void update() override;

        private:
            juce::Label mLabel;
            std::string mCurrentLabel;
            NumberField mNumberField;
            std::optional<float> mCurrentValue;
            JUCE_LEAK_DETECTOR(CellValue)
        };
    } // namespace Result
} // namespace Track

ANALYSE_FILE_END
