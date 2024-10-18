#include "AnlPluginListSearchPath.h"

ANALYSE_FILE_BEGIN

PluginList::SearchPath::SearchPath(Accessor& accessor)
: mAccessor(accessor)
{
    auto const updateButtonsStates = [this]()
    {
#if JUCE_MAC
        auto const fileSearchPath = mFileSearchPathTable.getFileSearchPath();
        mApplyButton.setEnabled(mUseEnvVariable != mAccessor.getAttr<AttrType::useEnvVariable>() || mQuarantineMode != mAccessor.getAttr<AttrType::quarantineMode>() || fileSearchPath != mAccessor.getAttr<AttrType::searchPath>());
        mResetButton.setEnabled(mApplyButton.isEnabled());
        mDefaultButton.setEnabled(mUseEnvVariable != true || mQuarantineMode != QuarantineMode::force || fileSearchPath != getDefaultSearchPath());
#else
        auto const fileSearchPath = mFileSearchPathTable.getFileSearchPath();
        mApplyButton.setEnabled(mUseEnvVariable != mAccessor.getAttr<AttrType::useEnvVariable>() || fileSearchPath != mAccessor.getAttr<AttrType::searchPath>());
        mResetButton.setEnabled(mApplyButton.isEnabled());
        mDefaultButton.setEnabled(mUseEnvVariable != true || fileSearchPath != getDefaultSearchPath());
#endif
    };

    auto const warnForChanges = []
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
        mAccessor.setAttr<AttrType::useEnvVariable>(mUseEnvVariable, NotificationType::synchronous);
#if JUCE_MAC
        mAccessor.setAttr<AttrType::quarantineMode>(mQuarantineMode, NotificationType::synchronous);
#endif
        mAccessor.setAttr<AttrType::searchPath>(mFileSearchPathTable.getFileSearchPath(), NotificationType::synchronous);
        updateButtonsStates();
        warnForChanges();
    };

    mResetButton.onClick = [=, this]()
    {
        mUseEnvVariable = mAccessor.getAttr<AttrType::useEnvVariable>();
#if JUCE_MAC
        mQuarantineMode = mAccessor.getAttr<AttrType::quarantineMode>();
#endif
        mFileSearchPathTable.setFileSearchPath(mAccessor.getAttr<AttrType::searchPath>(), juce::NotificationType::sendNotificationSync);
        updateButtonsStates();
    };

    mDefaultButton.onClick = [=, this]()
    {
        mUseEnvVariable = true;
#if JUCE_MAC
        mQuarantineMode = QuarantineMode::force;
#endif
        mFileSearchPathTable.setFileSearchPath(getDefaultSearchPath(), juce::NotificationType::sendNotificationSync);
        updateButtonsStates();
    };

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
        juce::PopupMenu blacklistSubmenu;
        auto const blackListFile = getBlackListFile();
        juce::StringArray blackListedPlugins;
        blackListedPlugins.addTokens(blackListFile.loadFileAsString(), "\n", "\"");
        blackListedPlugins.removeEmptyStrings();
        for(auto& blackListedPlugin : blackListedPlugins)
        {
            juce::File const pluginFile(blackListedPlugin);
            blacklistSubmenu.addItem(juce::translate("Clear FLNAME").replace("FLNAME", pluginFile.getFileName()), true, false, [=]
                                     {
                                         juce::StringArray newList;
                                         newList.addTokens(blackListFile.loadFileAsString(), "\n", "\"");
                                         newList.removeEmptyStrings();
                                         newList.removeString(blackListedPlugin);
                                         blackListFile.replaceWithText(newList.joinIntoString("\n"));
                                     });
        }
        if(!blackListedPlugins.isEmpty())
        {
            blacklistSubmenu.addSeparator();
        }
        blacklistSubmenu.addItem(juce::translate("Clear all"), !blackListedPlugins.isEmpty(), false, [=]
                                 {
                                     blackListFile.deleteFile();
                                 });
        blacklistSubmenu.addItem(juce::translate("Show file"), !blackListedPlugins.isEmpty(), false, [=]
                                 {
                                     blackListFile.revealToUser();
                                 });
        menu.addSubMenu(juce::translate("Blacklisted plugins"), blacklistSubmenu);
#if JUCE_MAC
        juce::PopupMenu quarantineSubmenu;
        auto const addModeItem = [&](juce::String const& name, QuarantineMode mode)
        {
            quarantineSubmenu.addItem(name, mQuarantineMode != mode, mQuarantineMode == mode, [=, this]()
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
        menu.addSubMenu(juce::translate("Quarantine management"), quarantineSubmenu);
#endif
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(mOptionButton));
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
            case AttrType::quarantineMode:
            case AttrType::useEnvVariable:
            {
                mUseEnvVariable = acsr.getAttr<AttrType::useEnvVariable>();
#if JUCE_MAC
                mQuarantineMode = acsr.getAttr<AttrType::quarantineMode>();
#endif
                updateButtonsStates();
            }
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
    addAndMakeVisible(mOptionButton);
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
        mOptionButton.setBounds(bottomBounds);
    }
    mSeparator.setBounds(bounds.removeFromBottom(1));
    mFileSearchPathTable.setBounds(bounds);
}

void PluginList::SearchPath::warnBeforeClosing()
{
#if JUCE_MAC
    if(mFileSearchPathTable.getFileSearchPath() != mAccessor.getAttr<AttrType::searchPath>() || mUseEnvVariable != mAccessor.getAttr<AttrType::useEnvVariable>() || mQuarantineMode != mAccessor.getAttr<AttrType::quarantineMode>())
#else
    if(mFileSearchPathTable.getFileSearchPath() != mAccessor.getAttr<AttrType::searchPath>() || mUseEnvVariable != mAccessor.getAttr<AttrType::useEnvVariable>())
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
