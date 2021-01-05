#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    namespace Renderer
    {
        class Frame
        : public juce::Component
        {
        public:
            Frame(Accessor& accessor, Zoom::Accessor& zoomAccessor);
            ~Frame() override;
            
            // juce::Component
            void resized() override;
            void paint(juce::Graphics& g) override;
            
        private:
            Accessor& mAccessor;
            Zoom::Accessor& mZoomAccessor;
            std::vector<std::reference_wrapper<Zoom::Accessor>> mZoomAccessors;
            
            Accessor::Listener mListener;
            Accessor::Receiver mReceiver;
            
            Zoom::Accessor::Listener mZoomListener;
            juce::Label mInformation;
            juce::Image mImage;
            
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Frame)
        };
    }
}

ANALYSE_FILE_END
