#import "apple/shared/importer.h"

#import <CommonCrypto/CommonDigest.h>

#include "apple/shared/asset_manifest.h"
#include "apple/shared/iso9660.h"
#include "apple/shared/mix_validator.h"

#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>

namespace
{
void Set_Import_Error(NSError** reported_error, NSInteger code, NSString* description)
{
    if (reported_error != nullptr && *reported_error == nil) {
        *reported_error = [NSError errorWithDomain:@"RatouchImport"
                                               code:code
                                           userInfo:@{NSLocalizedDescriptionKey : description}];
    }
}

BOOL Ensure_Runtime_Main_Mix(NSURL* root, NSError** reported_error)
{
    NSFileManager* files = [NSFileManager defaultManager];
    NSURL* runtime_main = [root URLByAppendingPathComponent:@"MAIN.MIX"];
    std::string validation_error;
    if (Ratouch_Validate_MIX(runtime_main.path.fileSystemRepresentation, validation_error)) {
        return YES;
    }

    if ([files fileExistsAtPath:runtime_main.path]) {
        Set_Import_Error(reported_error, 7, @"MAIN.MIX exists but is not a valid MIX archive.");
        return NO;
    }

    NSArray<NSString*>* base_discs = @[@"allied/MAIN.MIX", @"soviet/MAIN.MIX"];
    for (NSString* relative_path in base_discs) {
        NSURL* source = [root URLByAppendingPathComponent:relative_path];
        validation_error.clear();
        if (!Ratouch_Validate_MIX(source.path.fileSystemRepresentation, validation_error)) {
            continue;
        }

        NSError* link_error = nil;
        if ([files linkItemAtURL:source toURL:runtime_main error:&link_error]) {
            return YES;
        }
        Set_Import_Error(reported_error,
                         7,
                         [NSString stringWithFormat:@"Could not prepare MAIN.MIX for gameplay: %@",
                                                    link_error.localizedDescription]);
        return NO;
    }

    Set_Import_Error(reported_error, 7, @"A valid base-game MAIN.MIX archive was not found.");
    return NO;
}

BOOL Has_Required_Data_At_Root(NSURL* root, NSError** reported_error = nullptr)
{
    NSURL* core = [root URLByAppendingPathComponent:@"REDALERT.MIX"];
    std::string validation_error;
    if (!Ratouch_Validate_MIX(core.path.fileSystemRepresentation, validation_error)) {
        return NO;
    }

    if (!Ensure_Runtime_Main_Mix(root, reported_error)) {
        return NO;
    }
    validation_error.clear();
    NSURL* main = [root URLByAppendingPathComponent:@"MAIN.MIX"];
    return Ratouch_Validate_MIX(main.path.fileSystemRepresentation, validation_error);
}

NSURL* Asset_Parent()
{
    return [Ratouch_Asset_Root() URLByDeletingLastPathComponent];
}

NSURL* Pending_Asset_Root()
{
    return [Asset_Parent() URLByAppendingPathComponent:@".pending-game-data" isDirectory:YES];
}

bool Read_Asset_Identity(NSURL* url, RatouchAssetIdentity& identity, NSError** reported_error)
{
    std::ifstream input(url.fileSystemRepresentation, std::ios::binary | std::ios::ate);
    if (!input) {
        Set_Import_Error(reported_error, 4, [NSString stringWithFormat:@"%@ could not be opened.", url.lastPathComponent]);
        return false;
    }
    const std::streamoff end = input.tellg();
    if (end < 0) {
        Set_Import_Error(reported_error, 4, [NSString stringWithFormat:@"%@ has an invalid size.", url.lastPathComponent]);
        return false;
    }
    input.seekg(0, std::ios::beg);

    CC_SHA256_CTX context;
    CC_SHA256_Init(&context);
    std::vector<char> buffer(1024 * 1024);
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = input.gcount();
        if (count > 0) {
            CC_SHA256_Update(&context, buffer.data(), static_cast<CC_LONG>(count));
        }
    }
    if (!input.eof()) {
        Set_Import_Error(reported_error, 4, [NSString stringWithFormat:@"%@ could not be read.", url.lastPathComponent]);
        return false;
    }

    unsigned char digest[CC_SHA256_DIGEST_LENGTH];
    CC_SHA256_Final(digest, &context);
    std::ostringstream hash;
    hash << std::hex << std::setfill('0');
    for (unsigned char byte : digest) {
        hash << std::setw(2) << static_cast<unsigned>(byte);
    }
    identity = {url.lastPathComponent.UTF8String, static_cast<uint64_t>(end), hash.str()};
    return true;
}

NSUInteger Import_Asset_URLs_To_Root(NSArray<NSURL*>* urls, NSURL* destination, NSError** reported_error)
{
    NSFileManager* files = [NSFileManager defaultManager];
    NSURL* parent = destination.URLByDeletingLastPathComponent;
    if (![files createDirectoryAtURL:parent withIntermediateDirectories:YES attributes:nil error:reported_error]) {
        return 0;
    }
    NSURL* transaction = [parent URLByAppendingPathComponent:
                                     [NSString stringWithFormat:@".import-%@", NSUUID.UUID.UUIDString]
                                            isDirectory:YES];
    NSURL* payload = [transaction URLByAppendingPathComponent:@"payload" isDirectory:YES];
    NSURL* iso_staging = [transaction URLByAppendingPathComponent:@"iso" isDirectory:YES];
    if (![files createDirectoryAtURL:payload withIntermediateDirectories:YES attributes:nil error:reported_error]
        || ![files createDirectoryAtURL:iso_staging withIntermediateDirectories:YES attributes:nil error:reported_error]) {
        [files removeItemAtURL:transaction error:nil];
        return 0;
    }

    NSMutableArray<NSURL*>* candidates = [NSMutableArray array];
    NSArray* keys = @[NSURLIsDirectoryKey, NSURLIsRegularFileKey];
    __block NSError* enumeration_error = nil;
    for (NSURL* url in urls) {
        NSNumber* is_directory = nil;
        [url getResourceValue:&is_directory forKey:NSURLIsDirectoryKey error:nil];
        if (is_directory.boolValue) {
            NSDirectoryEnumerator* enumerator = [files enumeratorAtURL:url
                                             includingPropertiesForKeys:keys
                                                                options:NSDirectoryEnumerationSkipsHiddenFiles
                                                           errorHandler:^BOOL(NSURL*, NSError* error) {
                if (enumeration_error == nil) {
                    enumeration_error = error;
                }
                return NO;
            }];
            for (NSURL* child in enumerator) {
                NSNumber* regular = nil;
                [child getResourceValue:&regular forKey:NSURLIsRegularFileKey error:nil];
                if (regular.boolValue && [child.pathExtension caseInsensitiveCompare:@"mix"] == NSOrderedSame) {
                    [candidates addObject:child];
                }
            }
            if (enumeration_error != nil) {
                if (reported_error != nullptr && *reported_error == nil) {
                    *reported_error = enumeration_error;
                }
                [files removeItemAtURL:transaction error:nil];
                return 0;
            }
        } else if ([url.pathExtension caseInsensitiveCompare:@"mix"] == NSOrderedSame) {
            [candidates addObject:url];
        } else if ([url.pathExtension caseInsensitiveCompare:@"iso"] == NSOrderedSame) {
            std::vector<std::string> extracted;
            std::string iso_error;
            if (!Ratouch_Extract_ISO_MIX_Files(url.fileSystemRepresentation,
                                               iso_staging.path.fileSystemRepresentation,
                                               extracted,
                                               iso_error)
                && (reported_error == nullptr || *reported_error == nil)) {
                Set_Import_Error(reported_error, 2, [NSString stringWithUTF8String:iso_error.c_str()]);
            } else {
                for (const std::string& name : extracted) {
                    [candidates addObject:[iso_staging URLByAppendingPathComponent:
                                                         [NSString stringWithUTF8String:name.c_str()]]];
                }
            }
        }
    }

    NSUInteger imported = 0;
    std::vector<RatouchAssetIdentity> identities;
    NSMutableArray<NSDictionary*>* provenance_assets = [NSMutableArray array];
    std::set<std::string> targets;
    for (NSURL* source in candidates) {
        std::string validation_error;
        if (!Ratouch_Validate_MIX(source.path.fileSystemRepresentation, validation_error)) {
            Set_Import_Error(reported_error,
                             3,
                             [NSString stringWithFormat:@"%@ is not a valid MIX file: %s",
                                                        source.lastPathComponent,
                                                        validation_error.c_str()]);
            [files removeItemAtURL:transaction error:nil];
            return 0;
        }

        RatouchAssetIdentity identity;
        if (!Read_Asset_Identity(source, identity, reported_error)) {
            [files removeItemAtURL:transaction error:nil];
            return 0;
        }
        std::string relative_path;
        std::string manifest_error;
        if (!Ratouch_Resolve_Asset_Target(identity, relative_path, manifest_error)) {
            Set_Import_Error(reported_error, 5, [NSString stringWithUTF8String:manifest_error.c_str()]);
            [files removeItemAtURL:transaction error:nil];
            return 0;
        }
        if (!targets.insert(relative_path).second) {
            Set_Import_Error(reported_error,
                             6,
                             [NSString stringWithFormat:@"More than one selected file maps to %s.", relative_path.c_str()]);
            [files removeItemAtURL:transaction error:nil];
            return 0;
        }

        NSString* relative = [NSString stringWithUTF8String:relative_path.c_str()];
        NSURL* target = [payload URLByAppendingPathComponent:relative];
        if (![files createDirectoryAtURL:target.URLByDeletingLastPathComponent
              withIntermediateDirectories:YES
                               attributes:nil
                                    error:reported_error]
            || ![files copyItemAtURL:source toURL:target error:reported_error]) {
            [files removeItemAtURL:transaction error:nil];
            return 0;
        }
        identities.push_back(identity);
        [provenance_assets addObject:@{
            @"sourceName" : source.lastPathComponent,
            @"relativePath" : relative,
            @"size" : @(identity.Size),
            @"sha256" : [NSString stringWithUTF8String:identity.SHA256.c_str()]
        }];
        ++imported;
    }

    if (!Has_Required_Data_At_Root(payload, reported_error)) {
        Set_Import_Error(reported_error,
                         7,
                         @"The selection must contain valid REDALERT.MIX and base-game MAIN.MIX data.");
        [files removeItemAtURL:transaction error:nil];
        return 0;
    }

    const BOOL complete_steam = Ratouch_Is_Complete_Steam_2229840(identities);
    NSDictionary* provenance = @{
        @"schemaVersion" : @1,
        @"source" : complete_steam ? @"Steam" : @"User-supplied files",
        @"sourceId" : complete_steam ? @"2229840" : @"unknown",
        @"importedAt" : [NSISO8601DateFormatter stringFromDate:NSDate.date
                                                      timeZone:NSTimeZone.localTimeZone
                                                       formatOptions:NSISO8601DateFormatWithInternetDateTime],
        @"assets" : provenance_assets
    };
    NSData* provenance_data = [NSJSONSerialization dataWithJSONObject:provenance
                                                               options:NSJSONWritingPrettyPrinted
                                                                 error:reported_error];
    if (provenance_data == nil
        || ![provenance_data writeToURL:[payload URLByAppendingPathComponent:@"provenance.json"]
                                options:NSDataWritingAtomic
                                  error:reported_error]) {
        [files removeItemAtURL:transaction error:nil];
        return 0;
    }

    BOOL installed = NO;
    if ([files fileExistsAtPath:destination.path]) {
        installed = [files replaceItemAtURL:destination
                              withItemAtURL:payload
                             backupItemName:nil
                                    options:0
                           resultingItemURL:nil
                                      error:reported_error];
    } else {
        installed = [files moveItemAtURL:payload toURL:destination error:reported_error];
    }
    [files removeItemAtURL:transaction error:nil];
    return installed ? imported : 0;
}

BOOL Copy_User_State_Into_Pending(NSURL* active, NSURL* pending, NSError** reported_error)
{
    NSFileManager* files = [NSFileManager defaultManager];
    if (![files fileExistsAtPath:active.path]) {
        return YES;
    }

    NSArray* keys = @[NSURLIsDirectoryKey, NSURLIsRegularFileKey];
    __block NSError* enumeration_error = nil;
    NSDirectoryEnumerator* enumerator = [files enumeratorAtURL:active
                                    includingPropertiesForKeys:keys
                                                       options:0
                                                  errorHandler:^BOOL(NSURL*, NSError* error) {
        if (enumeration_error == nil) {
            enumeration_error = error;
        }
        return NO;
    }];
    const NSString* active_path = active.path.stringByStandardizingPath;
    for (NSURL* child in enumerator) {
        NSString* child_path = child.path.stringByStandardizingPath;
        if (![child_path hasPrefix:[active_path stringByAppendingString:@"/"]]) {
            continue;
        }
        NSString* relative = [child_path substringFromIndex:active_path.length + 1];
        NSString* extension = child.pathExtension.lowercaseString;
        if ([relative isEqualToString:@"provenance.json"]
            || [extension isEqualToString:@"mix"]
            || [extension isEqualToString:@"iso"]) {
            continue;
        }

        NSNumber* is_directory = nil;
        NSNumber* is_regular = nil;
        [child getResourceValue:&is_directory forKey:NSURLIsDirectoryKey error:nil];
        [child getResourceValue:&is_regular forKey:NSURLIsRegularFileKey error:nil];
        NSURL* target = [pending URLByAppendingPathComponent:relative isDirectory:is_directory.boolValue];
        if (is_directory.boolValue) {
            if (![files createDirectoryAtURL:target
                  withIntermediateDirectories:YES
                                   attributes:nil
                                        error:reported_error]) {
                return NO;
            }
        } else if (is_regular.boolValue) {
            if ([files fileExistsAtPath:target.path] && ![files removeItemAtURL:target error:reported_error]) {
                return NO;
            }
            if (![files createDirectoryAtURL:target.URLByDeletingLastPathComponent
                  withIntermediateDirectories:YES
                                   attributes:nil
                                        error:reported_error]
                || ![files copyItemAtURL:child toURL:target error:reported_error]) {
                return NO;
            }
        }
    }
    if (enumeration_error != nil) {
        if (reported_error != nullptr && *reported_error == nil) {
            *reported_error = enumeration_error;
        }
        return NO;
    }
    return YES;
}
}

NSURL* Ratouch_Asset_Root(void)
{
    NSString* override = NSProcessInfo.processInfo.environment[@"RATOUCH_ASSET_ROOT"];
    if (override.length > 0) {
        return [NSURL fileURLWithPath:override isDirectory:YES];
    }
    NSURL* support = [[[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory
                                                              inDomains:NSUserDomainMask] firstObject];
    return [[support URLByAppendingPathComponent:@"Ratouch" isDirectory:YES]
        URLByAppendingPathComponent:@"vanillara" isDirectory:YES];
}

BOOL Ratouch_Has_Core_Data(void)
{
    return Has_Required_Data_At_Root(Ratouch_Asset_Root());
}

NSUInteger Ratouch_Import_Asset_URLs(NSArray<NSURL*>* urls, NSError** reported_error)
{
    return Import_Asset_URLs_To_Root(urls, Ratouch_Asset_Root(), reported_error);
}

NSUInteger Ratouch_Stage_Asset_URLs(NSArray<NSURL*>* urls, NSError** reported_error)
{
    return Import_Asset_URLs_To_Root(urls, Pending_Asset_Root(), reported_error);
}

BOOL Ratouch_Has_Pending_Asset_Import(void)
{
    return Has_Required_Data_At_Root(Pending_Asset_Root());
}

BOOL Ratouch_Apply_Pending_Asset_Import(NSError** reported_error)
{
    NSURL* pending = Pending_Asset_Root();
    if (![[NSFileManager defaultManager] fileExistsAtPath:pending.path]) {
        return YES;
    }
    if (!Has_Required_Data_At_Root(pending)) {
        Set_Import_Error(reported_error, 8, @"The pending game-data replacement is incomplete; current data was retained.");
        return NO;
    }

    NSFileManager* files = [NSFileManager defaultManager];
    NSURL* destination = Ratouch_Asset_Root();
    if (!Copy_User_State_Into_Pending(destination, pending, reported_error)) {
        return NO;
    }
    if ([files fileExistsAtPath:destination.path]) {
        return [files replaceItemAtURL:destination
                        withItemAtURL:pending
                       backupItemName:nil
                              options:0
                     resultingItemURL:nil
                                error:reported_error];
    }
    return [files moveItemAtURL:pending toURL:destination error:reported_error];
}

NSString* Ratouch_Asset_Provenance_Summary(void)
{
    NSURL* receipt = [Ratouch_Asset_Root() URLByAppendingPathComponent:@"provenance.json"];
    NSData* data = [NSData dataWithContentsOfURL:receipt];
    NSDictionary* provenance = data != nil ? [NSJSONSerialization JSONObjectWithData:data options:0 error:nil] : nil;
    if (![provenance isKindOfClass:NSDictionary.class]) {
        return @"Imported source unavailable";
    }

    NSString* source = [provenance[@"source"] isKindOfClass:NSString.class] ? provenance[@"source"] : @"User-supplied files";
    NSString* source_id = [provenance[@"sourceId"] isKindOfClass:NSString.class] ? provenance[@"sourceId"] : @"unknown";
    NSArray* assets = [provenance[@"assets"] isKindOfClass:NSArray.class] ? provenance[@"assets"] : @[];
    NSString* identity = [source_id isEqualToString:@"unknown"] ? source
                                                                  : [NSString stringWithFormat:@"%@ %@", source, source_id];
    return [NSString stringWithFormat:@"%@ • %lu validated file%@",
                                      identity,
                                      (unsigned long)assets.count,
                                      assets.count == 1 ? @"" : @"s"];
}
