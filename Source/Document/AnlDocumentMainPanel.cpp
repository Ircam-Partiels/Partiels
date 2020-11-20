#include "AnlDocumentMainPanel.h"
#include "AnlDocumentAudioReader.h"
#include "../Plugin/AnlPluginListScanner.h"

ANALYSE_FILE_BEGIN

Document::MainPanel::MainPanel(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch (attribute)
        {
            case AttrType::analyzers:
            {
                auto const& anlAcsrs = mAccessor.getAccessors<AttrType::analyzers>();
                mSections.erase(std::remove_if(mSections.begin(), mSections.end(), [&](auto const& section)
                {
                    return std::none_of(anlAcsrs.cbegin(), anlAcsrs.cend(), [&](auto const anlAcsr)
                    {
                        return &(section->accessor) == &(anlAcsr.get());
                    });
                }), mSections.end());
                
                for(size_t i = mSections.size(); i < anlAcsrs.size(); ++i)
                {
                    auto section = std::make_unique<Section>(anlAcsrs[i], mAccessor.getAccessors<AttrType::timeZoom>()[0]);
                    anlWeakAssert(section != nullptr);
                    if(section != nullptr)
                    {
                        addAndMakeVisible(section->renderer);
                        addAndMakeVisible(section->separator);
                        mSections.push_back(std::move(section));
                    }
                }
                resized();
            }
                break;
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Document::MainPanel::~MainPanel()
{
    mAccessor.removeListener(mListener);
}

void Document::MainPanel::resized()
{
    auto constexpr size = 100;
    auto bounds = getLocalBounds();
    for(auto& section : mSections)
    {
        if(section != nullptr)
        {
            auto lbounds = bounds.removeFromTop(size);
            section->separator.setBounds(lbounds.removeFromBottom(2));
            section->renderer.setBounds(lbounds);
        }
    }
}

ANALYSE_FILE_END
