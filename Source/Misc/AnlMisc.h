#pragma once

#include "../../Dependencies/Misc/Source/Misc.h"
namespace Anl = Misc;

#define ANALYSE_FILE_BEGIN \
    namespace Misc         \
    {
#define ANALYSE_FILE_END }

#define anlWeakAssert MiscWeakAssert
#define anlStrongAssert MiscStrongAssert

#define anlDebug MiscDebug
#define anlError MiscError

ANALYSE_FILE_BEGIN

namespace Utils
{
    template <class TargetClass>
    TargetClass* findComponentOfClass(juce::Component* component)
    {
        if(component == nullptr)
        {
            return nullptr;
        }
        if(auto* target = dynamic_cast<TargetClass*>(component))
        {
            return target;
        }
        return component->findParentComponentOfClass<TargetClass>();
    }
} // namespace Utils

// https://timur.audio/using-locks-in-real-time-audio-processing-safely
struct audio_spin_mutex
{
    void lock() noexcept
    {
        if(!try_lock())
        {
            for(int i = 20; i >= 0; --i)
            {
                if(try_lock())
                {
                    return;
                }
            }

            while(!try_lock())
            {
                std::this_thread::yield();
            }
        }
    }

    bool try_lock() noexcept
    {
        return !flag.test_and_set(std::memory_order_acquire);
    }

    void unlock() noexcept
    {
        flag.clear(std::memory_order_release);
    }

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

ANALYSE_FILE_END
