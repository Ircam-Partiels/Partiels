#include "AnlGroupExporter.h"
#include "../Track/AnlTrackRenderer.h"
#include "AnlGroupPlot.h"

ANALYSE_FILE_BEGIN

juce::Image Group::Exporter::toImage(Accessor const& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, int width, int height, int scaledWidth, int scaledHeight)
{
    juce::Image image(juce::Image::PixelFormat::ARGB, scaledWidth, scaledHeight, true);
    juce::Graphics g(image);
    g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::highResamplingQuality);
    auto const scaleWidth = static_cast<float>(scaledWidth) / static_cast<float>(width);
    auto const scaleHeight = static_cast<float>(scaledHeight) / static_cast<float>(height);
    g.addTransform(juce::AffineTransform::scale(scaleWidth, scaleHeight));
    g.fillAll(accessor.getAttr<AttrType::colour>());
    auto const bounds = juce::Rectangle<int>(0, 0, width, height);
    auto const& laf = juce::Desktop::getInstance().getDefaultLookAndFeel();
    auto const& layout = accessor.getAttr<AttrType::layout>();
    std::vector<bool> channelVisibility(channels.size(), false);
    for(auto const& channel : channels)
    {
        if(channel < channelVisibility.size())
        {
            channelVisibility[channel] = true;
        }
    }
    auto const referenceTrackAcsr = Tools::getReferenceTrackAcsr(accessor);
    for(auto it = layout.crbegin(); it != layout.crend(); ++it)
    {
        auto const trackAcsr = Tools::getTrackAcsr(accessor, *it);
        if(trackAcsr.has_value() && trackAcsr.value().get().getAttr<Track::AttrType::showInGroup>())
        {
            auto const isSelected = referenceTrackAcsr.has_value() && std::addressof(trackAcsr.value().get()) == std::addressof(referenceTrackAcsr.value().get());
            auto const colour = isSelected ? laf.findColour(Decorator::ColourIds::normalBorderColourId) : juce::Colours::transparentBlack;
            Track::Renderer::paint(trackAcsr.value().get(), timeZoomAccessor, g, bounds, channelVisibility, colour);
        }
    }
    return image;
}

juce::Result Group::Exporter::toImage(Accessor& accessor, Zoom::Accessor const& timeZoomAccessor, std::set<size_t> const& channels, juce::File const& file, int width, int height, int scaledWidth, int scaledHeight, std::atomic<bool> const& shouldAbort)
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

    auto const image = toImage(accessor, timeZoomAccessor, channels, width, height, scaledWidth, scaledHeight);
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

ANALYSE_FILE_END
