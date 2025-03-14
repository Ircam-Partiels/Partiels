#include "AnlComponentSnapshot.h"

ANALYSE_FILE_BEGIN

ComponentSnapshot::ComponentSnapshot()
{
    setWantsKeyboardFocus(true);
}

void ComponentSnapshot::takeSnapshot(juce::Component& component, juce::String const& name, juce::Colour const& backgroundColour)
{
    auto const createSnapshot = [&](juce::Rectangle<int> areaToGrab, float scaleFactor) -> juce::Image
    {
        auto const r = areaToGrab.getIntersection(component.getLocalBounds());
        if(r.isEmpty())
        {
            return {};
        }

        auto const w = static_cast<int>(std::round(scaleFactor * static_cast<float>(r.getWidth())));
        auto const h = static_cast<int>(std::round(scaleFactor * static_cast<float>(r.getHeight())));

        juce::Image image(component.isOpaque() || backgroundColour.isOpaque() ? juce::Image::PixelFormat::RGB : juce::Image::PixelFormat::ARGB, w, h, true);
        juce::Graphics g(image);
        if(w != getWidth() || h != getHeight())
        {
            g.addTransform(juce::AffineTransform::scale(static_cast<float>(w) / static_cast<float>(r.getWidth()), static_cast<float>(h) / static_cast<float>(r.getHeight())));
        }
        g.setOrigin(-r.getPosition());
        g.fillAll(backgroundColour);
        component.paintEntireComponent(g, false);
        return image;
    };

    juce::MouseCursor::showWaitCursor();
    auto const date = juce::File::createLegalFileName(juce::Time::getCurrentTime().toString(true, true));
    auto const desktop = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userDesktopDirectory);
    auto const file(desktop.getNonexistentChildFile(juce::File::createLegalFileName(name) + "_" + date, ".png"));
    juce::TemporaryFile temp(file);
    auto* imageFormat = juce::ImageFileFormat::findImageFormatForFileExtension(temp.getFile());
    MiscWeakAssert(imageFormat != nullptr);
    if(imageFormat == nullptr)
    {
        juce::MouseCursor::hideWaitCursor();
        return;
    }
    const auto* display = juce::Desktop::getInstance().getDisplays().getDisplayForRect(component.getScreenBounds(), true);
    auto const bounds = juce::Desktop::getInstance().getDisplays().logicalToPhysical(component.getScreenBounds());
    auto const scale = (display != nullptr ? static_cast<float>(display->scale) : 1.0f) * juce::Component::getApproximateScaleFactorForComponent(&component);
    auto const image = createSnapshot(bounds.withZeroOrigin(), scale);
    {
        juce::FileOutputStream stream(temp.getFile());
        if(!stream.openedOk())
        {
            MiscWeakAssert(false);
            juce::MouseCursor::hideWaitCursor();
            return;
        }

        if(!imageFormat->writeImageToStream(image, stream))
        {
            MiscWeakAssert(false);
            juce::MouseCursor::hideWaitCursor();
            return;
        }
    }
    if(!temp.overwriteTargetFileWithTemporary())
    {
        MiscWeakAssert(false);
        return;
    }

    {
        auto constexpr snapshotWidth = 240;
        auto const snapshotHeight = std::max(image.getHeight() * (snapshotWidth / image.getWidth()), 32);
        mImage = juce::Image(juce::Image::PixelFormat::ARGB, snapshotWidth, snapshotHeight, true);
        juce::Graphics g(mImage);
        g.drawImageAt(image.rescaled(snapshotWidth, snapshotHeight), 0, 0, false);
        g.setColour(juce::Colours::black);
        g.drawRect(0, 0, snapshotWidth, snapshotHeight);
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
    mAlpha = std::max(mAlpha - 0.05f, 0.0f);
    repaint();
    if(mAlpha <= 0.0f)
    {
        stopTimer();
    }
}

juce::ApplicationCommandTarget* ComponentSnapshot::getNextCommandTarget()
{
    return findFirstTargetParentComponent();
}

void ComponentSnapshot::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    commands.add(CommandIDs::performSnapshot);
}

void ComponentSnapshot::getCommandInfo(juce::CommandID const commandID, juce::ApplicationCommandInfo& result)
{
    switch(commandID)
    {
        case CommandIDs::performSnapshot:
        {
            result.setInfo(juce::translate("Take Snapshot"), juce::translate("Take a Snapshot of the Component"), "Export", 0);
            result.defaultKeypresses.add(juce::KeyPress('c', juce::ModifierKeys::noModifiers, 0));
            result.setActive(true);
            break;
        }
    }
}

bool ComponentSnapshot::perform(juce::ApplicationCommandTarget::InvocationInfo const& info)
{
    switch(info.commandID)
    {
        case CommandIDs::performSnapshot:
        {
            takeSnapshot();
            return true;
        }
    }
    return false;
}

ANALYSE_FILE_END
