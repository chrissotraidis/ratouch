#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <SDL.h>

#include <atomic>
#include <cmath>

#include "ios_controls.h"
#include "ios_modifier.h"
#include "ratouch_settings.h"
#import "apple/shared/importer.h"

namespace
{
UIWindow* Command_Window = nil;
RatouchModifierState Modifier_State;
NSString* const Command_Side_Defaults_Key = @"RatouchCommandTabOnRight";
NSString* const Long_Press_Defaults_Key = @"RatouchLongPressMilliseconds";
NSString* const Drag_Threshold_Defaults_Key = @"RatouchDragThreshold";
NSString* const Invert_Pan_Defaults_Key = @"RatouchInvertPan";
NSString* const Haptics_Defaults_Key = @"RatouchTouchHaptics";

constexpr uint64_t Long_Press_Values[] = {450, 600, 750};
constexpr float Drag_Threshold_Values[] = {6.0f, 8.0f, 12.0f};

std::atomic<uint64_t> Long_Press_Milliseconds{600};
std::atomic<float> Drag_Threshold{8.0f};
std::atomic<bool> Invert_Pan{false};
std::atomic<bool> Touch_Haptics{true};
std::atomic<bool> Sidebar_Visible{false};

NSUInteger Long_Press_Index(uint64_t milliseconds)
{
    for (NSUInteger index = 0; index < sizeof(Long_Press_Values) / sizeof(Long_Press_Values[0]); ++index) {
        if (Long_Press_Values[index] == milliseconds) {
            return index;
        }
    }
    return 1;
}

NSUInteger Drag_Threshold_Index(float threshold)
{
    for (NSUInteger index = 0; index < sizeof(Drag_Threshold_Values) / sizeof(Drag_Threshold_Values[0]); ++index) {
        if (Drag_Threshold_Values[index] == threshold) {
            return index;
        }
    }
    return 1;
}

void Push_Touch_Preferences_Changed()
{
    SDL_Event event = {};
    event.type = SDL_USEREVENT;
    event.user.code = RATOUCH_IOS_TOUCH_PREFERENCES_CHANGED;
    SDL_PushEvent(&event);
}

void Load_Touch_Preferences()
{
    NSUserDefaults* defaults = NSUserDefaults.standardUserDefaults;
    [defaults registerDefaults:@{
        Long_Press_Defaults_Key : @600,
        Drag_Threshold_Defaults_Key : @8,
        Invert_Pan_Defaults_Key : @NO,
        Haptics_Defaults_Key : @YES,
    }];
    const uint64_t long_press = static_cast<uint64_t>([defaults integerForKey:Long_Press_Defaults_Key]);
    const float drag_threshold = [defaults floatForKey:Drag_Threshold_Defaults_Key];
    Long_Press_Milliseconds.store(Long_Press_Values[Long_Press_Index(long_press)], std::memory_order_release);
    Drag_Threshold.store(Drag_Threshold_Values[Drag_Threshold_Index(drag_threshold)], std::memory_order_release);
    Invert_Pan.store([defaults boolForKey:Invert_Pan_Defaults_Key], std::memory_order_release);
    Touch_Haptics.store([defaults boolForKey:Haptics_Defaults_Key], std::memory_order_release);
    Push_Touch_Preferences_Changed();
}

void Push_Key_Event(SDL_Scancode scancode, bool release)
{
    SDL_Event event = {};
    event.type = release ? SDL_KEYUP : SDL_KEYDOWN;
    event.key.state = release ? SDL_RELEASED : SDL_PRESSED;
    event.key.repeat = 0;
    event.key.keysym.scancode = scancode;
    event.key.keysym.sym = SDL_GetKeyFromScancode(scancode);
    SDL_PushEvent(&event);
}

void Push_Key_Tap(SDL_Scancode scancode)
{
    Push_Key_Event(scancode, false);
    Push_Key_Event(scancode, true);
}

SDL_Scancode Modifier_Scancode(RatouchModifier modifier)
{
    switch (modifier) {
    case RatouchModifier::ForceAttack:
        return SDL_SCANCODE_LCTRL;
    case RatouchModifier::ForceMove:
        return SDL_SCANCODE_LALT;
    case RatouchModifier::AddSelection:
        return SDL_SCANCODE_LSHIFT;
    case RatouchModifier::QueueMove:
        return SDL_SCANCODE_Q;
    case RatouchModifier::None:
        return SDL_SCANCODE_UNKNOWN;
    }
    return SDL_SCANCODE_UNKNOWN;
}

void Refresh_Modifier_UI(RatouchModifier modifier);

void Toggle_One_Shot_Modifier(RatouchModifier modifier)
{
    const RatouchModifier previous = Modifier_State.Toggle(modifier);
    if (previous != RatouchModifier::None) {
        Push_Key_Event(Modifier_Scancode(previous), true);
    }
    const RatouchModifier active = Modifier_State.Active();
    if (active != RatouchModifier::None) {
        Push_Key_Event(Modifier_Scancode(active), false);
    }
    Refresh_Modifier_UI(active);
}
}

@interface RatouchPassthroughWindow : UIWindow
@end

@implementation RatouchPassthroughWindow

- (UIView*)hitTest:(CGPoint)point withEvent:(UIEvent*)event
{
    UIView* target = [super hitTest:point withEvent:event];
    return target == self.rootViewController.view ? nil : target;
}

@end

@interface RatouchAboutViewController : UIViewController
@end

@implementation RatouchAboutViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.systemBackgroundColor;
    self.preferredContentSize = CGSizeMake(700, 700);

    UILabel* title = [[UILabel alloc] init];
    title.text = @"RAtouch";
    title.font = [UIFont preferredFontForTextStyle:UIFontTextStyleLargeTitle];
    title.adjustsFontForContentSizeCategory = YES;
    title.numberOfLines = 0;

    UILabel* summary = [[UILabel alloc] init];
    summary.text = @"Active alpha • Engine and platform code only";
    summary.font = [UIFont preferredFontForTextStyle:UIFontTextStyleSubheadline];
    summary.adjustsFontForContentSizeCategory = YES;
    summary.textColor = UIColor.secondaryLabelColor;
    summary.numberOfLines = 0;

    NSString* licensePath = [NSBundle.mainBundle pathForResource:@"License" ofType:@"txt"];
    NSString* license = licensePath.length > 0
                            ? [NSString stringWithContentsOfFile:licensePath encoding:NSUTF8StringEncoding error:nil]
                            : nil;
    NSString* notice = @"RAtouch is an unofficial modified port. It contains no commercial game data and is not "
                        @"affiliated with, endorsed by, or supported by Electronic Arts. No trademark, publicity, "
                        @"or game-asset rights are granted. There is no warranty. You may convey the covered engine "
                        @"code under GPL-3.0 and the additional terms reproduced below.\n\n";

    UITextView* legal = [[UITextView alloc] init];
    legal.translatesAutoresizingMaskIntoConstraints = NO;
    legal.editable = NO;
    legal.selectable = YES;
    legal.alwaysBounceVertical = YES;
    legal.showsVerticalScrollIndicator = YES;
    legal.backgroundColor = UIColor.secondarySystemBackgroundColor;
    legal.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    legal.adjustsFontForContentSizeCategory = YES;
    legal.text = [notice stringByAppendingString:license ?: @"The bundled GPL-3.0 license text is unavailable."];
    legal.accessibilityLabel = @"RAtouch license and Electronic Arts additional terms";
    legal.layer.cornerRadius = 12;
    legal.textContainerInset = UIEdgeInsetsMake(16, 16, 16, 16);

    UIButton* source = [UIButton buttonWithType:UIButtonTypeSystem];
    [source setTitle:@"Get the source" forState:UIControlStateNormal];
    source.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    source.titleLabel.adjustsFontForContentSizeCategory = YES;
    source.accessibilityHint = @"Open the complete RAtouch source repository";
    [source addTarget:self action:@selector(openSource) forControlEvents:UIControlEventTouchUpInside];
    [source.heightAnchor constraintGreaterThanOrEqualToConstant:48].active = YES;

    UIButton* done = [UIButton buttonWithType:UIButtonTypeSystem];
    [done setTitle:@"Done" forState:UIControlStateNormal];
    done.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    done.titleLabel.adjustsFontForContentSizeCategory = YES;
    done.accessibilityLabel = @"Close about and license";
    done.accessibilityHint = @"Return to Controls";
    [done addTarget:self action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
    [done.heightAnchor constraintGreaterThanOrEqualToConstant:48].active = YES;

    UIStackView* header = [[UIStackView alloc] initWithArrangedSubviews:@[title, done]];
    header.axis = UILayoutConstraintAxisHorizontal;
    header.alignment = UIStackViewAlignmentCenter;
    header.spacing = 16;
    [done setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];

    UIStackView* actions = [[UIStackView alloc] initWithArrangedSubviews:@[source]];
    actions.axis = UILayoutConstraintAxisHorizontal;
    actions.distribution = UIStackViewDistributionFillEqually;

    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[header, summary, legal, actions]];
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 14;
    [self.view addSubview:stack];

    UILayoutGuide* safe = self.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [stack.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:24],
        [stack.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-24],
        [stack.topAnchor constraintEqualToAnchor:safe.topAnchor constant:20],
        [stack.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor constant:-20],
        [legal.heightAnchor constraintGreaterThanOrEqualToConstant:240],
    ]];
}

- (void)openSource
{
    NSURL* source = [NSURL URLWithString:@"https://github.com/chrissotraidis/ratouch"];
    [UIApplication.sharedApplication openURL:source options:@{} completionHandler:nil];
}

- (void)close
{
    [self dismissViewControllerAnimated:!UIAccessibilityIsReduceMotionEnabled() completion:nil];
}

@end

@interface RatouchDataViewController : UIViewController <UIDocumentPickerDelegate>
@property(nonatomic, strong) UILabel* statusLabel;
@property(nonatomic, strong) UIButton* exportButton;
@end

@implementation RatouchDataViewController

- (NSArray<NSURL*>*)saveURLs
{
    NSArray<NSURL*>* children = [[NSFileManager defaultManager] contentsOfDirectoryAtURL:Ratouch_Asset_Root()
                                                           includingPropertiesForKeys:nil
                                                                              options:NSDirectoryEnumerationSkipsHiddenFiles
                                                                                error:nil];
    NSPredicate* saves = [NSPredicate predicateWithBlock:^BOOL(NSURL* url, NSDictionary*) {
        return [url.lastPathComponent.lowercaseString hasPrefix:@"savegame."];
    }];
    return [[children filteredArrayUsingPredicate:saves]
        sortedArrayUsingComparator:^NSComparisonResult(NSURL* left, NSURL* right) {
            return [left.lastPathComponent localizedStandardCompare:right.lastPathComponent];
        }];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.systemBackgroundColor;
    self.preferredContentSize = CGSizeMake(620, 560);

    UILabel* title = [[UILabel alloc] init];
    title.text = @"Game data";
    title.font = [UIFont preferredFontForTextStyle:UIFontTextStyleLargeTitle];
    title.adjustsFontForContentSizeCategory = YES;
    title.numberOfLines = 0;

    UIButton* done = [UIButton buttonWithType:UIButtonTypeSystem];
    [done setTitle:@"Done" forState:UIControlStateNormal];
    done.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    done.titleLabel.adjustsFontForContentSizeCategory = YES;
    done.accessibilityLabel = @"Close game data";
    done.accessibilityHint = @"Return to Controls";
    [done addTarget:self action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
    [done.heightAnchor constraintGreaterThanOrEqualToConstant:48].active = YES;

    UIStackView* header = [[UIStackView alloc] initWithArrangedSubviews:@[title, done]];
    header.translatesAutoresizingMaskIntoConstraints = NO;
    header.axis = UILayoutConstraintAxisHorizontal;
    header.alignment = UIStackViewAlignmentCenter;
    header.spacing = 16;
    [done setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];
    [title setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                           forAxis:UILayoutConstraintAxisHorizontal];

    UILabel* sourceHeading = [[UILabel alloc] init];
    sourceHeading.text = @"Imported source";
    sourceHeading.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    sourceHeading.adjustsFontForContentSizeCategory = YES;

    UILabel* source = [[UILabel alloc] init];
    source.text = Ratouch_Asset_Provenance_Summary();
    source.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    source.adjustsFontForContentSizeCategory = YES;
    source.numberOfLines = 0;
    source.accessibilityLabel = [NSString stringWithFormat:@"Imported source: %@", source.text];

    UILabel* privacy = [[UILabel alloc] init];
    privacy.text = @"Your legally acquired files stay in this app's private Application Support folder. They are never bundled with RAtouch or uploaded.";
    privacy.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    privacy.adjustsFontForContentSizeCategory = YES;
    privacy.textColor = UIColor.secondaryLabelColor;
    privacy.numberOfLines = 0;

    UIButton* replace = [UIButton buttonWithType:UIButtonTypeSystem];
    [replace setTitle:@"Replace on next launch" forState:UIControlStateNormal];
    replace.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    replace.titleLabel.adjustsFontForContentSizeCategory = YES;
    replace.accessibilityHint = @"Choose a complete MIX, folder, or ISO set to validate without changing the running game";
    [replace addTarget:self action:@selector(chooseReplacement) forControlEvents:UIControlEventTouchUpInside];
    [replace.heightAnchor constraintGreaterThanOrEqualToConstant:50].active = YES;

    self.exportButton = [UIButton buttonWithType:UIButtonTypeSystem];
    const NSUInteger saveCount = self.saveURLs.count;
    [self.exportButton setTitle:saveCount == 0
                                    ? @"No saves to export"
                                    : [NSString stringWithFormat:@"Export %lu save%@ to Files",
                                                                      (unsigned long)saveCount,
                                                                      saveCount == 1 ? @"" : @"s"]
                       forState:UIControlStateNormal];
    self.exportButton.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    self.exportButton.titleLabel.adjustsFontForContentSizeCategory = YES;
    self.exportButton.enabled = saveCount > 0;
    self.exportButton.accessibilityHint = @"Choose a Files location and copy the current Red Alert saves there";
    [self.exportButton addTarget:self action:@selector(exportSaves) forControlEvents:UIControlEventTouchUpInside];
    [self.exportButton.heightAnchor constraintGreaterThanOrEqualToConstant:50].active = YES;

    self.statusLabel = [[UILabel alloc] init];
    self.statusLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleFootnote];
    self.statusLabel.adjustsFontForContentSizeCategory = YES;
    self.statusLabel.numberOfLines = 0;
    self.statusLabel.textColor = UIColor.secondaryLabelColor;
    self.statusLabel.text = Ratouch_Has_Pending_Asset_Import()
                                ? @"A validated replacement is ready and will become active on the next cold launch. Current saves and settings will be preserved."
                                : @"Replacement data is staged separately. The running game, saves, and settings remain untouched until the next cold launch.";

    UIStackView* actions = [[UIStackView alloc] initWithArrangedSubviews:@[replace, self.exportButton]];
    actions.axis = UILayoutConstraintAxisVertical;
    actions.distribution = UIStackViewDistributionFillEqually;
    actions.spacing = 10;

    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[
        sourceHeading, source, privacy, actions, self.statusLabel
    ]];
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 16;

    UIScrollView* scroll = [[UIScrollView alloc] init];
    scroll.translatesAutoresizingMaskIntoConstraints = NO;
    scroll.alwaysBounceVertical = YES;
    scroll.showsVerticalScrollIndicator = YES;
    [scroll addSubview:stack];
    [self.view addSubview:header];
    [self.view addSubview:scroll];

    UILayoutGuide* safe = self.view.safeAreaLayoutGuide;
    UILayoutGuide* content = scroll.contentLayoutGuide;
    UILayoutGuide* frame = scroll.frameLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [scroll.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor],
        [scroll.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor],
        [header.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:28],
        [header.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-28],
        [header.topAnchor constraintEqualToAnchor:safe.topAnchor constant:16],
        [scroll.topAnchor constraintEqualToAnchor:header.bottomAnchor constant:8],
        [scroll.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor],
        [stack.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:28],
        [stack.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-28],
        [stack.topAnchor constraintEqualToAnchor:content.topAnchor constant:8],
        [stack.bottomAnchor constraintEqualToAnchor:content.bottomAnchor constant:-24],
        [stack.widthAnchor constraintEqualToAnchor:frame.widthAnchor constant:-56],
    ]];
}

- (void)chooseReplacement
{
    NSArray<UTType*>* types = @[UTTypeData, UTTypeFolder, UTTypeDiskImage];
    UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:types
                                                                                                       asCopy:YES];
    picker.delegate = self;
    picker.allowsMultipleSelection = YES;
    [self presentViewController:picker animated:!UIAccessibilityIsReduceMotionEnabled() completion:nil];
}

- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    NSMutableArray<NSURL*>* accessible = [NSMutableArray array];
    for (NSURL* url in urls) {
        [url startAccessingSecurityScopedResource];
        [accessible addObject:url];
    }
    NSError* error = nil;
    const NSUInteger count = Ratouch_Stage_Asset_URLs(accessible, &error);
    for (NSURL* url in accessible) {
        [url stopAccessingSecurityScopedResource];
    }
    if (count > 0 && Ratouch_Has_Pending_Asset_Import()) {
        self.statusLabel.textColor = UIColor.systemGreenColor;
        self.statusLabel.text = [NSString stringWithFormat:@"Validated %lu file%@. The replacement will become active on the next cold launch; current saves and settings will be preserved.",
                                                           (unsigned long)count,
                                                           count == 1 ? @"" : @"s"];
    } else {
        self.statusLabel.textColor = UIColor.systemOrangeColor;
        self.statusLabel.text = [NSString stringWithFormat:@"Replacement not staged: %@",
                                                           error.localizedDescription
                                                               ?: @"choose a complete REDALERT.MIX and base-game MAIN.MIX set."];
    }
    UIAccessibilityPostNotification(UIAccessibilityAnnouncementNotification, self.statusLabel.text);
}

- (void)exportSaves
{
    NSArray<NSURL*>* saves = self.saveURLs;
    if (saves.count == 0) {
        return;
    }
    UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForExportingURLs:saves
                                                                                                   asCopy:YES];
    [self presentViewController:picker animated:!UIAccessibilityIsReduceMotionEnabled() completion:nil];
}

- (void)close
{
    [self dismissViewControllerAnimated:!UIAccessibilityIsReduceMotionEnabled() completion:nil];
}

@end

@interface RatouchControlsViewController : UIViewController
@property(nonatomic, strong) UISegmentedControl* holdControl;
@property(nonatomic, strong) UISegmentedControl* dragControl;
@property(nonatomic, strong) UISwitch* panSwitch;
@property(nonatomic, strong) UISwitch* hapticsSwitch;
@property(nonatomic, strong) UISegmentedControl* scaleFilterControl;
@property(nonatomic, strong) UISegmentedControl* aspectControl;
@property(nonatomic, strong) UISlider* musicSlider;
@property(nonatomic, strong) UISlider* soundSlider;
@property(nonatomic, strong) UILabel* musicValue;
@property(nonatomic, strong) UILabel* soundValue;
@end

@implementation RatouchControlsViewController

- (UIView*)preferenceRowWithTitle:(NSString*)title control:(UIView*)control
{
    UILabel* label = [[UILabel alloc] init];
    label.text = title;
    label.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    label.adjustsFontForContentSizeCategory = YES;
    label.numberOfLines = 0;
    UIStackView* row = [[UIStackView alloc] initWithArrangedSubviews:@[label, control]];
    row.axis = UILayoutConstraintAxisHorizontal;
    row.alignment = UIStackViewAlignmentCenter;
    row.distribution = UIStackViewDistributionFill;
    row.spacing = 16;
    [row.heightAnchor constraintGreaterThanOrEqualToConstant:48].active = YES;
    return row;
}

- (UIView*)sliderRowWithTitle:(NSString*)title slider:(UISlider*)slider valueLabel:(UILabel*)valueLabel
{
    UILabel* label = [[UILabel alloc] init];
    label.text = title;
    label.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    label.adjustsFontForContentSizeCategory = YES;
    label.numberOfLines = 0;

    valueLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    valueLabel.adjustsFontForContentSizeCategory = YES;
    valueLabel.textColor = UIColor.secondaryLabelColor;
    [valueLabel setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];

    UIStackView* header = [[UIStackView alloc] initWithArrangedSubviews:@[label, valueLabel]];
    header.axis = UILayoutConstraintAxisHorizontal;
    header.distribution = UIStackViewDistributionFill;
    header.spacing = 12;

    UIStackView* row = [[UIStackView alloc] initWithArrangedSubviews:@[header, slider]];
    row.axis = UILayoutConstraintAxisVertical;
    row.spacing = 4;
    [row.heightAnchor constraintGreaterThanOrEqualToConstant:64].active = YES;
    return row;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.systemBackgroundColor;
    self.preferredContentSize = CGSizeMake(620, 680);

    UILabel* title = [[UILabel alloc] init];
    title.text = @"Controls";
    title.font = [UIFont preferredFontForTextStyle:UIFontTextStyleLargeTitle];
    title.adjustsFontForContentSizeCategory = YES;

    UILabel* guide = [[UILabel alloc] init];
    guide.numberOfLines = 0;
    guide.text = @"Tap — select, order, or use the original UI\nDrag one finger — select a group\nHold one finger — deselect / right-click\nDrag two fingers — scroll the map\nPinch — zoom the presentation\nDouble-tap with two fingers — reset zoom";
    guide.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    guide.adjustsFontForContentSizeCategory = YES;
    guide.textColor = UIColor.secondaryLabelColor;

    UILabel* externalInput = [[UILabel alloc] init];
    externalInput.text = @"Keyboard & pointer";
    externalInput.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    externalInput.adjustsFontForContentSizeCategory = YES;

    UILabel* externalInputGuide = [[UILabel alloc] init];
    externalInputGuide.numberOfLines = 0;
    externalInputGuide.text = @"Hardware keyboard — original shortcuts\nTrackpad or mouse — point, click, drag, and right-click";
    externalInputGuide.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    externalInputGuide.adjustsFontForContentSizeCategory = YES;
    externalInputGuide.textColor = UIColor.secondaryLabelColor;

    UILabel* tuning = [[UILabel alloc] init];
    tuning.text = @"Tuning";
    tuning.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    tuning.adjustsFontForContentSizeCategory = YES;

    UILabel* displayAndAudio = [[UILabel alloc] init];
    displayAndAudio.text = @"Display & audio";
    displayAndAudio.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    displayAndAudio.adjustsFontForContentSizeCategory = YES;

    self.scaleFilterControl = [[UISegmentedControl alloc] initWithItems:@[@"Sharp", @"Smooth"]];
    self.scaleFilterControl.selectedSegmentIndex = Ratouch_Apple_App_Setting(RATOUCH_SETTING_SCALE_FILTER);
    self.scaleFilterControl.accessibilityLabel = @"Display filter";
    [self.scaleFilterControl addTarget:self
                                action:@selector(scaleFilterChanged:)
                      forControlEvents:UIControlEventValueChanged];

    self.aspectControl = [[UISegmentedControl alloc] initWithItems:@[@"Preserve", @"Fill"]];
    self.aspectControl.selectedSegmentIndex = Ratouch_Apple_App_Setting(RATOUCH_SETTING_ASPECT_MODE);
    self.aspectControl.accessibilityLabel = @"Aspect handling";
    [self.aspectControl addTarget:self action:@selector(aspectChanged:) forControlEvents:UIControlEventValueChanged];

    const int musicVolume = Ratouch_Apple_App_Setting(RATOUCH_SETTING_MUSIC_VOLUME);
    self.musicValue = [[UILabel alloc] init];
    self.musicValue.text = [NSString stringWithFormat:@"%d%%", musicVolume];
    self.musicSlider = [[UISlider alloc] init];
    self.musicSlider.minimumValue = 0;
    self.musicSlider.maximumValue = 100;
    self.musicSlider.value = musicVolume;
    self.musicSlider.continuous = YES;
    self.musicSlider.accessibilityLabel = @"Music volume";
    self.musicSlider.accessibilityValue = self.musicValue.text;
    [self.musicSlider addTarget:self action:@selector(musicChanged:) forControlEvents:UIControlEventValueChanged];

    const int soundVolume = Ratouch_Apple_App_Setting(RATOUCH_SETTING_SOUND_VOLUME);
    self.soundValue = [[UILabel alloc] init];
    self.soundValue.text = [NSString stringWithFormat:@"%d%%", soundVolume];
    self.soundSlider = [[UISlider alloc] init];
    self.soundSlider.minimumValue = 0;
    self.soundSlider.maximumValue = 100;
    self.soundSlider.value = soundVolume;
    self.soundSlider.continuous = YES;
    self.soundSlider.accessibilityLabel = @"Sound volume";
    self.soundSlider.accessibilityValue = self.soundValue.text;
    [self.soundSlider addTarget:self action:@selector(soundChanged:) forControlEvents:UIControlEventValueChanged];

    self.holdControl = [[UISegmentedControl alloc] initWithItems:@[@"Short", @"Balanced", @"Long"]];
    self.holdControl.selectedSegmentIndex = Long_Press_Index(Long_Press_Milliseconds.load(std::memory_order_acquire));
    self.holdControl.accessibilityLabel = @"Hold duration";
    [self.holdControl addTarget:self action:@selector(holdChanged:) forControlEvents:UIControlEventValueChanged];

    self.dragControl = [[UISegmentedControl alloc] initWithItems:@[@"Tight", @"Balanced", @"Relaxed"]];
    self.dragControl.selectedSegmentIndex = Drag_Threshold_Index(Drag_Threshold.load(std::memory_order_acquire));
    self.dragControl.accessibilityLabel = @"Drag threshold";
    [self.dragControl addTarget:self action:@selector(dragChanged:) forControlEvents:UIControlEventValueChanged];

    self.panSwitch = [[UISwitch alloc] init];
    self.panSwitch.on = Invert_Pan.load(std::memory_order_acquire);
    self.panSwitch.accessibilityLabel = @"Invert two-finger pan";
    [self.panSwitch addTarget:self action:@selector(panChanged:) forControlEvents:UIControlEventValueChanged];

    self.hapticsSwitch = [[UISwitch alloc] init];
    self.hapticsSwitch.on = Touch_Haptics.load(std::memory_order_acquire);
    self.hapticsSwitch.accessibilityLabel = @"Long-press haptic";
    [self.hapticsSwitch addTarget:self action:@selector(hapticsChanged:) forControlEvents:UIControlEventValueChanged];

    UIButton* data = [UIButton buttonWithType:UIButtonTypeSystem];
    [data setTitle:@"Game data" forState:UIControlStateNormal];
    data.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    data.titleLabel.adjustsFontForContentSizeCategory = YES;
    data.accessibilityHint = @"Show the imported source, stage replacement data, or export saves to Files";
    [data addTarget:self action:@selector(showData) forControlEvents:UIControlEventTouchUpInside];
    [data.heightAnchor constraintGreaterThanOrEqualToConstant:48].active = YES;

    UIButton* about = [UIButton buttonWithType:UIButtonTypeSystem];
    [about setTitle:@"About & license" forState:UIControlStateNormal];
    about.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    about.titleLabel.adjustsFontForContentSizeCategory = YES;
    about.accessibilityHint = @"Show credits, the complete license, Electronic Arts additional terms, and source link";
    [about addTarget:self action:@selector(showAbout) forControlEvents:UIControlEventTouchUpInside];
    [about.heightAnchor constraintGreaterThanOrEqualToConstant:48].active = YES;

    UIButton* done = [UIButton buttonWithType:UIButtonTypeSystem];
    [done setTitle:@"Done" forState:UIControlStateNormal];
    done.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    done.titleLabel.adjustsFontForContentSizeCategory = YES;
    done.accessibilityLabel = @"Close controls";
    done.accessibilityHint = @"Return to the running game";
    [done addTarget:self action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
    [done.heightAnchor constraintGreaterThanOrEqualToConstant:48].active = YES;

    UIStackView* header = [[UIStackView alloc] initWithArrangedSubviews:@[title, done]];
    header.translatesAutoresizingMaskIntoConstraints = NO;
    header.axis = UILayoutConstraintAxisHorizontal;
    header.alignment = UIStackViewAlignmentCenter;
    header.spacing = 16;
    [done setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];
    [title setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                           forAxis:UILayoutConstraintAxisHorizontal];

    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[
        guide,
        externalInput,
        externalInputGuide,
        displayAndAudio,
        [self preferenceRowWithTitle:@"Display filter" control:self.scaleFilterControl],
        [self preferenceRowWithTitle:@"Aspect handling" control:self.aspectControl],
        [self sliderRowWithTitle:@"Music volume" slider:self.musicSlider valueLabel:self.musicValue],
        [self sliderRowWithTitle:@"Sound volume" slider:self.soundSlider valueLabel:self.soundValue],
        tuning,
        [self preferenceRowWithTitle:@"Hold duration" control:self.holdControl],
        [self preferenceRowWithTitle:@"Drag threshold" control:self.dragControl],
        [self preferenceRowWithTitle:@"Invert two-finger pan" control:self.panSwitch],
        [self preferenceRowWithTitle:@"Long-press haptic" control:self.hapticsSwitch],
        data,
        about,
    ]];
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 14;

    UIScrollView* scroll = [[UIScrollView alloc] init];
    scroll.translatesAutoresizingMaskIntoConstraints = NO;
    scroll.alwaysBounceVertical = YES;
    scroll.showsVerticalScrollIndicator = YES;
    [scroll addSubview:stack];
    [self.view addSubview:header];
    [self.view addSubview:scroll];

    UILayoutGuide* safe = self.view.safeAreaLayoutGuide;
    UILayoutGuide* content = scroll.contentLayoutGuide;
    UILayoutGuide* frame = scroll.frameLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [scroll.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor],
        [scroll.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor],
        [header.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:28],
        [header.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-28],
        [header.topAnchor constraintEqualToAnchor:safe.topAnchor constant:16],
        [scroll.topAnchor constraintEqualToAnchor:header.bottomAnchor constant:8],
        [scroll.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor],
        [stack.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:28],
        [stack.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-28],
        [stack.topAnchor constraintEqualToAnchor:content.topAnchor constant:8],
        [stack.bottomAnchor constraintEqualToAnchor:content.bottomAnchor constant:-24],
        [stack.widthAnchor constraintEqualToAnchor:frame.widthAnchor constant:-56],
    ]];
}

- (void)scaleFilterChanged:(UISegmentedControl*)sender
{
    Ratouch_Apple_Request_App_Setting(RATOUCH_SETTING_SCALE_FILTER, (int)sender.selectedSegmentIndex);
}

- (void)aspectChanged:(UISegmentedControl*)sender
{
    Ratouch_Apple_Request_App_Setting(RATOUCH_SETTING_ASPECT_MODE, (int)sender.selectedSegmentIndex);
}

- (void)musicChanged:(UISlider*)sender
{
    const int value = (int)lroundf(sender.value);
    self.musicValue.text = [NSString stringWithFormat:@"%d%%", value];
    sender.accessibilityValue = self.musicValue.text;
    Ratouch_Apple_Request_App_Setting(RATOUCH_SETTING_MUSIC_VOLUME, value);
}

- (void)soundChanged:(UISlider*)sender
{
    const int value = (int)lroundf(sender.value);
    self.soundValue.text = [NSString stringWithFormat:@"%d%%", value];
    sender.accessibilityValue = self.soundValue.text;
    Ratouch_Apple_Request_App_Setting(RATOUCH_SETTING_SOUND_VOLUME, value);
}

- (void)holdChanged:(UISegmentedControl*)sender
{
    const uint64_t value = Long_Press_Values[sender.selectedSegmentIndex];
    Long_Press_Milliseconds.store(value, std::memory_order_release);
    [NSUserDefaults.standardUserDefaults setInteger:value forKey:Long_Press_Defaults_Key];
    Push_Touch_Preferences_Changed();
}

- (void)dragChanged:(UISegmentedControl*)sender
{
    const float value = Drag_Threshold_Values[sender.selectedSegmentIndex];
    Drag_Threshold.store(value, std::memory_order_release);
    [NSUserDefaults.standardUserDefaults setFloat:value forKey:Drag_Threshold_Defaults_Key];
    Push_Touch_Preferences_Changed();
}

- (void)panChanged:(UISwitch*)sender
{
    Invert_Pan.store(sender.on, std::memory_order_release);
    [NSUserDefaults.standardUserDefaults setBool:sender.on forKey:Invert_Pan_Defaults_Key];
    Push_Touch_Preferences_Changed();
}

- (void)hapticsChanged:(UISwitch*)sender
{
    Touch_Haptics.store(sender.on, std::memory_order_release);
    [NSUserDefaults.standardUserDefaults setBool:sender.on forKey:Haptics_Defaults_Key];
}

- (void)showAbout
{
    RatouchAboutViewController* about = [[RatouchAboutViewController alloc] init];
    about.modalPresentationStyle = UIModalPresentationFormSheet;
    [self presentViewController:about animated:!UIAccessibilityIsReduceMotionEnabled() completion:nil];
}

- (void)showData
{
    RatouchDataViewController* data = [[RatouchDataViewController alloc] init];
    data.modalPresentationStyle = UIModalPresentationFormSheet;
    [self presentViewController:data animated:!UIAccessibilityIsReduceMotionEnabled() completion:nil];
}

- (void)close
{
    [self dismissViewControllerAnimated:!UIAccessibilityIsReduceMotionEnabled() completion:nil];
}

@end

@interface RatouchGroupsViewController : UIViewController
@property(nonatomic, strong) UISegmentedControl* actionControl;
@property(nonatomic, strong) NSArray<UIButton*>* groupButtons;
@end

@implementation RatouchGroupsViewController

- (SDL_Scancode)scancodeForGroup:(NSInteger)group
{
    static const SDL_Scancode scancodes[] = {
        SDL_SCANCODE_1,
        SDL_SCANCODE_2,
        SDL_SCANCODE_3,
        SDL_SCANCODE_4,
        SDL_SCANCODE_5,
        SDL_SCANCODE_6,
        SDL_SCANCODE_7,
        SDL_SCANCODE_8,
        SDL_SCANCODE_9,
        SDL_SCANCODE_0,
    };
    return group >= 0 && group < 10 ? scancodes[group] : SDL_SCANCODE_UNKNOWN;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.systemBackgroundColor;
    self.preferredContentSize = CGSizeMake(560, 430);

    UILabel* title = [[UILabel alloc] init];
    title.text = @"Control groups";
    title.font = [UIFont preferredFontForTextStyle:UIFontTextStyleLargeTitle];
    title.adjustsFontForContentSizeCategory = YES;
    title.numberOfLines = 0;

    UILabel* guide = [[UILabel alloc] init];
    guide.numberOfLines = 0;
    guide.text = @"Recall switches to a saved formation. Assign selected replaces a slot with the units selected now.";
    guide.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    guide.adjustsFontForContentSizeCategory = YES;
    guide.textColor = UIColor.secondaryLabelColor;

    self.actionControl = [[UISegmentedControl alloc] initWithItems:@[@"Recall", @"Assign selected"]];
    self.actionControl.selectedSegmentIndex = 0;
    self.actionControl.accessibilityLabel = @"Control group action";
    [self.actionControl addTarget:self action:@selector(actionChanged) forControlEvents:UIControlEventValueChanged];

    NSMutableArray<UIButton*>* buttons = [[NSMutableArray alloc] initWithCapacity:10];
    NSMutableArray<UIStackView*>* rows = [[NSMutableArray alloc] initWithCapacity:2];
    for (NSInteger rowIndex = 0; rowIndex < 2; ++rowIndex) {
        NSMutableArray<UIButton*>* rowButtons = [[NSMutableArray alloc] initWithCapacity:5];
        for (NSInteger column = 0; column < 5; ++column) {
            const NSInteger group = rowIndex * 5 + column;
            UIButton* button = [UIButton buttonWithType:UIButtonTypeSystem];
            button.tag = group;
            [button setTitle:group == 9 ? @"0" : [NSString stringWithFormat:@"%ld", (long)group + 1]
                  forState:UIControlStateNormal];
            button.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleTitle2];
            button.titleLabel.adjustsFontForContentSizeCategory = YES;
            button.backgroundColor = UIColor.secondarySystemBackgroundColor;
            button.layer.cornerRadius = 10;
            button.layer.borderWidth = 1;
            button.layer.borderColor = UIColor.separatorColor.CGColor;
            [button.heightAnchor constraintGreaterThanOrEqualToConstant:56].active = YES;
            [button addTarget:self action:@selector(performGroup:) forControlEvents:UIControlEventTouchUpInside];
            [rowButtons addObject:button];
            [buttons addObject:button];
        }
        UIStackView* row = [[UIStackView alloc] initWithArrangedSubviews:rowButtons];
        row.axis = UILayoutConstraintAxisHorizontal;
        row.distribution = UIStackViewDistributionFillEqually;
        row.spacing = 10;
        [rows addObject:row];
    }
    self.groupButtons = buttons;
    [self actionChanged];

    UIStackView* grid = [[UIStackView alloc] initWithArrangedSubviews:rows];
    grid.axis = UILayoutConstraintAxisVertical;
    grid.distribution = UIStackViewDistributionFillEqually;
    grid.spacing = 10;

    UIButton* done = [UIButton buttonWithType:UIButtonTypeSystem];
    [done setTitle:@"Done" forState:UIControlStateNormal];
    done.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    done.titleLabel.adjustsFontForContentSizeCategory = YES;
    done.accessibilityLabel = @"Close control groups";
    done.accessibilityHint = @"Return to the running game";
    [done addTarget:self action:@selector(close) forControlEvents:UIControlEventTouchUpInside];
    [done.heightAnchor constraintGreaterThanOrEqualToConstant:48].active = YES;

    UIStackView* header = [[UIStackView alloc] initWithArrangedSubviews:@[title, done]];
    header.translatesAutoresizingMaskIntoConstraints = NO;
    header.axis = UILayoutConstraintAxisHorizontal;
    header.alignment = UIStackViewAlignmentCenter;
    header.distribution = UIStackViewDistributionFill;
    header.spacing = 16;
    [done setContentHuggingPriority:UILayoutPriorityRequired forAxis:UILayoutConstraintAxisHorizontal];
    [title setContentCompressionResistancePriority:UILayoutPriorityDefaultLow
                                           forAxis:UILayoutConstraintAxisHorizontal];

    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[
        self.actionControl,
        grid,
        guide,
    ]];
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 16;

    UIScrollView* scroll = [[UIScrollView alloc] init];
    scroll.translatesAutoresizingMaskIntoConstraints = NO;
    scroll.alwaysBounceVertical = YES;
    scroll.showsVerticalScrollIndicator = YES;
    [scroll addSubview:stack];
    [self.view addSubview:header];
    [self.view addSubview:scroll];

    UILayoutGuide* safe = self.view.safeAreaLayoutGuide;
    UILayoutGuide* content = scroll.contentLayoutGuide;
    UILayoutGuide* frame = scroll.frameLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [scroll.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor],
        [scroll.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor],
        [header.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:28],
        [header.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-28],
        [header.topAnchor constraintEqualToAnchor:safe.topAnchor constant:16],
        [scroll.topAnchor constraintEqualToAnchor:header.bottomAnchor constant:8],
        [scroll.bottomAnchor constraintEqualToAnchor:safe.bottomAnchor],
        [stack.leadingAnchor constraintEqualToAnchor:content.leadingAnchor constant:28],
        [stack.trailingAnchor constraintEqualToAnchor:content.trailingAnchor constant:-28],
        [stack.topAnchor constraintEqualToAnchor:content.topAnchor constant:8],
        [stack.bottomAnchor constraintEqualToAnchor:content.bottomAnchor constant:-24],
        [stack.widthAnchor constraintEqualToAnchor:frame.widthAnchor constant:-56],
    ]];
}

- (void)actionChanged
{
    const BOOL assigning = self.actionControl.selectedSegmentIndex == 1;
    for (UIButton* button in self.groupButtons) {
        const NSInteger label = button.tag == 9 ? 0 : button.tag + 1;
        button.accessibilityLabel = [NSString stringWithFormat:@"Control group %ld", (long)label];
        button.accessibilityHint = assigning ? @"Replace this group with the currently selected units"
                                             : @"Recall this group and replace the current selection";
    }
}

- (void)performGroup:(UIButton*)sender
{
    const SDL_Scancode scancode = [self scancodeForGroup:sender.tag];
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        return;
    }
    const BOOL assigning = self.actionControl.selectedSegmentIndex == 1;
    Ratouch_iOS_Cancel_One_Shot_Modifier();
    if (assigning) {
        Push_Key_Event(SDL_SCANCODE_LCTRL, false);
    }
    Push_Key_Tap(scancode);
    if (assigning) {
        Push_Key_Event(SDL_SCANCODE_LCTRL, true);
    }
    const NSInteger label = sender.tag == 9 ? 0 : sender.tag + 1;
    UIAccessibilityPostNotification(
        UIAccessibilityAnnouncementNotification,
        [NSString stringWithFormat:assigning ? @"Assigned control group %ld" : @"Recalled control group %ld",
                                   (long)label]);
    [self close];
}

- (void)close
{
    [self dismissViewControllerAnimated:!UIAccessibilityIsReduceMotionEnabled() completion:nil];
}

@end

@interface RatouchCommandViewController : UIViewController
@property(nonatomic, strong) UIButton* commandButton;
@property(nonatomic, strong) UIView* palette;
@property(nonatomic, strong) UIButton* sideButton;
@property(nonatomic, strong) NSLayoutConstraint* commandLeadingConstraint;
@property(nonatomic, strong) NSLayoutConstraint* commandTrailingConstraint;
@property(nonatomic, strong) NSLayoutConstraint* paletteLeadingConstraint;
@property(nonatomic, strong) NSLayoutConstraint* paletteTrailingConstraint;
@property(nonatomic) BOOL commandOnRight;
@property(nonatomic) BOOL sidebarVisible;
- (void)closePalette;
- (void)refreshModifier:(RatouchModifier)modifier;
- (void)updateSideConstraints;
- (void)updateSidebarInset;
@end


@implementation RatouchCommandViewController

- (UIButton*)actionButtonWithTitle:(NSString*)title
                         subtitle:(NSString*)subtitle
                         scancode:(SDL_Scancode)scancode
{
    UIButton* button = [UIButton buttonWithType:UIButtonTypeSystem];
    button.translatesAutoresizingMaskIntoConstraints = NO;
    button.backgroundColor = [UIColor colorWithWhite:0.12 alpha:0.98];
    button.layer.cornerRadius = 10;
    button.layer.borderWidth = 1;
    button.layer.borderColor = [UIColor colorWithRed:0.94 green:0.23 blue:0.17 alpha:0.65].CGColor;
    [button setTitle:title forState:UIControlStateNormal];
    [button setTitleColor:[UIColor colorWithRed:0.96 green:0.91 blue:0.81 alpha:1.0]
                 forState:UIControlStateNormal];
    UIFont* baseFont = [UIFont systemFontOfSize:15 weight:UIFontWeightSemibold];
    button.titleLabel.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleBody]
        scaledFontForFont:baseFont
        maximumPointSize:20];
    button.titleLabel.adjustsFontForContentSizeCategory = YES;
    button.accessibilityLabel = title;
    button.accessibilityHint = subtitle;
    button.tag = (NSInteger)scancode;
    [button addTarget:self action:@selector(performCommand:) forControlEvents:UIControlEventTouchUpInside];
    return button;
}

- (UIButton*)modifierButtonWithTitle:(NSString*)title
                  accessibilityLabel:(NSString*)accessibilityLabel
                             modifier:(RatouchModifier)modifier
{
    UIButton* button = [self actionButtonWithTitle:title
                                          subtitle:@"Arm for the next tactical tap"
                                          scancode:SDL_SCANCODE_UNKNOWN];
    button.accessibilityLabel = accessibilityLabel;
    button.tag = static_cast<NSInteger>(modifier);
    [button removeTarget:self action:@selector(performCommand:) forControlEvents:UIControlEventTouchUpInside];
    [button addTarget:self action:@selector(performModifier:) forControlEvents:UIControlEventTouchUpInside];
    return button;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = UIColor.clearColor;

    self.commandButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.commandButton.translatesAutoresizingMaskIntoConstraints = NO;
    self.commandButton.backgroundColor = [UIColor colorWithRed:0.88 green:0.16 blue:0.12 alpha:0.96];
    self.commandButton.layer.cornerRadius = 12;
    self.commandButton.layer.shadowColor = UIColor.blackColor.CGColor;
    self.commandButton.layer.shadowOpacity = 0.45;
    self.commandButton.layer.shadowRadius = 8;
    [self.commandButton setTitle:@"COMMANDS" forState:UIControlStateNormal];
    [self.commandButton setTitleColor:UIColor.whiteColor forState:UIControlStateNormal];
    UIFont* commandFont = [UIFont monospacedSystemFontOfSize:12 weight:UIFontWeightBold];
    self.commandButton.titleLabel.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleCaption1]
        scaledFontForFont:commandFont
        maximumPointSize:14];
    self.commandButton.titleLabel.adjustsFontForContentSizeCategory = YES;
    self.commandButton.accessibilityLabel = @"Open command palette";
    [self.commandButton addTarget:self action:@selector(togglePalette) forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:self.commandButton];

    UIButton* stop = [self actionButtonWithTitle:@"Stop" subtitle:@"Stop selected units" scancode:SDL_SCANCODE_S];
    UIButton* guard = [self actionButtonWithTitle:@"Guard" subtitle:@"Guard with selected units" scancode:SDL_SCANCODE_G];
    UIButton* scatter = [self actionButtonWithTitle:@"Scatter" subtitle:@"Scatter selected units" scancode:SDL_SCANCODE_X];
    UIButton* next = [self actionButtonWithTitle:@"Next" subtitle:@"Select the next unit" scancode:SDL_SCANCODE_N];
    UIButton* base = [self actionButtonWithTitle:@"Base" subtitle:@"Center the view on your base" scancode:SDL_SCANCODE_H];
    UIButton* view = [self actionButtonWithTitle:@"View" subtitle:@"Select all units currently in view" scancode:SDL_SCANCODE_E];
    UIButton* attack = [self modifierButtonWithTitle:@"Attack+"
                                 accessibilityLabel:@"Force attack next tap"
                                            modifier:RatouchModifier::ForceAttack];
    UIButton* move = [self modifierButtonWithTitle:@"Move+"
                               accessibilityLabel:@"Force move next tap"
                                          modifier:RatouchModifier::ForceMove];
    UIButton* add = [self modifierButtonWithTitle:@"Add+"
                              accessibilityLabel:@"Add to selection on next tap or drag"
                                         modifier:RatouchModifier::AddSelection];
    UIButton* queue = [self modifierButtonWithTitle:@"Queue+"
                                accessibilityLabel:@"Queue move on next tap"
                                           modifier:RatouchModifier::QueueMove];
    UIButton* guide = [self actionButtonWithTitle:@"Controls"
                                         subtitle:@"Show the input guide and touch tuning"
                                         scancode:SDL_SCANCODE_UNKNOWN];
    [guide removeTarget:self action:@selector(performCommand:) forControlEvents:UIControlEventTouchUpInside];
    [guide addTarget:self action:@selector(showControls) forControlEvents:UIControlEventTouchUpInside];

    UIButton* groups = [self actionButtonWithTitle:@"Groups"
                                          subtitle:@"Assign or recall control groups 1 through 0"
                                          scancode:SDL_SCANCODE_UNKNOWN];
    [groups removeTarget:self action:@selector(performCommand:) forControlEvents:UIControlEventTouchUpInside];
    [groups addTarget:self action:@selector(showGroups) forControlEvents:UIControlEventTouchUpInside];

    self.sideButton = [self actionButtonWithTitle:@"Move right"
                                         subtitle:@"Move the command tab to the right edge"
                                         scancode:SDL_SCANCODE_UNKNOWN];
    [self.sideButton removeTarget:self action:@selector(performCommand:) forControlEvents:UIControlEventTouchUpInside];
    [self.sideButton addTarget:self action:@selector(toggleCommandSide) forControlEvents:UIControlEventTouchUpInside];

    UIStackView* firstRow = [[UIStackView alloc] initWithArrangedSubviews:@[stop, guard]];
    firstRow.axis = UILayoutConstraintAxisHorizontal;
    firstRow.distribution = UIStackViewDistributionFillEqually;
    firstRow.spacing = 8;

    UIStackView* secondRow = [[UIStackView alloc] initWithArrangedSubviews:@[scatter, next]];
    secondRow.axis = UILayoutConstraintAxisHorizontal;
    secondRow.distribution = UIStackViewDistributionFillEqually;
    secondRow.spacing = 8;

    UIStackView* thirdRow = [[UIStackView alloc] initWithArrangedSubviews:@[base, view]];
    thirdRow.axis = UILayoutConstraintAxisHorizontal;
    thirdRow.distribution = UIStackViewDistributionFillEqually;
    thirdRow.spacing = 8;

    UIStackView* fourthRow = [[UIStackView alloc] initWithArrangedSubviews:@[attack, move]];
    fourthRow.axis = UILayoutConstraintAxisHorizontal;
    fourthRow.distribution = UIStackViewDistributionFillEqually;
    fourthRow.spacing = 8;

    UIStackView* fifthRow = [[UIStackView alloc] initWithArrangedSubviews:@[add, queue]];
    fifthRow.axis = UILayoutConstraintAxisHorizontal;
    fifthRow.distribution = UIStackViewDistributionFillEqually;
    fifthRow.spacing = 8;

    UIStackView* sixthRow = [[UIStackView alloc] initWithArrangedSubviews:@[groups, guide]];
    sixthRow.axis = UILayoutConstraintAxisHorizontal;
    sixthRow.distribution = UIStackViewDistributionFillEqually;
    sixthRow.spacing = 8;

    UIStackView* seventhRow = [[UIStackView alloc] initWithArrangedSubviews:@[self.sideButton]];
    seventhRow.axis = UILayoutConstraintAxisHorizontal;
    seventhRow.distribution = UIStackViewDistributionFillEqually;

    UIStackView* rows = [[UIStackView alloc]
        initWithArrangedSubviews:@[firstRow, secondRow, thirdRow, fourthRow, fifthRow, sixthRow, seventhRow]];
    rows.translatesAutoresizingMaskIntoConstraints = NO;
    rows.axis = UILayoutConstraintAxisVertical;
    rows.distribution = UIStackViewDistributionFillEqually;
    rows.spacing = 8;

    self.palette = [[UIView alloc] init];
    self.palette.translatesAutoresizingMaskIntoConstraints = NO;
    self.palette.backgroundColor = [UIColor colorWithWhite:0.025 alpha:0.94];
    self.palette.layer.cornerRadius = 14;
    self.palette.layer.borderWidth = 1;
    self.palette.layer.borderColor = [UIColor colorWithWhite:1 alpha:0.12].CGColor;
    self.palette.hidden = YES;
    self.palette.accessibilityViewIsModal = YES;
    [self.palette addSubview:rows];
    [self.view addSubview:self.palette];

    UILayoutGuide* safe = self.view.safeAreaLayoutGuide;
    self.commandLeadingConstraint = [self.commandButton.leadingAnchor constraintEqualToAnchor:safe.leadingAnchor constant:8];
    self.commandTrailingConstraint = [self.commandButton.trailingAnchor constraintEqualToAnchor:safe.trailingAnchor constant:-8];
    self.paletteLeadingConstraint = [self.palette.leadingAnchor constraintEqualToAnchor:self.commandButton.trailingAnchor
                                                                                 constant:8];
    self.paletteTrailingConstraint = [self.palette.trailingAnchor constraintEqualToAnchor:self.commandButton.leadingAnchor
                                                                                   constant:-8];
    [NSLayoutConstraint activateConstraints:@[
        [self.commandButton.centerYAnchor constraintEqualToAnchor:safe.centerYAnchor],
        [self.commandButton.widthAnchor constraintEqualToConstant:92],
        [self.commandButton.heightAnchor constraintEqualToConstant:44],

        [self.palette.centerYAnchor constraintEqualToAnchor:self.commandButton.centerYAnchor],
        [self.palette.widthAnchor constraintEqualToConstant:320],
        [self.palette.topAnchor constraintGreaterThanOrEqualToAnchor:safe.topAnchor constant:8],
        [self.palette.bottomAnchor constraintLessThanOrEqualToAnchor:safe.bottomAnchor constant:-8],

        [rows.leadingAnchor constraintEqualToAnchor:self.palette.leadingAnchor constant:8],
        [rows.trailingAnchor constraintEqualToAnchor:self.palette.trailingAnchor constant:-8],
        [rows.topAnchor constraintEqualToAnchor:self.palette.topAnchor constant:8],
        [rows.bottomAnchor constraintEqualToAnchor:self.palette.bottomAnchor constant:-8],
    ]];
    NSLayoutConstraint* preferredPaletteHeight = [self.palette.heightAnchor constraintEqualToConstant:392];
    preferredPaletteHeight.priority = UILayoutPriorityDefaultHigh;
    preferredPaletteHeight.active = YES;
    self.commandOnRight = [NSUserDefaults.standardUserDefaults boolForKey:Command_Side_Defaults_Key];
    self.sidebarVisible = Sidebar_Visible.load(std::memory_order_acquire);
    [self updateSideConstraints];
}

- (void)viewDidLayoutSubviews
{
    [super viewDidLayoutSubviews];
    [self updateSidebarInset];
}

- (void)togglePalette
{
    const BOOL opening = self.palette.hidden;
    if (opening) {
        self.palette.hidden = NO;
        [self.commandButton setTitle:@"CLOSE" forState:UIControlStateNormal];
        self.commandButton.accessibilityLabel = @"Close command palette";
        UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, self.palette);
    } else {
        [self closePalette];
    }
}

- (void)performCommand:(UIButton*)sender
{
    Ratouch_iOS_Cancel_One_Shot_Modifier();
    Push_Key_Tap((SDL_Scancode)sender.tag);
    [self closePalette];
    UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, self.commandButton);
}

- (void)performModifier:(UIButton*)sender
{
    Toggle_One_Shot_Modifier(static_cast<RatouchModifier>(sender.tag));
    [self closePalette];
    UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, self.commandButton);
}

- (void)showControls
{
    [self closePalette];
    RatouchControlsViewController* controls = [[RatouchControlsViewController alloc] init];
    controls.modalPresentationStyle = UIModalPresentationFormSheet;
    [self presentViewController:controls animated:!UIAccessibilityIsReduceMotionEnabled() completion:nil];
}

- (void)showGroups
{
    Ratouch_iOS_Cancel_One_Shot_Modifier();
    [self closePalette];
    RatouchGroupsViewController* groups = [[RatouchGroupsViewController alloc] init];
    groups.modalPresentationStyle = UIModalPresentationFormSheet;
    [self presentViewController:groups animated:!UIAccessibilityIsReduceMotionEnabled() completion:nil];
}

- (void)toggleCommandSide
{
    self.commandOnRight = !self.commandOnRight;
    [NSUserDefaults.standardUserDefaults setBool:self.commandOnRight forKey:Command_Side_Defaults_Key];
    [self closePalette];
    [self updateSideConstraints];
    UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, self.commandButton);
}

- (void)updateSideConstraints
{
    [NSLayoutConstraint deactivateConstraints:@[
        self.commandLeadingConstraint,
        self.commandTrailingConstraint,
        self.paletteLeadingConstraint,
        self.paletteTrailingConstraint,
    ]];
    if (self.commandOnRight) {
        [NSLayoutConstraint activateConstraints:@[self.commandTrailingConstraint, self.paletteTrailingConstraint]];
        [self.sideButton setTitle:@"Move left" forState:UIControlStateNormal];
        self.sideButton.accessibilityLabel = @"Move command tab to left edge";
        self.sideButton.accessibilityHint = @"Places the command tab on the left side and remembers this choice";
    } else {
        [NSLayoutConstraint activateConstraints:@[self.commandLeadingConstraint, self.paletteLeadingConstraint]];
        [self.sideButton setTitle:@"Move right" forState:UIControlStateNormal];
        self.sideButton.accessibilityLabel = @"Move command tab to right edge";
        self.sideButton.accessibilityHint = @"Places the command tab on the right side and remembers this choice";
    }
    [self.view setNeedsLayout];
    [self.view layoutIfNeeded];
}

- (void)updateSidebarInset
{
    const CGFloat safeWidth = CGRectGetWidth(self.view.safeAreaLayoutGuide.layoutFrame);
    const CGFloat sidebarWidth = self.sidebarVisible ? safeWidth * 0.25 : 0.0;
    self.commandTrailingConstraint.constant = -(8.0 + sidebarWidth);
}

- (void)closePalette
{
    self.palette.hidden = YES;
    [self refreshModifier:Modifier_State.Active()];
}

- (void)refreshModifier:(RatouchModifier)modifier
{
    NSString* title = @"COMMANDS";
    NSString* label = @"Open command palette";
    if (modifier == RatouchModifier::ForceAttack) {
        title = @"ATTACK+";
        label = @"Force attack armed. Open command palette";
    } else if (modifier == RatouchModifier::ForceMove) {
        title = @"MOVE+";
        label = @"Force move armed. Open command palette";
    } else if (modifier == RatouchModifier::AddSelection) {
        title = @"ADD+";
        label = @"Add selection armed. Open command palette";
    } else if (modifier == RatouchModifier::QueueMove) {
        title = @"QUEUE+";
        label = @"Queue move armed. Open command palette";
    }
    [self.commandButton setTitle:title forState:UIControlStateNormal];
    self.commandButton.accessibilityLabel = label;
    self.commandButton.backgroundColor = modifier == RatouchModifier::None
                                             ? [UIColor colorWithRed:0.88 green:0.16 blue:0.12 alpha:0.96]
                                             : [UIColor colorWithRed:0.96 green:0.55 blue:0.12 alpha:0.98];
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}

@end

namespace
{
void Refresh_Modifier_UI(RatouchModifier modifier)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (Command_Window != nil) {
            [(RatouchCommandViewController*)Command_Window.rootViewController refreshModifier:modifier];
        }
    });
}
}

void Ratouch_Install_Command_Overlay(void)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (Command_Window != nil) {
            return;
        }

        Load_Touch_Preferences();

        UIWindowScene* activeScene = nil;
        for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
            if ([scene isKindOfClass:UIWindowScene.class]
                && (scene.activationState == UISceneActivationStateForegroundActive
                    || scene.activationState == UISceneActivationStateForegroundInactive)) {
                activeScene = (UIWindowScene*)scene;
                break;
            }
        }

        Command_Window = activeScene != nil ? [[RatouchPassthroughWindow alloc] initWithWindowScene:activeScene]
                                            : [[RatouchPassthroughWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
        Command_Window.frame = activeScene != nil ? activeScene.coordinateSpace.bounds : UIScreen.mainScreen.bounds;
        Command_Window.windowLevel = UIWindowLevelNormal + 1.0;
        Command_Window.backgroundColor = UIColor.clearColor;
        Command_Window.rootViewController = [[RatouchCommandViewController alloc] init];
        Command_Window.hidden = YES;
    });
}

void Ratouch_Set_Command_Overlay_Visible(bool visible)
{
    dispatch_async(dispatch_get_main_queue(), ^{
        if (Command_Window == nil) {
            return;
        }
        if (!visible) {
            Ratouch_iOS_Cancel_One_Shot_Modifier();
            [(RatouchCommandViewController*)Command_Window.rootViewController closePalette];
        }
        Command_Window.hidden = !visible;
    });
}

void Ratouch_Set_iOS_Sidebar_Visible(bool visible)
{
    Sidebar_Visible.store(visible, std::memory_order_release);
    dispatch_async(dispatch_get_main_queue(), ^{
        if (Command_Window == nil) {
            return;
        }
        RatouchCommandViewController* controller =
            (RatouchCommandViewController*)Command_Window.rootViewController;
        controller.sidebarVisible = visible;
        [controller updateSidebarInset];
        [controller.view setNeedsLayout];
    });
}

void Ratouch_iOS_Consume_One_Shot_Modifier(void)
{
    const RatouchModifier consumed = Modifier_State.Consume();
    if (consumed != RatouchModifier::None) {
        Push_Key_Event(Modifier_Scancode(consumed), true);
        Refresh_Modifier_UI(RatouchModifier::None);
    }
}

void Ratouch_iOS_Cancel_One_Shot_Modifier(void)
{
    const RatouchModifier canceled = Modifier_State.Cancel();
    if (canceled != RatouchModifier::None) {
        Push_Key_Event(Modifier_Scancode(canceled), true);
        Refresh_Modifier_UI(RatouchModifier::None);
    }
}

void Ratouch_iOS_Confirm_Long_Press(void)
{
    if (!Touch_Haptics.load(std::memory_order_acquire)) {
        return;
    }
    dispatch_async(dispatch_get_main_queue(), ^{
        UIImpactFeedbackGenerator* feedback =
            [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
        [feedback prepare];
        [feedback impactOccurred];
    });
}

uint64_t Ratouch_iOS_Long_Press_Milliseconds(void)
{
    return Long_Press_Milliseconds.load(std::memory_order_acquire);
}

float Ratouch_iOS_Drag_Threshold(void)
{
    return Drag_Threshold.load(std::memory_order_acquire);
}

bool Ratouch_iOS_Invert_Pan(void)
{
    return Invert_Pan.load(std::memory_order_acquire);
}
