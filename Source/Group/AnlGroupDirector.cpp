#include "AnlGroupDirector.h"
#include "AnlGroupTools.h"

ANALYSE_FILE_BEGIN

Group::Director::Director(Accessor& accessor)
: mAccessor(accessor)
{
    mAccessor.onAttrUpdated = [&](AttrType attribute, NotificationType notification)
    {
        switch(attribute)
        {
            case AttrType::identifier:
            case AttrType::name:
            case AttrType::height:
            case AttrType::colour:
            case AttrType::expanded:
            case AttrType::focused:
                break;
            case AttrType::layout:
            case AttrType::tracks:
            {
                auto trackAcsrs = mAccessor.getAttr<AttrType::tracks>();
                auto const layout = mAccessor.getAttr<AttrType::layout>();
                auto& zoomAcsr = mAccessor.getAcsr<AcsrType::zoom>();
                for(auto const& identifier : layout)
                {
                    auto trackAcsr = Tools::getTrackAcsr(mAccessor, identifier);
                    if(trackAcsr.has_value())
                    {
                        trackAcsr->get().setAttr<Track::AttrType::zoomAcsr>(std::ref(zoomAcsr), notification);
                    }
                }

            }
                break;
        }
    };
}

Group::Director::~Director()
{
    mAccessor.onAttrUpdated = nullptr;
    mAccessor.onAccessorInserted = nullptr;
    mAccessor.onAccessorErased = nullptr;
}

ANALYSE_FILE_END
