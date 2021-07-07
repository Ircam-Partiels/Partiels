#include "AnlDocumentReaderLayoutPanel.h"

ANALYSE_FILE_BEGIN

Document::ReaderLayoutPanel::ReaderLayoutPanel(Director& director)
: FloatingWindowContainer("Audio Reader Layout", *this)
, mDirector(director)
{
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::reader:
            {
                auto const layout = acsr.getAttr<AttrType::reader>();
                mAudioFileLayoutTable.setLayout(layout, juce::NotificationType::sendNotificationSync);
            }
            break;
            case AttrType::layout:
            case AttrType::viewport:
            case AttrType::path:
            case AttrType::grid:
                break;
        }
    };

    mAudioFileLayoutTable.onLayoutChanged = [this]()
    {
        auto const layout = mAudioFileLayoutTable.getLayout();
        mAudioFileLayoutTable.onSelectionChanged();
        mApplyButton.setEnabled(layout != mAccessor.getAttr<AttrType::reader>());
        mResetButton.setEnabled(mApplyButton.isEnabled());
    };

    mAudioFileLayoutTable.onSelectionChanged = [this]()
    {
        auto const selection = mAudioFileLayoutTable.getSelection();
        auto const layout = mAudioFileLayoutTable.getLayout();
        auto const fileLayout = layout.empty() ? AudioFileLayout{} : ((selection.empty() || *selection.cbegin() >= layout.size()) ? layout.back() : layout[*selection.cbegin()]);
        if(fileLayout.file != mFileInfoPanel.getFile())
        {
            auto reader = std::unique_ptr<juce::AudioFormatReader>(mDirector.getAudioFormatManager().createReaderFor(fileLayout.file));
            mFileInfoPanel.setAudioFormatReader(fileLayout.file, reader.get());
        }
    };

    mApplyButton.onClick = [this]()
    {
        mDirector.startAction();
        mAccessor.setAttr<AttrType::reader>(mAudioFileLayoutTable.getLayout(), NotificationType::synchronous);
        mDirector.endAction(ActionState::newTransaction, juce::translate("Change Audio Reader Layout"));
        mAudioFileLayoutTable.onLayoutChanged();
    };

    mResetButton.onClick = [this]()
    {
        mAudioFileLayoutTable.setLayout(mAccessor.getAttr<AttrType::reader>(), juce::NotificationType::sendNotificationSync);
        mAudioFileLayoutTable.onLayoutChanged();
    };

    mFloatingWindow.onCloseButtonPressed = [this]()
    {
        if(mApplyButton.isEnabled())
        {
            if(AlertWindow::showOkCancel(AlertWindow::MessageType::question, "Apply audio reader modification?", "The audio reader layout has been modified but the changes were not applied. Would you like to apply the changes or to discard the changes?"))
            {
                mApplyButton.onClick();
            }
            else
            {
                mResetButton.onClick();
            }
        }
        return true;
    };

    addAndMakeVisible(mAudioFileLayoutTable);
    addAndMakeVisible(mSeparator);
    addAndMakeVisible(mApplyButton);
    addAndMakeVisible(mResetButton);
    addAndMakeVisible(mInfoSeparator);
    addAndMakeVisible(mFileInfoPanel);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    setSize(300, 400);
}

Document::ReaderLayoutPanel::~ReaderLayoutPanel()
{
    mAccessor.removeListener(mListener);
}

void Document::ReaderLayoutPanel::resized()
{
    auto bounds = getLocalBounds();
    {
        auto applyResetBounds = bounds.removeFromBottom(30).withSizeKeepingCentre(201, 24);
        mApplyButton.setBounds(applyResetBounds.removeFromLeft(100));
        applyResetBounds.removeFromLeft(1);
        mResetButton.setBounds(applyResetBounds);
    }
    mSeparator.setBounds(bounds.removeFromBottom(1));
    mFileInfoPanel.setBounds(bounds.removeFromBottom(168));
    mInfoSeparator.setBounds(bounds.removeFromBottom(1));
    mAudioFileLayoutTable.setBounds(bounds);
}

ANALYSE_FILE_END
