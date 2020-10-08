
#include "AnlApplicationInstance.h"

ANALYSE_FILE_BEGIN

Application::Interface::Interface()
{
    addAndMakeVisible(mHeader);
}

void Application::Interface::resized()
{
    mHeader.setBounds(getLocalBounds().removeFromTop(60));
}

ANALYSE_FILE_END
