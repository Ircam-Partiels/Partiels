#pragma once

#include "../Tools/AnlModelAccessor.h"
#include "../Tools/AnlSignalBroadcaster.h"
#include "../Tools/AnlAtomicManager.h"

#include <vamp-hostsdk/PluginLoader.h>
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    //! @brief The data model of a track
    //! @details The analyzer model contains the information that represent a analyzer plugin and
    //! its results.
    struct Model
    {
        JUCE_COMPILER_WARNING("remove unecessary members")
        
        enum class Attribute
        {
            key,
            name,
            sampleRate,
            numChannels,
            
            parameters,
            programName,
            resultFile,
            projectFile
        };
        
        enum class Signal
        {
            pluginInstanceChanged
        };
        
        juce::String key; //!< The unique key of the analyser
        juce::String name; //!< The name of the analyser (can't be different to the plugin name)
        double sampleRate; //!< The sample rate used for the analysis
        unsigned long numChannels; //!< The number of channels used for the analysis
        
        std::map<juce::String, double> parameters; //!< The parameters values associated to their names
        juce::String programName; //!< The name of the program
        juce::File resultFile; //!< The file containing the saved results
        juce::File projectFile; //!< The path of the project
        
        bool operator==(Model const& rhd) const
        {
            return key == rhd.key && parameters == rhd.parameters && programName == rhd.programName && resultFile == rhd.resultFile && projectFile == rhd.projectFile;
        }
        
        std::unique_ptr<juce::XmlElement> toXml() const;
        static Model fromXml(juce::XmlElement const& xml, Model defaultModel = {});

        JUCE_LEAK_DETECTOR(Model)
    };
    
    class Accessor
    : public Tools::ModelAccessor<Accessor, Model, Model::Attribute>
    , public Tools::SignalBroadcaster<Accessor, Model::Signal>
    , public Tools::AtomicManager<Vamp::Plugin>
    {
    public:
        using Tools::ModelAccessor<Accessor, Model, Model::Attribute>::ModelAccessor;
        ~Accessor() override = default;
        void fromModel(Model const& model, juce::NotificationType const notification) override;
    };
}

ANALYSE_FILE_END
