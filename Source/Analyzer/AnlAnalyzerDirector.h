#pragma once

#include "AnlAnalyzerModel.h"

ANALYSE_FILE_BEGIN

namespace Analyzer
{
    class Director
    : public Accessor::Sanitizer
    {
    public:
        Director(Accessor& accessor);
        ~Director() override;
        
        // Accessor::Sanitizer
        void updated(Accessor& accessor, AttrType type, NotificationType notification) override;
        
    private:
        
        Accessor& mAccessor;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Director)
    };
}

ANALYSE_FILE_END
