
#pragma once

#include "../Tools/AnlMisc.h"
#include "../Tools/AnlModel.h"

ANALYSE_FILE_BEGIN

namespace Layout
{
    namespace StrechableContainer
    {
        enum AttrType : size_t
        {
            sizes
        };
        
        using Container = Model::Container
        < Model::Attr<AttrType::sizes, std::vector<int>, Model::Flag::basic>
        >;
        
        class Accessor
        : public Model::Accessor<Accessor, Container>
        {
        public:
            using Model::Accessor<Accessor, Container>::Accessor;
        };
        
        class Section
        : public juce::Component
        {
        public:
            
            enum ColourIds : int
            {
                  resizerActiveColourId = 0x2005300
                , resizerInactiveColourId = 0x2005301
            };
            
            enum Orientation : bool
            {
                  vertical
                , horizontal
            };
            
            Section(Accessor& acessor, Orientation = Orientation::vertical);
            ~Section() override;
            
            Orientation getOrientation() const;
            
            void setContent(size_t index, juce::Component* content, int minimumSize = 0);
            void setOrientation(Orientation orientation);
            
            // juce::Component
            void resized() override;
            
        private:
            
            struct Holder
            : public juce::Component
            {
                Holder(Section& owner, size_t index);
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
                
                Section& mOwner;
                size_t const mIndex;
                ResizerBar mResizer;
                juce::Component* mContent = nullptr;
                int mMinimumSize = 0;
            };
            
            Accessor& mAccessor;
            Accessor::Listener mListener;
            Orientation mOrientation;
            juce::Viewport mViewport;
            juce::Component mInnerContainer;
            std::vector<std::unique_ptr<Holder>> mHolders;
            
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Section)
        };
    }
}

ANALYSE_FILE_END
