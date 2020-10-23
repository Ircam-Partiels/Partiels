
#pragma once

#include "../Tools/AnlModelAccessor.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    //! @brief The data model of a plugin list
    //! @details ...
    struct Model
    {
        enum class Attribute
        {
            descriptions,
            sortColumn,
            sortIsFowards
        };
        
        enum ColumnType
        {
            Name = 1,
            Maker = 2,
            Api = 3,
            Category = 4,
            Details = 5
        };
        
        struct Description
        {
            juce::String name {}; //!< The name of the plugin
            juce::String maker {}; //!< The maker of the plugin
            unsigned int api {}; //!< The API version used by the plugin
            std::set<juce::String> categories {}; //!< The categories of the plugin
            juce::String details {}; //!< Further information about the plugin
            
            bool operator==(Description const& rhd) const
            {
                return name == rhd.name && maker == rhd.maker && api == rhd.api && categories == rhd.categories && details == rhd.details;
            }
        };
        
        std::map<juce::String, Description> descriptions {}; //!< The descriptions of the plugins associated the plugin keys
        ColumnType sortColumn {ColumnType::Name}; //<! The column used to sort the list of plugins
        bool sortIsFowards {true}; //<! If the sorting is forward
        
        Model() {} // This is to avoid a clang error with default
        std::unique_ptr<juce::XmlElement> toXml() const;
        static Model fromXml(juce::XmlElement const& xml, Model defaultModel = {});
        static std::set<Attribute> getAttributeTypes();
        
        JUCE_LEAK_DETECTOR(Model)
    };
    
    class Accessor
    : public Tools::ModelAccessor<Accessor, Model, Model::Attribute>
    {
    public:
        using Tools::ModelAccessor<Accessor, Model, Model::Attribute>::ModelAccessor;
        ~Accessor() override = default;
        void fromModel(Model const& model, juce::NotificationType const notification) override;
    };
}

ANALYSE_FILE_END
