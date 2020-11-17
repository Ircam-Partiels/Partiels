#include "AnlModel.h"

ANALYSE_FILE_BEGIN

struct DummyAttr
{
    bool operator==(DummyAttr const&) const { return true; }
    bool operator!=(DummyAttr const&) const { return false; }
};

namespace XmlParser
{
    template<>
    void toXml<DummyAttr>(juce::XmlElement& xml, juce::Identifier const& attributeName, DummyAttr const& value)
    {
        juce::ignoreUnused(value);
        xml.setAttribute(attributeName, "DummyAttr");
    }
    
    template<>
    auto fromXml<DummyAttr>(juce::XmlElement const& xml, juce::Identifier const& attributeName, DummyAttr const& value)
    -> DummyAttr
    {
        juce::ignoreUnused(xml, attributeName, value);
        anlWeakAssert(xml.hasAttribute(attributeName));
        anlWeakAssert(xml.getStringAttribute(attributeName) == "DummyAttr");
        return DummyAttr();
    }
}

class ModelUnitTest
: public juce::UnitTest
{
public:
    
    ModelUnitTest() : juce::UnitTest("Model", "Model") {}
    
    ~ModelUnitTest() override = default;
    
    void runTest() override
    {
        std::vector<std::unique_ptr<std::monostate>> vec;
        
        // Declare the name (type) of the attributes of the data model
        enum AttrType : size_t
        {
            attr0,
            attr1,
            attr2,
            attr3,
            attr4,
            attr5,
            attr6
        };
        
        // Declare the data model container
        using ModelCtnr = Model::Container
        < Model::Attr<AttrType::attr0, int, Model::AttrFlag::basic>
        , Model::Attr<AttrType::attr1, int, Model::AttrFlag::notifying>
        , Model::Attr<AttrType::attr2, float, Model::AttrFlag::saveable>
        , Model::Attr<AttrType::attr3, std::vector<int>, Model::AttrFlag::ignored>
        , Model::Attr<AttrType::attr4, std::string, Model::AttrFlag::notifying>
        , Model::Attr<AttrType::attr5, std::vector<double>, Model::AttrFlag::saveable | Model::AttrFlag::comparable>
        , Model::Attr<AttrType::attr6, DummyAttr, Model::AttrFlag::basic>
        >;
        
        // Declare the data model accessor
        class ModelAcsr
        : public Model::Accessor<ModelAcsr, ModelCtnr>
        {
            using Model::Accessor<ModelAcsr, ModelCtnr>::Accessor;
        };
        
        // Declare the data model listener
        using ModelLtnr = ModelAcsr::Listener;
        
        beginTest("attribute flags");
        {
            expect(magic_enum::enum_integer(Model::AttrFlag::ignored) == 0);
            expect(magic_enum::enum_integer(Model::AttrFlag::notifying) == 1);
            expect(magic_enum::enum_integer(Model::AttrFlag::saveable) == 2);
            expect(magic_enum::enum_integer(Model::AttrFlag::comparable) == 4);
            expect(magic_enum::enum_integer(Model::AttrFlag::basic) == 7);
        }
        
        beginTest("accessor default constructor");
        {
            ModelCtnr ctnr;
            ModelAcsr acsr {ctnr};
            expectEquals(acsr.getValue<AttrType::attr0>(), 0);
            expectEquals(acsr.getValue<AttrType::attr1>(), 0);
            expectEquals(acsr.getValue<AttrType::attr2>(), 0.0f);
            expect(acsr.getValue<AttrType::attr3>() == std::vector<int>{});
            expectEquals(acsr.getValue<AttrType::attr4>(), std::string{});
            expect(acsr.getValue<AttrType::attr5>() == std::vector<double>{});
        }
        
        beginTest("accessor constructor with model");
        {
            ModelCtnr ctnr({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            ModelAcsr acsr(ctnr);
            expectEquals(acsr.getValue<AttrType::attr0>(), 1);
            expectEquals(acsr.getValue<AttrType::attr1>(), 2);
            expectEquals(acsr.getValue<AttrType::attr2>(), 3.0f);
            expect(acsr.getValue<AttrType::attr3>() == std::vector<int>{4, 5, 6});
            expectEquals(acsr.getValue<AttrType::attr4>(), std::string{"Jules"});
            expect(acsr.getValue<AttrType::attr5>() == std::vector<double>{7.0, 8.0});
        }
        
        beginTest("accessor setting attribute");
        {
            ModelCtnr ctnr({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            ModelAcsr acsr(ctnr);
            acsr.setValue<AttrType::attr0>(2, NotificationType::synchronous);
            acsr.setValue<AttrType::attr1>(3, NotificationType::synchronous);
            acsr.setValue<AttrType::attr2>(4.0f, NotificationType::synchronous);
            acsr.setValue<AttrType::attr3>({5, 6, 7}, NotificationType::synchronous);
            acsr.setValue<AttrType::attr4>("Jim", NotificationType::synchronous);
            acsr.setValue<AttrType::attr5>({8.0, 9.0}, NotificationType::synchronous);
            expectEquals(acsr.getValue<AttrType::attr0>(), 2);
            expectEquals(acsr.getValue<AttrType::attr1>(), 3);
            expectEquals(acsr.getValue<AttrType::attr2>(), 4.0f);
            expect(acsr.getValue<AttrType::attr3>() == std::vector<int>{5, 6, 7});
            expectEquals(acsr.getValue<AttrType::attr4>(), std::string{"Jim"});
            expect(acsr.getValue<AttrType::attr5>() == std::vector<double>{8.0, 9.0});
        }
        
        beginTest("accessor from model");
        {
            ModelCtnr ctnr1;
            ModelAcsr acsr1(ctnr1);
            ModelCtnr ctnr2({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            ModelAcsr acsr2(ctnr2);
            acsr1.fromModel(acsr2.getModel(), NotificationType::synchronous);
            expectEquals(acsr1.getValue<AttrType::attr0>(), acsr2.getValue<AttrType::attr0>());
            expectNotEquals(acsr1.getValue<AttrType::attr1>(), acsr2.getValue<AttrType::attr1>());
            expectEquals(acsr1.getValue<AttrType::attr2>(), acsr2.getValue<AttrType::attr2>());
            expect(acsr1.getValue<AttrType::attr3>() != acsr2.getValue<AttrType::attr3>());
            expectNotEquals(acsr1.getValue<AttrType::attr4>(), acsr2.getValue<AttrType::attr4>());
            expect(acsr1.getValue<AttrType::attr5>() == acsr2.getValue<AttrType::attr5>());
        }
        
        beginTest("accessor xml");
        {
            ModelCtnr ctnr1;
            ModelAcsr acsr1(ctnr1);
            ModelCtnr ctnr2({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            ModelAcsr acsr2(ctnr2);
            auto xml = acsr2.toXml("Test");
            expect(xml != nullptr);
            if(xml != nullptr)
            {
                acsr1.fromXml(*xml.get(), "Test", NotificationType::synchronous);
            }
            expectEquals(acsr1.getValue<AttrType::attr0>(), acsr2.getValue<AttrType::attr0>());
            expectNotEquals(acsr1.getValue<AttrType::attr1>(), acsr2.getValue<AttrType::attr1>());
            expectEquals(acsr1.getValue<AttrType::attr2>(), acsr2.getValue<AttrType::attr2>());
            expect(acsr1.getValue<AttrType::attr3>() != acsr2.getValue<AttrType::attr3>());
            expectNotEquals(acsr1.getValue<AttrType::attr4>(), acsr2.getValue<AttrType::attr4>());
            expect(acsr1.getValue<AttrType::attr5>() == acsr2.getValue<AttrType::attr5>());
        }
            
        beginTest("is equivalent");
        {
            ModelCtnr ctnr1;
            ModelAcsr acsr1(ctnr1);
            ModelCtnr ctnr2({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            ModelAcsr acsr2(ctnr2);
            expect(acsr1.isEquivalentTo(acsr2.getModel()) == false);
            expect(acsr2.isEquivalentTo(acsr1.getModel()) == false);
            acsr1.fromModel(acsr2.getModel());
            expect(acsr1.isEquivalentTo(acsr2.getModel()) == true);
            expect(acsr2.isEquivalentTo(acsr1.getModel()) == true);
        }
        
        beginTest("accessor listener");
        {
            std::array<bool, magic_enum::enum_count<AttrType>()> notifications;
            
            ModelLtnr ltnr;
            ltnr.onChanged = [&](ModelAcsr const& acsr, AttrType attribute)
            {
                juce::ignoreUnused(acsr);
                notifications[magic_enum::enum_integer(attribute)] = true;
            };
            
            ModelAcsr acsr({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            
            std::fill(notifications.begin(), notifications.end(), false);
            acsr.addListener(ltnr, NotificationType::synchronous);
            expect(notifications[magic_enum::enum_integer(AttrType::attr0)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr1)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr2)] == false);
            expect(notifications[magic_enum::enum_integer(AttrType::attr3)] == false);
            expect(notifications[magic_enum::enum_integer(AttrType::attr4)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr5)] == false);
            
            std::fill(notifications.begin(), notifications.end(), false);
            acsr.setValue<AttrType::attr0>(2, NotificationType::synchronous);
            acsr.setValue<AttrType::attr1>(3, NotificationType::synchronous);
            acsr.setValue<AttrType::attr2>(4.0f, NotificationType::synchronous);
            acsr.setValue<AttrType::attr3>({5, 6, 7}, NotificationType::synchronous);
            acsr.setValue<AttrType::attr4>("Jim", NotificationType::synchronous);
            acsr.setValue<AttrType::attr5>({7.0, 8.0}, NotificationType::synchronous);
            expect(notifications[magic_enum::enum_integer(AttrType::attr0)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr1)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr2)] == false);
            expect(notifications[magic_enum::enum_integer(AttrType::attr3)] == false);
            expect(notifications[magic_enum::enum_integer(AttrType::attr4)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr5)] == false);
            
            acsr.removeListener(ltnr);
        }
    }
};

static ModelUnitTest modelUnitTest;

ANALYSE_FILE_END
