
#include "Application/AnlApplicationInstance.h"
#include "Application/AnlApplicationCommandLine.h"

extern "C" int main(int argc, char* argv[])
{
    return Anl::Application::CommandLine::run(argc, argv);
}
