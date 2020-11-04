//#include "AnlModel.h"
//
//ANALYSE_FILE_BEGIN
//
//template<class data_t> Model::Accessor<data_t>::Accessor(data_t&& data)
//: mData(std::move(data))
//{
//}
//
//template<class data_t> std::unique_ptr<juce::XmlElement> Model::Accessor<data_t>::toXml(juce::StringRef const& name) const
//{
//    auto xml = std::make_unique<juce::XmlElement>(name);
//    anlWeakAssert(xml != nullptr);
//    if(xml == nullptr)
//    {
//        return nullptr;
//    }
//    
//    detail::for_each(mData, [&](auto const& d)
//    {
//        if(d.saved)
//        {
//            xml->setAttribute(d.name.c_str(), d.value);
//        }
//    });
//    return xml;
//}
//
//template<class data_t> data_t Model::Accessor<data_t>::fromXml(juce::XmlElement const& xml, juce::StringRef const& name, data_t model)
//{
//    anlWeakAssert(xml.hasTagName(name));
//    if(!xml.hasTagName(name))
//    {
//        return {};
//    }
//
//    detail::for_each(model.mData, [&](auto& d)
//    {
//        if(d.saved)
//        {
//            anlWeakAssert(xml.hasAttribute(d.name.c_str()));
//            d.value = xml.getIntAttribute(d.name.c_str(), d.value);
//        }
//    });
//
//    return std::move(model);
//}
//
//ANALYSE_FILE_END
