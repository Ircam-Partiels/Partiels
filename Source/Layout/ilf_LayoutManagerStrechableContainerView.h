
#pragma once

#include "ilf_LayoutManagerDetachablePanelController.h"

ILF_WARNING_BEGIN
ILF_NAMESPACE_BEGIN

namespace LayoutManager
{
    namespace Strechable
    {
        namespace Container
        {
            class View
            : public juce::Component
            , private Controller::Listener
            {
            public:
                
                enum ColourIds : int
                {
                    resizerActiveColourId = 0x2005300,
                    resizerInactiveColourId = 0x2005301
                };
                
                enum Orientation
                {
                    vertical,
                    horizontal
                };
                
                View(Controller& controller, Orientation = Orientation::vertical);
                ~View() override;
                
                Orientation getOrientation() const;
                
                void setContent(size_t index, juce::Component* content, int minimumSize = 0);
                void setOrientation(Orientation orientation);
                
                // juce::Component
                void resized() override;
                
            private:
                
                // Controller::Listener
                void layoutManagerStrechableContainerAttributeChanged(Controller* controller, Controller::Listener::AttributeType type, juce::var const& value) override;
                
                
                struct Holder
                : public juce::Component
                {
                    Holder(View& owner, size_t index);
                    ~Holder() override = default;
                    
                    void resized() override;
                    
                    void setContent(juce::Component* content, int minimumSize);
                private:
                    
                    class ResizerBar :
                    public juce::Component
                    {
                    public:
                        ResizerBar(Holder& owner);
                        ~ResizerBar() override = default;
                        
                        void paint(juce::Graphics& g) override;
                        void mouseDown(juce::MouseEvent const& event) override;
                        void mouseDrag(juce::MouseEvent const& event) override;
                        
                    private:
                        Holder& mOwner;
                        int mSavedSize;
                    };
                    
                    View& mOwner;
                    size_t const mIndex;
                    ResizerBar mResizer;
                    juce::Component* mContent = nullptr;
                    int mMinimumSize = 0;
                };
                
                Controller& mController;
                Orientation mOrientation;
                juce::Viewport mViewport;
                juce::Component mInnerContainer;
                std::vector<std::unique_ptr<Holder>> mHolders;
                
                JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(View)
            };
        }
    }
}

ILF_NAMESPACE_END
ILF_WARNING_END


