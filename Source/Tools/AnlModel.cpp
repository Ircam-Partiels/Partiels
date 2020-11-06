#include "AnlModel.h"

ANALYSE_FILE_BEGIN

class ModelUnitTest
: public juce::UnitTest
{
public:
    
    ModelUnitTest() : juce::UnitTest("Model Util Test", "Model") {}
    
    ~ModelUnitTest() override = default;
    
    void runTest() override
    {
        using AttrFlag = Anl::Model::AttrFlag;
        
        // Declare the name (type) of the attributes of the data model
        enum AttrType : size_t
        {
            attr0,
            attr1,
            attr2,
            attr3,
            attr4
        };
        
        // Declare the data model container
        using Ctnr = Anl::Model::Container
        <Anl::Model::Attr<AttrType::attr0, int, AttrFlag::all>
        ,Anl::Model::Attr<AttrType::attr1, int, AttrFlag::notifying>
        ,Anl::Model::Attr<AttrType::attr2, float, AttrFlag::saveable>
        ,Anl::Model::Attr<AttrType::attr3, std::vector<int>, AttrFlag::ignored>
        ,Anl::Model::Attr<AttrType::attr4, std::string, AttrFlag::notifying>
        >;
        
        // Declare the data model accessor
        using ModelAcsr = Anl::Model::Accessor<Ctnr>;
        // Declare the data model listener
        using ModelLtnr = ModelAcsr::Listener;
        
        beginTest("Test attribute flags");
        {
            expect(magic_enum::enum_integer(AttrFlag::ignored) == 0);
            expect(magic_enum::enum_integer(AttrFlag::notifying) == 1);
            expect(magic_enum::enum_integer(AttrFlag::saveable) == 2);
            expect(magic_enum::enum_integer(AttrFlag::comparable) == 4);
            expect(magic_enum::enum_integer(AttrFlag::all) == 7);
        }
        
        beginTest("Test accessor default constructor");
        {
            ModelAcsr acsr;
            expectEquals(acsr.getValue<AttrType::attr0>(), 0);
            expectEquals(acsr.getValue<AttrType::attr1>(), 0);
            expectEquals(acsr.getValue<AttrType::attr2>(), 0.0f);
            expect(acsr.getValue<AttrType::attr3>() == std::vector<int>{});
            expectEquals(acsr.getValue<AttrType::attr4>(), std::string{});
        }
        
        beginTest("Test accessor constructor with model");
        {
            ModelAcsr acsr({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}});
            expectEquals(acsr.getValue<AttrType::attr0>(), 1);
            expectEquals(acsr.getValue<AttrType::attr1>(), 2);
            expectEquals(acsr.getValue<AttrType::attr2>(), 3.0f);
            expect(acsr.getValue<AttrType::attr3>() == std::vector<int>{4, 5, 6});
            expectEquals(acsr.getValue<AttrType::attr4>(), std::string{"Jules"});
        }
        
        beginTest("Test accessor setting attribute");
        {
            ModelAcsr acsr({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}});
            acsr.setValue<AttrType::attr0>(1, NotificationType::synchronous);
            acsr.setValue<AttrType::attr1>(2, NotificationType::synchronous);
            acsr.setValue<AttrType::attr2>(3.0f, NotificationType::synchronous);
            acsr.setValue<AttrType::attr3>(std::vector<int>{4, 5, 6}, NotificationType::synchronous);
            acsr.setValue<AttrType::attr4>(std::string{"Jim"}, NotificationType::synchronous);
            expectEquals(acsr.getValue<AttrType::attr0>(), 1);
            expectEquals(acsr.getValue<AttrType::attr1>(), 2);
            expectEquals(acsr.getValue<AttrType::attr2>(), 3.0f);
            expect(acsr.getValue<AttrType::attr3>() == std::vector<int>{4, 5, 6});
            expectEquals(acsr.getValue<AttrType::attr4>(), std::string{"Jim"});
        }
        
        beginTest("Test accessor from model");
        {
            ModelAcsr acsr1;
            ModelAcsr acsr2({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}});
            acsr1.fromModel(acsr2.getModel(), NotificationType::synchronous);
            expectEquals(acsr1.getValue<AttrType::attr0>(), acsr2.getValue<AttrType::attr0>());
            expectNotEquals(acsr1.getValue<AttrType::attr1>(), acsr2.getValue<AttrType::attr1>());
            expectEquals(acsr1.getValue<AttrType::attr2>(), acsr2.getValue<AttrType::attr2>());
            expect(acsr1.getValue<AttrType::attr3>() != acsr2.getValue<AttrType::attr3>());
            expectNotEquals(acsr1.getValue<AttrType::attr4>(), acsr2.getValue<AttrType::attr4>());
        }
        
        beginTest("Test accessor from/to xml");
        {
            ModelAcsr acsr1;
            ModelAcsr acsr2({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}});
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
        }
        
        beginTest("Test accessor listener");
        {
            std::array<bool, magic_enum::enum_count<AttrType>()> notifications;
            
            ModelLtnr ltnr;
            ltnr.onChanged = [&](ModelAcsr const& acsr, AttrType attribute)
            {
                juce::ignoreUnused(acsr);
                notifications[magic_enum::enum_integer(attribute)] = true;
            };
            
            ModelAcsr acsr({{1}, {2}, {3.0f}, {{4, 5, 6}}, {"Jules"}});
            
            std::fill(notifications.begin(), notifications.end(), false);
            acsr.addListener(ltnr, NotificationType::synchronous);
            expect(notifications[magic_enum::enum_integer(AttrType::attr0)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr1)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr2)] == false);
            expect(notifications[magic_enum::enum_integer(AttrType::attr3)] == false);
            expect(notifications[magic_enum::enum_integer(AttrType::attr4)] == true);
            
            std::fill(notifications.begin(), notifications.end(), false);
            acsr.setValue<AttrType::attr0>(2, NotificationType::synchronous);
            acsr.setValue<AttrType::attr1>(3, NotificationType::synchronous);
            acsr.setValue<AttrType::attr2>(4.0f, NotificationType::synchronous);
            acsr.setValue<AttrType::attr3>(std::vector<int>{5, 6, 7}, NotificationType::synchronous);
            acsr.setValue<AttrType::attr4>("Jim", NotificationType::synchronous);
            expect(notifications[magic_enum::enum_integer(AttrType::attr0)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr1)] == true);
            expect(notifications[magic_enum::enum_integer(AttrType::attr2)] == false);
            expect(notifications[magic_enum::enum_integer(AttrType::attr3)] == false);
            expect(notifications[magic_enum::enum_integer(AttrType::attr4)] == true);
        }
//
//        ModelAcsr acsr1({{1}, {2}, {3.0f}, {{1, 2, 3}}, {"john"}});
//
//
//        ModelLtnr ltnr;
//        ltnr.onChanged = [&](ModelAcsr const& acsr, AttrType attribute)
//        {
//            std::cout << (&acsr == &acsr1 ? "acsr1" : "acsr2") << " changed "<< magic_enum::enum_name(attribute) << ": ";
//            switch(attribute)
//            {
//                case AttrType::attr0:
//                    std::cout << acsr.getValue<AttrType::attr0>() << "\n";
//                    break;
//                case AttrType::attr1:
//                    std::cout << acsr.getValue<AttrType::attr1>() << "\n";
//                    break;
//                case AttrType::attr2:
//                    std::cout << acsr.getValue<AttrType::attr2>() << "\n";
//                    break;
//                case AttrType::attr3:
//                {
//                    for(auto const& v : acsr.getValue<AttrType::attr3>())
//                    {
//                        std::cout << v << " ";
//                    }
//                    std::cout << "\n";
//                }
//                    break;
//                case AttrType::attr4:
//                    std::cout << acsr.getValue<AttrType::attr4>() << "\n";
//                    break;
//            }
//        };
//        acsr1.addListener(ltnr, NotificationType::synchronous);
//        acsr2.addListener(ltnr, NotificationType::synchronous);
//
//        auto xml = acsr1.toXml("state1");
//        std::cout << "acsr1: "<< xml->toString() << "\n";
//        acsr1.setValue<AttrType::attr0>(acsr1.getValue<AttrType::attr1>() + 2);
//        acsr1.setValue<AttrType::attr2>(acsr1.getValue<AttrType::attr2>() - 2.2f);
//        xml = acsr1.toXml("state2");
//        std::cout << "acsr1: "<< xml->toString() << "\n";
//        acsr2.fromXml(*xml.get(), "state2");
//        xml = acsr2.toXml("state3");
//        std::cout << "acsr2: "<< xml->toString() << "\n";
//        acsr2.setValue<AttrType::attr0>(acsr2.getValue<AttrType::attr0>() + 2);
//        acsr2.setValue<AttrType::attr2>(acsr2.getValue<AttrType::attr2>() - 2.2f);
//        acsr1.fromModel(acsr2.getModel());
//        xml = acsr1.toXml("state4");
//        std::cout << "acsr1: "<< xml->toString() << "\n";
    }
};

static ModelUnitTest fadeScaleMapperTests;

ANALYSE_FILE_END
