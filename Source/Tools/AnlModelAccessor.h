#pragma once

#include "AnlListenerList.h"

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

    private:
        
        ListenerList<Listener> mListeners;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelAccessor)
    };
}

#define MODEL_ACCESSOR_COMPARE_AND_SET(attr, changed) \
if(mModel.attr != model.attr) { changed.insert(Attribute::attr); mModel.attr = model.attr; }


ANALYSE_FILE_END
