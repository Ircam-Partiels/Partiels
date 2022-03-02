#pragma once

#include "AnlTrackDirector.h"
#include "AnlTrackProgressBar.h"
#include "AnlTrackPropertyGraphicalSection.h"
#include "AnlTrackPropertyPluginSection.h"
#include "AnlTrackPropertyProcessorSection.h"

ANALYSE_FILE_BEGIN

namespace Track
{
    class PropertyPanel
    : public FloatingWindowContainer
    , public juce::DragAndDropContainer
    {
    public:
        PropertyPanel(Director& director);
        ~PropertyPanel() override;

        // juce::Component
        void resized() override;

    private:
        Director& mDirector;
        Accessor& mAccessor{mDirector.getAccessor()};
        Accessor::Listener mListener{typeid(*this).name()};
        ComponentListener mComponentListener;

        PropertyText mPropertyName;

        PropertyProcessorSection mPropertyProcessorSection{mDirector};
        ConcertinaTable mProcessorSection{juce::translate("PROCESSOR"), true,
                                          juce::translate("The processor parameters of the track")};

        PropertyGraphicalSection mPropertyGraphicalSection{mDirector};
        ConcertinaTable mGraphicalSection{juce::translate("GRAPHICAL"), true,
                                          juce::translate("The graphical parameters of the track")};

        PropertyPluginSection mPropertyPluginSection{mDirector};
        ConcertinaTable mPluginSection{juce::translate("PLUGIN"), true,
                                       juce::translate("The plugin information")};

        juce::Viewport mViewport;
    };
} // namespace Track

ANALYSE_FILE_END
