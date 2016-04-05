//
//  IPAInstallerHelper.m
//  IPAInstaller
//
//  Created by iMokhles on 30/03/16.
//  Copyright © 2016 iMokhles. All rights reserved.
//

#import "IPAInstallerHelper.h"

void addRoundedRectToPath(CGContextRef context, CGRect rect, float ovalWidth, float ovalHeight)
{
    float fw, fh;
    if (ovalWidth == 0 || ovalHeight == 0) {
        CGContextAddRect(context, rect);
        return;
    }
    CGContextSaveGState(context);
    CGContextTranslateCTM (context, CGRectGetMinX(rect), CGRectGetMinY(rect));
    CGContextScaleCTM (context, ovalWidth, ovalHeight);
    fw = CGRectGetWidth (rect) / ovalWidth;
    fh = CGRectGetHeight (rect) / ovalHeight;
    CGContextMoveToPoint(context, fw, fh/2);
    CGContextAddArcToPoint(context, fw, fh, fw/2, fh, 1);
    CGContextAddArcToPoint(context, 0, fh, 0, fh/2, 1);
    CGContextAddArcToPoint(context, 0, 0, fw/2, 0, 1);
    CGContextAddArcToPoint(context, fw, 0, fw, fh/2, 1);
    CGContextClosePath(context);
    CGContextRestoreGState(context);
}

@implementation IPAInstallerHelper

+ (void)ensurePathAt:(NSString *)path
{
    NSError *error;
    NSFileManager *fm = [NSFileManager defaultManager];
    if ( [fm fileExistsAtPath:path] == false ) {
        [fm createDirectoryAtPath:path
      withIntermediateDirectories:YES
                       attributes:nil
                            error:&error];
        if (error) {
            NSLog(@"Ensure Error: %@", error);
        }
        NSLog(@"Creating the missed path");
    }
}

+ (BOOL)deleteFileAtPath:(NSString *)filePath {
    BOOL deleted = NO;
    NSError *error = nil;
    [[NSFileManager defaultManager] removeItemAtPath:filePath error:&error];
    if (error) {
        deleted = NO;
        NSLog(@"Error while deleting file : %@", error.localizedDescription);
    } else {
        deleted = YES;
    }
    return deleted;
}

+ (NSString *)tempPath {
    return NSTemporaryDirectory();
}

+ (NSString *)ipainstallerSubPath {
    NSString *appTempPath = [[self tempPath] stringByAppendingPathComponent:@"TweakApp"];
    [self ensurePathAt:appTempPath];
    return appTempPath;
}

+ (NSString *)ipaExtractedPath {
    NSString *appTempExtractedPath = [[self ipainstallerSubPath] stringByAppendingPathComponent:@"ExtractedPath"];
    [self ensurePathAt:appTempExtractedPath];
    return appTempExtractedPath;
}
+ (NSString *)payloadExtractedPath {
    NSString *appTempPayloadExtractedPath = [[self ipaExtractedPath] stringByAppendingPathComponent:@"Payload"];
    [self ensurePathAt:appTempPayloadExtractedPath];
    return appTempPayloadExtractedPath;
}

+ (NSImage *)roundCornersImage:(NSImage *)image CornerRadius:(NSInteger)radius {
    int w = (int) image.size.width;
    int h = (int) image.size.height;
    
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(NULL, w, h, 8, 4 * w, colorSpace, kCGImageAlphaPremultipliedFirst);
    
    CGContextBeginPath(context);
    CGRect rect = CGRectMake(0, 0, w, h);
    addRoundedRectToPath(context, rect, radius, radius);
    CGContextClosePath(context);
    CGContextClip(context);
    
    CGImageRef cgImage = [[NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]] CGImage];
    
    CGContextDrawImage(context, CGRectMake(0, 0, w, h), cgImage);
    
    CGImageRef imageMasked = CGBitmapContextCreateImage(context);
    CGContextRelease(context);
    CGColorSpaceRelease(colorSpace);
    
    NSImage *tmpImage = [[NSImage alloc] initWithCGImage:imageMasked size:image.size];
    NSData *imageData = [tmpImage TIFFRepresentation];
    NSImage *lastImage = [[NSImage alloc] initWithData:imageData];
    
    return lastImage;
}

+ (NSArray *)getCertifications
{
    NSTask *getCerTask = [[NSTask alloc] init];
    NSPipe *pie = [NSPipe pipe];
    [getCerTask setLaunchPath:@"/usr/bin/security"];
    [getCerTask setArguments:@[@"find-identity", @"-v", @"-p", @"codesigning"]];
    [getCerTask setStandardOutput:pie];
    [getCerTask setStandardError:pie];
    [getCerTask launch];
    //[getCerTask waitUntilExit];
    
    NSFileHandle *fileHandle = [pie fileHandleForReading];
    NSString *securityResult = [[NSString alloc] initWithData:[fileHandle readDataToEndOfFile] encoding:NSASCIIStringEncoding];
    NSArray *rawResult = [securityResult componentsSeparatedByString:@"\""];
    NSMutableArray *tempGetCertsResult = [NSMutableArray arrayWithCapacity:20];
    for (int i = 0; i <= rawResult.count - 2; i+=2) {
        if (rawResult.count - 1 < i + 1) {
            
        } else {
            [tempGetCertsResult addObject:[rawResult objectAtIndex:i+1]];
        }
    }
    return tempGetCertsResult;
}

+ (BOOL)copyDylibFile:(NSString *)dylibFile toPath:(NSString *)path {
    NSString *command = [NSString stringWithFormat:@"cp %@ %@", dylibFile , path];
    NSString *script =  [NSString stringWithFormat:@"do shell script \"%@\" with administrator privileges", command];
    NSAppleScript *appleScript = [[NSAppleScript new] initWithSource:script];
    return ([appleScript executeAndReturnError:nil] != nil);
}

+ (BOOL)ApplyCommandWithRoot:(NSString *)commandString {
    NSString *command = [NSString stringWithFormat:@"%@", commandString];
    NSString *script =  [NSString stringWithFormat:@"do shell script \"%@\" with administrator privileges", command];
    NSAppleScript *appleScript = [[NSAppleScript new] initWithSource:script];
    return ([appleScript executeAndReturnError:nil] != nil);
}

+ (BOOL)excuteSignCommandWithFilePath:(NSString *)ipaPath certName:(NSString *)certName profilePath:(NSString *)profilePath newBID:(NSString *)bundleID appName:(NSString *)appName {
    
    NSString *script =  [NSString stringWithFormat:@"do shell script \"cd %@; ./AppSignerCMD -f %@ -s \"%@\" -p \"%@\" -i %@ -n %@ -o %@\" with administrator privileges", [self ipaExtractedPath], ipaPath, certName, profilePath, bundleID, appName, [NSString stringWithFormat:@"%@-signed.ipa", ipaPath.stringByDeletingPathExtension]];
    NSAppleScript *appleScript = [[NSAppleScript new] initWithSource:script];
    return ([appleScript executeAndReturnError:nil] != nil);
}

+ (void)createShellFileWithContents:(NSString *)contents {
    
//    [self ApplyCommandWithRoot:[NSString stringWithFormat:@"%@ >> %@", contents, [[self ipaExtractedPath] stringByAppendingPathComponent:@"TweakApp.sh"]]];
    
    [[NSFileManager defaultManager] createFileAtPath:[[self ipaExtractedPath] stringByAppendingPathComponent:@"TweakApp.sh"] contents:nil attributes:nil];
    [contents writeToFile:[[self ipaExtractedPath] stringByAppendingPathComponent:@"TweakApp.sh"] atomically:YES encoding:NSUTF8StringEncoding error:nil];

    
}
@end
