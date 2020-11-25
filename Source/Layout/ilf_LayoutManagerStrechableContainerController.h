
#pragma once

#include "ilf_LayoutManagerStrechableContainerModel.h"

ILF_WARNING_BEGIN
ILF_WARNING_IGNORE_NO_OUT_OF_LINE_METHOD_DEFINITION
ILF_NAMESPACE_BEGIN

namespace LayoutManager
{
    namespace Strechable
    {
        namespace Container
        {
            class Controller
            {
            public:
                Controller(Model& model);
                ~Controller() = default;
                
                Model const& getModel() const;
                
                std::vector<int> getSizes() const;
                void setSizes(std::vector<int> sizes, juce::NotificationType const notification);
                
                void fromModel(Model const& model, juce::NotificationType const notification);
                void fromXml(juce::XmlElement const& xml, juce::NotificationType const notification);
                std::unique_ptr<juce::XmlElement> toXml() const;
                
                //! @brief The listener
                class Listener
                {
                public:
                    Listener() = default;
                    virtual ~Listener() = default;
                    
                    enum class AttributeType
                    {
                        sizes
                    };
                    
                    virtual void layoutManagerStrechableContainerAttributeChanged(Controller* controller, AttributeType type, juce::var const& value) = 0;
                };
                
                void addListener(Listener* listener, juce::NotificationType const notification);
                void removeListener(Listener* listener);
            private:
                
                void notifyListeners(Listener::AttributeType type, juce::var const& value, juce::NotificationType const notification);
                
                Model& mModel;
                juce::ListenerList<Listener> mListeners;
                
                JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Controller)
            };
        }
    }
}

ILF_NAMESPACE_END
ILF_WARNING_END
