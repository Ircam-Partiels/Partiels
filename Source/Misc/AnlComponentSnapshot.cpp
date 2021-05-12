#include "AnlComponentSnapshot.h"
#include <CursorsData.h>

ANALYSE_FILE_BEGIN

void ComponentSnapshot::showCameraCursor(bool state)
{
    static auto const camera = juce::MouseCursor(juce::ImageCache::getFromMemory(CursorsData::photocamera_png, CursorsData::photocamera_pngSize).rescaled(48, 48, juce::Graphics::ResamplingQuality::highResamplingQuality), 12, 12, 2.0f);
    setMouseCursor(state ? camera : juce::MouseCursor::NormalCursor);
}

void ComponentSnapshot::takeSnapshot(juce::Component& component, juce::String const& name)
{
    juce::MouseCursor::showWaitCursor();
    auto const date = juce::File::createLegalFileName(juce::Time::getCurrentTime().toString(true, true));
    auto const desktop = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
    auto const file(desktop.getNonexistentChildFile(juce::File::createLegalFileName(name) + "_" + date, ".jpg"));
    juce::TemporaryFile temp(file);
    auto* imageFormat = juce::ImageFileFormat::findImageFormatForFileExtension(temp.getFile());
    anlWeakAssert(imageFormat != nullptr);
    if(imageFormat == nullptr)
    {
        juce::MouseCursor::hideWaitCursor();
        return;
    }

    juce::FileOutputStream stream(temp.getFile());
    if(!stream.openedOk())
    {
        anlWeakAssert(false);
        juce::MouseCursor::hideWaitCursor();
        return;
    }
    const auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(component.getScreenBounds(), true);
    auto const bounds = juce::Desktop::getInstance().getDisplays().logicalToPhysical(component.getScreenBounds());
    auto const scale = (display != nullptr ? static_cast<float>(display->scale) : 1.0f) * juce::Component::getApproximateScaleFactorForComponent(&component);
    auto const image = component.createComponentSnapshot(bounds.withZeroOrigin(), true, scale);

    if(!imageFormat->writeImageToStream(image, stream))
    {
        anlWeakAssert(false);
        juce::MouseCursor::hideWaitCursor();
        return;
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        anlWeakAssert(false);
        return;
    }

    {
        auto constexpr snapshotWidth = 240;
        auto const snapshotHeight = std::max(image.getHeight() * (snapshotWidth / image.getWidth()), 32);
        mImage = juce::Image(juce::Image::PixelFormat::ARGB, snapshotWidth, snapshotHeight, true);
        juce::Graphics g(mImage);
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.drawRect(0, 0, snapshotWidth, snapshotHeight);
        g.drawImageAt(image.rescaled(snapshotWidth - 8, snapshotHeight - 8), 4, 4);
    }

    juce::MouseCursor::hideWaitCursor();
    mAlpha = 1.0f;
    startTimer(20);
    repaint();
}

void ComponentSnapshot::paintOverChildren(juce::Graphics& g)
{
    g.fillAll(juce::Colours::white.withAlpha(std::max(mAlpha, 0.0f)));
    if(mAlpha > 0.0f && mImage.isValid())
    {
        g.drawImageAt(mImage, 0, 0);
    }
}

void ComponentSnapshot::timerCallback()
{
    mAlpha = std::max(mAlpha - 0.025f, 0.0f);
    repaint();
    if(mAlpha <= 0.0f)
    {
        stopTimer();
    }
}

ANALYSE_FILE_END
