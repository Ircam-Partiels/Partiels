#pragma once

#include "../Tools/AnlModelAccessor.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    //! @brief The data model of a track
    //! @details The analyzer model contains the information that represent a analyzer plugin and
    //! its results.
    struct Model
    {
        enum class Attribute
        {
            key,
            parameters,
            programName,
            resultFile,
            projectFile
        };
        
        juce::String key; //!< The unique key of the analyser
        std::map<juce::String, double> parameters; //!< The parameters values associated to their names
        juce::String programName; //!< The name of the program
        juce::File resultFile; //!< The file containing the saved results
        juce::File projectFile; //!< The path of the project
        
        std::unique_ptr<juce::XmlElement> toXml() const;
        static Model fromXml(juce::XmlElement const& xml, Model defaultModel = {});
        static std::set<Attribute> getAttributeTypes();

        JUCE_LEAK_DETECTOR(Model)
    };
    
    class Accessor : public Tools::ModelAccessor<Accessor, Model, Model::Attribute>
    {
    public:
        using Tools::ModelAccessor<Accessor, Model, Model::Attribute>::ModelAccessor;
        ~Accessor() override = default;
        void fromModel(Model const& model, juce::NotificationType const notification) override;
    };
}

ANALYSE_FILE_END
