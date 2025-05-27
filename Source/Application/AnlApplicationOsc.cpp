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

void Application::Osc::Sender::sendTrack(Track::Accessor const& trackAcsr, double time, std::optional<size_t> channel, std::optional<size_t> bin)
{
    if(!trackAcsr.getAttr<Track::AttrType::sendViaOsc>())
    {
        return;
    }
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

    auto const results = trackAcsr.getAttr<Track::AttrType::results>();
    auto const access = results.getReadAccess();
    if(static_cast<bool>(access))
    {
        auto const& identifier = trackAcsr.getAttr<Track::AttrType::identifier>();
        if(auto const allMakers = results.getMarkers())
        {
            if(!allMakers->empty())
            {
                auto channelIndex = std::min(channel.value_or(0_z), allMakers->size() - 1_z);
                auto const endChannel = channel.value_or(allMakers->size() - 1_z) + 1_z;
                while(channelIndex < endChannel)
                {
                    auto const channelMakers = allMakers->at(channelIndex);
                    if(!channelMakers.empty())
                    {
                        auto const it = std::prev(std::upper_bound(std::next(channelMakers.cbegin()), channelMakers.cend(), time, Track::Result::upper_cmp<Track::Result::Data::Marker>));
                        sendResult(identifier, channelIndex, std::distance(channelMakers.cbegin(), it), it, [](juce::OSCMessage& message, auto iter)
                                   {
                                       message.addString(juce::String(std::get<2>(*iter)));
                                   });
                    }
                    ++channelIndex;
                }
            }
        }
        else if(auto const allPoints = results.getPoints())
        {
            if(!allPoints->empty())
            {
                auto channelIndex = std::min(channel.value_or(0_z), allPoints->size() - 1_z);
                auto const endChannel = channel.value_or(allPoints->size() - 1_z) + 1_z;
                while(channelIndex < endChannel)
                {
                    auto const channelPoints = allPoints->at(channelIndex);
                    if(!channelPoints.empty())
                    {
                        auto const it = std::prev(std::upper_bound(std::next(channelPoints.cbegin()), channelPoints.cend(), time, Track::Result::upper_cmp<Track::Result::Data::Point>));
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
                    ++channelIndex;
                }
            }
        }
        else if(auto const allColumns = results.getColumns())
        {
            if(!allColumns->empty())
            {
                auto channelIndex = std::min(channel.value_or(0_z), allColumns->size() - 1_z);
                auto const endChannel = channel.value_or(allColumns->size() - 1_z) + 1_z;
                while(channelIndex < endChannel)
                {
                    auto const channelColmuns = allColumns->at(channelIndex);
                    if(!channelColmuns.empty())
                    {
                        auto const it = std::prev(std::upper_bound(std::next(channelColmuns.cbegin()), channelColmuns.cend(), time, Track::Result::upper_cmp<Track::Result::Data::Column>));
                        if(!bin.has_value())
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
                        else
                        {
                            auto const binIndex = bin.value();
                            sendResult(identifier, channelIndex, std::distance(channelColmuns.cbegin(), it), it, [&](juce::OSCMessage& message, auto iter)
                                       {
                                           message.addInt32(static_cast<int32_t>(binIndex));
                                           if(binIndex < std::get<2>(*iter).size())
                                           {
                                               message.addFloat32(std::get<2>(*iter).at(binIndex));
                                           }
                                           else
                                           {
                                               message.addString("-");
                                           }
                                       });
                        }
                    }
                    ++channelIndex;
                }
            }
        }
    }
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

Application::Osc::TrackDispatcher::TrackDispatcher(Sender& sender)
: mSender(sender)
, mLayoutNotifier(typeid(*this).name(), Instance::get().getDocumentAccessor(), [this]()
                  {
                      synchronize(mSender.isConnected());
                  },
                  {Group::AttrType::layout}, {Track::AttrType::identifier, Track::AttrType::sendViaOsc, Track::AttrType::description, Track::AttrType::results})
{
    mSender.addChangeListener(this);

    auto const documentAcsrUpdate = [this]([[maybe_unused]] Document::Accessor const& documentAcsr, Document::AcsrType type, [[maybe_unused]] auto const& acsr)
    {
        if(type == Document::AcsrType::tracks)
        {
            synchronize(mSender.isConnected());
        }
    };
    mDocumentListener.onAccessorInserted = documentAcsrUpdate;
    mDocumentListener.onAccessorErased = documentAcsrUpdate;
    Instance::get().getDocumentAccessor().addListener(mDocumentListener, NotificationType::synchronous);
    synchronize(mSender.isConnected());
}

Application::Osc::TrackDispatcher::~TrackDispatcher()
{
    Instance::get().getDocumentAccessor().removeListener(mDocumentListener);
    mSender.removeChangeListener(this);
    mLayoutNotifier.onLayoutUpdated = nullptr;
    synchronize(false);
}

void Application::Osc::TrackDispatcher::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    MiscWeakAssert(source == std::addressof(mSender));
    synchronize(mSender.isConnected());
}

void Application::Osc::TrackDispatcher::synchronize(bool connect)
{
    auto const zoomAttrChanged = [this](Zoom::Accessor const& accessor, Zoom::AttrType attribute)
    {
        switch(attribute)
        {
            case Zoom::AttrType::minimumLength:
            case Zoom::AttrType::anchor:
                break;
            case Zoom::AttrType::globalRange:
            case Zoom::AttrType::visibleRange:
            {
                auto const getZoomInfo = [&]() -> std::tuple<juce::String, Track::FrameType>
                {
                    auto const& documentAcsr = Instance::get().getDocumentAccessor();
                    for(auto const& trackAcsr : documentAcsr.getAcsrs<Document::AcsrType::tracks>())
                    {
                        if(std::addressof(accessor) == std::addressof(trackAcsr.get().getAcsr<Track::AcsrType::valueZoom>()))
                        {
                            return {trackAcsr.get().getAttr<Track::AttrType::identifier>(), Track::FrameType::value};
                        }
                        if(std::addressof(accessor) == std::addressof(trackAcsr.get().getAcsr<Track::AcsrType::binZoom>()))
                        {
                            return {trackAcsr.get().getAttr<Track::AttrType::identifier>(), Track::FrameType::vector};
                        }
                    }
                    return {"", Track::FrameType::label};
                };

                auto const info = getZoomInfo();
                if(std::get<0>(info).isNotEmpty())
                {
                    juce::OSCMessage message("/" + std::get<0>(info));
                    message.addString("zoom");
                    message.addString(std::get<1>(info) == Track::FrameType::vector ? "bin" : "value");
                    auto const globalRange = accessor.getAttr<Zoom::AttrType::globalRange>();
                    message.addFloat32(static_cast<float>(globalRange.getStart()));
                    message.addFloat32(static_cast<float>(globalRange.getEnd()));
                    auto const visibleRange = accessor.getAttr<Zoom::AttrType::visibleRange>();
                    message.addFloat32(static_cast<float>(visibleRange.getStart()));
                    message.addFloat32(static_cast<float>(visibleRange.getEnd()));
                    mSender.send(message);
                }
                break;
            }
        }
    };

    auto const trackAttrChanged = [this](Track::Accessor const& accessor, Track::AttrType attribute)
    {
        switch(attribute)
        {
            case Track::AttrType::identifier:
            case Track::AttrType::sendViaOsc:
            case Track::AttrType::file:
            case Track::AttrType::results:
            case Track::AttrType::edit:
            case Track::AttrType::description:
            case Track::AttrType::key:
            case Track::AttrType::input:
            case Track::AttrType::state:
            case Track::AttrType::sampleRate:
            case Track::AttrType::height:
            case Track::AttrType::colours:
            case Track::AttrType::font:
            case Track::AttrType::lineWidth:
            case Track::AttrType::unit:
            case Track::AttrType::labelLayout:
            case Track::AttrType::channelsLayout:
            case Track::AttrType::showInGroup:
            case Track::AttrType::zoomValueMode:
            case Track::AttrType::zoomLogScale:
            case Track::AttrType::zoomLink:
            case Track::AttrType::zoomAcsr:
            case Track::AttrType::graphics:
            case Track::AttrType::warnings:
            case Track::AttrType::processing:
            case Track::AttrType::focused:
            case Track::AttrType::grid:
            case Track::AttrType::hasPluginColourMap:
                break;
            case Track::AttrType::extraThresholds:
            {
                juce::OSCMessage message("/" + accessor.getAttr<Track::AttrType::identifier>());
                message.addString("thresholds");
                auto const& extraThresholds = accessor.getAttr<Track::AttrType::extraThresholds>();
                for(auto const& extraThreshold : extraThresholds)
                {
                    if(extraThreshold.has_value())
                    {
                        message.addFloat32(extraThreshold.value());
                    }
                }
                if(!extraThresholds.empty())
                {
                    mSender.send(message);
                }
                break;
            };
            case Track::AttrType::name:
            {
                juce::OSCMessage message("/" + accessor.getAttr<Track::AttrType::identifier>());
                message.addString("name");
                message.addString(accessor.getAttr<Track::AttrType::name>());
                mSender.send(message);
                break;
            }
        }
    };

    auto& documentAcsr = Instance::get().getDocumentAccessor();
    juce::OSCMessage message("/tracks");
    auto documentLayout = documentAcsr.getAttr<Document::AttrType::layout>();
    std::reverse(documentLayout.begin(), documentLayout.end());
    for(auto const& groupId : documentLayout)
    {
        if(Document::Tools::hasGroupAcsr(documentAcsr, groupId))
        {
            auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupId);
            auto groupLayout = groupAcsr.getAttr<Group::AttrType::layout>();
            std::reverse(groupLayout.begin(), groupLayout.end());
            for(auto const& trackId : groupLayout)
            {
                if(Document::Tools::hasTrackAcsr(documentAcsr, trackId))
                {
                    auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, trackId);
                    if(connect && trackAcsr.getAttr<Track::AttrType::sendViaOsc>())
                    {
                        if(Track::Tools::getFrameType(trackAcsr).value_or(Track::FrameType::label) == Track::FrameType::vector)
                        {
                            message.addString(trackAcsr.getAttr<Track::AttrType::identifier>());
                        }
                    }
                }
            }
        }
    }
    mSender.send(message);

    mZoomListeners.clear();
    mTrackListeners.clear();
    for(auto& trackAcsr : documentAcsr.getAcsrs<Document::AcsrType::tracks>())
    {
        auto const trackConnected = connect && trackAcsr.get().getAttr<Track::AttrType::sendViaOsc>() && Document::Tools::hasItem(documentAcsr, trackAcsr.get().getAttr<Track::AttrType::identifier>());
        if(trackConnected)
        {
            auto trackListener = std::make_unique<Track::Accessor::SmartListener>(typeid(*this).name(), trackAcsr.get(), trackAttrChanged);
            MiscWeakAssert(trackListener != nullptr);
            if(trackListener != nullptr)
            {
                mTrackListeners.push_back(std::move(trackListener));
                switch(Track::Tools::getFrameType(trackAcsr.get()).value_or(Track::FrameType::label))
                {
                    case Track::FrameType::label:
                        break;
                    case Track::FrameType::value:
                    {
                        auto valueZoomListener = std::make_unique<Zoom::Accessor::SmartListener>(typeid(*this).name(), trackAcsr.get().getAcsr<Track::AcsrType::valueZoom>(), zoomAttrChanged);
                        MiscWeakAssert(valueZoomListener != nullptr);
                        if(valueZoomListener != nullptr)
                        {
                            mZoomListeners.push_back(std::move(valueZoomListener));
                        }
                        break;
                    }
                    case Track::FrameType::vector:
                    {
                        auto valueZoomListener = std::make_unique<Zoom::Accessor::SmartListener>(typeid(*this).name(), trackAcsr.get().getAcsr<Track::AcsrType::valueZoom>(), zoomAttrChanged);
                        MiscWeakAssert(valueZoomListener != nullptr);
                        if(valueZoomListener != nullptr)
                        {
                            mZoomListeners.push_back(std::move(valueZoomListener));
                        }
                        auto binZoomListener = std::make_unique<Zoom::Accessor::SmartListener>(typeid(*this).name(), trackAcsr.get().getAcsr<Track::AcsrType::binZoom>(), zoomAttrChanged);
                        MiscWeakAssert(binZoomListener != nullptr);
                        if(binZoomListener != nullptr)
                        {
                            mZoomListeners.push_back(std::move(binZoomListener));
                        }
                        break;
                    }
                }
            }
        }
    }
}

Application::Osc::TransportDispatcher::TransportDispatcher(Sender& sender)
: mSender(sender)
{
    mSender.addChangeListener(this);
    changeListenerCallback(std::addressof(mSender));

    mTransportListener.onAttrChanged = [this](Transport::Accessor const& accessor, Transport::AttrType const& attribute)
    {
        auto const sendTracks = [&](double time)
        {
            auto const& documentAcsr = Instance::get().getDocumentAccessor();
            auto const& trackAcsrs = documentAcsr.getAcsrs<Document::AcsrType::tracks>();
            for(auto const& trackAcsr : trackAcsrs)
            {
                mSender.sendTrack(trackAcsr.get(), time, {}, {});
            }
        };

        switch(attribute)
        {
            case Transport::AttrType::playback:
            {
                juce::OSCMessage message("/playback");
                message.addInt32(accessor.getAttr<Transport::AttrType::playback>() ? 1 : 0);
                mSender.send(message);
                if(accessor.getAttr<Transport::AttrType::playback>())
                {
                    sendTracks(accessor.getAttr<Transport::AttrType::startPlayhead>());
                }
                break;
            }
            case Transport::AttrType::startPlayhead:
            {
                if(!accessor.getAttr<Transport::AttrType::playback>())
                {
                    sendTracks(accessor.getAttr<Transport::AttrType::startPlayhead>());
                }
            }
            break;
            case Transport::AttrType::runningPlayhead:
            {
                if(accessor.getAttr<Transport::AttrType::playback>())
                {
                    sendTracks(accessor.getAttr<Transport::AttrType::runningPlayhead>());
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

Application::Osc::TransportDispatcher::~TransportDispatcher()
{
    mSender.removeChangeListener(this);
    auto& transportAcsr = Instance::get().getDocumentAccessor().getAcsr<Document::AcsrType::transport>();
    transportAcsr.removeListener(mTransportListener);
}

void Application::Osc::TransportDispatcher::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
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

Application::Osc::MouseDispatcher::MouseDispatcher(Sender& sender)
: mSender(sender)
{
    mSender.addChangeListener(this);
}

Application::Osc::MouseDispatcher::~MouseDispatcher()
{
    juce::Desktop::getInstance().removeGlobalMouseListener(this);
    mSender.removeChangeListener(this);
}

void Application::Osc::MouseDispatcher::changeListenerCallback([[maybe_unused]] juce::ChangeBroadcaster* source)
{
    MiscWeakAssert(source == std::addressof(mSender));
    if(mSender.isConnected())
    {
        juce::Desktop::getInstance().addGlobalMouseListener(this);
    }
    else
    {
        juce::Desktop::getInstance().removeGlobalMouseListener(this);
    }
}

void Application::Osc::MouseDispatcher::mouseMove(juce::MouseEvent const& event)
{
    auto const& documentAcsr = Instance::get().getDocumentAccessor();
    if(documentAcsr.getAcsr<Document::AcsrType::transport>().getAttr<Transport::AttrType::playback>())
    {
        return;
    }
    auto const isMouseOver = !event.mods.isAnyMouseButtonDown() && event.mods.isAltDown();
    if(isMouseOver != mMouseOver)
    {
        mMouseOver = isMouseOver;
        juce::OSCMessage message("/mouseover");
        message.addInt32(mMouseOver ? 1 : 0);
        mSender.send(message);
    }

    if(!mMouseOver)
    {
        return;
    }

    auto const getGroupSection = [](juce::Component& component)
    {
        auto* plot = dynamic_cast<Group::Section*>(&component);
        if(plot != nullptr)
        {
            return plot;
        }
        return component.findParentComponentOfClass<Group::Section>();
    };

    auto const getTrackSection = [](juce::Component& component)
    {
        auto* plot = dynamic_cast<Track::Section*>(&component);
        if(plot != nullptr)
        {
            return plot;
        }
        return component.findParentComponentOfClass<Track::Section>();
    };

    auto const sendTrack = [&](juce::String const& identifier, juce::Point<int> const& pos, juce::Rectangle<int> const& bounds)
    {
        auto const timeRange = documentAcsr.getAcsr<Document::AcsrType::timeZoom>().getAttr<Zoom::AttrType::visibleRange>();
        if(timeRange.isEmpty())
        {
            return;
        }
        auto const& trackAcsr = Document::Tools::getTrackAcsr(documentAcsr, identifier);
        auto const channel = Track::Tools::getChannelVerticalRange(trackAcsr, bounds, pos.y - bounds.getY(), false);
        if(!channel.has_value() || std::get<1>(channel.value()).isEmpty())
        {
            return;
        }
        auto const scaleX = static_cast<double>(pos.x - bounds.getX()) / static_cast<double>(bounds.getWidth());
        auto const time = scaleX * timeRange.getLength() + timeRange.getStart();
        auto const verRange = std::get<1>(channel.value());
        auto const scaledY = static_cast<double>(verRange.getEnd() - pos.y) / static_cast<double>(verRange.getLength());
        auto const binRange = trackAcsr.getAcsr<Track::AcsrType::binZoom>().getAttr<Zoom::AttrType::visibleRange>();
        auto binIndex = scaledY * binRange.getLength() + binRange.getStart();
        if(trackAcsr.getAttr<Track::AttrType::zoomLogScale>() && Track::Tools::hasVerticalZoomInHertz(trackAcsr))
        {
            auto const numBins = trackAcsr.getAcsr<Track::AcsrType::binZoom>().getAttr<Zoom::AttrType::globalRange>().getEnd();
            auto const nyquist = trackAcsr.getAttr<Track::AttrType::sampleRate>() / 2.0;
            auto const midiMax = std::max(Track::Tools::getMidiFromHertz(nyquist), 1.0);
            auto const startMidi = Track::Tools::getHertzFromMidi(binIndex / numBins * midiMax);
            binIndex = startMidi / nyquist * numBins;
        }
        mSender.sendTrack(trackAcsr, time, std::get<0>(channel.value()), static_cast<size_t>(std::max(std::round(binIndex), 0.0)));
    };

    auto* component = event.source.getComponentUnderMouse();
    if(component == nullptr || event.mods.isAnyMouseButtonDown())
    {
        return;
    }
    if(auto const* groupSection = getGroupSection(*component))
    {
        auto const& groupAcsr = Document::Tools::getGroupAcsr(documentAcsr, groupSection->getIdentifier());
        auto const tracks = Group::Tools::getTrackAcsrs(groupAcsr);
        auto const pos = groupSection->getMouseXYRelative();
        auto const bounds = groupSection->getPlotBounds();
        for(auto const& track : tracks)
        {
            sendTrack(track.get().getAttr<Track::AttrType::identifier>(), pos, bounds);
        }
    }
    else if(auto const* trackSection = getTrackSection(*component))
    {
        sendTrack(trackSection->getIdentifier(), trackSection->getMouseXYRelative(), trackSection->getPlotBounds());
    }
    else if(mMouseOver)
    {
        mMouseOver = false;
        juce::OSCMessage message("/mouseover");
        message.addInt32(false);
        mSender.send(message);
    }
}

void Application::Osc::MouseDispatcher::mouseEnter(juce::MouseEvent const& event)
{
    mouseMove(event);
}

void Application::Osc::MouseDispatcher::mouseExit(juce::MouseEvent const& event)
{
    mouseMove(event);
}

void Application::Osc::MouseDispatcher::mouseDown(juce::MouseEvent const& event)
{
    mouseMove(event);
}

void Application::Osc::MouseDispatcher::mouseUp(juce::MouseEvent const& event)
{
    mouseMove(event);
}

void Application::Osc::MouseDispatcher::mouseWheelMove(juce::MouseEvent const& event, [[maybe_unused]] juce::MouseWheelDetails const& wheel)
{
    mouseMove(event);
}

void Application::Osc::MouseDispatcher::mouseMagnify(juce::MouseEvent const& event, [[maybe_unused]] float scaleFactor)
{
    mouseMove(event);
}

ANALYSE_FILE_END
