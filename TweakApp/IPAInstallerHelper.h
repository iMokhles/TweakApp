//
//  IPAInstallerHelper.h
//  IPAInstaller
//
//  Created by iMokhles on 30/03/16.
//  Copyright © 2016 iMokhles. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Quartz/Quartz.h>

/* C Prototypes */
void addRoundedRectToPath(CGContextRef context, CGRect rect, float ovalWidth, float ovalHeight);

@interface IPAInstallerHelper : NSObject
+ (NSString *)tempPath;
+ (NSString *)ipainstallerSubPath;
+ (NSString *)ipaExtractedPath;
+ (NSString *)payloadExtractedPath;
+ (BOOL)deleteFileAtPath:(NSString *)filePath;
+ (NSImage *)roundCornersImage:(NSImage *)image CornerRadius:(NSInteger)radius;
+ (NSArray *)getCertifications;
+ (BOOL)copyDylibFile:(NSString *)dylibFile toPath:(NSString *)path;
+ (BOOL)ApplyCommandWithRoot:(NSString *)commandString;
+ (BOOL)excuteSignCommandWithFilePath:(NSString *)ipaPath certName:(NSString *)certName profilePath:(NSString *)profilePath newBID:(NSString *)bundleID appName:(NSString *)appName;
+ (void)createShellFileWithContents:(NSString *)contents;
@end
