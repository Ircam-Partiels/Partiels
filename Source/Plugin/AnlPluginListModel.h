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
    };
    
    enum class QuarantineMode
    {
          system
        , force
        , ignore
    };
    
    enum class SignalType
    {
          rescan
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::useEnvVariable, bool, Model::Flag::basic>
    , Model::Attr<AttrType::quarantineMode, QuarantineMode, Model::Flag::basic>
    , Model::Attr<AttrType::searchPath, std::vector<juce::File>, Model::Flag::basic>
    >;
    // clang-format on

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer>
    , public Broadcaster<Accessor, SignalType>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer>::Accessor;
        Accessor();

        std::unique_ptr<juce::XmlElement> parseXml(juce::XmlElement const& xml, int version) override;
    };

    std::vector<juce::File> getDefaultSearchPath();
    void setEnvironment(Accessor const& accessor);

#if JUCE_MAC
    std::vector<juce::File> findLibrariesInQuarantine(Accessor const& accessor);
    bool removeLibrariesFromQuarantine(std::vector<juce::File> const& files);
#endif
} // namespace PluginList

ANALYSE_FILE_END
