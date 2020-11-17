#pragma once

#include "AnlAnalyzerModel.h"
#include "AnlAnalyzerPluginManager.h"
#include "../Tools/AnlPropertyLayout.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class PropertyPanel
    : public juce::Component
    , public Tools::AtomicManager<Vamp::Plugin>
    {
    public:
        
        PropertyPanel(Accessor& accessor);
        ~PropertyPanel() override;
        
        // juce::Component
        void resized() override;
        
    private:
        
        class Property
        : public Tools::PropertyPanel<juce::Label>
        {
        public:
            Property(juce::String const& text, juce::String const& tooltip);
            ~Property() override = default;
        };
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        
        std::vector<std::unique_ptr<Property>> mProperties;
        Tools::PropertyLayout mPropertyLayout;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PropertyPanel)
    };
}

ANALYSE_FILE_END
