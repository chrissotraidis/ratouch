#import <Foundation/Foundation.h>
#include "apple/shared/importer.h"

static NSURL* TestRoot = nil;

static void Require(BOOL condition, NSString* message)
{
    if (!condition) {
        NSLog(@"Importer regression failed: %@", message);
        if (TestRoot != nil) {
            [NSFileManager.defaultManager removeItemAtURL:TestRoot error:nil];
        }
        exit(1);
    }
}

int main()
{
    @autoreleasepool {
        NSFileManager* files = NSFileManager.defaultManager;
        NSURL* root = [files.temporaryDirectory URLByAppendingPathComponent:NSUUID.UUID.UUIDString isDirectory:YES];
        Require([files createDirectoryAtURL:root withIntermediateDirectories:YES attributes:nil error:nil], @"create fixture root");
        TestRoot = root;
        NSURL* active = [root URLByAppendingPathComponent:@"active" isDirectory:YES];
        setenv("RATOUCH_ASSET_ROOT", active.path.fileSystemRepresentation, 1);
        NSURL* brokenISO = [root URLByAppendingPathComponent:@"broken.iso"];
        [@"invalid ISO fixture" writeToURL:brokenISO atomically:YES encoding:NSUTF8StringEncoding error:nil];
        NSError* error = nil;
        Require(Ratouch_Import_Asset_URLs(@[brokenISO], &error) == 0 && error != nil,
                @"invalid ISO must be rejected");
        Require(![files fileExistsAtPath:active.path], @"invalid ISO must not install data");

        NSString* source = NSProcessInfo.processInfo.environment[@"RATOUCH_TEST_DATA_DIRECTORY"];
        if (source.length > 0) {
            NSMutableArray<NSURL*>* supported = [NSMutableArray array];
            for (NSURL* url in [files enumeratorAtURL:[NSURL fileURLWithPath:source isDirectory:YES]
                         includingPropertiesForKeys:nil options:NSDirectoryEnumerationSkipsHiddenFiles errorHandler:nil]) {
                if ([url.lastPathComponent caseInsensitiveCompare:@"REDALERT.MIX"] == NSOrderedSame
                    || [url.lastPathComponent caseInsensitiveCompare:@"MAIN1.MIX"] == NSOrderedSame) {
                    [supported addObject:url];
                }
            }
            Require(supported.count == 2, @"private fixture must contain one supported core and Allied archive");
            // Uses the unmodified production size/hash manifest and real MIX validator.
            error = nil;
            Require(Ratouch_Import_Asset_URLs(supported, &error) == 2 && error == nil && Ratouch_Has_Core_Data(),
                    @"supported private selection must import normally");
            NSURL* save = [active URLByAppendingPathComponent:@"SAVEGAME.001"];
            [@"player-save-marker" writeToURL:save atomically:YES encoding:NSUTF8StringEncoding error:nil];
            NSData* receipt = [NSData dataWithContentsOfURL:[active URLByAppendingPathComponent:@"provenance.json"]];
            NSMutableArray<NSURL*>* mixed = [NSMutableArray arrayWithObject:brokenISO];
            [mixed addObjectsFromArray:supported];
            error = nil;
            Require(Ratouch_Import_Asset_URLs(mixed, &error) == 0 && error != nil,
                    @"a failed ISO must abort the whole selection even when the other files are complete");
            Require([[NSData dataWithContentsOfURL:[active URLByAppendingPathComponent:@"provenance.json"]] isEqual:receipt],
                    @"a failed selection must retain the installed receipt");
            Require([[NSString stringWithContentsOfURL:save encoding:NSUTF8StringEncoding error:nil] isEqual:@"player-save-marker"],
                    @"a failed selection must retain saves");
            error = nil;
            Require(Ratouch_Stage_Asset_URLs(supported, &error) == 2 && error == nil, @"valid replacement must stage");
            error = nil;
            Require(Ratouch_Stage_Asset_URLs(mixed, &error) == 0 && error != nil && Ratouch_Has_Pending_Asset_Import(),
                    @"failed selection must retain the previous valid pending replacement");
            NSLog(@"Importer PASS invalidISO=true mixedSelectionRejected=true installedDataPreserved=true savesPreserved=true pendingPreserved=true productionValidation=true");
        } else {
            NSLog(@"Importer PASS invalidISO=true; supported-input preservation checks require RATOUCH_TEST_DATA_DIRECTORY");
        }
        Require([files removeItemAtURL:root error:nil], @"remove private test outputs");
    }
}
