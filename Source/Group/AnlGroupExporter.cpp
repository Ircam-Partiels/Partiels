#include "AnlGroupExporter.h"
#include "AnlGroupPlot.h"

ANALYSE_FILE_BEGIN

juce::Result Group::Exporter::toImage(Accessor& accessor, Zoom::Accessor const& timeZoomAccessor, juce::File const& file, int width, int height, std::atomic<bool> const& shouldAbort)
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
    if(imageFormat == nullptr)
    {
        return juce::Result::fail(juce::translate("The group ANLNAME can not be exported as image because the format of the file FLNAME is not supported.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    if(shouldAbort)
    {
        return juce::Result::fail(juce::translate("The export of the group ANLNAME to the file FLNAME has been aborted.").replace("ANLNAME", name).replace("FLNAME", file.getFullPathName()));
    }

    auto const getImage = [&]()
    {
        if(!lock.tryEnter())
        {
            return juce::Image{};
        }
        Zoom::Accessor tempTimeZoomAcsr;
        tempTimeZoomAcsr.copyFrom(timeZoomAccessor, NotificationType::synchronous);
        Plot plot(accessor, tempTimeZoomAcsr);
        plot.setSize(width, height);
        return plot.createComponentSnapshot({0, 0, width, height});
    };

    auto const image = getImage();

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
