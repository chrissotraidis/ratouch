#define SDL_MAIN_HANDLED
#include <SDL_main.h>

#import <UIKit/UIKit.h>
#import <objc/runtime.h>

@interface SDLUIKitDelegate : NSObject <UIApplicationDelegate>
+ (NSString*)getAppDelegateClassName;
@end

extern int SDL_main(int argc, char* argv[]);

namespace
{
NSString* Ratouch_App_Delegate_Class(id, SEL)
{
    return @"RatouchAppDelegate";
}
}

int main(int argc, char* argv[])
{
    Method delegateSelector = class_getClassMethod(SDLUIKitDelegate.class, @selector(getAppDelegateClassName));
    method_setImplementation(delegateSelector, reinterpret_cast<IMP>(Ratouch_App_Delegate_Class));
    return SDL_UIKitRunApp(argc, argv, SDL_main);
}
