#include "AnlApplicationModel.h"

ANALYSE_FILE_BEGIN

std::vector<juce::File> Application::Accessor::sanitize(std::vector<juce::File> const& files)
{
    std::vector<juce::File> copy = files;
    copy.erase(std::unique(copy.begin(), copy.end()), copy.end());
    copy.erase(std::remove_if(copy.begin(), copy.end(), [](auto const& file)
    {
        return !file.existsAsFile();
    }), copy.end());
    return copy;
}

template <>
void Application::Accessor::setValue<Application::AttrType::recentlyOpenedFilesList, std::vector<juce::File>>(std::vector<juce::File> const& value, NotificationType notification)
{
    ::Anl::Model::Accessor<Accessor, Container>::setValue<AttrType::recentlyOpenedFilesList, std::vector<juce::File>>(sanitize(value), notification);
}

ANALYSE_FILE_END
