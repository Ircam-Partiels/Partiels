#pragma once

#include "../Misc/AnlMisc.h"
#include "AnlApplicationLookAndFeel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    using Flag = Model::Flag;

    // clang-format off
    enum class AttrType : size_t
    {
          windowState
        , recentlyOpenedFilesList
        , currentDocumentFile
        , colourMode
        , showInfoBubble
    };

    using AttrContainer = Model::Container
    < Model::Attr<AttrType::windowState, juce::String, Flag::basic>
    , Model::Attr<AttrType::recentlyOpenedFilesList, std::vector<juce::File>, Flag::basic>
    , Model::Attr<AttrType::currentDocumentFile, juce::File, Flag::basic>
    , Model::Attr<AttrType::colourMode, LookAndFeel::ColourChart::Mode, Flag::basic>
    , Model::Attr<AttrType::showInfoBubble, bool, Flag::basic>
    >;
    // clang-format on

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer>::Accessor;
        // clang-format off
        Accessor()
        : Accessor(AttrContainer(
        {
              {juce::String{}}
            , {std::vector<juce::File>{}}
            , {juce::File{}}
            , {LookAndFeel::ColourChart::Mode::night}
            , {true}
        }))
        {
        }
        // clang-format on

        template <attr_enum_type type, typename value_v>
        void setAttr(value_v const& value, NotificationType notification)
        {
            if constexpr(type == AttrType::recentlyOpenedFilesList)
            {
                auto sanitize = [](std::vector<juce::File> const& files)
                {
                    std::set<juce::File> duplicates;
                    std::vector<juce::File> copy;
                    for(auto const& file : files)
                    {
                        if(file.existsAsFile() && duplicates.insert(file).second)
                        {
                            copy.push_back(file);
                        }
                    }
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
} // namespace Application

ANALYSE_FILE_END
