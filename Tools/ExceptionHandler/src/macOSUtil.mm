#import <Cocoa/Cocoa.h>

void
SetBackgroundMode()
{
    [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
}

void
SetForegroundMode()
{
   [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
}
