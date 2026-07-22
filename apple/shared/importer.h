#ifndef RATOUCH_APPLE_IMPORTER_H
#define RATOUCH_APPLE_IMPORTER_H

#ifdef __OBJC__
#import <Foundation/Foundation.h>

NSURL* Ratouch_Asset_Root(void);
BOOL Ratouch_Has_Core_Data(void);
NSUInteger Ratouch_Import_Asset_URLs(NSArray<NSURL*>* urls, NSError** reported_error);
NSUInteger Ratouch_Stage_Asset_URLs(NSArray<NSURL*>* urls, NSError** reported_error);
BOOL Ratouch_Apply_Pending_Asset_Import(NSError** reported_error);
BOOL Ratouch_Has_Pending_Asset_Import(void);
NSString* Ratouch_Asset_Provenance_Summary(void);
#endif

#endif
