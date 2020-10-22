#pragma once

#include "AnlMisc.h"

ANALYSE_FILE_BEGIN

namespace Tools
{
    template <class T> class AtomicManager
    {
    public:
        
        ~AtomicManager()
        {
            setInstance({});
        }
        
        std::shared_ptr<T> getInstance()
        {
            return std::atomic_load(&mInstance);
        }
        
        std::shared_ptr<T> const getInstance() const
        {
            return std::atomic_load(&mInstance);
        }
        
        void setInstance(std::shared_ptr<T> instance)
        {
            anlStrongAssert(instance == nullptr || instance != getInstance());
            if(instance != getInstance())
            {
                deleteInstance(std::atomic_exchange(&mInstance, instance));
            }
        }
        
    private:
        
        static void deleteInstance(std::shared_ptr<T> instance)
        {
            if(instance != nullptr && instance.use_count() > 1)
            {
                juce::MessageManager::callAsync([ptr = std::move(instance)]() mutable
                {
                    deleteInstance(std::move(ptr));
                    anlStrongAssert(ptr == nullptr);
                });
                anlStrongAssert(instance == nullptr);
            }
        }
        
        std::shared_ptr<T> mInstance {nullptr};
    };
}

ANALYSE_FILE_END
