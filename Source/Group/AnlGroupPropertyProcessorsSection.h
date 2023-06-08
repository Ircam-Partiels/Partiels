#pragma once

#include "AnlGroupDirector.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

namespace Group
{
    class PropertyProcessorsSection
    : public juce::Component
    {
    public:
        PropertyProcessorsSection(Director& director);
        ~PropertyProcessorsSection() override = default;

        // juce::Component
        void resized() override;

    private:
        void askToModifyProcessors(std::function<bool(bool)> prepare, std::function<void(void)> perform, std::function<bool(Track::Accessor const&)> trackFilter);
        void applyParameterValue(Plugin::Parameter const& parameter, float value);
        void setWindowType(Plugin::WindowType const& windowType);
        void setBlockSize(size_t const blockSize);
        void setStepSize(size_t const stepSize);
        void setInputTrack(juce::String const& identifier);

        void updateContent();
        void updateWindowType();
        void updateBlockSize();
        void updateStepSize();
        void updateInputTrack();
        void updateParameters();
        void updateState();

        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};

        PropertyList mPropertyWindowType;
        PropertyList mPropertyBlockSize;
        PropertyList mPropertyStepSize;
        PropertyList mPropertyInputTrack;
        std::map<int, juce::String> mPropertyInputTrackList;
        std::map<std::string, std::unique_ptr<juce::Component>> mParameterProperties;

        LayoutNotifier mLayoutNotifier;
        LayoutNotifier mStateNotifier;
    };
} // namespace Group

ANALYSE_FILE_END
