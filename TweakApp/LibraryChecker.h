//
//  LibraryChecker.h
//  AppInstaller
//
//  Created by Artem on 04/12/14.
//  Copyright (c) 2014 Globus-ltd. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface LibraryChecker : NSObject
+ (BOOL)isLibraryInstalled;
+ (BOOL)installLibraries;
@end
