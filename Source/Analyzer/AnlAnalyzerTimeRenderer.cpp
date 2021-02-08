#include "AnlAnalyzerTimeRenderer.h"

ANALYSE_FILE_BEGIN

Analyzer::TimeRenderer::TimeRenderer(Accessor& accessor, Zoom::Accessor& timeZoomAccessor)
: mAccessor(accessor)
, mTimeZoomAccessor(timeZoomAccessor)
{
    mInformation.setEditable(false);
    mInformation.setInterceptsMouseClicks(false, false);
    addChildComponent(mInformation);
    
    mListener.onAttrChanged = [&](Accessor const& acsr, AttrType attribute)
    {
        if(attribute == AttrType::results)
        {
            repaint();
        }
        else if(attribute == AttrType::processing)
        {
            if(acsr.getAttr<AttrType::processing>())
            {
                mInformation.setVisible(true);
                mInformation.setText("processing...", juce::NotificationType::dontSendNotification);
            }
            else
            {
                mInformation.setText("", juce::NotificationType::dontSendNotification);
            }
        }
    };
    
    mReceiver.onSignal = [&](Accessor const& acsr, SignalType signal, juce::var value)
    {
        juce::ignoreUnused(acsr, value);
        switch(signal)
        {
            case SignalType::image:
            {
                repaint();
            }
                break;
        }
    };
    
    mPlotListener.onAttrChanged = [&](Plot::Accessor const& acsr, Plot::AttrType attribute)
    {
        juce::ignoreUnused(acsr);
        if(attribute == Plot::AttrType::colourPlain || attribute == Plot::AttrType::colourMap)
        {
            repaint();
        }
    };
    
    mZoomListener.onAttrChanged = [&](Zoom::Accessor const& acsr, Zoom::AttrType attribute)
    {
        juce::ignoreUnused(acsr, attribute);
        repaint();
    };
    
    mAccessor.addReceiver(mReceiver);
    mAccessor.addListener(mListener, NotificationType::synchronous);
    auto& plotAcsr = mAccessor.getAccessor<AcsrType::plot>(0);
    plotAcsr.addListener(mPlotListener, NotificationType::synchronous);
    plotAcsr.getAccessor<Plot::AcsrType::valueZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    plotAcsr.getAccessor<Plot::AcsrType::binZoom>(0).addListener(mZoomListener, NotificationType::synchronous);
    mTimeZoomAccessor.addListener(mZoomListener, NotificationType::synchronous);
}

Analyzer::TimeRenderer::~TimeRenderer()
{
    mTimeZoomAccessor.removeListener(mZoomListener);
    auto& plotAcsr = mAccessor.getAccessor<AcsrType::plot>(0);
    plotAcsr.getAccessor<Plot::AcsrType::binZoom>(0).removeListener(mZoomListener);
    plotAcsr.getAccessor<Plot::AcsrType::valueZoom>(0).removeListener(mZoomListener);
    plotAcsr.removeListener(mPlotListener);
    mAccessor.removeListener(mListener);
    mAccessor.removeReceiver(mReceiver);
}

void Analyzer::TimeRenderer::resized()
{
    mInformation.setBounds(getLocalBounds().removeFromRight(200).removeFromTop(80));
}

void Analyzer::TimeRenderer::paint(juce::Graphics& g)
{
    auto const bounds = getLocalBounds();
    auto const width = bounds.getWidth();
    auto const height = bounds.getHeight();
    
    auto const& results = mAccessor.getAttr<AttrType::results>();
    if(results.empty() && width > 0)
    {
        return;
    }

    auto const& plotAcsr = mAccessor.getAccessor<AcsrType::plot>(0);
    auto const timeRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();

    auto const realTimeRange = juce::Range<Vamp::RealTime>{Vamp::RealTime::fromSeconds(timeRange.getStart()), Vamp::RealTime::fromSeconds(timeRange.getEnd())};

    auto const timeLength = timeRange.getLength();
    auto timeToPixel = [&](Vamp::RealTime const&  timestamp)
    {
        auto const time = (((timestamp.sec * 1000.0 + timestamp.msec()) / 1000.0) - timeRange.getStart()) / timeLength;
        return static_cast<int>(time * width);
    };

    auto const resultRatio = static_cast<double>(results.size()) / static_cast<double>(width);
    auto const resultIncrement = static_cast<size_t>(std::floor(std::max(resultRatio, 1.0)));

    if(results.front().values.empty())
    {
        g.setColour(plotAcsr.getAttr<Plot::AttrType::colourPlain>());
        for(size_t i = 0; i < results.size(); i += resultIncrement)
        {
            if(realTimeRange.contains(results[i].timestamp))
            {
                auto const x = timeToPixel(results[i].timestamp);
                g.drawVerticalLine(x, 0.0f, static_cast<float>(height));
            }
            else if(results[i].timestamp >= realTimeRange.getEnd())
            {
                break;
            }
        }
    }
    else if(results.front().values.size() == 1)
    {
        auto const clip = g.getClipBounds();
        g.setColour(plotAcsr.getAttr<Plot::AttrType::colourPlain>());
        auto const valueRange = plotAcsr.getAccessor<Plot::AcsrType::valueZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
        auto valueToPixel = [&](float const value)
        {
            auto const ratio = 1.0f - (value - valueRange.getStart()) / valueRange.getLength();
            return static_cast<int>(ratio * height);
        };
        juce::Point<float> pt;

        for(size_t i = 0; i < results.size(); i += resultIncrement)
        {
            auto const next = i + resultIncrement;
            auto const isVisible = realTimeRange.contains(results[i].timestamp) || (next < results.size() && realTimeRange.contains(results[next].timestamp));

            auto const x = timeToPixel(results[i].timestamp);
            auto const y = valueToPixel(results[i].values[0]);
            juce::Point<float> const npt{static_cast<float>(x), static_cast<float>(y)};
            if(x >= clip.getX() && x <= clip.getRight() && isVisible && i > 0)
            {
                g.drawLine({pt, npt});
            }
            else if(results[i].timestamp >= realTimeRange.getEnd() || x > clip.getRight())
            {
                break;
            }
            pt = npt;
        }
    }
    else
    {
        auto image = mAccessor.getImage();
        if(image == nullptr || image->isNull())
        {
            return;
        }

        auto const& binZoomAcsr = plotAcsr.getAccessor<Plot::AcsrType::binZoom>(0);
        auto const binVisibleRange = binZoomAcsr.getAttr<Zoom::AttrType::visibleRange>();
        auto const binGlobalRange = binZoomAcsr.getAttr<Zoom::AttrType::globalRange>();

        auto const globalTimeRange = mTimeZoomAccessor.getAttr<Zoom::AttrType::globalRange>();

        auto const valueRange = juce::Range<double>(binGlobalRange.getEnd() - binVisibleRange.getEnd(), binGlobalRange.getEnd() - binVisibleRange.getStart());

        auto const deltaX = static_cast<float>(timeRange.getStart() / globalTimeRange.getLength() * static_cast<double>(image->getWidth()));
        auto const deltaY = static_cast<float>(valueRange.getStart() / binGlobalRange.getLength() * static_cast<double>(image->getHeight()));

        auto const scaleX = static_cast<float>(globalTimeRange.getLength() / timeRange.getLength() * static_cast<double>(width) / static_cast<double>(image->getWidth()));
        auto const scaleY = static_cast<float>(binGlobalRange.getLength() / valueRange.getLength() * static_cast<double>(height) / static_cast<double>(image->getHeight()));

        auto const transform = juce::AffineTransform::translation(-deltaX, -deltaY).scaled(scaleX, scaleY);

        g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::lowResamplingQuality);
        g.drawImageTransformed(*image.get(), transform);
    }
}

void Analyzer::TimeRenderer::mouseMove(juce::MouseEvent const& event)
{
//    juce::ignoreUnused(event);
//    repaint();
//    auto const& results = mAccessor.getAttr<AttrType::results>();
//    if(results.empty() && getWidth() > 0)
//    {
//        return;
//    }
//    juce::String text;
//    
//    auto const timeRange = mZoomAccessor.getAttr<Zoom::AttrType::visibleRange>();
//    auto const time = static_cast<double>(event.x) / static_cast<double>(getWidth()) * timeRange.getLength() + timeRange.getStart();
//    auto const rtr = Vamp::RealTime::fromSeconds(time);
//    text += Format::secondsToString(time) + "\n";
//    if(results.front().values.empty())
//    {
//        
//    }
//    else if(results.front().values.size() == 1)
//    {
//        for(size_t i = 0; i < results.size(); i++)
//        {
//            if(results[i].timestamp >= rtr)
//            {
//                text += juce::String(results[i].values[0]) + results[i].label;
//                break;
//            }
//        }
//    }
//    else
//    {
//        auto const valueRange = mAccessor.getAccessor<AcsrType::binZoom>(0).getAttr<Zoom::AttrType::visibleRange>();
//        for(size_t i = 0; i < results.size(); i++)
//        {
//            if(results[i].timestamp >= rtr)
//            {
//                auto const index = std::max(std::min((1.0 - static_cast<double>(event.y) / static_cast<double>(getHeight())), 1.0), 0.0) * valueRange.getLength() + valueRange.getStart();
//                text += juce::String(static_cast<long>(index)) + "\n";
//                text += juce::String(results[i].values[static_cast<size_t>(index)] / valueRange.getLength()) + results[i].label;
//                break;
//            }
//        }
//    }
//    mInformation.setText(text, juce::NotificationType::dontSendNotification);
}

void Analyzer::TimeRenderer::mouseEnter(juce::MouseEvent const& event)
{
    mInformation.setVisible(true);
    mouseMove(event);
}

void Analyzer::TimeRenderer::mouseExit(juce::MouseEvent const& event)
{
    juce::ignoreUnused(event);
    mInformation.setVisible(false);
}

ANALYSE_FILE_END
