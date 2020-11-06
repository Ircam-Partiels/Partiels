#include "AnlZoomStateModel.h"
#include "../Tools/AnlStringParser.h"

ANALYSE_FILE_BEGIN

Zoom::State::range_type Zoom::State::Accessor::sanitize(range_type const& visible, range_type const& global, double minLength)
{
    return global.constrainRange(visible.withEnd(std::max(visible.getStart() + minLength, visible.getEnd())));
}

template <>
void Zoom::State::Accessor::setValue<Zoom::State::AttrType::visibleRange, Zoom::State::range_type>(range_type const& value, NotificationType notification)
{
    ::Anl::Model::Accessor<Accessor, Container>::setValue<AttrType::visibleRange, Zoom::State::range_type>(sanitize(value, getValue<AttrType::globalRange>(), getValue<AttrType::minimumLength>()), notification);
}

template <>
void Zoom::State::Accessor::setValue<Zoom::State::AttrType::globalRange, Zoom::State::range_type>(range_type const& value, NotificationType notification)
{
    ::Anl::Model::Accessor<Accessor, Container>::setValue<AttrType::globalRange, Zoom::State::range_type>(value, notification);
    setValue<AttrType::visibleRange, Zoom::State::range_type>(getValue<AttrType::visibleRange>(), notification);
}

template <>
void Zoom::State::Accessor::setValue<Zoom::State::AttrType::minimumLength, double>(double const& value, NotificationType notification)
{
    ::Anl::Model::Accessor<Accessor, Container>::setValue<AttrType::minimumLength, double>(value, notification);
    setValue<AttrType::visibleRange, Zoom::State::range_type>(getValue<AttrType::visibleRange>(), notification);
}

class ZoomStateModelUnitTest
: public juce::UnitTest
{
public:
    
    ZoomStateModelUnitTest() : juce::UnitTest("Model - Zoom::State", "Model") {}
    
    ~ZoomStateModelUnitTest() override = default;
    
    void runTest() override
    {
        using namespace Zoom::State;
        Accessor acsr{{{range_type{0.0, 100.0}}, {range_type{0.0, 100.0}}, {1.0}}};
        beginTest("sanitize");
        {
            acsr.setValue<AttrType::visibleRange>(range_type{0.0, 50.0}, NotificationType::synchronous);
            expect(acsr.getValue<AttrType::visibleRange>() == range_type{0.0, 50.0});
            acsr.setValue<AttrType::visibleRange>(range_type{0.0, 120.0}, NotificationType::synchronous);
            expect(acsr.getValue<AttrType::visibleRange>() == range_type{0.0, 100.0});
            acsr.setValue<AttrType::globalRange>(range_type{10.0, 80.0}, NotificationType::synchronous);
            expect(acsr.getValue<AttrType::visibleRange>() == range_type{10.0, 80.0});
            acsr.setValue<AttrType::visibleRange>(range_type{10.0, 20.0}, NotificationType::synchronous);
            expect(acsr.getValue<AttrType::visibleRange>() == range_type{10.0, 20.0});
            acsr.setValue<AttrType::minimumLength>(30.0, NotificationType::synchronous);
            expect(acsr.getValue<AttrType::visibleRange>() == range_type{10.0, 40.0});
        }
        beginTest("from/to xml");
        {
            auto const xml = acsr.toXml("Test");
        }
        
    }
};

static ZoomStateModelUnitTest zoomStateModelUnitTest;

ANALYSE_FILE_END
