#include "AnlPluginListSearchPath.h"

ANALYSE_FILE_BEGIN

PluginList::SearchPath::SearchPath(Accessor& accessor)
: mAccessor(accessor)
{
    auto const updateButtonsStates = [this]()
    {
#if JUCE_MAC
        auto const fileSearchPath = mFileSearchPathTable.getFileSearchPath();
        mApplyButton.setEnabled(mUseEnvVariable != mAccessor.getAttr<AttrType::useEnvVariable>() || mIncludePreReleases != mAccessor.getAttr<AttrType::includePreReleases>() || mQuarantineMode != mAccessor.getAttr<AttrType::quarantineMode>() || fileSearchPath != mAccessor.getAttr<AttrType::searchPath>());
        mResetButton.setEnabled(mApplyButton.isEnabled());
        mDefaultButton.setEnabled(mUseEnvVariable != true || mIncludePreReleases != false || mQuarantineMode != QuarantineMode::force || fileSearchPath != getDefaultSearchPath());
#else
        auto const fileSearchPath = mFileSearchPathTable.getFileSearchPath();
        mApplyButton.setEnabled(mUseEnvVariable != mAccessor.getAttr<AttrType::useEnvVariable>() || mIncludePreReleases != mAccessor.getAttr<AttrType::includePreReleases>() || fileSearchPath != mAccessor.getAttr<AttrType::searchPath>());
        mResetButton.setEnabled(mApplyButton.isEnabled());
        mDefaultButton.setEnabled(mUseEnvVariable != true || mIncludePreReleases != false || fileSearchPath != getDefaultSearchPath());
#endif
    };

    mFileSearchPathTable.onFileSearchPathChanged = [=]()
    {
        updateButtonsStates();
    };

    mApplyButton.onClick = [=, this]()
    {
        mAccessor.setAttr<AttrType::useEnvVariable>(mUseEnvVariable, NotificationType::synchronous);
        mAccessor.setAttr<AttrType::includePreReleases>(mIncludePreReleases, NotificationType::synchronous);
#if JUCE_MAC
        mAccessor.setAttr<AttrType::quarantineMode>(mQuarantineMode, NotificationType::synchronous);
#endif
        mAccessor.setAttr<AttrType::searchPath>(mFileSearchPathTable.getFileSearchPath(), NotificationType::synchronous);
        PluginList::setEnvironment(mAccessor, true);
        mAccessor.sendSignal(SignalType::rescan, {true}, NotificationType::synchronous);
        updateButtonsStates();
    };

    mResetButton.onClick = [=, this]()
    {
        mUseEnvVariable = mAccessor.getAttr<AttrType::useEnvVariable>();
        mIncludePreReleases = mAccessor.getAttr<AttrType::includePreReleases>();
#if JUCE_MAC
        mQuarantineMode = mAccessor.getAttr<AttrType::quarantineMode>();
#endif
        mFileSearchPathTable.setFileSearchPath(mAccessor.getAttr<AttrType::searchPath>(), juce::NotificationType::sendNotificationSync);
        updateButtonsStates();
    };

    mDefaultButton.onClick = [=, this]()
    {
        mUseEnvVariable = true;
        mIncludePreReleases = false;
#if JUCE_MAC
        mQuarantineMode = QuarantineMode::force;
#endif
        mFileSearchPathTable.setFileSearchPath(getDefaultSearchPath(), juce::NotificationType::sendNotificationSync);
        updateButtonsStates();
    };

    mRescanButton.onClick = [=, this]()
    {
        mAccessor.sendSignal(SignalType::rescan, {true}, NotificationType::synchronous);
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
        menu.addItem(juce::translate("Include pre-release versions of online plugins"), true, mIncludePreReleases, [=, this]()
                     {
                         if(safePointer.get() == nullptr)
                         {
                             return;
                         }
                         mIncludePreReleases = !mIncludePreReleases;
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
            case AttrType::includePreReleases:
            {
                mUseEnvVariable = acsr.getAttr<AttrType::useEnvVariable>();
                mIncludePreReleases = acsr.getAttr<AttrType::includePreReleases>();
#if JUCE_MAC
                mQuarantineMode = acsr.getAttr<AttrType::quarantineMode>();
#endif
                updateButtonsStates();
            }
            break;
            case AttrType::webReferences:
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
    addAndMakeVisible(mRescanButton);
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
        mApplyButton.setBounds(bottomBounds.removeFromLeft(79));
        bottomBounds.removeFromLeft(1);
        mResetButton.setBounds(bottomBounds.removeFromLeft(79));
        bottomBounds.removeFromLeft(1);
        mDefaultButton.setBounds(bottomBounds.removeFromLeft(79));
        bottomBounds.removeFromLeft(1);
        mRescanButton.setBounds(bottomBounds.removeFromLeft(79));
        bottomBounds.removeFromLeft(1);
        mOptionButton.setBounds(bottomBounds);
    }
    mSeparator.setBounds(bounds.removeFromBottom(1));
    mFileSearchPathTable.setBounds(bounds);
}

void PluginList::SearchPath::askToClose(std::function<void(bool)> callback)
{
    if(!mApplyButton.isEnabled())
    {
        if(callback != nullptr)
        {
            callback(true);
        }
        return;
    }

    auto const options = juce::MessageBoxOptions()
                             .withIconType(juce::AlertWindow::QuestionIcon)
                             .withTitle(juce::translate("Apply plugin settings modification?"))
                             .withMessage(juce::translate("The plugin settings have been modified but the changes were not applied. Would you like to apply the changes or to discard the changes?"))
                             .withButton(juce::translate("Apply"), 1)
                             .withButton(juce::translate("Discard"), 2)
                             .withButton(juce::translate("Cancel"), 0);

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
                                     else if(windowResult == 2)
                                     {
                                         mResetButton.onClick();
                                     }
                                     if(callback != nullptr)
                                     {
                                         callback(windowResult != 0);
                                     }
                                 });
}

ANALYSE_FILE_END
