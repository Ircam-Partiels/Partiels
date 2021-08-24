#include "AnlPluginListSearchPath.h"
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

PluginList::SearchPath::SearchPath(Accessor& accessor)
: FloatingWindowContainer("Plugin Search Path", *this, true)
, mAccessor(accessor)
{
    auto updateButtonsStates = [this]()
    {
        auto const useEnvVariable = mEnvVariableButton.getToggleState();
        auto const fileSearchPath = mFileSearchPathTable.getFileSearchPath();
        mApplyButton.setEnabled(useEnvVariable != mAccessor.getAttr<AttrType::useEnvVariable>() || fileSearchPath != mAccessor.getAttr<AttrType::searchPath>());
        mResetButton.setEnabled(mApplyButton.isEnabled());
        mDefaultButton.setEnabled(useEnvVariable != true || fileSearchPath != getDefaultSearchPath());
    };

    auto warnForChanges = []
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::InfoIcon)
                                 .withTitle(juce::translate("The plugin search path has been modified!"))
                                 .withMessage(juce::translate("The pugin search path has been modified. Restart the application in order to rescan the plugins."))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    };

    mFileSearchPathTable.onFileSearchPathChanged = [=]()
    {
        updateButtonsStates();
    };

    mApplyButton.onClick = [=, this]()
    {
        mAccessor.setAttr<AttrType::useEnvVariable>(mEnvVariableButton.getToggleState(), NotificationType::synchronous);
        mAccessor.setAttr<AttrType::searchPath>(mFileSearchPathTable.getFileSearchPath(), NotificationType::synchronous);
        updateButtonsStates();
        warnForChanges();
    };

    mResetButton.onClick = [=, this]()
    {
        mEnvVariableButton.setToggleState(mAccessor.getAttr<AttrType::useEnvVariable>(), juce::NotificationType::dontSendNotification);
        mFileSearchPathTable.setFileSearchPath(mAccessor.getAttr<AttrType::searchPath>(), juce::NotificationType::sendNotificationSync);
        updateButtonsStates();
    };

    mDefaultButton.onClick = [=, this]()
    {
        mEnvVariableButton.setToggleState(true, juce::NotificationType::dontSendNotification);
        mFileSearchPathTable.setFileSearchPath(getDefaultSearchPath(), juce::NotificationType::sendNotificationSync);
        updateButtonsStates();
    };

    mFloatingWindow.onCloseButtonPressed = [this]()
    {
        if(mFileSearchPathTable.getFileSearchPath() != mAccessor.getAttr<AttrType::searchPath>() || mEnvVariableButton.getToggleState() != mAccessor.getAttr<AttrType::useEnvVariable>())
        {
            auto const options = juce::MessageBoxOptions()
                                     .withIconType(juce::AlertWindow::QuestionIcon)
                                     .withTitle(juce::translate("Apply plugin search path modification?"))
                                     .withMessage(juce::translate("The pugin search path has been modified but the changes were not applied. Would you like to apply the changes or to discard the changes?"))
                                     .withButton(juce::translate("Apply"))
                                     .withButton(juce::translate("Discard"));

            juce::WeakReference<juce::Component> safePointer(this);
            juce::AlertWindow::showAsync(options, [=, this](int windowResult)
                                         {
                                             if(safePointer.get() == nullptr)
                                             {
                                                 return;
                                             }
                                             if(windowResult == 1)
                                             {
                                                 mApplyButton.onClick();
                                             }
                                             else
                                             {
                                                 mResetButton.onClick();
                                             }
                                         });
        }
        return true;
    };

    mEnvVariableInfo.setText(juce::translate("Env. variable"), juce::NotificationType::dontSendNotification);
    mEnvVariableInfo.setTooltip(juce::translate("Toggle the use of the VAMP_PATH environment variable"));
    mEnvVariableInfo.setEditable(false);
    mEnvVariableButton.setTooltip(juce::translate("Toggle the use of the VAMP_PATH environment variable"));
    mEnvVariableButton.onClick = [=]()
    {
        updateButtonsStates();
    };

    mListener.onAttrChanged = [=, this](Accessor const& acsr, AttrType attribute)
    {
        switch(attribute)
        {
            case AttrType::searchPath:
            {
                mFileSearchPathTable.setFileSearchPath(acsr.getAttr<AttrType::searchPath>(), juce::NotificationType::dontSendNotification);
                updateButtonsStates();
            }
            break;
            case AttrType::useEnvVariable:
            {
                mEnvVariableButton.setToggleState(acsr.getAttr<AttrType::useEnvVariable>(), juce::NotificationType::dontSendNotification);
                updateButtonsStates();
            }
            break;
            case AttrType::sortColumn:
            case AttrType::sortIsFowards:
                break;
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
    updateButtonsStates();

    setWantsKeyboardFocus(true);
    addAndMakeVisible(mFileSearchPathTable);
    addAndMakeVisible(mSeparator);
    addAndMakeVisible(mApplyButton);
    addAndMakeVisible(mResetButton);
    addAndMakeVisible(mDefaultButton);
    addAndMakeVisible(mEnvVariableInfo);
    addAndMakeVisible(mEnvVariableButton);
    setSize(405, 200);
}

PluginList::SearchPath::~SearchPath()
{
    mAccessor.removeListener(mListener);
}

void PluginList::SearchPath::resized()
{
    auto bounds = getLocalBounds();
    {
        auto bottomBounds = bounds.removeFromBottom(30).withSizeKeepingCentre(403, 24);
        mApplyButton.setBounds(bottomBounds.removeFromLeft(100));
        bottomBounds.removeFromLeft(1);
        mResetButton.setBounds(bottomBounds.removeFromLeft(100));
        bottomBounds.removeFromLeft(1);
        mDefaultButton.setBounds(bottomBounds.removeFromLeft(100));
        bottomBounds.removeFromLeft(1);
        mEnvVariableButton.setBounds(bottomBounds.removeFromLeft(24).reduced(4));
        mEnvVariableInfo.setBounds(bottomBounds);
    }
    mSeparator.setBounds(bounds.removeFromBottom(1));
    mFileSearchPathTable.setBounds(bounds);
}

ANALYSE_FILE_END
