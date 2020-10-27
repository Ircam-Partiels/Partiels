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
            isLooping
        };
        
        enum class Signal
        {
            movePlayhead,
            togglePlayback,
            playheadPosition
        };
        
        Model() = default;
        Model(Model const& other);
        Model(Model&& other);
        
        ~Model() = default;
        
        juce::File file; //!< The audio file associated with the document
        std::vector<std::unique_ptr<Analyzer::Model>> analyzers; //!< The analyzers of the document
        bool isLooping; //!< If the loop is active
        
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
        
        JUCE_COMPILER_WARNING("aki?")
        // std::vector<std::reference_wrapper<Analyzer::Accessor>> getAnalyzerAccessors();
        Analyzer::Accessor& getAnalyzerAccessor(size_t index);
    private:
        std::vector<std::unique_ptr<Analyzer::Accessor>> mAnalyzerAccessors;
    };
}

ANALYSE_FILE_END
