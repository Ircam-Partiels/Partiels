#pragma once

#include "AnlMisc.h"

ANALYSE_FILE_BEGIN

class SharedMutexBase
{
public:
    
    void setMutex(std::mutex* mutex)
    {
        mSharedMutex = mutex;
    }
    
    std::mutex& getMutex()
    {
        return mSharedMutex == nullptr ? mMutex : *mSharedMutex;
    }
    
private:
    
    std::mutex mMutex;
    std::mutex* mSharedMutex = nullptr;
};

template<class listener_t>
class Notifier
: public SharedMutexBase
, private juce::AsyncUpdater
{
public:
    
    Notifier() = default;
    ~Notifier() override
    {
        anlStrongAssert(mListeners.empty());
    }
    
    bool add(listener_t& listener)
    {
        std::unique_lock<std::mutex> listenerLock(mListenerMutex);
        return mListeners.insert(&listener).second;
    }
    
    void remove(listener_t& listener)
    {
        std::unique_lock<std::mutex> listenerLock(mListenerMutex);
        mListeners.erase(&listener);
    }
    
    void notify(std::function<void(listener_t&)> method, NotificationType notification)
    {
        if(notification == NotificationType::asynchronous)
        {
            triggerAsyncUpdate();
            std::unique_lock<std::mutex> queueLock(mQueueMutex);
            mQueue.push(method);
        }
        else
        {
            std::unique_lock<std::mutex> listenerLock(mListenerMutex);
            for(auto* listener : mListeners)
            {
                std::unique_lock<std::mutex> notifyingLock(getMutex());
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
            
            notify(method, NotificationType::synchronous);
            
            queueLock.lock();
        }
    }
    
    std::queue<std::function<void(listener_t&)>> mQueue;
    std::mutex mQueueMutex;
    std::set<listener_t*> mListeners;
    std::mutex mListenerMutex;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Notifier)
};

ANALYSE_FILE_END
