#include "AnlFloatingWindow.h"

#define MemoryBlock DumbBlock
#define Point CarbonDummyPointName
#define Component CarbonDummyCompName

#import <Cocoa/Cocoa.h>
#import <IOKit/pwr_mgt/IOPMLib.h>
#import <ScriptingBridge/ScriptingBridge.h>

#undef Point
#undef Component
#undef MemoryBlock

ANALYSE_FILE_BEGIN

void FloatingWindow::setFloatingProperty(juce::Component& component, bool state)
{
    juce::ComponentPeer* componentPeer = component.getPeer();
    anlStrongAssert(componentPeer);
    if(componentPeer == nullptr)
    {
        return;
    }
    componentPeer->setAlwaysOnTop(true);
    NSView* peer = reinterpret_cast<NSView*>(componentPeer->getNativeHandle());
    anlStrongAssert(peer);
    if(peer == nullptr)
    {
        return;
    }
    NSWindow* window = [peer window];
    anlStrongAssert(window);
    if(window == nullptr)
    {
        return;
    }
    if(state)
    {
        [window setHidesOnDeactivate:YES];
    }
    else
    {
        [window setHidesOnDeactivate:NO];
    }
}

ANALYSE_FILE_END
