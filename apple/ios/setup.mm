#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "apple/shared/importer.h"

@interface SDLUIKitDelegate : NSObject <UIApplicationDelegate>
+ (NSString*)getAppDelegateClassName;
- (void)hideLaunchScreen;
- (void)postFinishLaunch;
@end

namespace
{
UIWindow* Setup_Window = nil;
NSArray<UIWindow*>* Hidden_Windows = nil;

BOOL Has_Core_Data()
{
    return Ratouch_Has_Core_Data();
}

NSUInteger Import_Mix_Files(NSArray<NSURL*>* urls, NSError** reportedError)
{
    return Ratouch_Import_Asset_URLs(urls, reportedError);
}

void Present_Setup(void (^ready)(void));
}

@interface RatouchSetupViewController : UIViewController <UIDocumentPickerDelegate>
@property(nonatomic, copy) void (^completion)(BOOL);
@property(nonatomic, strong) UILabel* statusLabel;
@property(nonatomic, strong) UIButton* continueButton;
@end

@implementation RatouchSetupViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor colorWithRed:0.035 green:0.047 blue:0.071 alpha:1.0];

    UILabel* eyebrow = [[UILabel alloc] init];
    eyebrow.translatesAutoresizingMaskIntoConstraints = NO;
    eyebrow.text = @"RATOUCH";
    eyebrow.textColor = [UIColor colorWithRed:0.42 green:0.84 blue:0.70 alpha:1.0];
    UIFont* eyebrowBase = [UIFont monospacedSystemFontOfSize:14 weight:UIFontWeightSemibold];
    eyebrow.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleCaption1]
        scaledFontForFont:eyebrowBase
        maximumPointSize:18];
    eyebrow.adjustsFontForContentSizeCategory = YES;
    [eyebrow setContentCompressionResistancePriority:UILayoutPriorityRequired
                                             forAxis:UILayoutConstraintAxisVertical];

    UILabel* title = [[UILabel alloc] init];
    title.translatesAutoresizingMaskIntoConstraints = NO;
    title.text = @"Bring your game data";
    title.textColor = UIColor.whiteColor;
    UIFont* titleBase = [UIFont systemFontOfSize:38 weight:UIFontWeightBold];
    title.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleLargeTitle]
        scaledFontForFont:titleBase
        maximumPointSize:56];
    title.adjustsFontForContentSizeCategory = YES;
    title.numberOfLines = 0;
    [title setContentCompressionResistancePriority:UILayoutPriorityRequired
                                           forAxis:UILayoutConstraintAxisVertical];

    UILabel* body = [[UILabel alloc] init];
    body.translatesAutoresizingMaskIntoConstraints = NO;
    body.numberOfLines = 0;
    body.text = @"This app contains no commercial game assets. Choose legally acquired MIX files, a folder containing them, or a supported ISO. Your files stay on this device.";
    body.textColor = [UIColor colorWithWhite:0.78 alpha:1.0];
    body.font = [UIFont preferredFontForTextStyle:UIFontTextStyleBody];
    body.adjustsFontForContentSizeCategory = YES;
    [body setContentCompressionResistancePriority:UILayoutPriorityRequired
                                          forAxis:UILayoutConstraintAxisVertical];

    UIButton* choose = [UIButton buttonWithType:UIButtonTypeSystem];
    choose.translatesAutoresizingMaskIntoConstraints = NO;
    [choose setTitle:@"Choose files or folder" forState:UIControlStateNormal];
    UIFont* chooseBase = [UIFont systemFontOfSize:18 weight:UIFontWeightSemibold];
    choose.titleLabel.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleHeadline]
        scaledFontForFont:chooseBase
        maximumPointSize:24];
    choose.titleLabel.adjustsFontForContentSizeCategory = YES;
    choose.backgroundColor = [UIColor colorWithRed:0.42 green:0.84 blue:0.70 alpha:1.0];
    [choose setTitleColor:[UIColor colorWithRed:0.02 green:0.08 blue:0.07 alpha:1.0] forState:UIControlStateNormal];
    choose.layer.cornerRadius = 12;
    choose.accessibilityLabel = @"Choose game data files or folder";
    [choose addTarget:self action:@selector(chooseFiles) forControlEvents:UIControlEventTouchUpInside];

    self.continueButton = [UIButton buttonWithType:UIButtonTypeSystem];
    self.continueButton.translatesAutoresizingMaskIntoConstraints = NO;
    [self.continueButton setTitle:@"Continue" forState:UIControlStateNormal];
    UIFont* continueBase = [UIFont systemFontOfSize:17 weight:UIFontWeightSemibold];
    self.continueButton.titleLabel.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleHeadline]
        scaledFontForFont:continueBase
        maximumPointSize:24];
    self.continueButton.titleLabel.adjustsFontForContentSizeCategory = YES;
    [self.continueButton addTarget:self action:@selector(continueToGame) forControlEvents:UIControlEventTouchUpInside];

    self.statusLabel = [[UILabel alloc] init];
    self.statusLabel.translatesAutoresizingMaskIntoConstraints = NO;
    self.statusLabel.numberOfLines = 0;
    UIFont* statusBase = [UIFont monospacedSystemFontOfSize:13 weight:UIFontWeightRegular];
    self.statusLabel.font = [[UIFontMetrics metricsForTextStyle:UIFontTextStyleFootnote]
        scaledFontForFont:statusBase
        maximumPointSize:20];
    self.statusLabel.adjustsFontForContentSizeCategory = YES;
    [self.statusLabel setContentCompressionResistancePriority:UILayoutPriorityRequired
                                                      forAxis:UILayoutConstraintAxisVertical];

    UIStackView* stack = [[UIStackView alloc] initWithArrangedSubviews:@[
        eyebrow, title, body, choose, self.continueButton, self.statusLabel
    ]];
    stack.translatesAutoresizingMaskIntoConstraints = NO;
    stack.axis = UILayoutConstraintAxisVertical;
    stack.spacing = 18;

    UIScrollView* scroll = [[UIScrollView alloc] init];
    scroll.translatesAutoresizingMaskIntoConstraints = NO;
    scroll.alwaysBounceVertical = YES;
    scroll.showsVerticalScrollIndicator = YES;
    UIView* contentView = [[UIView alloc] init];
    contentView.translatesAutoresizingMaskIntoConstraints = NO;
    [contentView addSubview:stack];
    [scroll addSubview:contentView];
    [self.view addSubview:scroll];

    UILayoutGuide* guide = self.view.safeAreaLayoutGuide;
    UILayoutGuide* content = scroll.contentLayoutGuide;
    UILayoutGuide* frame = scroll.frameLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [scroll.leadingAnchor constraintEqualToAnchor:guide.leadingAnchor],
        [scroll.trailingAnchor constraintEqualToAnchor:guide.trailingAnchor],
        [scroll.topAnchor constraintEqualToAnchor:guide.topAnchor],
        [scroll.bottomAnchor constraintEqualToAnchor:guide.bottomAnchor],
        [contentView.leadingAnchor constraintEqualToAnchor:content.leadingAnchor],
        [contentView.trailingAnchor constraintEqualToAnchor:content.trailingAnchor],
        [contentView.topAnchor constraintEqualToAnchor:content.topAnchor],
        [contentView.bottomAnchor constraintEqualToAnchor:content.bottomAnchor],
        [contentView.widthAnchor constraintEqualToAnchor:frame.widthAnchor],
        [stack.centerXAnchor constraintEqualToAnchor:contentView.centerXAnchor],
        [stack.centerYAnchor constraintEqualToAnchor:contentView.centerYAnchor],
        [stack.topAnchor constraintGreaterThanOrEqualToAnchor:contentView.topAnchor constant:40],
        [stack.bottomAnchor constraintLessThanOrEqualToAnchor:contentView.bottomAnchor constant:-40],
        [stack.leadingAnchor constraintGreaterThanOrEqualToAnchor:contentView.leadingAnchor constant:56],
        [stack.trailingAnchor constraintLessThanOrEqualToAnchor:contentView.trailingAnchor constant:-56],
        [stack.widthAnchor constraintLessThanOrEqualToConstant:620],
        [choose.heightAnchor constraintGreaterThanOrEqualToConstant:54],
        [self.continueButton.heightAnchor constraintGreaterThanOrEqualToConstant:44],
    ]];
    NSLayoutConstraint* preferredContentHeight = [contentView.heightAnchor constraintEqualToAnchor:frame.heightAnchor];
    preferredContentHeight.priority = UILayoutPriorityDefaultLow;
    preferredContentHeight.active = YES;
    [self refreshStatusWithMessage:nil];
}

- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskLandscape;
}

- (void)refreshStatusWithMessage:(NSString*)message
{
    const BOOL ready = Has_Core_Data();
    self.continueButton.enabled = ready;
    self.continueButton.alpha = ready ? 1.0 : 0.35;
    self.statusLabel.textColor = ready
                                     ? [UIColor colorWithRed:0.42 green:0.84 blue:0.70 alpha:1.0]
                                     : [UIColor colorWithRed:0.95 green:0.68 blue:0.35 alpha:1.0];
    self.statusLabel.text = message ?: (ready ? @"Core game data found. Ready to continue."
                                             : @"Required: REDALERT.MIX and base-game MAIN.MIX data");
}

- (void)chooseFiles
{
    NSArray<UTType*>* types = @[UTTypeData, UTTypeFolder, UTTypeDiskImage];
    UIDocumentPickerViewController* picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:types
                                                                                                       asCopy:YES];
    picker.delegate = self;
    picker.allowsMultipleSelection = YES;
    [self presentViewController:picker animated:YES completion:nil];
}

- (void)continueToGame
{
    if (Has_Core_Data() && self.completion != nil) {
        self.completion(YES);
    }
}

- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    NSMutableArray<NSURL*>* accessible = [NSMutableArray array];
    for (NSURL* url in urls) {
        [url startAccessingSecurityScopedResource];
        [accessible addObject:url];
    }

    NSError* error = nil;
    const NSUInteger count = Import_Mix_Files(accessible, &error);
    for (NSURL* url in accessible) {
        [url stopAccessingSecurityScopedResource];
    }

    if (Has_Core_Data()) {
        [self refreshStatusWithMessage:[NSString stringWithFormat:@"Imported %lu MIX file%@. Ready to continue.",
                                                                 (unsigned long)count,
                                                                 count == 1 ? @"" : @"s"]];
    } else if (error != nil) {
        [self refreshStatusWithMessage:[NSString stringWithFormat:@"Import failed: %@", error.localizedDescription]];
    } else {
        [self refreshStatusWithMessage:@"Required REDALERT.MIX and base-game MAIN.MIX data were not found."];
    }
}

@end

namespace
{
void Present_Setup(void (^ready)(void))
{
    UIWindowScene* activeScene = nil;
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
        if ([scene isKindOfClass:UIWindowScene.class]
            && (scene.activationState == UISceneActivationStateForegroundActive
                || scene.activationState == UISceneActivationStateForegroundInactive)) {
            activeScene = (UIWindowScene*)scene;
            break;
        }
    }

    Setup_Window = activeScene != nil ? [[UIWindow alloc] initWithWindowScene:activeScene]
                                      : [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    Setup_Window.frame = activeScene != nil ? activeScene.coordinateSpace.bounds : UIScreen.mainScreen.bounds;
    Setup_Window.windowLevel = UIWindowLevelNormal + 2.0;
    Setup_Window.backgroundColor = [UIColor colorWithRed:0.035 green:0.047 blue:0.071 alpha:1.0];
    RatouchSetupViewController* controller = [[RatouchSetupViewController alloc] init];
    controller.completion = ^(BOOL success) {
        if (!success) {
            return;
        }
        Setup_Window.hidden = YES;
        for (UIWindow* window in Hidden_Windows) {
            window.hidden = NO;
        }
        Hidden_Windows = nil;
        Setup_Window = nil;
        ready();
    };
    Setup_Window.rootViewController = controller;
    controller.view.frame = Setup_Window.bounds;
    controller.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    if (activeScene != nil) {
        NSMutableArray<UIWindow*>* hidden = [NSMutableArray array];
        for (UIWindow* window in activeScene.windows) {
            if (window != Setup_Window) {
                [hidden addObject:window];
                window.hidden = YES;
            }
        }
        Hidden_Windows = [hidden copy];
    }
    [Setup_Window makeKeyAndVisible];
    [Setup_Window layoutIfNeeded];
    NSLog(@"Ratouch setup presented on main thread: %@, frame: %@, controller: %@, color: %@",
          NSThread.isMainThread ? @"yes" : @"no",
          NSStringFromCGRect(Setup_Window.frame),
          Setup_Window.rootViewController,
          controller.view.backgroundColor);
}
}

@interface RatouchAppDelegate : SDLUIKitDelegate
@end

@implementation RatouchAppDelegate

- (void)startGame
{
    [super postFinishLaunch];
}

- (void)postFinishLaunch
{
    NSError* pendingError = nil;
    if (!Ratouch_Apply_Pending_Asset_Import(&pendingError)) {
        NSLog(@"RAtouch kept the current game data because the pending replacement failed: %@",
              pendingError.localizedDescription);
    }
    if (Has_Core_Data()) {
        [self startGame];
        return;
    }

    [self hideLaunchScreen];
    Present_Setup(^{
        [self startGame];
    });
}

@end

bool Ratouch_Ensure_Game_Data()
{
    return Has_Core_Data();
}
