#include "AnlGroupExporter.h"
#include "../Track/AnlTrackRenderer.h"
#include "../Document/AnlDocumentExporter.h"
#include "AnlGroupPlot.h"

ANALYSE_FILE_BEGIN

juce::Image Group::Exporter::toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, int width, int height, int scaledWidth, int scaledHeight, Zoom::Grid::OutsideGridOptions outsideGridOptions)
{
    // Determine if we need to expand the image for outside grid labels
    auto actualWidth = width;
    auto actualHeight = height;
    auto actualScaledWidth = scaledWidth;
    auto actualScaledHeight = scaledHeight;
    auto contentBounds = juce::Rectangle<int>(0, 0, width, height);
    
    if(outsideGridOptions != Zoom::Grid::OutsideGridOptions::none)
    {
        // Calculate space needed for outside labels - use Track constants
        auto leftPadding = 0;
        auto rightPadding = 0;
        auto topPadding = 0;
        auto bottomPadding = 0;
        
        switch(outsideGridOptions)
        {
            case Zoom::Grid::OutsideGridOptions::left:
                leftPadding = Track::Exporter::outsideGridLabelMaxWidth + Track::Exporter::outsideGridTickHeight;
                break;
            case Zoom::Grid::OutsideGridOptions::right:
                rightPadding = Track::Exporter::outsideGridLabelMaxWidth + Track::Exporter::outsideGridTickHeight;
                break;
            case Zoom::Grid::OutsideGridOptions::top:
                topPadding = Track::Exporter::outsideGridTickHeight + 20; // Height for text
                break;
            case Zoom::Grid::OutsideGridOptions::bottom:
                bottomPadding = Track::Exporter::outsideGridTickHeight + 20; // Height for text
                break;
            default:
                break;
        }
        
        actualWidth = width + leftPadding + rightPadding;
        actualHeight = height + topPadding + bottomPadding;
        actualScaledWidth = scaledWidth + static_cast<int>((leftPadding + rightPadding) * (static_cast<float>(scaledWidth) / static_cast<float>(width)));
        actualScaledHeight = scaledHeight + static_cast<int>((topPadding + bottomPadding) * (static_cast<float>(scaledHeight) / static_cast<float>(height)));
        contentBounds = juce::Rectangle<int>(leftPadding, topPadding, width, height);
    }

    juce::Image image(juce::Image::PixelFormat::ARGB, actualScaledWidth, actualScaledHeight, true);
    juce::Graphics g(image);
    g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::highResamplingQuality);
    auto const scaleWidth = static_cast<float>(actualScaledWidth) / static_cast<float>(actualWidth);
    auto const scaleHeight = static_cast<float>(actualScaledHeight) / static_cast<float>(actualHeight);
    g.addTransform(juce::AffineTransform::scale(scaleWidth, scaleHeight));
    g.fillAll(accessor.getAttr<AttrType::colour>());
    auto const& laf = juce::Desktop::getInstance().getDefaultLookAndFeel();

    auto const referenceTrackAcsr = Tools::getReferenceTrackAcsr(accessor);
    auto const& layout = accessor.getAttr<AttrType::layout>();
    for(auto it = layout.crbegin(); it != layout.crend(); ++it)
    {
        auto const trackAcsr = Tools::getTrackAcsr(accessor, *it);
        if(trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>())
        {
            auto const channelLayout = trackAcsr.value().get().getAttr<Track::AttrType::channelsLayout>();
            auto channelVisibility = channels.empty() ? channelLayout : std::vector<bool>(channelLayout.size(), false);
            for(auto const& channel : channels)
            {
                if(channel < channelVisibility.size())
                {
                    channelVisibility[channel] = true;
                }
            }
            if(std::any_of(channelVisibility.cbegin(), channelVisibility.cend(), [](auto const visibility) { return visibility; }))
            {
                auto colour = trackAcsr.value().get().getAttr<Track::AttrType::colours>().results;
                if(referenceTrackAcsr.has_value() && *it == referenceTrackAcsr.value())
                {
                    colour = colour.withAlpha(1.0f);
                }
                else
                {
                    colour = colour.withAlpha(0.5f);
                }
                // Create dummy options for the track renderer
                Document::Exporter::Options dummyOptions;
                dummyOptions.outsideGridLabels = outsideGridOptions;
                Track::Renderer::paint(trackAcsr.value().get(), timeZoomAccessor, g, contentBounds, channelVisibility, colour, &dummyOptions);
            }
        }
    }
    
    if(outsideGridOptions != Zoom::Grid::OutsideGridOptions::none)
    {
        // Draw outside grid labels for group (only time labels supported)
        g.setColour(laf.findColour(Decorator::ColourIds::normalBorderColourId));
        
        auto const stringifyTime = [](double const timeInSeconds) { return Tools::getStringTime(timeInSeconds); };
        
        switch(outsideGridOptions)
        {
            case Zoom::Grid::OutsideGridOptions::top:
            {
                auto const labelBounds = juce::Rectangle<int>(
                    contentBounds.getX(),
                    0,
                    contentBounds.getWidth(),
                    Track::Exporter::outsideGridTickHeight + 20);
                
                Zoom::Grid::paintOutsideHorizontal(g, timeZoomAccessor.getAcsr<Zoom::AcsrType::grid>(), 
                                                timeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>(), 
                                                labelBounds, 
                                                stringifyTime, 
                                                Track::Exporter::outsideGridLabelMaxWidth, 
                                                Zoom::Grid::Justification::horizontallyCentred);
                break;
            }
            case Zoom::Grid::OutsideGridOptions::bottom:
            {
                auto const labelBounds = juce::Rectangle<int>(
                    contentBounds.getX(),
                    actualHeight - Track::Exporter::outsideGridTickHeight - 20,
                    contentBounds.getWidth(),
                    Track::Exporter::outsideGridTickHeight + 20);
                
                Zoom::Grid::paintOutsideHorizontal(g, timeZoomAccessor.getAcsr<Zoom::AcsrType::grid>(), 
                                                timeZoomAccessor.getAttr<Zoom::AttrType::visibleRange>(), 
                                                labelBounds, 
                                                stringifyTime, 
                                                Track::Exporter::outsideGridLabelMaxWidth, 
                                                Zoom::Grid::Justification::horizontallyCentred);
                break;
            }
            default:
                // Group exports typically only support top/bottom (time) labels
                break;
        }
    }
    
    return image;
}

juce::Result Group::Exporter::toImage(Accessor& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, juce::File const& file, int width, int height, int scaledWidth, int scaledHeight, std::atomic<bool> const& shouldAbort, Zoom::Grid::OutsideGridOptions outsideGridOptions)
{
    juce::MessageManager::Lock lock;
    if(!lock.tryEnter())
    {
        return juce::Result::fail("Invalid threaded access to model");
    }
    auto const name = accessor.getAttr<AttrType::name>();
    lock.exit();

    if(width <= 0 || height <= 0)
    {
        return juce::Result::fail(juce::translate("The group ANLNAME can not be exported as image because image size is not valid.").replace("ANLNAME", name));
    }
    juce::TemporaryFile temp(file);

    auto* imageFormat = juce::ImageFileFormat::findImageFormatForFileExtension(temp.getFile());
    auto const xDensity = std::round(static_cast<double>(scaledWidth) / static_cast<double>(width) * 72.0);
    auto const yDensity = std::round(static_cast<double>(scaledHeight) / static_cast<double>(height) * 72.0);
    if(auto* pngFormat = dynamic_cast<juce::PNGImageFormat*>(imageFormat))
    {
        pngFormat->setDensity(static_cast<juce::uint32>(xDensity), static_cast<juce::uint32>(yDensity));
    }
    else if(auto* jpegFormat = dynamic_cast<juce::JPEGImageFormat*>(imageFormat))
    {
        jpegFormat->setDensity(static_cast<juce::uint16>(xDensity), static_cast<juce::uint16>(yDensity));
    }
    else
    {
        return juce::Result::fail(juce::translate("The group ANLNAME can not be exported as image because the format of the file FLNAME is not supported.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    if(shouldAbort)
    {
        return juce::Result::fail(juce::translate("The export of the group ANLNAME to the file FLNAME has been aborted.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    auto const image = toImage(accessor, timeZoomAccessor, channels, width, height, scaledWidth, scaledHeight, outsideGridOptions);
    if(image.isValid())
    {
        juce::FileOutputStream stream(temp.getFile());
        if(!stream.openedOk())
        {
            return juce::Result::fail(juce::translate("The group ANLNAME can not be exported as image because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
        }

        if(!imageFormat->writeImageToStream(image, stream))
        {
            return juce::Result::fail(juce::translate("The group ANLNAME can not be exported as image because the output stream of the file FLNAME cannot be written.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
        }
    }
    else
    {
        return juce::Result::fail(juce::translate("The group ANLNAME can not be exported as image because the image cannot be created.").replace("ANLNAME", name));
    }

    if(shouldAbort)
    {
        return juce::Result::fail(juce::translate("The export of the group ANLNAME to the file FLNAME has been aborted.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The group ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", accessor.getAttr<AttrType::name>()).replace("FLNAME", file.getFullPathName()));
    }
    return juce::Result::ok();
}

juce::Image Group::Exporter::toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, int width, int height, int scaledWidth, int scaledHeight, Document::Exporter::Options const& options)
{
    return toImage(accessor, timeZoomAccessor, channels, width, height, scaledWidth, scaledHeight, options.outsideGridLabels);
}

juce::Result Group::Exporter::toImage(Accessor& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, juce::File const& file, int width, int height, int scaledWidth, int scaledHeight, std::atomic<bool> const& shouldAbort, Document::Exporter::Options const& options)
{
    return toImage(accessor, timeZoomAccessor, channels, file, width, height, scaledWidth, scaledHeight, shouldAbort, options.outsideGridLabels);
}

ANALYSE_FILE_END
