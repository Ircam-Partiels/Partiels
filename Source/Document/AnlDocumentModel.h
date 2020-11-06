#pragma once

#include "../Analyzer/AnlAnalyzerModel.h"
#include "../Zoom/AnlZoomStateModel.h"
#include "../Tools/AnlBroadcaster.h"

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
            isLooping,
            gain,
            isPlaybackStarted,
            playheadPosition,
            analyzers
        };
        
        enum class Signal
        {
            movePlayhead,
            audioInstanceChanged
        };
        
        Model() {}
        Model(Model const& other);
        Model(Model&& other);
        
        ~Model() = default;
        bool operator==(Model const& other) const;
        bool operator!=(Model const& other) const;
        
        juce::File file; //!< The audio file associated with the document (saved/compared)
        bool isLooping = false; //!< If the loop is active (saved/compared)
        double gain = 1.0; //! The gain of the playback between 0 and 1 (saved/compared)
        bool isPlaybackStarted = false; //! If the playback is started (unsaved/not compared)
        double playheadPosition = 0.0; // The position of the playhead (unsaved/not compared)
        Zoom::State::Accessor zoomStateTime {{{Zoom::State::range_type{0.0, 60.0}}, {Zoom::State::range_type{0.0, 60.0}}, {0.001}}};  // The zoom state of the time (saved/not compared)
        std::vector<std::unique_ptr<Analyzer::Model>> analyzers; //!< The analyzers of the document (saved/compared)
        
        std::unique_ptr<juce::XmlElement> toXml() const;
        static Model fromXml(juce::XmlElement const& xml, Model defaultModel = {});
        
        JUCE_LEAK_DETECTOR(Model)
    };
    
    class Accessor
    : public Tools::ModelAccessor<Accessor, Model, Model::Attribute>
    , public Broadcaster<Accessor, Model::Signal>
    {
    public:
        using Tools::ModelAccessor<Accessor, Model, Model::Attribute>::ModelAccessor;
        ~Accessor() override = default;
        void fromModel(Model const& model, NotificationType const notification) override;
        
        Analyzer::Accessor& getAnalyzerAccessor(size_t index);
        Zoom::State::Accessor& getZoomStateTimeAccessor();
    private:
        std::vector<std::unique_ptr<Analyzer::Accessor>> mAnalyzerAccessors;
    };
}

ANALYSE_FILE_END
