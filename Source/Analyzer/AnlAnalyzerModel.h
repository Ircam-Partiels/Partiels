#pragma once

#include "../Misc/AnlModel.h"
#include "../Misc/AnlBroadcaster.h"
#include "../Misc/AnlAtomicManager.h"
#include "../Zoom/AnlZoomModel.h"
#include "../Plugin/AnlPluginModel.h"
#include "../Plot/AnlPlotModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    enum class AttrType : size_t
    {
          identifier
        , name
        , key
        , description
        , state
        
        , results
        , time
        , warnings
        , processing
    };
    
    enum class AcsrType : size_t
    {
          plot
    };
    
    enum class SignalType
    {
          image
    };
    
    enum class WarningType
    {
          feature
        , zoomMode
        , resultType
    };
    
    using AttrContainer = Model::Container
    < Model::Attr<AttrType::identifier, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::name, juce::String, Model::Flag::basic>
    , Model::Attr<AttrType::key, Plugin::Key, Model::Flag::basic>
    , Model::Attr<AttrType::description, Plugin::Description, Model::Flag::basic>
    , Model::Attr<AttrType::state, Plugin::State, Model::Flag::basic>
    
    , Model::Attr<AttrType::results, std::vector<Plugin::Result>, Model::Flag::notifying>
    , Model::Attr<AttrType::time, double, Model::Flag::notifying>
    , Model::Attr<AttrType::warnings, std::map<WarningType, juce::String>, Model::Flag::notifying>
    , Model::Attr<AttrType::processing, bool, Model::Flag::notifying>
    >;
    
    using AcsrContainer = Model::Container
    < Model::Acsr<AcsrType::plot, Plot::Accessor, Model::Flag::saveable | Model::Flag::notifying, 1>
    >;

    class Accessor
    : public Model::Accessor<Accessor, AttrContainer, AcsrContainer>
    , public Broadcaster<Accessor, SignalType>
    {
    public:
        using Model::Accessor<Accessor, AttrContainer, AcsrContainer>::Accessor;
        
        Accessor()
        : Accessor(AttrContainer(  {""}
                                 , {""}
                                 , {}
                                 , {}
                                 , {}
                                 , {}
                                 , {0.0}
                                 , {}
                                 , {false}))
        {
        }
        
        std::shared_ptr<juce::Image const> getImage() const;
        void setImage(std::shared_ptr<juce::Image> image, NotificationType notification);
    private:
            
        AtomicManager<juce::Image> mImageManager;
    };
}

ANALYSE_FILE_END
