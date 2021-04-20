#pragma once

#include "AnlBasics.h"

ANALYSE_FILE_BEGIN

template <class T>
class AtomicManager
{
public:
    AtomicManager() = default;

    ~AtomicManager()
    {
        setInstance({});
    }

    std::shared_ptr<T> getInstance()
    {
#if __cpp_lib_atomic_shared_ptr
        return mInstance.load();
#else
        return std::atomic_load(&mInstance);
#endif
    }

    std::shared_ptr<const T> getInstance() const
    {
#if __cpp_lib_atomic_shared_ptr
        return mInstance.load();
#else
        return std::atomic_load(&mInstance);
#endif
    }

    void setInstance(std::shared_ptr<T> instance)
    {
        anlStrongAssert(instance == nullptr || instance != getInstance());
        if(instance != getInstance())
        {
#if __cpp_lib_atomic_shared_ptr
            deleteInstance(mInstance.exchange(instance));
#else
            deleteInstance(std::atomic_exchange(&mInstance, instance));
#endif
        }
    }

private:
    static void deleteInstance(std::shared_ptr<T> instance)
    {
        if(instance != nullptr && instance.use_count() > 1)
        {
            juce::MessageManager::callAsync([instance = std::move(instance)]() mutable
                                            {
                                                deleteInstance(std::move(instance));
                                                anlStrongAssert(instance == nullptr);
                                            });
            anlStrongAssert(instance == nullptr);
        }
    }

#if __cpp_lib_atomic_shared_ptr
    std::atomic<std::shared_ptr<T>> mInstance{nullptr};
#else
    std::shared_ptr<T> mInstance{nullptr};
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AtomicManager)
};
ANALYSE_FILE_END
