#include "MiscModel.h"

MISC_FILE_BEGIN

struct DummyAttr
{
    bool operator==(DummyAttr const&) const { return true; }
    bool operator!=(DummyAttr const&) const { return false; }
};

static void to_json(nlohmann::json&, DummyAttr const&) {}

static void from_json(nlohmann::json const&, DummyAttr&) {}

namespace XmlParser
{
    template <>
    void toXml<DummyAttr>(juce::XmlElement& xml, juce::Identifier const& attributeName, DummyAttr const& value)
    {
        juce::ignoreUnused(value);
        xml.setAttribute(attributeName, "DummyAttr");
    }

    template <>
    auto fromXml<DummyAttr>(juce::XmlElement const& xml, juce::Identifier const& attributeName, DummyAttr const& value)
        -> DummyAttr
    {
        juce::ignoreUnused(xml, attributeName, value);
        MiscWeakAssert(xml.hasAttribute(attributeName));
        MiscWeakAssert(xml.getStringAttribute(attributeName) == "DummyAttr");
        return DummyAttr();
    }
} // namespace XmlParser

class ModelUnitTest
: public juce::UnitTest
{
public:
    ModelUnitTest()
    : juce::UnitTest("Model", "Model")
    {
    }

    ~ModelUnitTest() override = default;

    void runTest() override
    {
        std::vector<std::unique_ptr<std::monostate>> vec;

        // Declare the name (type) of the attributes of the data model
        // clang-format off
        enum AttrType : size_t
        {
              attr0
            , attr1
            , attr2
            , attr3
            , attr4
            , attr5
            , attr6
        };
        // clang-format on

        // Declare the data model container
        // clang-format off
        using ModelCtnr = Model::Container
        < Model::Attr<AttrType::attr0, int, Model::Flag::basic>
        , Model::Attr<AttrType::attr1, int, Model::Flag::notifying>
        , Model::Attr<AttrType::attr2, float, Model::Flag::saveable>
        , Model::Attr<AttrType::attr3, std::vector<int>, Model::Flag::ignored>
        , Model::Attr<AttrType::attr4, std::string, Model::Flag::notifying>
        , Model::Attr<AttrType::attr5, std::vector<double>, Model::Flag::saveable | Model::Flag::comparable>
        , Model::Attr<AttrType::attr6, DummyAttr, Model::Flag::basic>
        >;
        // clang-format on

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
            expect(magic_enum::enum_integer(Model::Flag::ignored) == 0);
            expect(magic_enum::enum_integer(Model::Flag::notifying) == 1);
            expect(magic_enum::enum_integer(Model::Flag::saveable) == 2);
            expect(magic_enum::enum_integer(Model::Flag::comparable) == 4);
            expect(magic_enum::enum_integer(Model::Flag::basic) == 7);
        }

        beginTest("accessor default constructor");
        {
            ModelCtnr ctnr;
            ModelAcsr acsr{ctnr};
            expectEquals(acsr.getAttr<AttrType::attr0>(), 0);
            expectEquals(acsr.getAttr<AttrType::attr1>(), 0);
            expectEquals(acsr.getAttr<AttrType::attr2>(), 0.0f);
            expect(acsr.getAttr<AttrType::attr3>() == std::vector<int>{});
            expectEquals(acsr.getAttr<AttrType::attr4>(), std::string{});
            expect(acsr.getAttr<AttrType::attr5>() == std::vector<double>{});
        }

        beginTest("accessor constructor with model");
        {
            ModelCtnr ctnr({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            ModelAcsr acsr(ctnr);
            expectEquals(acsr.getAttr<AttrType::attr0>(), 1);
            expectEquals(acsr.getAttr<AttrType::attr1>(), 2);
            expectEquals(acsr.getAttr<AttrType::attr2>(), 3.0f);
            expect(acsr.getAttr<AttrType::attr3>() == std::vector<int>{4, 5, 6});
            expectEquals(acsr.getAttr<AttrType::attr4>(), std::string{"Jules"});
            expect(acsr.getAttr<AttrType::attr5>() == std::vector<double>{7.0, 8.0});
        }

        beginTest("accessor setting attribute");
        {
            ModelCtnr ctnr({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            ModelAcsr acsr(ctnr);
            acsr.setAttr<AttrType::attr0>(2, NotificationType::synchronous);
            acsr.setAttr<AttrType::attr1>(3, NotificationType::synchronous);
            acsr.setAttr<AttrType::attr2>(4.0f, NotificationType::synchronous);
            acsr.setAttr<AttrType::attr3>({5, 6, 7}, NotificationType::synchronous);
            acsr.setAttr<AttrType::attr4>("Jim", NotificationType::synchronous);
            acsr.setAttr<AttrType::attr5>({8.0, 9.0}, NotificationType::synchronous);
            expectEquals(acsr.getAttr<AttrType::attr0>(), 2);
            expectEquals(acsr.getAttr<AttrType::attr1>(), 3);
            expectEquals(acsr.getAttr<AttrType::attr2>(), 4.0f);
            expect(acsr.getAttr<AttrType::attr3>() == std::vector<int>{5, 6, 7});
            expectEquals(acsr.getAttr<AttrType::attr4>(), std::string{"Jim"});
            expect(acsr.getAttr<AttrType::attr5>() == std::vector<double>{8.0, 9.0});
        }

        beginTest("accessor from container");
        {
            ModelCtnr ctnr1;
            ModelAcsr acsr1(ctnr1);
            ModelCtnr ctnr2({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            ModelAcsr acsr2(ctnr2);
            acsr1.copyFrom(acsr2, NotificationType::synchronous);
            expectEquals(acsr1.getAttr<AttrType::attr0>(), acsr2.getAttr<AttrType::attr0>());
            expectNotEquals(acsr1.getAttr<AttrType::attr1>(), acsr2.getAttr<AttrType::attr1>());
            expectEquals(acsr1.getAttr<AttrType::attr2>(), acsr2.getAttr<AttrType::attr2>());
            expect(acsr1.getAttr<AttrType::attr3>() != acsr2.getAttr<AttrType::attr3>());
            expectNotEquals(acsr1.getAttr<AttrType::attr4>(), acsr2.getAttr<AttrType::attr4>());
            expect(acsr1.getAttr<AttrType::attr5>() == acsr2.getAttr<AttrType::attr5>());
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
            expectEquals(acsr1.getAttr<AttrType::attr0>(), acsr2.getAttr<AttrType::attr0>());
            expectNotEquals(acsr1.getAttr<AttrType::attr1>(), acsr2.getAttr<AttrType::attr1>());
            expectEquals(acsr1.getAttr<AttrType::attr2>(), acsr2.getAttr<AttrType::attr2>());
            expect(acsr1.getAttr<AttrType::attr3>() != acsr2.getAttr<AttrType::attr3>());
            expectNotEquals(acsr1.getAttr<AttrType::attr4>(), acsr2.getAttr<AttrType::attr4>());
            expect(acsr1.getAttr<AttrType::attr5>() == acsr2.getAttr<AttrType::attr5>());
        }

        beginTest("accessor json");
        {
            ModelAcsr acsr1;
            expectEquals(acsr1.toJson().dump(), nlohmann::json::string_t("{\"attr0\":0,\"attr2\":0.0,\"attr5\":[],\"attr6\":null}"));
            acsr1.fromJson("{\"attr0\":1,\"attr2\":3.0,\"attr5\":[7.0,8.0],\"attr6\":null}"_json, NotificationType::synchronous);
            expectEquals(acsr1.toJson().dump(), nlohmann::json::string_t("{\"attr0\":1,\"attr2\":3.0,\"attr5\":[7.0,8.0],\"attr6\":null}"));
        }

        beginTest("is equivalent");
        {
            ModelCtnr ctnr1;
            ModelAcsr acsr1(ctnr1);
            ModelCtnr ctnr2({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            ModelAcsr acsr2(ctnr2);
            expect(acsr1.isEquivalentTo(acsr2) == false);
            expect(acsr2.isEquivalentTo(acsr1) == false);
            acsr1.copyFrom(acsr2, NotificationType::synchronous);
            expect(acsr1.isEquivalentTo(acsr2) == true);
            expect(acsr2.isEquivalentTo(acsr1) == true);
        }

        beginTest("accessor listener");
        {
            std::array<bool, magic_enum::enum_count<AttrType>()> notifications;

            ModelLtnr ltnr{"Listener"};
            ltnr.onAttrChanged = [&](ModelAcsr const& acsr, AttrType attribute)
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
            acsr.setAttr<AttrType::attr0>(2, NotificationType::synchronous);
            acsr.setAttr<AttrType::attr1>(3, NotificationType::synchronous);
            acsr.setAttr<AttrType::attr2>(4.0f, NotificationType::synchronous);
            acsr.setAttr<AttrType::attr3>({5, 6, 7}, NotificationType::synchronous);
            acsr.setAttr<AttrType::attr4>("Jim", NotificationType::synchronous);
            acsr.setAttr<AttrType::attr5>({7.0, 8.0}, NotificationType::synchronous);
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

MISC_FILE_END
