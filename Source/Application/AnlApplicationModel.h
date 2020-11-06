#pragma once

#include "../Tools/AnlModelAccessor.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    struct Model
    {
        enum class Attribute
        {
            windowState,
            recentlyOpenedFilesList,
            currentDocumentFile
        };
        
        juce::String windowState;
        std::vector<juce::File> recentlyOpenedFilesList;
        juce::File currentDocumentFile;
        
        std::unique_ptr<juce::XmlElement> toXml() const;
        static Model fromXml(juce::XmlElement const& xml, Model defaultModel = {});
        
        JUCE_LEAK_DETECTOR(Model)
    };
    
    class Accessor
    : public Tools::ModelAccessor<Accessor, Model, Model::Attribute>
    {
    public:
        using Tools::ModelAccessor<Accessor, Model, Model::Attribute>::ModelAccessor;
        ~Accessor() override = default;
        void fromModel(Model const& model, NotificationType const notification) override;
    };
}

ANALYSE_FILE_END
