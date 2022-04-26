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

ANALYSE_FILE_END
