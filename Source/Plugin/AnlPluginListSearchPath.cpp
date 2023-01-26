#include "AnlPluginListSearchPath.h"
#include <vamp-hostsdk/PluginHostAdapter.h>

ANALYSE_FILE_BEGIN

PluginList::SearchPath::WindowContainer::WindowContainer(SearchPath& searchPath)
: FloatingWindowContainer(juce::translate("Plugin Settings"), searchPath, true)
, mSearchPath(searchPath)
, mTooltip(&mSearchPath)
{
    mFloatingWindow.onCloseButtonPressed = [this]()
    {
        mSearchPath.warnBeforeClosing();
        return true;
    };
}

PluginList::SearchPath::WindowContainer::~WindowContainer()
{
    mFloatingWindow.onCloseButtonPressed = nullptr;
}

PluginList::SearchPath::SearchPath(Accessor& accessor)
: mAccessor(accessor)
{
    auto updateButtonsStates = [this]()
    {
#if JUCE_MAC
        auto const fileSearchPath = mFileSearchPathTable.getFileSearchPath();
        mApplyButton.setEnabled(mUseEnvVariable != mAccessor.getAttr<AttrType::useEnvVariable>() || mQuarantineMode != mAccessor.getAttr<AttrType::quarantineMode>() || fileSearchPath != mAccessor.getAttr<AttrType::searchPath>());
        mResetButton.setEnabled(mApplyButton.isEnabled());
        mDefaultButton.setEnabled(mUseEnvVariable != true || mQuarantineMode != QuarantineMode::force || fileSearchPath != getDefaultSearchPath());
#else
        auto const useEnvVariable = mEnvVariableButton.getToggleState();
        auto const fileSearchPath = mFileSearchPathTable.getFileSearchPath();
        mApplyButton.setEnabled(useEnvVariable != mAccessor.getAttr<AttrType::useEnvVariable>() || fileSearchPath != mAccessor.getAttr<AttrType::searchPath>());
        mResetButton.setEnabled(mApplyButton.isEnabled());
        mDefaultButton.setEnabled(useEnvVariable != true || fileSearchPath != getDefaultSearchPath());
#endif
    };

    auto warnForChanges = []
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::InfoIcon)
                                 .withTitle(juce::translate("The plugin search paths have been modified!"))
                                 .withMessage(juce::translate("The pugin search paths have been modified. Restart the application in order to rescan the plugins."))
                                 .withButton(juce::translate("Ok"));
        juce::AlertWindow::showAsync(options, nullptr);
    };

    mFileSearchPathTable.onFileSearchPathChanged = [=]()
    {
        updateButtonsStates();
    };

    mApplyButton.onClick = [=, this]()
    {
#if JUCE_MAC
        mAccessor.setAttr<AttrType::useEnvVariable>(mUseEnvVariable, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::quarantineMode>(mQuarantineMode, NotificationType::synchronous);
#else
        mAccessor.setAttr<AttrType::useEnvVariable>(mEnvVariableButton.getToggleState(), NotificationType::synchronous);
#endif
        mAccessor.setAttr<AttrType::searchPath>(mFileSearchPathTable.getFileSearchPath(), NotificationType::synchronous);
        updateButtonsStates();
        warnForChanges();
    };

    mResetButton.onClick = [=, this]()
    {
#if JUCE_MAC
        mUseEnvVariable = mAccessor.getAttr<AttrType::useEnvVariable>();
        mQuarantineMode = mAccessor.getAttr<AttrType::quarantineMode>();
#else
        mEnvVariableButton.setToggleState(mAccessor.getAttr<AttrType::useEnvVariable>(), juce::NotificationType::dontSendNotification);
#endif
        mFileSearchPathTable.setFileSearchPath(mAccessor.getAttr<AttrType::searchPath>(), juce::NotificationType::sendNotificationSync);
        updateButtonsStates();
    };

    mDefaultButton.onClick = [=, this]()
    {
#if JUCE_MAC
        mUseEnvVariable = true;
        mQuarantineMode = QuarantineMode::force;
#else
        mEnvVariableButton.setToggleState(true, juce::NotificationType::dontSendNotification);
#endif
        mFileSearchPathTable.setFileSearchPath(getDefaultSearchPath(), juce::NotificationType::sendNotificationSync);
        updateButtonsStates();
    };

#if JUCE_MAC
    mOptionButton.onClick = [=, this]()
    {
        juce::PopupMenu menu;
        juce::WeakReference<juce::Component> safePointer(this);
        menu.addItem(juce::translate("Use environment variable"), true, mUseEnvVariable, [=, this]()
                     {
                         if(safePointer.get() == nullptr)
                         {
                             return;
                         }
                         mUseEnvVariable = !mUseEnvVariable;
                         updateButtonsStates();
                     });
        juce::PopupMenu submenu;
        auto const addModeItem = [&](juce::String const& name, QuarantineMode mode)
        {
            submenu.addItem(name, mQuarantineMode != mode, mQuarantineMode == mode, [=, this]()
                            {
                                if(safePointer.get() == nullptr)
                                {
                                    return;
                                }
                                mQuarantineMode = mode;
                                updateButtonsStates();
                            });
        };
        addModeItem(juce::translate("Keep the default system mechanism"), QuarantineMode::system);
        addModeItem(juce::translate("Attemp to open quarantined libraries"), QuarantineMode::force);
        addModeItem(juce::translate("Ignore quarantined libraries"), QuarantineMode::ignore);
        menu.addSubMenu(juce::translate("Quarantine management"), submenu);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mOptionButton));
    };
#else
    mEnvVariableInfo.setText(juce::translate("Env. variable"), juce::NotificationType::dontSendNotification);
    mEnvVariableInfo.setTooltip(juce::translate("Toggle the use of the VAMP_PATH environment variable"));
    mEnvVariableInfo.setEditable(false);
    mEnvVariableButton.setTooltip(juce::translate("Toggle the use of the VAMP_PATH environment variable"));
    mEnvVariableButton.onClick = [=]()
    {
        updateButtonsStates();
    };
#endif

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
#if JUCE_MAC
            case AttrType::useEnvVariable:
            case AttrType::quarantineMode:
            {
                mUseEnvVariable = acsr.getAttr<AttrType::useEnvVariable>();
                mQuarantineMode = acsr.getAttr<AttrType::quarantineMode>();
                updateButtonsStates();
            }
            break;
#else
            case AttrType::useEnvVariable:
            {
                mEnvVariableButton.setToggleState(acsr.getAttr<AttrType::useEnvVariable>(), juce::NotificationType::dontSendNotification);
                updateButtonsStates();
            }
            break;
            case AttrType::quarantineMode:
#endif
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
#if JUCE_MAC
    addAndMakeVisible(mOptionButton);
#else
    addAndMakeVisible(mEnvVariableInfo);
    addAndMakeVisible(mEnvVariableButton);
#endif
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
#if JUCE_MAC
        mOptionButton.setBounds(bottomBounds);
#else
        mEnvVariableButton.setBounds(bottomBounds.removeFromLeft(24).reduced(4));
        mEnvVariableInfo.setBounds(bottomBounds);
#endif
    }
    mSeparator.setBounds(bounds.removeFromBottom(1));
    mFileSearchPathTable.setBounds(bounds);
}

void PluginList::SearchPath::warnBeforeClosing()
{
#if JUCE_MAC
    if(mFileSearchPathTable.getFileSearchPath() != mAccessor.getAttr<AttrType::searchPath>() || mUseEnvVariable != mAccessor.getAttr<AttrType::useEnvVariable>() || mQuarantineMode != mAccessor.getAttr<AttrType::quarantineMode>())
#else
    if(mFileSearchPathTable.getFileSearchPath() != mAccessor.getAttr<AttrType::searchPath>() || mEnvVariableButton.getToggleState() != mAccessor.getAttr<AttrType::useEnvVariable>())
#endif
    {
        auto const options = juce::MessageBoxOptions()
                                 .withIconType(juce::AlertWindow::QuestionIcon)
                                 .withTitle(juce::translate("Apply plugin settings modification?"))
                                 .withMessage(juce::translate("The pugin settings have been modified but the changes were not applied. Would you like to apply the changes or to discard the changes?"))
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
}

ANALYSE_FILE_END
