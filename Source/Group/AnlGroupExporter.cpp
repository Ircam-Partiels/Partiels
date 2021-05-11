#include "AnlGroupExporter.h"
#include "../Transport/AnlTransportModel.h"
#include "AnlGroupPlot.h"

ANALYSE_FILE_BEGIN

juce::Result Group::Exporter::toImage(Accessor& accessor, Zoom::Accessor& timeZoomAccessor, juce::File const& file, int width, int height)
{
    if(width <= 0 || height <= 0)
    {
        return juce::Result::fail(juce::translate("The group ANLNAME can not be exported as image because image size is not valid.").replace("ANLNAME", accessor.getAttr<AttrType::name>()));
    }
    juce::TemporaryFile temp(file);

    auto* imageFormat = juce::ImageFileFormat::findImageFormatForFileExtension(temp.getFile());
    if(imageFormat == nullptr)
    {
        return juce::Result::fail(juce::translate("The group ANLNAME can not be exported as image because the format of the file FLNAME is not supported.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }

    juce::FileOutputStream stream(temp.getFile());
    if(!stream.openedOk())
    {
        return juce::Result::fail(juce::translate("The group ANLNAME can not be exported as image because the output stream of the file FLNAME cannot be opened.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }

    Transport::Accessor transportAccessor;
    Plot plot(accessor, transportAccessor, timeZoomAccessor);
    plot.setSize(width, height);
    auto const image = plot.createComponentSnapshot({0, 0, width, height});
    if(!imageFormat->writeImageToStream(image, stream))
    {
        return juce::Result::fail(juce::translate("The group ANLNAME can not be exported as image because the output stream of the file FLNAME cannot be written.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }

    if(!temp.overwriteTargetFileWithTemporary())
    {
        return juce::Result::fail(juce::translate("The group ANLNAME can not be written to the file FLNAME. Ensure you have the right access to this file.").replace("ANLNAME", accessor.getAttr<AttrType::name>().replace("FLNAME", file.getFullPathName())));
    }
    return juce::Result::ok();
}

ANALYSE_FILE_END
