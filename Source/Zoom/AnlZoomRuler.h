#pragma once

#include "AnlZoomModel.h"

ANALYSE_FILE_BEGIN

namespace Zoom
{
    class Ruler
    : public juce::Component
    {
    public:
        
        enum ColourIds : int
        {
            backgroundColourId = 0x2001000,
            tickColourId = 0x2001001,
            textColourId = 0x2001002,
            anchorColourId = 0x2001003,
            selectionColourId = 0x2001004
        };
        
        enum Orientation : bool
        {
            vertical,
            horizontal
        };
        
        Ruler(Accessor& accessor, Orientation orientation, size_t primaryTickInterval = 3, double tickReferenceValue = 0.0, double tickPowerInterval = 2.0, double divisionFactor = 1.0, double maxStringWidth = 25.0);
        
        ~Ruler() override;
        
        //! @brief Sets the power and division factors that determine the space between each tick
        void setTickPowerInterval(double tickPowerInterval, double divisionFactor = 1.0);
        
        //! @brief Sets the power factor that determine the space between each tick
        double getTickPowerInterval() const;
        
        //! @brief Sets the division factor that determine the space between each tick
        double getTickPowerDivisionFactor() const;
        
        //! @brief Sets number of ticks between each primary tick
        void setPrimaryTickInterval(size_t primaryTickInterval);
        
        //! @brief Gets number of ticks between each primary tick
        size_t getPrimaryTickInterval() const;
        
        //! @brief Sets the tick reference value that is used to determine the value of other ticks
        void setTickReferenceValue(double tickReferenceValue);
        
        //! @brief Gets the tick reference value that is used to determine the value of other ticks
        double getTickReferenceValue() const;
        
        //!@brief Sets the maximum pixel width needed for to be value represented as a string. This is only used in the case of horizontally oriented rulers
        void setMaximumStringWidth(double maxStringWidth);
        
        //!@brief Sets the visual orientation of the rule. To be vertical or horizontal.
        void setOrientation(Orientation orientation);
        
        //!@brief Gets the orientation that is used to determine how to draw the ruler
        Orientation getOrientation();
        
        std::function<void()> onDoubleClick = nullptr;
        std::function<void(int)> onMouseDown = nullptr;
        std::function<void(void)> onRightClick = nullptr;
        
        //!@brief Sets the method used for formatting and printing a specific value as a string
        void setValueAsStringMethod(std::function<juce::String(double)> valueAsStringMethod);
        
        //! @brief Sets the methods used to convert between Zoom range and a custom range
        void setRangeConversionMethods(std::function<double(double)> fromZoomToRange, std::function<double(double)> toZoomFromRange);
        
        static juce::String valueAsStringKiloShorthand(double);
        
        static double fromZoomRangePassThrough(double);
        static double toZoomRangePassThrough(double);
        
        // Juce::Component
        void mouseDown(juce::MouseEvent const& event) override;
        void mouseDrag(juce::MouseEvent const& event) override;
        void mouseUp(juce::MouseEvent const& event) override;
        void mouseDoubleClick(juce::MouseEvent const& event) override;
        void paint(juce::Graphics& g) override;
        
    private:
        
        juce::Range<double> calculateSelectedValueRange(juce::MouseEvent const& event);
        
        std::function<double(double)> mFromZoomRange = fromZoomRangePassThrough;
        std::function<double(double)> mToZoomRange = toZoomRangePassThrough;
        
        std::function<juce::String(double)> mGetValueAsString = valueAsStringKiloShorthand;
        
        enum class NavigationMode
        {
            zoom,
            translate,
            select
        };
        
        Accessor& mAccessor;
        Accessor::Listener mListener;
        Accessor::Receiver mReceiver;
        
        size_t mPrimaryTickInterval = 3;
        double mTickReferenceValue = 0.0;
        double mTickPowerInterval = 0.0;
        double mTickPowerDivisionFactor = 1.0;
        double mMaximumStringWidth = 60.0;
        
        juce::Range<double> mInitialValueRange;
        juce::Range<double> mSelectedValueRange {0.0, 0.0};
        bool mZooming = false;
        double mAnchor = 0.0;
        juce::Point<float> mPrevMousePos {0.0, 0.0};
        
        Orientation mOrientation;
        NavigationMode mNavigationMode {NavigationMode::zoom};
    };
}

ANALYSE_FILE_END
