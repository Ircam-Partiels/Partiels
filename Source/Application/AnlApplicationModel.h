#pragma once

#include "../Misc/AnlModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    using Flag = Model::Flag;
    
    enum class AttrType : size_t
    {
          windowState
        , recentlyOpenedFilesList
        , currentDocumentFile
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::windowState, juce::String, Flag::basic>
    , Model::Attr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>, Flag::basic>
    , Model::Attr<AttrType::currentDocumentFile, juce::File, Flag::basic>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, AttrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer>::Accessor;
        
        template <attr_enum_type type, typename value_v>
        void setAttr(value_v const& value, NotificationType notification)
        {
            if constexpr(type == AttrType::recentlyOpenedFilesList)
            {
                auto sanitize = [](std::vector<juce::File> const& files)
                {
                    std::vector<juce::File> copy = files;
                    copy.erase(std::unique(copy.begin(), copy.end()), copy.end());
                    copy.erase(std::remove_if(copy.begin(), copy.end(), [](auto const& file)
                    {
                        return !file.existsAsFile();
                    }), copy.end());
                    return copy;
                };
                
                Anl::Model::Accessor<Accessor, AttrContainer>::setAttr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>>(sanitize(value), notification);
            }
            else
            {
                Model::Accessor<Accessor, AttrContainer>::setAttr<type, value_v>(value, notification);
            }
        }
    };
}

ANALYSE_FILE_END
