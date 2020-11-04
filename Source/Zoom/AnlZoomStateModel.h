#pragma once

#include "../Tools/AnlModelAccessor.h"
#include "../Tools/AnlSignalBroadcaster.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    namespace State
    {
        //! @brief The data model of a zoom state
        //! @details This model represent the state of a zoom. The global range defines the minimum and maximum values
        //! that can be used for the visible range and the minimum length defines the minimum distance between the minimum
        //! and maximum values of the visible range.
        struct Model
        {
            enum class Attribute
            {
                range
            };
            
            enum class Signal
            {
                moveAnchorBegin,
                moveAnchorEnd,
                moveAnchorPerform
            };
            
            Model(juce::Range<double> vr = {0.0, 0.0});
            
            juce::Range<double> range;
            
            std::unique_ptr<juce::XmlElement> toXml() const;
            static Model fromXml(juce::XmlElement const& xml, Model defaultModel = {});
            
            bool operator!=(Model const& other) const;
            bool operator==(Model const& other) const;
            
            JUCE_LEAK_DETECTOR(Model)
        };
        
        class Accessor
        : public Tools::ModelAccessor<Accessor, Model, Model::Attribute>
        , public Tools::SignalBroadcaster<Accessor, Model::Signal>
        {
        public:
            using Tools::ModelAccessor<Accessor, Model, Model::Attribute>::ModelAccessor;
            Accessor(Model& model, juce::Range<double> const& globalRange, double minimumLength);
            ~Accessor() override = default;
            void fromModel(Model const& model, juce::NotificationType const notification) override;
            
            void setContraints(juce::Range<double> const& globalRange, double minimumLength, juce::NotificationType const notification);
            std::tuple<juce::Range<double>, double> getContraints() const;
        private:
            juce::Range<double> mGlobalRange;
            double mMinimumLength;
        };
    }
}

ANALYSE_FILE_END
