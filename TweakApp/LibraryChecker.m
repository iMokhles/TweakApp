//
//  LibraryChecker.m
//  AppInstaller
//
//  Created by Artem on 04/12/14.
//  Copyright (c) 2014 Globus-ltd. All rights reserved.
//

#import "LibraryChecker.h"
#import "DJProgressHUD.h"

#define kLibDestinationPath @"/usr/local/lib/"
#define kLibExtention @"dylib"
#define kLibSourcePath @"Libs"

@implementation LibraryChecker

+ (NSString *)librariesPath {
    return [[NSBundle mainBundle] pathForResource:kLibSourcePath ofType:nil];
}

+ (BOOL)installLibrariesFromPath:(NSString *)path {
    
    [DJProgressHUD showStatus:@"Installing Libraries" FromView:[[NSApplication sharedApplication] keyWindow].contentView];
    
    path = [path stringByAppendingPathComponent:@"."];
    NSString *command = [NSString stringWithFormat:@"cp -a %@ %@", path , kLibDestinationPath];
    NSString *script =  [NSString stringWithFormat:@"do shell script \"%@\" with administrator privileges", command];
    NSAppleScript *appleScript = [[NSAppleScript new] initWithSource:script];
    return ([appleScript executeAndReturnError:nil] != nil);
}

+ (BOOL)isLibraryInstalled {
    
    NSString *librariesPath = [LibraryChecker librariesPath];
    NSArray *libraries;
    NSFileManager *fm = [NSFileManager defaultManager];
    if ([fm fileExistsAtPath:librariesPath]) {
        libraries = [fm contentsOfDirectoryAtPath:librariesPath error:nil];
    }
    
    BOOL installed = NO;
    
    for (NSString *library in libraries) {
        
        installed = [[NSFileManager defaultManager] fileExistsAtPath:[kLibDestinationPath stringByAppendingPathComponent:library]];
        if (!installed) {
            break;
        } else {
            [DJProgressHUD dismiss];
        }
    }
    
    return installed;
}

+ (BOOL)installLibraries {
    
    NSString *path = [[NSBundle mainBundle] pathForResource:kLibSourcePath ofType:nil];
    if ([LibraryChecker installLibrariesFromPath:path]) {
        [DJProgressHUD dismiss];
    } else {
        NSLog(@"Error");
    }
    return [LibraryChecker isLibraryInstalled];
}


@end
