#pragma once

#include "AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
    template<class ListenerClass> class ListenerList
    : private juce::AsyncUpdater
    {
    public:
        
        ListenerList() = default;
        ~ListenerList() override = default;
        
        bool add(ListenerClass& listener)
        {
            std::unique_lock<std::mutex> listenerLock(mListenerMutex);
            return mListeners.insert(&listener).second;
        }
        
        void remove(ListenerClass& listener)
        {
            std::unique_lock<std::mutex> listenerLock(mListenerMutex);
            mListeners.erase(&listener);
        }
        
        void notify(std::function<void(ListenerClass&)> method, juce::NotificationType notification)
        {
            if(notification == juce::NotificationType::sendNotificationAsync)
            {
                triggerAsyncUpdate();
                std::unique_lock<std::mutex> queueLock(mQueueMutex);
                mQueue.push(method);
            }
            else if(notification != juce::NotificationType::dontSendNotification)
            {
                std::unique_lock<std::mutex> listenerLock(mListenerMutex);
                for(auto* listener : mListeners)
                {
                    method(*listener);
                }
            }
        }
        
    private:
        
        // juce::AsyncUpdater
        void handleAsyncUpdate() override
        {
            std::unique_lock<std::mutex> queueLock(mQueueMutex);
            while(!mQueue.empty())
            {
                auto const method = mQueue.front();
                mQueue.pop();
                queueLock.unlock();
                
                notify(method, juce::NotificationType::sendNotificationAsync);
                
                queueLock.lock();
            }
        }
        
        std::queue<std::function<void(ListenerClass&)>> mQueue;
        std::mutex mQueueMutex;
        std::set<ListenerClass*> mListeners;
        std::mutex mListenerMutex;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ListenerList)
    };
}

ANALYSE_FILE_END
