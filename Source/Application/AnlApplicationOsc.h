#pragma once

#include "../Document/AnlDocumentTools.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    namespace Osc
    {
        // clang-format off
        enum class AttrType : size_t
        {
              name
            , port
        };
        
        using AttrContainer = Model::Container
        < Model::Attr<AttrType::name, juce::String, Model::Flag::basic>
        , Model::Attr<AttrType::port, int, Model::Flag::basic>
        >;
        // clang-format on

        class Accessor
        : public Model::Accessor<Accessor, AttrContainer>
        {
        public:
            using Model::Accessor<Accessor, AttrContainer>::Accessor;
            // clang-format off
            Accessor()
            : Accessor(AttrContainer({
                                          {"127.0.0.1"}
                                        , {9001}
            }))
            {
            }
            // clang-format on
        };

        class Sender
        : public juce::ChangeBroadcaster
        {
        public:
            Sender(Accessor& accessor);
            ~Sender();

            Accessor& getAccessor();

            bool send(juce::OSCBundle const& bundle);
            bool send(juce::OSCMessage const& message);
            bool connect();
            bool disconnect();
            bool isConnected() const;

            juce::StringArray getMessages() const;
            void clearMessages();

            void sendTrack(Track::Accessor const& trackAcsr, double time, std::optional<size_t> channel, std::optional<size_t> bin);

        private:
            Accessor& mAccessor;
            Osc::Accessor::Listener mListener{typeid(*this).name()};
            juce::OSCSender mSender;
            bool mIsConnected{false};
            juce::StringArray mMessages;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sender)
        };

        class SettingsContent
        : public juce::Component
        , private juce::ChangeListener
        {
        public:
            SettingsContent(Sender& sender);
            ~SettingsContent() override;

            // juce::Component
            void resized() override;

        private:
            // juce::ChangeListener
            void changeListenerCallback(juce::ChangeBroadcaster* source) override;

            Sender& mSender;
            Accessor& mAccessor;
            Accessor::Listener mListener{typeid(*this).name()};
            PropertyText mPropertyName;
            PropertyNumber mPropertyPort;
            PropertyTextButton mConnect;
            juce::TextEditor mMessages;
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsContent)
        };

        class SettingsPanel
        : public HideablePanel
        {
        public:
            SettingsPanel(Sender& sender);
            ~SettingsPanel() override;

        private:
            SettingsContent mContent;
        };

        class TrackDispatcher
        : public juce::ChangeListener
        {
        public:
            TrackDispatcher(Sender& sender);
            ~TrackDispatcher() override;

        private:
            // juce::ChangeListener
            void changeListenerCallback(juce::ChangeBroadcaster* source) override;

            void synchosize(bool connect);

            Sender& mSender;
            Document::LayoutNotifier mLayoutNotifier;
            Track::Accessor::Listener mTrackListener{typeid(*this).name()};
            Zoom::Accessor::Listener mZoomListener{typeid(*this).name()};

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackDispatcher)
        };

        class TransportDispatcher
        : public juce::ChangeListener
        {
        public:
            TransportDispatcher(Sender& sender);
            ~TransportDispatcher() override;

        private:
            // juce::ChangeListener
            void changeListenerCallback(juce::ChangeBroadcaster* source) override;

            Sender& mSender;
            Transport::Accessor::Listener mTransportListener{typeid(*this).name()};

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportDispatcher)
        };

        class MouseDispatcher
        : public juce::ChangeListener
        , private juce::MouseListener
        {
        public:
            MouseDispatcher(Sender& sender);
            ~MouseDispatcher() override;

        private:
            // juce::ChangeListener
            void changeListenerCallback(juce::ChangeBroadcaster* source) override;

            // juce::MouseListener
            void mouseMove(juce::MouseEvent const& event) override;
            void mouseEnter(juce::MouseEvent const& event) override;
            void mouseExit(juce::MouseEvent const& event) override;
            void mouseDown(juce::MouseEvent const& event) override;
            void mouseUp(juce::MouseEvent const& event) override;
            void mouseWheelMove(juce::MouseEvent const& event, juce::MouseWheelDetails const& wheel) override;
            void mouseMagnify(juce::MouseEvent const& event, float scaleFactor) override;

            Sender& mSender;
            bool mMouseOver{false};

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MouseDispatcher)
        };
    } // namespace Osc
} // namespace Application

ANALYSE_FILE_END
