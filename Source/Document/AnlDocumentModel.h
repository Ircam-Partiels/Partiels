#pragma once

#include "../Analyzer/AnlAnalyzerModel.h"
#include "../Zoom/AnlZoomStateModel.h"
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
            isLooping,
            gain
        };
        
        enum class Signal
        {
            movePlayhead,
            togglePlayback,
            playheadPosition
        };
        
        Model() {}
        Model(Model const& other);
        Model(Model&& other);
        
        ~Model() = default;
        bool operator==(Model const& other) const;
        bool operator!=(Model const& other) const;
        
        juce::File file; //!< The audio file associated with the document
        std::vector<std::unique_ptr<Analyzer::Model>> analyzers; //!< The analyzers of the document
        bool isLooping; //!< If the loop is active
        double gain = 1.0; //! The gain of the playback between 0 and 1
        
        Zoom::State::Model zoomStateTime {{0.0, 60.0}};
        
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
        
        Analyzer::Accessor& getAnalyzerAccessor(size_t index);
        Zoom::State::Accessor& getZoomStateTimeAccessor();
    private:
        Zoom::State::Accessor mZoomStateTimeAccessor {mModel.zoomStateTime, {0.0, 60.0}, 0.001};
        std::vector<std::unique_ptr<Analyzer::Accessor>> mAnalyzerAccessors;
    };
}

ANALYSE_FILE_END
