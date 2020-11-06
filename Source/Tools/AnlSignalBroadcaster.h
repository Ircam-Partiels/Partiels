#pragma once

#include "AnlNotifier.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
    template<class Owner, class Signal> class SignalBroadcaster
    {
    public:
        
        SignalBroadcaster() = default;
        virtual ~SignalBroadcaster() = default;
    
        class Receiver
        {
        public:
            Receiver() = default;
            virtual ~Receiver() = default;
            
            std::function<void(Owner const& controller, Signal signal, juce::var value)> onSignal = nullptr;
        };
        
        void addReceiver(Receiver& listener)
        {
            mReceivers.add(listener);
        }
        
        void removeReceiver(Receiver& listener)
        {
            mReceivers.remove(listener);
        }
        
        void sendSignal(Signal signal, juce::var value, NotificationType const notification)
        {
            mReceivers.notify([=](Receiver& listener)
            {
                anlWeakAssert(listener.onSignal != nullptr);
                if(listener.onSignal != nullptr)
                {
                    listener.onSignal(*static_cast<Owner*>(this), signal, value);
                }
            }, notification);
        }
        
    private:
        
        Notifier<Receiver> mReceivers;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SignalBroadcaster)
    };
}

ANALYSE_FILE_END
