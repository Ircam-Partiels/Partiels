#include "AnlAnalyzerDirector.h"
#include "AnlAnalyzerProcessor.h"

ANALYSE_FILE_BEGIN

Analyzer::Director::Director(Accessor& accessor)
: mAccessor(accessor)
{
    mAccessor.setSanitizer(this);
}

Analyzer::Director::~Director()
{
    mAccessor.setSanitizer(nullptr);
}

void Analyzer::Director::updated(Accessor& accessor, AttrType type, NotificationType notification)
{
    
}

ANALYSE_FILE_END
