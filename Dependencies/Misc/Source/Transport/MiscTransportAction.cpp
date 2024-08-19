#include "MiscTransportAction.h"

MISC_FILE_BEGIN

Transport::Action::Restorer::Restorer(std::function<Accessor&(void)> accessorGetter, double newPlayhead, juce::Range<double> const& newSelection)
: mAccessorGetter(accessorGetter)
, mSavedPlayhead(mAccessorGetter().getAttr<AttrType::startPlayhead>())
, mSavedSelection(mAccessorGetter().getAttr<AttrType::selection>())
, mNewPlayhead(newPlayhead)
, mNewSelection(newSelection)
{
}

bool Transport::Action::Restorer::perform()
{
    auto& accessor = mAccessorGetter();
    accessor.setAttr<AttrType::startPlayhead>(mNewPlayhead, NotificationType::synchronous);
    accessor.setAttr<AttrType::selection>(mNewSelection, NotificationType::synchronous);
    return true;
}

bool Transport::Action::Restorer::undo()
{
    auto& accessor = mAccessorGetter();
    accessor.setAttr<AttrType::startPlayhead>(mSavedPlayhead, NotificationType::synchronous);
    accessor.setAttr<AttrType::selection>(mSavedSelection, NotificationType::synchronous);
    return true;
}

MISC_FILE_END
