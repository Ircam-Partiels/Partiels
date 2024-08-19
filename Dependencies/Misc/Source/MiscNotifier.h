#pragma once

#include "MiscBasics.h"

MISC_FILE_BEGIN

template <class listener_t>
class Notifier
: private juce::AsyncUpdater
{
public:
    Notifier() = default;
    ~Notifier() override
    {
        MiscWeakAssert(mListeners.empty());
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
            std::unique_lock<std::mutex> queueLock(mQueueMutex);
            mQueue.push(method);
            queueLock.unlock();
            triggerAsyncUpdate();
        }
        else
        {
            std::unique_lock<std::mutex> listenerLock(mListenerMutex);
            auto const listeners = mListeners;
            listenerLock.unlock();
            for(auto* listener : listeners)
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

MISC_FILE_END
