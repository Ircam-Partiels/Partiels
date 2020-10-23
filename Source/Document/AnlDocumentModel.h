#pragma once

#include "../Analyzer/AnlAnalyzerModel.h"
#include "../Tools/AnlSignalBroadcaster.h"

ANALYSE_FILE_BEGIN

namespace Document
{
    //! @brief The data model of a document
    //! @brief The document is a container for a set of analysis
    struct Model
    {
        enum class Attribute
        {
            file,
            analyzers,
            loop
        };
        
        enum class Signal
        {
            movePlayhead,
            togglePlayback,
            toggleLooping
        };
        
        juce::File file; //!< The audio file associated with the document
        std::vector<Analyzer::Model> analyzers; //!< The analyzers of the document
        juce::Range<double> loop; //!< The loop range of the dovument
        
        std::unique_ptr<juce::XmlElement> toXml() const;
        static Model fromXml(juce::XmlElement const& xml, Model defaultModel = {});
        static std::set<Attribute> getAttributeTypes();
        
        JUCE_LEAK_DETECTOR(Model)
    };
    
    class Accessor
    : public Tools::ModelAccessor<Accessor, Model, Model::Attribute>
    , public Tools::SignalBroadcaster<Accessor, Model::Signal>
    {
    public:
        using Tools::ModelAccessor<Accessor, Model, Model::Attribute>::ModelAccessor;
        ~Accessor() override = default;
        void fromModel(Model const& model, juce::NotificationType const notification) override;
    };
}

ANALYSE_FILE_END
