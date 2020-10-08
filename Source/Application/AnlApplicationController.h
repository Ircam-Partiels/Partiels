#pragma once

#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

namespace Application
{
    class Controller
    {
    public:
        Controller(Model& model);
        ~Controller() = default;
        
        Model const& getModel() const;
        
        juce::String getWindowState() const;
        std::vector<juce::File> const& getRecentlyOpenedFilesList() const;
        std::vector<juce::File> const& getCurrentOpenedFilesList() const;
        juce::File getCurrentDocumentFile() const;
        
        void setWindowState(juce::String const& state, juce::NotificationType const notification);
        void setRecentlyOpenedFilesList(std::vector<juce::File> files, juce::NotificationType const notification);
        void setCurrentOpenedFilesList(std::vector<juce::File> files, juce::NotificationType const notification);
        void setCurrentDocumentFile(juce::File const& file, juce::NotificationType const notification);
        
        void fromModel(Model const& model, juce::NotificationType const notification);
        void fromXml(juce::XmlElement const& xml, juce::NotificationType const notification);
        std::unique_ptr<juce::XmlElement> toXml() const;
        
        //! @brief The listener
        class Listener
        {
        public:
            Listener() = default;
            virtual ~Listener() = default;
            
            enum class AttributeType
            {
                windowState,
                recentlyOpenedFilesList,
                currentOpenedFilesList,
                currentDocumentFile
            };
            
            virtual void applicationAttributeChanged(Controller* controller, AttributeType type, juce::var const& value) = 0;
        };
        
        void addListener(Listener* listener, juce::NotificationType const notification);
        void removeListener(Listener* listener);
    private:
        
        void notifyListeners(Listener::AttributeType type, juce::var const& value, juce::NotificationType const notification);
        
        Model& mModel;
        juce::ListenerList<Listener> mListeners;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Controller)
    };
}

ANALYSE_FILE_END

