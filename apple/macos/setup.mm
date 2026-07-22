#import <AppKit/AppKit.h>

#include <SDL.h>

#import "apple/shared/importer.h"
#include "common/ratouch_settings.h"

void Toggle_Video_Fullscreen();
int Ratouch_Mac_Pointer_Sensitivity();
void Ratouch_Mac_Set_Pointer_Sensitivity(int sensitivity);

namespace
{
NSString* Ratouch_License_Text()
{
    NSString* path = [NSBundle.mainBundle pathForResource:@"License" ofType:@"txt"];
    NSString* text = path.length > 0 ? [NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil]
                                     : nil;
    return text ?: @"The bundled GPL-3.0 license text is unavailable.";
}

void Present_Import_Error(NSError* error)
{
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"RAtouch could not import those files";
    alert.informativeText = error.localizedDescription
                                ?: @"Choose a folder or files that include REDALERT.MIX and base-game MAIN.MIX data.";
    alert.alertStyle = NSAlertStyleWarning;
    [alert addButtonWithTitle:@"Choose again"];
    [alert runModal];
}
}

@interface RatouchMacMenuController : NSObject
@property(nonatomic, strong) NSTextField* pointerSpeedValue;
@property(nonatomic, strong) NSTextField* musicVolumeValue;
@property(nonatomic, strong) NSTextField* soundVolumeValue;
- (void)showAbout:(id)sender;
- (void)showSettings:(id)sender;
- (void)pointerSpeedChanged:(NSSlider*)sender;
- (void)scaleFilterChanged:(NSSegmentedControl*)sender;
- (void)aspectChanged:(NSSegmentedControl*)sender;
- (void)musicVolumeChanged:(NSSlider*)sender;
- (void)soundVolumeChanged:(NSSlider*)sender;
- (void)requestQuit:(id)sender;
- (void)toggleFullScreen:(id)sender;
- (void)showGameData:(id)sender;
- (void)stageGameData:(id)sender;
- (void)openSource:(id)sender;
- (void)openInputGuide:(id)sender;
@end

@implementation RatouchMacMenuController

- (void)requestQuit:(id)sender
{
    (void)sender;
    // Save and engine cleanup must run on the thread that consumes SDL events.
    SDL_Event event = {};
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
}

- (void)showAbout:(id)sender
{
    (void)sender;
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"RAtouch";
    alert.informativeText = @"Active alpha • Engine and platform code only";

    NSString* notice = @"RAtouch is an unofficial modified port. It contains no commercial game data and is not "
                        @"affiliated with, endorsed by, or supported by Electronic Arts. No trademark, publicity, "
                        @"or game-asset rights are granted. There is no warranty. You may convey the covered engine "
                        @"code under GPL-3.0 and the additional terms reproduced below.\n\n";

    NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, 620, 460)];
    scroll.hasVerticalScroller = YES;
    scroll.borderType = NSBezelBorder;
    NSTextView* legal = [[NSTextView alloc] initWithFrame:NSMakeRect(0, 0, 600, 460)];
    legal.editable = NO;
    legal.selectable = YES;
    legal.font = [NSFont systemFontOfSize:12];
    legal.string = [notice stringByAppendingString:Ratouch_License_Text()];
    legal.textContainerInset = NSMakeSize(12, 12);
    legal.verticallyResizable = YES;
    legal.horizontallyResizable = NO;
    legal.textContainer.widthTracksTextView = YES;
    legal.textContainer.containerSize = NSMakeSize(600, CGFLOAT_MAX);
    legal.accessibilityLabel = @"RAtouch license and Electronic Arts additional terms";
    scroll.documentView = legal;
    alert.accessoryView = scroll;

    [alert addButtonWithTitle:@"Done"];
    [alert addButtonWithTitle:@"Get the Source"];
    if ([alert runModal] == NSAlertSecondButtonReturn) {
        [self openSource:nil];
    }
}

- (void)showSettings:(id)sender
{
    (void)sender;
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"RAtouch Settings";
    alert.informativeText = @"Display and audio changes apply when you return to the game. Pointer speed applies "
                             @"immediately. The original Options screen remains available.";

    NSTextField* displayHeading = [NSTextField labelWithString:@"Display"];
    displayHeading.font = [NSFont boldSystemFontOfSize:NSFont.systemFontSize];

    NSSegmentedControl* scaleFilter = [NSSegmentedControl segmentedControlWithLabels:@[@"Sharp", @"Smooth"]
                                                                            trackingMode:NSSegmentSwitchTrackingSelectOne
                                                                                  target:self
                                                                                  action:@selector(scaleFilterChanged:)];
    scaleFilter.selectedSegment = Ratouch_Apple_App_Setting(RATOUCH_SETTING_SCALE_FILTER);
    scaleFilter.accessibilityLabel = @"Display filter";
    NSStackView* scaleFilterRow = [NSStackView stackViewWithViews:@[
        [NSTextField labelWithString:@"Display filter"],
        scaleFilter,
    ]];
    scaleFilterRow.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    scaleFilterRow.distribution = NSStackViewDistributionFill;

    NSSegmentedControl* aspect = [NSSegmentedControl segmentedControlWithLabels:@[@"Preserve", @"Fill"]
                                                                      trackingMode:NSSegmentSwitchTrackingSelectOne
                                                                            target:self
                                                                            action:@selector(aspectChanged:)];
    aspect.selectedSegment = Ratouch_Apple_App_Setting(RATOUCH_SETTING_ASPECT_MODE);
    aspect.accessibilityLabel = @"Aspect handling";
    NSStackView* aspectRow = [NSStackView stackViewWithViews:@[
        [NSTextField labelWithString:@"Aspect handling"],
        aspect,
    ]];
    aspectRow.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    aspectRow.distribution = NSStackViewDistributionFill;

    NSTextField* audioHeading = [NSTextField labelWithString:@"Audio"];
    audioHeading.font = [NSFont boldSystemFontOfSize:NSFont.systemFontSize];

    const int music = Ratouch_Apple_App_Setting(RATOUCH_SETTING_MUSIC_VOLUME);
    self.musicVolumeValue = [NSTextField labelWithString:[NSString stringWithFormat:@"%d%%", music]];
    self.musicVolumeValue.alignment = NSTextAlignmentRight;
    NSStackView* musicHeader = [NSStackView stackViewWithViews:@[
        [NSTextField labelWithString:@"Music volume"],
        self.musicVolumeValue,
    ]];
    musicHeader.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    musicHeader.distribution = NSStackViewDistributionFill;
    NSSlider* musicSlider = [NSSlider sliderWithValue:music
                                             minValue:0
                                             maxValue:100
                                               target:self
                                               action:@selector(musicVolumeChanged:)];
    musicSlider.continuous = YES;
    musicSlider.accessibilityLabel = @"Music volume";

    const int sound = Ratouch_Apple_App_Setting(RATOUCH_SETTING_SOUND_VOLUME);
    self.soundVolumeValue = [NSTextField labelWithString:[NSString stringWithFormat:@"%d%%", sound]];
    self.soundVolumeValue.alignment = NSTextAlignmentRight;
    NSStackView* soundHeader = [NSStackView stackViewWithViews:@[
        [NSTextField labelWithString:@"Sound volume"],
        self.soundVolumeValue,
    ]];
    soundHeader.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    soundHeader.distribution = NSStackViewDistributionFill;
    NSSlider* soundSlider = [NSSlider sliderWithValue:sound
                                             minValue:0
                                             maxValue:100
                                               target:self
                                               action:@selector(soundVolumeChanged:)];
    soundSlider.continuous = YES;
    soundSlider.accessibilityLabel = @"Sound volume";

    NSTextField* inputHeading = [NSTextField labelWithString:@"Input"];
    inputHeading.font = [NSFont boldSystemFontOfSize:NSFont.systemFontSize];

    const int currentSpeed = Ratouch_Mac_Pointer_Sensitivity();
    NSTextField* pointerLabel = [NSTextField labelWithString:@"Pointer speed"];
    self.pointerSpeedValue = [NSTextField labelWithString:[NSString stringWithFormat:@"%d%%", currentSpeed]];
    self.pointerSpeedValue.alignment = NSTextAlignmentRight;
    [self.pointerSpeedValue.widthAnchor constraintEqualToConstant:52].active = YES;
    NSStackView* pointerHeader = [NSStackView stackViewWithViews:@[pointerLabel, self.pointerSpeedValue]];
    pointerHeader.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    pointerHeader.distribution = NSStackViewDistributionFill;

    NSSlider* pointerSpeed = [NSSlider sliderWithValue:currentSpeed
                                              minValue:25
                                              maxValue:200
                                                target:self
                                                action:@selector(pointerSpeedChanged:)];
    pointerSpeed.numberOfTickMarks = 8;
    pointerSpeed.allowsTickMarkValuesOnly = NO;
    pointerSpeed.continuous = YES;
    pointerSpeed.accessibilityLabel = @"Pointer speed";

    NSTextField* dataHeading = [NSTextField labelWithString:@"Data"];
    dataHeading.font = [NSFont boldSystemFontOfSize:NSFont.systemFontSize];
    NSString* dataStatus = Ratouch_Asset_Provenance_Summary();
    if (Ratouch_Has_Pending_Asset_Import()) {
        dataStatus = [dataStatus stringByAppendingString:@"\nValidated replacement ready for next launch"];
    }
    NSTextField* dataSummary = [NSTextField wrappingLabelWithString:dataStatus];
    dataSummary.textColor = NSColor.secondaryLabelColor;
    dataSummary.accessibilityLabel = [NSString stringWithFormat:@"Imported source: %@", dataStatus];
    [dataSummary.widthAnchor constraintEqualToConstant:380].active = YES;

    NSButton* showData = [NSButton buttonWithTitle:@"Show in Finder"
                                            target:self
                                            action:@selector(showGameData:)];
    NSButton* replaceData = [NSButton buttonWithTitle:@"Replace on next launch"
                                               target:self
                                               action:@selector(stageGameData:)];
    replaceData.toolTip = @"Validate a complete replacement now and activate it safely at the next cold launch";
    NSStackView* dataActions = [NSStackView stackViewWithViews:@[showData, replaceData]];
    dataActions.orientation = NSUserInterfaceLayoutOrientationHorizontal;
    dataActions.distribution = NSStackViewDistributionFillEqually;
    dataActions.spacing = 8;

    NSStackView* controls = [NSStackView stackViewWithViews:@[
        displayHeading,
        scaleFilterRow,
        aspectRow,
        audioHeading,
        musicHeader,
        musicSlider,
        soundHeader,
        soundSlider,
        inputHeading,
        pointerHeader,
        pointerSpeed,
        dataHeading,
        dataSummary,
        dataActions,
    ]];
    controls.orientation = NSUserInterfaceLayoutOrientationVertical;
    controls.alignment = NSLayoutAttributeLeading;
    controls.spacing = 8.0;
    controls.frame = NSMakeRect(0, 0, 380, 360);
    [scaleFilterRow.widthAnchor constraintEqualToConstant:380].active = YES;
    [aspectRow.widthAnchor constraintEqualToConstant:380].active = YES;
    [musicHeader.widthAnchor constraintEqualToConstant:380].active = YES;
    [musicSlider.widthAnchor constraintEqualToConstant:380].active = YES;
    [soundHeader.widthAnchor constraintEqualToConstant:380].active = YES;
    [soundSlider.widthAnchor constraintEqualToConstant:380].active = YES;
    [pointerHeader.widthAnchor constraintEqualToConstant:380].active = YES;
    [pointerSpeed.widthAnchor constraintEqualToConstant:380].active = YES;
    [dataActions.widthAnchor constraintEqualToConstant:380].active = YES;
    alert.accessoryView = controls;

    [alert addButtonWithTitle:@"Done"];
    [alert addButtonWithTitle:@"View Input Guide"];
    const NSModalResponse response = [alert runModal];
    Ratouch_Mac_Set_Pointer_Sensitivity((int)pointerSpeed.integerValue);
    if (response == NSAlertSecondButtonReturn) {
        [self openInputGuide:nil];
    }
    self.pointerSpeedValue = nil;
    self.musicVolumeValue = nil;
    self.soundVolumeValue = nil;
}

- (void)pointerSpeedChanged:(NSSlider*)sender
{
    const int speed = (int)sender.integerValue;
    Ratouch_Mac_Set_Pointer_Sensitivity(speed);
    self.pointerSpeedValue.stringValue = [NSString stringWithFormat:@"%d%%", speed];
}

- (void)scaleFilterChanged:(NSSegmentedControl*)sender
{
    Ratouch_Apple_Request_App_Setting(RATOUCH_SETTING_SCALE_FILTER, (int)sender.selectedSegment);
}

- (void)aspectChanged:(NSSegmentedControl*)sender
{
    Ratouch_Apple_Request_App_Setting(RATOUCH_SETTING_ASPECT_MODE, (int)sender.selectedSegment);
}

- (void)musicVolumeChanged:(NSSlider*)sender
{
    const int volume = (int)sender.integerValue;
    self.musicVolumeValue.stringValue = [NSString stringWithFormat:@"%d%%", volume];
    Ratouch_Apple_Request_App_Setting(RATOUCH_SETTING_MUSIC_VOLUME, volume);
}

- (void)soundVolumeChanged:(NSSlider*)sender
{
    const int volume = (int)sender.integerValue;
    self.soundVolumeValue.stringValue = [NSString stringWithFormat:@"%d%%", volume];
    Ratouch_Apple_Request_App_Setting(RATOUCH_SETTING_SOUND_VOLUME, volume);
}

- (void)toggleFullScreen:(id)sender
{
    (void)sender;
    Toggle_Video_Fullscreen();
}

- (void)showGameData:(id)sender
{
    (void)sender;
    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[Ratouch_Asset_Root()]];
}

- (void)stageGameData:(id)sender
{
    (void)sender;
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    panel.title = @"Choose replacement Red Alert game data";
    panel.message = @"Select a complete MIX set, supported ISO, or folder. RAtouch validates it now and keeps the running game unchanged until the next cold launch.";
    panel.prompt = @"Validate Replacement";
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = YES;
    panel.allowsMultipleSelection = YES;
    panel.resolvesAliases = YES;
    if ([panel runModal] != NSModalResponseOK) {
        return;
    }

    NSError* error = nil;
    const NSUInteger imported = Ratouch_Stage_Asset_URLs(panel.URLs, &error);
    NSAlert* result = [[NSAlert alloc] init];
    if (imported > 0 && Ratouch_Has_Pending_Asset_Import()) {
        result.messageText = @"Replacement ready for next launch";
        result.informativeText = [NSString stringWithFormat:@"Validated %lu file%@. Current saves, settings, and running assets remain untouched until RAtouch is launched again.",
                                                            (unsigned long)imported,
                                                            imported == 1 ? @"" : @"s"];
        result.alertStyle = NSAlertStyleInformational;
    } else {
        result.messageText = @"Replacement not staged";
        result.informativeText = error.localizedDescription
                                     ?: @"Choose a complete REDALERT.MIX and base-game MAIN.MIX set.";
        result.alertStyle = NSAlertStyleWarning;
    }
    [result addButtonWithTitle:@"Done"];
    [result runModal];
}

- (void)openSource:(id)sender
{
    (void)sender;
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://github.com/chrissotraidis/ratouch"]];
}

- (void)openInputGuide:(id)sender
{
    (void)sender;
    [[NSWorkspace sharedWorkspace]
        openURL:[NSURL URLWithString:@"https://github.com/chrissotraidis/ratouch/blob/main/docs/input-design.md"]];
}

@end

void Ratouch_Install_Mac_Shell()
{
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        static RatouchMacMenuController* controller = [[RatouchMacMenuController alloc] init];
        NSMenu* menuBar = [[NSMenu alloc] initWithTitle:@""];

        NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@"RAtouch"];
        NSMenuItem* appRoot = [[NSMenuItem alloc] initWithTitle:@"RAtouch" action:nil keyEquivalent:@""];
        appRoot.submenu = appMenu;
        [menuBar addItem:appRoot];

        NSMenuItem* about = [[NSMenuItem alloc] initWithTitle:@"About RAtouch"
                                                     action:@selector(showAbout:)
                                              keyEquivalent:@""];
        about.target = controller;
        [appMenu addItem:about];
        NSMenuItem* settings = [[NSMenuItem alloc] initWithTitle:@"Settings…"
                                                        action:@selector(showSettings:)
                                                 keyEquivalent:@","];
        settings.target = controller;
        [appMenu addItem:settings];
        [appMenu addItem:NSMenuItem.separatorItem];
        [appMenu addItem:[[NSMenuItem alloc] initWithTitle:@"Hide RAtouch"
                                                   action:@selector(hide:)
                                            keyEquivalent:@"h"]];
        NSMenuItem* hideOthers = [[NSMenuItem alloc] initWithTitle:@"Hide Others"
                                                           action:@selector(hideOtherApplications:)
                                                    keyEquivalent:@"h"];
        hideOthers.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagOption;
        [appMenu addItem:hideOthers];
        [appMenu addItem:[[NSMenuItem alloc] initWithTitle:@"Show All"
                                                   action:@selector(unhideAllApplications:)
                                            keyEquivalent:@""]];
        [appMenu addItem:NSMenuItem.separatorItem];
        NSMenuItem* quit = [[NSMenuItem alloc] initWithTitle:@"Quit RAtouch"
                                                     action:@selector(requestQuit:)
                                              keyEquivalent:@"q"];
        quit.target = controller;
        [appMenu addItem:quit];

        NSMenu* fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
        NSMenuItem* fileRoot = [[NSMenuItem alloc] initWithTitle:@"File" action:nil keyEquivalent:@""];
        fileRoot.submenu = fileMenu;
        [menuBar addItem:fileRoot];
        NSMenuItem* showData = [[NSMenuItem alloc] initWithTitle:@"Show Game Data in Finder"
                                                         action:@selector(showGameData:)
                                                  keyEquivalent:@""];
        showData.target = controller;
        [fileMenu addItem:showData];

        NSMenu* viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
        NSMenuItem* viewRoot = [[NSMenuItem alloc] initWithTitle:@"View" action:nil keyEquivalent:@""];
        viewRoot.submenu = viewMenu;
        [menuBar addItem:viewRoot];
        NSMenuItem* fullScreen = [[NSMenuItem alloc] initWithTitle:@"Toggle Full Screen"
                                                            action:@selector(toggleFullScreen:)
                                                     keyEquivalent:@"f"];
        fullScreen.target = controller;
        fullScreen.keyEquivalentModifierMask = NSEventModifierFlagControl | NSEventModifierFlagCommand;
        [viewMenu addItem:fullScreen];

        NSMenu* windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
        NSMenuItem* windowRoot = [[NSMenuItem alloc] initWithTitle:@"Window" action:nil keyEquivalent:@""];
        windowRoot.submenu = windowMenu;
        [menuBar addItem:windowRoot];
        [windowMenu addItem:[[NSMenuItem alloc] initWithTitle:@"Minimize"
                                                      action:@selector(performMiniaturize:)
                                               keyEquivalent:@"m"]];
        [windowMenu addItem:[[NSMenuItem alloc] initWithTitle:@"Zoom"
                                                      action:@selector(performZoom:)
                                               keyEquivalent:@""]];
        [NSApp setWindowsMenu:windowMenu];

        NSMenu* helpMenu = [[NSMenu alloc] initWithTitle:@"Help"];
        NSMenuItem* helpRoot = [[NSMenuItem alloc] initWithTitle:@"Help" action:nil keyEquivalent:@""];
        helpRoot.submenu = helpMenu;
        [menuBar addItem:helpRoot];
        NSMenuItem* inputGuide = [[NSMenuItem alloc] initWithTitle:@"RAtouch Input Guide"
                                                            action:@selector(openInputGuide:)
                                                     keyEquivalent:@""];
        inputGuide.target = controller;
        [helpMenu addItem:inputGuide];
        NSMenuItem* source = [[NSMenuItem alloc] initWithTitle:@"RAtouch Source Code"
                                                       action:@selector(openSource:)
                                                keyEquivalent:@""];
        source.target = controller;
        [helpMenu addItem:source];
        [NSApp setHelpMenu:helpMenu];

        [NSApp setMainMenu:menuBar];
        [NSApp activateIgnoringOtherApps:YES];
    }
}

bool Ratouch_Ensure_Mac_Game_Data()
{
    @autoreleasepool {
        NSError* pending_error = nil;
        if (!Ratouch_Apply_Pending_Asset_Import(&pending_error)) {
            NSLog(@"RAtouch kept the current game data because the pending replacement failed: %@",
                  pending_error.localizedDescription);
        }
        if (Ratouch_Has_Core_Data()) {
            return true;
        }

        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp activateIgnoringOtherApps:YES];

        while (!Ratouch_Has_Core_Data()) {
            NSAlert* welcome = [[NSAlert alloc] init];
            welcome.messageText = @"Bring your Red Alert game data";
            welcome.informativeText =
                @"RAtouch contains no commercial game assets. Choose legally acquired MIX files, an ISO, or a folder containing them. Your files stay in Application Support on this Mac.";
            welcome.alertStyle = NSAlertStyleInformational;
            [welcome addButtonWithTitle:@"Choose files or folder"];
            [welcome addButtonWithTitle:@"Quit"];
            if ([welcome runModal] != NSAlertFirstButtonReturn) {
                return false;
            }

            NSOpenPanel* panel = [NSOpenPanel openPanel];
            panel.title = @"Choose Red Alert game data";
            panel.message = @"Select MIX files, an ISO, or a folder containing the game data you legally own.";
            panel.prompt = @"Import";
            panel.canChooseFiles = YES;
            panel.canChooseDirectories = YES;
            panel.allowsMultipleSelection = YES;
            panel.resolvesAliases = YES;
            if ([panel runModal] != NSModalResponseOK) {
                continue;
            }

            NSError* error = nil;
            const NSUInteger imported = Ratouch_Import_Asset_URLs(panel.URLs, &error);
            if (imported == 0 || !Ratouch_Has_Core_Data()) {
                Present_Import_Error(error);
            }
        }

        return true;
    }
}
