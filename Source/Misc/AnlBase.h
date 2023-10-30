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

    void notifyListener(juce::ApplicationCommandManager& commandManager, juce::ApplicationCommandManagerListener& listener, std::vector<int> const& commandIds);
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

// clang-format off
enum ApplicationCommandIDs : int
{
      documentNew = 0x3001
    , documentOpen
    , documentSave
    , documentDuplicate
    , documentConsolidate
    , documentExport
    , documentImport
    , documentBatch
    
    , editUndo
    , editRedo
    , editSelectNextItem
    , editNewGroup
    , editNewTrack
    , editRemoveItem
    , editLoadTemplate
    
    , transportTogglePlayback
    , transportToggleLooping
    , transportToggleStopAtLoopEnd
    , transportToggleMagnetism
    , transportRewindPlayHead
    , transportMovePlayHeadBackward
    , transportMovePlayHeadForward
    
    , viewTimeZoomIn
    , viewTimeZoomOut
    , viewVerticalZoomIn
    , viewVerticalZoomOut
    , viewTimeZoomAnchorOnPlayhead
    , viewInfoBubble
    
    , helpOpenAudioSettings
    , helpOpenPluginSettings
    , helpOpenAbout
    , helpOpenProjectPage
    , helpAuthorize
    , helpSdifConverter
    , helpAutoUpdate
    , helpCheckForUpdate
};
// clang-format on

ANALYSE_FILE_END
