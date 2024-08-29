#include "AnlApplicationOsc.h"
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Osc::Sender::Sender(Accessor& accessor)
: mAccessor(accessor)
{
    mListener.onAttrChanged = [this]([[maybe_unused]] Accessor const& acrs, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::name:
            case AttrType::port:
            {
                disconnect();
                break;
            }
        }
    };
    mAccessor.addListener(mListener, NotificationType::synchronous);
}

Application::Osc::Sender::~Sender()
{
    mAccessor.removeListener(mListener);
    disconnect();
}

Application::Osc::Accessor& Application::Osc::Sender::getAccessor()
{
    return mAccessor;
}

bool Application::Osc::Sender::send(juce::OSCMessage const& message)
{
    if(!mIsConnected)
    {
        return false;
    }
    if(!mSender.send(message))
    {
        mMessages.add(juce::translate("Failed to send message!"));
        sendChangeMessage();
        return false;
    }
    return true;
}

bool Application::Osc::Sender::send(juce::OSCBundle const& bundle)
{
    if(!mIsConnected)
    {
        return false;
    }
    if(!mSender.send(bundle))
    {
        mMessages.add(juce::translate("Failed to send bundle!"));
        sendChangeMessage();
        return false;
    }
    return true;
}

bool Application::Osc::Sender::connect()
{
    if(!disconnect())
    {
        return false;
    }
    auto const& name = mAccessor.getAttr<AttrType::name>();
    auto const& port = mAccessor.getAttr<AttrType::port>();
    mIsConnected = mSender.connect(name, port);
    if(!mIsConnected)
    {
        mMessages.add(juce::translate("Failed to connect to host NAME at port PORT!").replace("NAME", name).replace("PORT", juce::String(port)));
    }
    else
    {
        mMessages.add(juce::translate("Connected to host NAME at port PORT.").replace("NAME", name).replace("PORT", juce::String(port)));
    }
    sendChangeMessage();
    Instance::get().getApplicationCommandManager().invokeDirectly(ApplicationCommandIDs::transportOscConnected, true);
    return mIsConnected;
}

bool Application::Osc::Sender::disconnect()
{
    if(mIsConnected)
    {
        mIsConnected = !mSender.disconnect();
        if(mIsConnected)
        {
            mMessages.add(juce::translate("Failed to disconnect from host!"));
        }
        else
        {
            mMessages.add(juce::translate("Disconnected from host."));
        }
        sendChangeMessage();
        Instance::get().getApplicationCommandManager().invokeDirectly(ApplicationCommandIDs::transportOscConnected, true);
    }
    return !mIsConnected;
}

bool Application::Osc::Sender::isConnected() const
{
    return mIsConnected;
}

juce::StringArray Application::Osc::Sender::getMessages() const
{
    return mMessages;
}

void Application::Osc::Sender::clearMessages()
{
    mMessages.clear();
}

Application::Osc::SettingsContent::SettingsContent(Sender& sender)
: mSender(sender)
, mAccessor(sender.getAccessor())
, mPropertyName(juce::translate("Name"), juce::translate("The name of the remote host to which messages will be send."), [this](auto const& name)
                {
                    mAccessor.setAttr<AttrType::name>(name, NotificationType::synchronous);
                })
, mPropertyPort(juce::translate("Port"), juce::translate("The port of the remote host to which messages will be send."), "", {0.0f, static_cast<float>(std::numeric_limits<int>::max())}, 1.0f, [this](auto const& port)
                {
                    mAccessor.setAttr<AttrType::port>(static_cast<int>(port), NotificationType::synchronous);
                })
, mConnect(juce::translate("Connect"), juce::translate("Connect to the remote host."), [this]()
           {
               if(mSender.isConnected())
               {
                   mSender.disconnect();
               }
               else
               {
                   mSender.connect();
               }
           })
{
    mMessages.setTooltip(juce::translate("The report of the OSC connection"));
    mMessages.setSize(300, 48);
    mMessages.setJustification(juce::Justification::left);
    mMessages.setMultiLine(true);
    mMessages.setReadOnly(true);
    mMessages.setScrollbarsShown(true);

    addAndMakeVisible(mPropertyName);
    addAndMakeVisible(mPropertyPort);
    addAndMakeVisible(mConnect);
    addAndMakeVisible(mMessages);

    mListener.onAttrChanged = [this](Accessor const& accessor, AttrType attr)
    {
        switch(attr)
        {
            case AttrType::name:
            {
                mPropertyName.entry.setText(accessor.getAttr<AttrType::name>(), juce::NotificationType::dontSendNotification);
                break;
            }
            case AttrType::port:
            {
                mPropertyPort.entry.setValue(static_cast<double>(accessor.getAttr<AttrType::port>()), juce::NotificationType::dontSendNotification);
                break;
            }
        }
    };
    setSize(300, 200);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    mSender.addChangeListener(this);
    changeListenerCallback(std::addressof(mSender));
}

Application::Osc::SettingsContent::~SettingsContent()
{
    mSender.removeChangeListener(this);
    mAccessor.removeListener(mListener);
}

void Application::Osc::SettingsContent::resized()
{
    auto bounds = getLocalBounds().withHeight(std::numeric_limits<int>::max());
    auto const setBounds = [&](juce::Component& component)
    {
        if(component.isVisible())
        {
            component.setBounds(bounds.removeFromTop(component.getHeight()));
        }
    };
    setBounds(mPropertyName);
    setBounds(mPropertyPort);
    setBounds(mConnect);
    setBounds(mMessages);
    setSize(bounds.getWidth(), bounds.getY() + 2);
}

void Application::Osc::SettingsContent::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    MiscWeakAssert(source == std::addressof(mSender));
    auto const connected = mSender.isConnected();
    mConnect.entry.setButtonText(connected ? juce::translate("Disconnect") : juce::translate("Connect"));
    mConnect.entry.setTooltip(connected ? juce::translate("Disconnect from the remote host.") : juce::translate("Connect to the remote host."));
    mMessages.setText(mSender.getMessages().joinIntoString("\n"));
    mMessages.moveCaretToEnd();
}

Application::Osc::SettingsPanel::SettingsPanel(Sender& sender)
: mContent(sender)
{
    setContent(juce::translate("OSC Settings"), &mContent);
}

Application::Osc::SettingsPanel::~SettingsPanel()
{
    setContent("", nullptr);
}

Application::Osc::Dispatcher::Dispatcher(Sender& sender)
: mSender(sender)
{
    mSender.addChangeListener(this);
    changeListenerCallback(std::addressof(mSender));

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& accessor, Transport::AttrType const& attribute)
    {
        switch(attribute)
        {
            case Transport::AttrType::playback:
            {
                juce::OSCMessage message("/playback");
                message.addInt32(accessor.getAttr<Transport::AttrType::playback>() ? 1 : 0);
                mSender.send(message);
                if(accessor.getAttr<Transport::AttrType::playback>())
                {
                    sendBundle(accessor.getAttr<Transport::AttrType::startPlayhead>());
                }
                break;
            }
            case Transport::AttrType::startPlayhead:
                break;
            case Transport::AttrType::runningPlayhead:
            {
                if(accessor.getAttr<Transport::AttrType::playback>())
                {
                    sendBundle(accessor.getAttr<Transport::AttrType::runningPlayhead>());
                }
                break;
            }
            case Transport::AttrType::looping:
            case Transport::AttrType::loopRange:
            case Transport::AttrType::stopAtLoopEnd:
            case Transport::AttrType::autoScroll:
            case Transport::AttrType::gain:
            case Transport::AttrType::markers:
            case Transport::AttrType::magnetize:
            case Transport::AttrType::selection:
                break;
        }
    };
}

Application::Osc::Dispatcher::~Dispatcher()
{
    mSender.removeChangeListener(this);
    auto& transportAcsr = Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>();
    transportAcsr.removeListener(mTransportListener);
}

void Application::Osc::Dispatcher::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    MiscWeakAssert(source == std::addressof(mSender));
    auto& transportAcsr = Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>();
    if(mSender.isConnected())
    {
        transportAcsr.addListener(mTransportListener, NotificationType::synchronous);
    }
    else
    {
        transportAcsr.removeListener(mTransportListener);
    }
}

void Application::Osc::Dispatcher::sendBundle(double time)
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    auto const& trackAcsrs = documentAcsr.getAcsrs<Document::AcsrType::tracks>();
    auto const sendResult = [&](auto const identifier, auto const channelIndex, auto const valueIndex, auto const it, auto const parser)
    {
        juce::OSCMessage message("/" + identifier);
        message.addInt32(static_cast<int32_t>(channelIndex));
        message.addInt32(static_cast<int32_t>(valueIndex));
        message.addFloat32(static_cast<float>(time));
        message.addFloat32(static_cast<float>(std::get<0>(*it)));
        message.addFloat32(static_cast<float>(std::get<1>(*it)));
        parser(message, it);
        for(auto const& extra : std::get<3>(*it))
        {
            message.addFloat32(extra);
        }
        mSender.send(message);
    };

    for(auto const& trackAcsr : trackAcsrs)
    {
        if(trackAcsr.get().getAttr<Track::AttrType::sendViaOsc>())
        {
            auto const results = trackAcsr.get().getAttr<Track::AttrType::results>();
            auto const access = results.getReadAccess();
            if(static_cast<bool>(access))
            {
                auto const& identifier = trackAcsr.get().getAttr<Track::AttrType::identifier>();
                if(auto const allMakers = results.getMarkers())
                {
                    for(auto channelIndex = 0_z; channelIndex < allMakers->size(); ++channelIndex)
                    {
                        auto const channelMakers = allMakers->at(channelIndex);
                        auto const it = std::lower_bound(channelMakers.cbegin(), channelMakers.cend(), time, Track::Result::lower_cmp<Track::Result::Data::Marker>);
                        if(it != channelMakers.cend())
                        {
                            sendResult(identifier, channelIndex, std::distance(channelMakers.cbegin(), it), it, [](juce::OSCMessage& message, auto iter)
                                       {
                                           message.addString(juce::String(std::get<2>(*iter)));
                                       });
                        }
                    }
                }
                else if(auto const allPoints = results.getPoints())
                {
                    for(auto channelIndex = 0_z; channelIndex < allPoints->size(); ++channelIndex)
                    {
                        auto const channelPoints = allPoints->at(channelIndex);
                        auto const it = std::lower_bound(channelPoints.cbegin(), channelPoints.cend(), time, Track::Result::lower_cmp<Track::Result::Data::Point>);
                        if(it != channelPoints.cend())
                        {
                            sendResult(identifier, channelIndex, std::distance(channelPoints.cbegin(), it), it, [](juce::OSCMessage& message, auto iter)
                                       {
                                           if(std::get<2>(*iter).has_value())
                                           {
                                               message.addFloat32(std::get<2>(*iter).value());
                                           }
                                           else
                                           {
                                               message.addString("-");
                                           }
                                       });
                        }
                    }
                }
                else if(auto const allColumns = results.getColumns())
                {
                    for(auto channelIndex = 0_z; channelIndex < allColumns->size(); ++channelIndex)
                    {
                        auto const channelColmuns = allColumns->at(channelIndex);
                        auto const it = std::lower_bound(channelColmuns.cbegin(), channelColmuns.cend(), time, Track::Result::lower_cmp<Track::Result::Data::Column>);
                        if(it != channelColmuns.cend())
                        {
                            sendResult(identifier, channelIndex, std::distance(channelColmuns.cbegin(), it), it, [](juce::OSCMessage& message, auto iter)
                                       {
                                           message.addInt32(static_cast<int32_t>(std::get<2>(*iter).size()));
                                           for(auto const& value : std::get<2>(*iter))
                                           {
                                               message.addFloat32(value);
                                           }
                                       });
                        }
                    }
                }
            }
        }
    }
}

ANALYSE_FILE_END
