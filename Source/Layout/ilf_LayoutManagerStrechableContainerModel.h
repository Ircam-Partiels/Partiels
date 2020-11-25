
#pragma once

#include "../ilf_LayoutManagerDetachablePanelModel.h"

ILF_WARNING_BEGIN
ILF_NAMESPACE_BEGIN

namespace LayoutManager
{
    namespace Strechable
    {
        namespace Container
        {
            struct Model
            {
                std::vector<int> sizes; //!< The sizes of the contents
                
                static Model fromXml(juce::XmlElement const& xml);
                std::unique_ptr<juce::XmlElement> toXml() const;
                
                bool operator!=(Model const& other) const;
                bool operator==(Model const& other) const;
                
                JUCE_LEAK_DETECTOR(Model)
            };
        }
    }
}

ILF_NAMESPACE_END
ILF_WARNING_END


