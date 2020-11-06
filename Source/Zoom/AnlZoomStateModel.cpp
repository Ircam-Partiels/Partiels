#include "AnlZoomStateModel.h"
#include "../Tools/AnlStringParser.h"

ANALYSE_FILE_BEGIN

template <>
void Zoom::State::Accessor::setValue<Zoom::State::AttrType::visibleRange, Zoom::State::range_type>(range_type const& value, NotificationType notification)
{
    auto sanitizeRange = [&](juce::Range<double> const& input)
    {
        auto const& globaleRange = getValue<AttrType::globalRange>();
        auto const& minimumLength = getValue<AttrType::minimumLength>();
        if(globaleRange.isEmpty())
        {
            return input.withEnd(std::max(input.getStart() + minimumLength, input.getEnd()));
        }
        return globaleRange.constrainRange(input.withEnd(std::max(input.getStart() + minimumLength, input.getEnd())));
    };
    ::Anl::Model::Accessor<Accessor, Container>::setValue<AttrType::visibleRange, Zoom::State::range_type>(sanitizeRange(value), notification);
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
    
    ZoomStateModelUnitTest() : juce::UnitTest("Model Util Test", "Model") {}
    
    ~ZoomStateModelUnitTest() override = default;
    
    void runTest() override
    {
        using namespace Zoom::State;
        Accessor acsr{{{range_type{0.0, 100.0}}, {range_type{0.0, 100.0}}, {0.001}}};
        Accessor::Listener ltnr;
        
        acsr.setValue<AttrType::visibleRange>(juce::Range<double>{0.0, 50.0}, NotificationType::synchronous);
        std::cout << acsr.getValue<AttrType::visibleRange>().getEnd() << "\n";
        acsr.setValue<AttrType::visibleRange>(juce::Range<double>{0.0, 120.0}, NotificationType::synchronous);
        std::cout << acsr.getValue<AttrType::visibleRange>().getEnd() << "\n";
        int zaza;
//        beginTest("Test attribute flags");
//        {
//            expect(magic_enum::enum_integer(AttrFlag::ignored) == 0);
//            expect(magic_enum::enum_integer(AttrFlag::notifying) == 1);
//            expect(magic_enum::enum_integer(AttrFlag::saveable) == 2);
//            expect(magic_enum::enum_integer(AttrFlag::comparable) == 4);
//            expect(magic_enum::enum_integer(AttrFlag::all) == 7);
//        }
//
    }
};

static ZoomStateModelUnitTest zoomStateModelUnitTest;

ANALYSE_FILE_END
