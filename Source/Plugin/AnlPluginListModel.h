#pragma once

#include "AnlPluginModel.h"

ANALYSE_FILE_BEGIN

namespace PluginList
{
    // clang-format off
    enum class AttrType : size_t
    {
          useEnvVariable
        , quarantineMode
        , searchPath
        , sortColumn
        , sortIsFowards
    };
    
    enum class QuarantineMode
    {
          system
        , force
        , ignore
    };
    
    enum ColumnType
    {
          name = 1
        , feature
        , maker
        , version
        , category
        , details
    };
    
    enum class SignalType
    {
          rescan
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::useEnvVariable, bool, Model::Flag::basic>
    , Model::Attr<AttrType::quarantineMode, QuarantineMode, Model::Flag::basic>
    , Model::Attr<AttrType::searchPath, std::vector<juce::File>, Model::Flag::basic>
    , Model::Attr<AttrType::sortColumn, ColumnType, Model::Flag::basic>
    , Model::Attr<AttrType::sortIsFowards, bool, Model::Flag::basic>
    >;
    // clang-format on

    std::vector<juce::File> getDefaultSearchPath();

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer>
    , public Broadcaster<Accessor, SignalType>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer>::Accessor;

        // clang-format off
        Accessor()
        : Accessor(AttrContainer(
        {
              {true}
            , {QuarantineMode::force}
            , {getDefaultSearchPath()}
            , {ColumnType::name}
            , {true}
        }))
        {
        }
        // clang-format on

        std::unique_ptr<juce::XmlElement> parseXml(juce::XmlElement const& xml, int version) override
        {
            auto copy = std::make_unique<juce::XmlElement>(xml);
            if(copy != nullptr && version <= 0x8)
            {
                XmlParser::toXml(*copy.get(), "useEnvVariable", true);
                XmlParser::toXml(*copy.get(), "searchPath", getDefaultSearchPath());
            }
            return copy;
        }
    };

    void setEnvironment(Accessor const& accessor);

#if JUCE_MAC
    std::vector<juce::File> findLibrariesInQuarantine(Accessor const& accessor);
    bool removeLibrariesFromQuarantine(std::vector<juce::File> const& files);
#endif
} // namespace PluginList

ANALYSE_FILE_END
