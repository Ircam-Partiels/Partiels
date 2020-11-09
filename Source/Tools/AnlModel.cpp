#include "AnlModel.h"

ANALYSE_FILE_BEGIN

template <> unsigned long Anl::Model::StringParser::fromString(juce::String const& string)
{
    return std::stoul(string.toStdString());
}

template <> juce::String Anl::Model::StringParser::toString(unsigned long const& value)
{
    return juce::String(std::to_string(value));
}

template <> juce::File Anl::Model::StringParser::fromString<juce::File>(juce::String const& string)
{
    return juce::File(string);
}

template <> juce::String Anl::Model::StringParser::toString<juce::File>(juce::File const& value)
{
    return value.getFullPathName();
}

template <> juce::Range<double> Anl::Model::StringParser::fromString<juce::Range<double>>(juce::String const& string)
{
    juce::StringArray stringArray;
    stringArray.addLines(string);
    anlStrongAssert(stringArray.size() == 2);
    auto const start = stringArray.size() >= 1 ? stringArray.getReference(0).getDoubleValue() : 0.0;
    auto const end = stringArray.size() >= 2 ? stringArray.getReference(1).getDoubleValue() : 0.0;
    return {start, end};
}

template <> juce::String Anl::Model::StringParser::toString<juce::Range<double>>(juce::Range<double> const& value)
{
    juce::StringArray stringArray;
    stringArray.ensureStorageAllocated(2);
    stringArray.add(juce::String(value.getStart()));
    stringArray.add(juce::String(value.getEnd()));
    return stringArray.joinIntoString("\n");
}

class ModelUnitTest
: public juce::UnitTest
{
public:
    
    ModelUnitTest() : juce::UnitTest("Model", "Model") {}
    
    ~ModelUnitTest() override = default;
    
    void runTest() override
    {
        struct DummyStringifier
        {
            DummyStringifier() = default;
            explicit DummyStringifier(juce::String const&) {}
            
            explicit operator juce::String() const { return ""; }
            bool operator==(DummyStringifier const&) { return true; }
            bool operator!=(DummyStringifier const&) { return false; }
        };
        
        using AttrFlag = Model::AttrFlag;
        
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
        using Ctnr = Model::Container
        < Model::Attr<AttrType::attr0, int, AttrFlag::all>
        , Model::Attr<AttrType::attr1, int, AttrFlag::notifying>
        , Model::Attr<AttrType::attr2, float, AttrFlag::saveable>
        , Model::Attr<AttrType::attr3, std::vector<int>, AttrFlag::ignored>
        , Model::Attr<AttrType::attr4, std::string, AttrFlag::notifying>
        , Model::Attr<AttrType::attr5, std::vector<double>, AttrFlag::saveable>
        , Model::Attr<AttrType::attr6, DummyStringifier, AttrFlag::all>
        >;
        
        // Declare the data model accessor
        class ModelAcsr
        : public Model::Accessor<ModelAcsr, Ctnr>
        {
            using Model::Accessor<ModelAcsr, Ctnr>::Accessor;
        };
        
        // Declare the data model listener
        using ModelLtnr = ModelAcsr::Listener;
        
        beginTest("attribute flags");
        {
            expect(magic_enum::enum_integer(AttrFlag::ignored) == 0);
            expect(magic_enum::enum_integer(AttrFlag::notifying) == 1);
            expect(magic_enum::enum_integer(AttrFlag::saveable) == 2);
            expect(magic_enum::enum_integer(AttrFlag::comparable) == 4);
            expect(magic_enum::enum_integer(AttrFlag::all) == 7);
        }
        
        beginTest("accessor default constructor");
        {
            ModelAcsr acsr;
            expectEquals(acsr.getValue<AttrType::attr0>(), 0);
            expectEquals(acsr.getValue<AttrType::attr1>(), 0);
            expectEquals(acsr.getValue<AttrType::attr2>(), 0.0f);
            expect(acsr.getValue<AttrType::attr3>() == std::vector<int>{});
            expectEquals(acsr.getValue<AttrType::attr4>(), std::string{});
            expect(acsr.getValue<AttrType::attr5>() == std::vector<double>{});
        }
        
        beginTest("accessor constructor with model");
        {
            ModelAcsr acsr({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            expectEquals(acsr.getValue<AttrType::attr0>(), 1);
            expectEquals(acsr.getValue<AttrType::attr1>(), 2);
            expectEquals(acsr.getValue<AttrType::attr2>(), 3.0f);
            expect(acsr.getValue<AttrType::attr3>() == std::vector<int>{4, 5, 6});
            expectEquals(acsr.getValue<AttrType::attr4>(), std::string{"Jules"});
            expect(acsr.getValue<AttrType::attr5>() == std::vector<double>{7.0, 8.0});
        }
        
        beginTest("accessor setting attribute");
        {
            ModelAcsr acsr({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
            acsr.setValue<AttrType::attr0>(2, NotificationType::synchronous);
            acsr.setValue<AttrType::attr1>(3, NotificationType::synchronous);
            acsr.setValue<AttrType::attr2>(4.0f, NotificationType::synchronous);
            acsr.setValue<AttrType::attr3>(std::vector<int>{5, 6, 7}, NotificationType::synchronous);
            acsr.setValue<AttrType::attr4>(std::string{"Jim"}, NotificationType::synchronous);
            acsr.setValue<AttrType::attr5>(std::vector<double>{8.0, 9.0}, NotificationType::synchronous);
            expectEquals(acsr.getValue<AttrType::attr0>(), 2);
            expectEquals(acsr.getValue<AttrType::attr1>(), 3);
            expectEquals(acsr.getValue<AttrType::attr2>(), 4.0f);
            expect(acsr.getValue<AttrType::attr3>() == std::vector<int>{5, 6, 7});
            expectEquals(acsr.getValue<AttrType::attr4>(), std::string{"Jim"});
            expect(acsr.getValue<AttrType::attr5>() == std::vector<double>{8.0, 9.0});
        }
        
        beginTest("accessor from model");
        {
            ModelAcsr acsr1;
            ModelAcsr acsr2({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
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
            ModelAcsr acsr1;
            ModelAcsr acsr2({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}, {{7.0, 8.0}}, {}});
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
            acsr.setValue<AttrType::attr3>(std::vector<int>{5, 6, 7}, NotificationType::synchronous);
            acsr.setValue<AttrType::attr4>("Jim", NotificationType::synchronous);
            acsr.setValue<AttrType::attr5>(std::vector<double>{7.0, 8.0}, NotificationType::synchronous);
            expect(notifications[magic_enum::enum_integer(AttrType::attr0)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr1)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr2)] == false);
            expect(notifications[magic_enum::enum_integer(AttrType::attr3)] == false);
            expect(notifications[magic_enum::enum_integer(AttrType::attr4)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr5)] == false);
        }
    }
};

static ModelUnitTest modelUnitTest;

ANALYSE_FILE_END
