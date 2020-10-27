#pragma once

#include "AnlListenerList.h"
#include "AnlStringParser.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
    template<class Owner, class Model, class Attribute> class ModelAccessor
    {
    public:
        
        ModelAccessor(Model& model) : mModel(model)
        {
        }
        
        virtual ~ModelAccessor() = default;
        
        Model const& getModel() const
        {
            return mModel;
        }
        
        virtual void fromModel(Model const& model, juce::NotificationType const notification) = 0;
        
        void fromXml(juce::XmlElement const& xml, Model model, juce::NotificationType const notification)
        {
            fromModel(Model::fromXml(xml, model), notification);
        }
        
        class Listener
        {
        public:
            Listener() = default;
            virtual ~Listener() = default;
            
            std::function<void(Owner& controller, Attribute A)> onChanged = nullptr;
        };
        
        virtual void addListener(Listener& listener, juce::NotificationType const notification)
        {
            if(mListeners.add(listener))
            {
                notifyListener(Model::getAttributeTypes(), {&listener}, notification);
            }
        }
        
        void removeListener(Listener& listener)
        {
            mListeners.remove(listener);
        }
        
    protected:
        
        void notifyListener(std::set<Attribute> const& attrs, std::set<Listener*> const& listeners, juce::NotificationType const notification)
        {
            for(auto const attr : attrs)
            {
                mListeners.notify([=](Listener& listener)
                {
                    anlWeakAssert(listener.onChanged != nullptr);
                    if((listeners.empty() || listeners.count(&listener) > 0) && listener.onChanged != nullptr)
                    {
                        listener.onChanged(*static_cast<Owner*>(this), attr);
                    }
                }, notification);
            }
        }
        
        Model& mModel;
        
    protected:

        template<typename T> static void compareAndSet(std::set<Attribute>& attrs, Attribute attr, T& currentValue, T const& newValue)
        {
            if(currentValue != newValue)
            {
                attrs.insert(attr);
                currentValue = newValue;
            }
        }
        
        template<typename A, typename T> static void compareAndSet(std::set<Attribute>& attrs, Attribute attr, std::vector<std::unique_ptr<A>>& accessors, std::vector<std::unique_ptr<T>>& currentValues, std::vector<std::unique_ptr<T>> const& newValues, juce::NotificationType const notification)
        {
            // Updates the values and sanitize the accessors and the models
            for(size_t i = 0; i < std::min(accessors.size(), newValues.size()); ++i)
            {
                anlStrongAssert(currentValues[i] != nullptr);
                if(currentValues[i] == nullptr)
                {
                    currentValues[i] = std::make_unique<T>();
                }
                anlStrongAssert(accessors[i] != nullptr);
                if(accessors[i] == nullptr && currentValues[i] != nullptr)
                {
                    accessors[i] = std::make_unique<A>(*(currentValues[i]).get());
                }
                else if(accessors[i] != nullptr && currentValues[i] == nullptr)
                {
                    accessors[i] = nullptr;
                }
                anlStrongAssert(newValues[i] != nullptr);
                if(newValues[i] != nullptr && accessors[i] != nullptr)
                {
                    accessors[i]->fromModel(*(newValues[i].get()), notification);
                }
            }
            
            // Creates and removes the accessors and the models
            if(currentValues.size() != newValues.size())
            {
                attrs.insert(attr);
                for(auto i = currentValues.size(); i < newValues.size(); ++i)
                {
                    anlStrongAssert(newValues[i] != nullptr);
                    currentValues.push_back(newValues[i] != nullptr ? std::make_unique<T>(*(newValues[i].get())) : std::make_unique<T>());
                }
        
                for(auto i = accessors.size(); i < currentValues.size(); ++i)
                {
                    anlStrongAssert(currentValues[i] != nullptr);
                    accessors.push_back(currentValues[i] != nullptr ? std::make_unique<A>(*(currentValues[i].get())) : std::unique_ptr<A>());
                }
                accessors.resize(newValues.size());
                currentValues.resize(newValues.size());
            }
        }
        
    private:
        
        ListenerList<Listener> mListeners;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelAccessor)
    };
}

#define MODEL_ACCESSOR_COMPARE_AND_SET(attr, changed) \
compareAndSet(changed, Attribute::attr, mModel.attr, model.attr)

#define MODEL_ACCESSOR_COMPARE_AND_SET_VECTOR(attr, changed, accessors) \
compareAndSet(changed, Attribute::attr, accessors, mModel.attr, model.attr, notification)

ANALYSE_FILE_END
