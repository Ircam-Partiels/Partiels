#include "MiscZoomModel.h"

MISC_FILE_BEGIN

void Zoom::Accessor::setGlobalRange(Range const& globalRange, NotificationType notification)
{
    Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<AttrType::globalRange, Range>(globalRange.withLength(std::max(globalRange.getLength(), getAttr<AttrType::minimumLength>())), notification);
    setVisibleRange(getAttr<AttrType::visibleRange>(), notification);
    setAnchor(getAttr<AttrType::anchor>(), notification);
}

void Zoom::Accessor::setMinimumLength(double const& minimumLength, NotificationType notification)
{
    Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<AttrType::minimumLength, double>(std::max(minimumLength, 0.0), notification);
    setGlobalRange(getAttr<AttrType::globalRange>(), notification);
}

void Zoom::Accessor::setVisibleRange(Range const& visibleRange, NotificationType notification)
{
    auto const minimumLength = getAttr<AttrType::minimumLength>();
    auto const globalRange = getAttr<AttrType::globalRange>();
    Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<AttrType::visibleRange, Range>(globalRange.constrainRange(visibleRange.withLength(std::max(visibleRange.getLength(), minimumLength))), notification);
}

void Zoom::Accessor::setAnchor(std::tuple<bool, double> const& anchor, NotificationType notification)
{
    Model::Accessor<Accessor, AttrContainer, AcsrContainer>::setAttr<AttrType::anchor, std::tuple<bool, double>>(std::make_tuple(std::get<0_z>(anchor), getAttr<AttrType::globalRange>().clipValue(std::get<1_z>(anchor))), notification);
}

class ZoomModelUnitTest
: public juce::UnitTest
{
public:
    ZoomModelUnitTest()
    : juce::UnitTest("Model - Zoom::State", "Model")
    {
    }

    ~ZoomModelUnitTest() override = default;

    void runTest() override
    {
        using namespace Zoom;
        Accessor acsr(Range{0.0, 100.0}, 1.0);

        beginTest("sanitize");
        {
            acsr.setAttr<AttrType::visibleRange>(Range{0.0, 50.0}, NotificationType::synchronous);
            expect(acsr.getAttr<AttrType::visibleRange>() == Range{0.0, 50.0});
            acsr.setAttr<AttrType::visibleRange>(Range{0.0, 120.0}, NotificationType::synchronous);
            expect(acsr.getAttr<AttrType::visibleRange>() == Range{0.0, 100.0});
            acsr.setAttr<AttrType::globalRange>(Range{10.0, 80.0}, NotificationType::synchronous);
            expect(acsr.getAttr<AttrType::visibleRange>() == Range{10.0, 80.0});
            acsr.setAttr<AttrType::visibleRange>(Range{10.0, 20.0}, NotificationType::synchronous);
            expect(acsr.getAttr<AttrType::visibleRange>() == Range{10.0, 20.0});
            acsr.setAttr<AttrType::minimumLength>(30.0, NotificationType::synchronous);
            expect(acsr.getAttr<AttrType::visibleRange>() == Range{10.0, 40.0});
        }

        beginTest("xml");
        {
            auto const xml = acsr.toXml("Test");
            expect(xml != nullptr);
            if(xml != nullptr)
            {
                Accessor acsr2(Range{0.0, 100.0}, 1.0);
                acsr2.fromXml(*xml.get(), "Test", NotificationType::synchronous);
                expect(acsr.getAttr<AttrType::visibleRange>() == acsr2.getAttr<AttrType::visibleRange>());
                expect(acsr.getAttr<AttrType::globalRange>() == acsr2.getAttr<AttrType::globalRange>());
                expect(std::abs(acsr.getAttr<AttrType::minimumLength>() - acsr2.getAttr<AttrType::minimumLength>()) < std::numeric_limits<double>::epsilon());
            }
        }
    }
};

static ZoomModelUnitTest zoomStateModelUnitTest;

MISC_FILE_END
