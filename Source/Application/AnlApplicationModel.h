#pragma once

#include "../Tools/AnlModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    using AttrFlag = Model::AttrFlag;
    
    enum AttrType : size_t
    {
        windowState,
        recentlyOpenedFilesList,
        currentDocumentFile
    };
    
    using Container = Model::Container
    < Model::Attr<AttrType::windowState, juce::String, AttrFlag::basic>
    , Model::Attr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>, AttrFlag::basic>
    , Model::Attr<AttrType::currentDocumentFile, juce::File, AttrFlag::basic>
    >;
    
    class Accessor
    : public Model::Accessor<Accessor, Container>
    {
    public:
        using Model::Accessor<Accessor, Container>::Accessor;
        using enum_type = Model::Accessor<Accessor, Container>::enum_type;
        
        template <enum_type type, typename value_v>
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
                
                Anl::Model::Accessor<Accessor, Container>::setAttr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>>(sanitize(value), notification);
            }
            else
            {
                Model::Accessor<Accessor, Container>::setAttr<type, value_v>(value, notification);
            }
        }
    };
}

ANALYSE_FILE_END
