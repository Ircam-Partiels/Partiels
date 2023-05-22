#include "AnlTrackHierarchyManager.h"

ANALYSE_FILE_BEGIN

void Track::HierarchyManager::addHierarchyListener(Listener& listener, NotificationType notification)
{
    if(mNotifier.add(listener))
    {
        mNotifier.notify([=, this](Listener& l)
                         {
                             if(std::addressof(listener) == std::addressof(l))
                             {
                                 if(listener.onHierarchyChanged != nullptr)
                                 {
                                     listener.onHierarchyChanged(*this);
                                 }
                             }
                         },
                         notification);
    }
}

void Track::HierarchyManager::removeHierarchyListener(Listener& listener)
{
    mNotifier.remove(listener);
}

void Track::HierarchyManager::notifyHierarchyChanged(NotificationType notification)
{
    mNotifier.notify([=, this](Listener& listener)
                     {
                         if(listener.onHierarchyChanged != nullptr)
                         {
                             listener.onHierarchyChanged(*this);
                         }
                     },
                     notification);
}

void Track::HierarchyManager::notifyResultsChanged(juce::String const& identifier, NotificationType notification)
{
    mNotifier.notify([=, this](Listener& listener)
                     {
                         if(listener.onResultsChanged != nullptr)
                         {
                             listener.onResultsChanged(*this, identifier);
                         }
                     },
                     notification);
}

ANALYSE_FILE_END
