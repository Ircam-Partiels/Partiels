#include "AnlZoomModel.h"

ANALYSE_FILE_BEGIN

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

ANALYSE_FILE_END
