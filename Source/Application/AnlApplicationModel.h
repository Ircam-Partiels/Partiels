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
            Model::Accessor<Accessor, Container>::setAttr<type, value_v>(value, notification);
        }
        
        template <>
        void setAttr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>>(std::vector<juce::File> const& value, NotificationType notification);
    private:
        static std::vector<juce::File> sanitize(std::vector<juce::File> const& files);
    };
}

ANALYSE_FILE_END
