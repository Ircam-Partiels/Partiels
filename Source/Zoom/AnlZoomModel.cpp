#include "AnlZoomModel.h"
#include "../Tools/AnlStringParser.h"

ANALYSE_FILE_BEGIN

Zoom::range_type Zoom::Accessor::sanitize(range_type const& visible, range_type const& global, double minLength)
{
    return global.constrainRange(visible.withEnd(std::max(visible.getStart() + minLength, visible.getEnd())));
}

template <>
void Zoom::Accessor::setValue<Zoom::AttrType::visibleRange, Zoom::range_type>(range_type const& value, NotificationType notification)
{
    ::Anl::Model::Accessor<Accessor, Container>::setValue<AttrType::visibleRange, Zoom::range_type>(sanitize(value, getValue<AttrType::globalRange>(), getValue<AttrType::minimumLength>()), notification);
}

template <>
void Zoom::Accessor::setValue<Zoom::AttrType::globalRange, Zoom::range_type>(range_type const& value, NotificationType notification)
{
    ::Anl::Model::Accessor<Accessor, Container>::setValue<AttrType::globalRange, Zoom::range_type>(value, notification);
    setValue<AttrType::visibleRange, Zoom::range_type>(getValue<AttrType::visibleRange>(), notification);
}

template <>
void Zoom::Accessor::setValue<Zoom::AttrType::minimumLength, double>(double const& value, NotificationType notification)
{
    ::Anl::Model::Accessor<Accessor, Container>::setValue<AttrType::minimumLength, double>(value, notification);
    setValue<AttrType::visibleRange, Zoom::range_type>(getValue<AttrType::visibleRange>(), notification);
}

class ZoomStateModelUnitTest
: public juce::UnitTest
{
public:
    
    ZoomStateModelUnitTest() : juce::UnitTest("Model - Zoom::State", "Model") {}
    
    ~ZoomStateModelUnitTest() override = default;
    
    void runTest() override
    {
        using namespace Zoom;
        Accessor acsr{{{range_type{0.0, 100.0}}, {1.0}, {range_type{0.0, 100.0}}}};
        
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
        
        beginTest("xml");
        {
            auto const xml = acsr.toXml("Test");
            expect(xml != nullptr);
            if(xml != nullptr)
            {
                Accessor acsr2{{{range_type{0.0, 100.0}}, {1.0}, {range_type{0.0, 100.0}}}};
                acsr2.fromXml(*xml.get(), "Test", NotificationType::synchronous);
                expect(acsr.getValue<AttrType::visibleRange>() == acsr2.getValue<AttrType::visibleRange>());
                expect(acsr.getValue<AttrType::globalRange>() != acsr2.getValue<AttrType::globalRange>());
                expect(std::abs(acsr.getValue<AttrType::minimumLength>() - acsr2.getValue<AttrType::minimumLength>()) > std::numeric_limits<double>::epsilon());
            }
        }
    }
};

static ZoomStateModelUnitTest zoomStateModelUnitTest;

ANALYSE_FILE_END
