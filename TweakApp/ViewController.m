//
//  ViewController.m
//  TweakApp
//
//  Created by iMokhles on 04/04/16.
//  Copyright © 2016 iMokhles. All rights reserved.
//

#import "ViewController.h"
#import "LibraryChecker.h"
#import "MobileDeviceAccess.h"
#import "IPAInstallerHelper.h"
#import "SSZipArchive.h"
#import "GCDTask.h"
#import "STPrivilegedTask.h"

@interface ViewController () <MobileDeviceAccessListener, SSZipArchiveDelegate, NSTableViewDelegate, NSTableViewDataSource> {
    NSArray *certsArray;
    NSString *certificateName;
    NSString *appBinaryStatic;
    NSString *appPathStatic;
}
@end

static NSImage *redImage = nil;
static NSImage *greenImage = nil;
static BOOL isFirstExtract = YES;

@implementation ViewController
@synthesize deviceConnectedLogoView;

- (void)viewDidLoad {
    [super viewDidLoad];
    [self copyResourcesToTempPath];
    greenImage = [IPAInstallerHelper roundCornersImage:[NSImage imageNamed:@"greenImage"] CornerRadius:10];
    redImage = [IPAInstallerHelper roundCornersImage:[NSImage imageNamed:@"redImage"] CornerRadius:10];
    [deviceConnectedLogoView setImage:redImage];
    // Do any additional setup after loading the view.
    [[MobileDeviceAccess singleton] setListener:self];
    [self checkLibraries];
    
    certsArray = [IPAInstallerHelper getCertifications];
    if (certsArray.count < 1) {
        [self updateProgressLabel:@"You don't have any developer certificate installed"];
    }
    NSLog(@"%@", certsArray);
    [_certsTableView reloadData];
    
    NSData *shellContents = [NSString stringWithFormat:
                             @"#!/bin/bash\n\n"
                             @"CERT_NAME=\"%@\"", certificateName];
    [IPAInstallerHelper createShellFileWithContents:nil];
}

- (void)checkLibraries {
    BOOL librariesInstalled = [LibraryChecker isLibraryInstalled];
    self.installLibraries.hidden = librariesInstalled;
}

- (void)draggedFileAtPath:(NSURL *)path {
    [self uploadIPA:path];
}

- (void)uploadIPA:(NSURL *)url {
    if ([url.path.pathExtension isEqualToString:@"ipa"]) {
        _ipaTextField.stringValue = url.path;
    } else if ([url.path.pathExtension isEqualToString:@"dylib"]) {
        _dylibTextField.stringValue = url.path;
    } else if ([url.path.pathExtension isEqualToString:@"mobileprovision"]) {
        _profileTextField.stringValue = url.path;
    }
    _uniButton.title = @"Extract";
}

- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}

- (IBAction)installLibrariesTapped:(id)sender {
    BOOL success = [LibraryChecker installLibraries];
    self.installLibraries.hidden = success;
}

- (IBAction)uniButtonTapped:(NSButton *)sender {
    if ([sender.title isEqualToString:@"Sign"]) {
        [self signAppBeforeAnyProcess];
    } else if ([sender.title isEqualToString:@"Extract"]) {
        [_progressBarLabel setStringValue:@"Extracting"];
        
        if (isFirstExtract) {
            NSArray * files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:[IPAInstallerHelper payloadExtractedPath] error:nil];
            if (files.count > 0) {
                if ([IPAInstallerHelper deleteFileAtPath:[IPAInstallerHelper payloadExtractedPath]]) {
                    NSLog(@"Cleaning old extraction");
                    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
                        [SSZipArchive unzipFileAtPath:self.ipaTextField.stringValue toDestination:[IPAInstallerHelper ipaExtractedPath] delegate:self];
                    });
                    
                }
            } else {
                dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
                    [SSZipArchive unzipFileAtPath:self.ipaTextField.stringValue toDestination:[IPAInstallerHelper ipaExtractedPath] delegate:self];
                });
            }
        } else {
            NSArray * files = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:[IPAInstallerHelper payloadExtractedPath] error:nil];
            if (files.count > 0) {
                if ([IPAInstallerHelper deleteFileAtPath:[IPAInstallerHelper payloadExtractedPath]]) {
                    NSLog(@"Cleaning old extraction");
                    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
                        [SSZipArchive unzipFileAtPath:[NSString stringWithFormat:@"%@-signed.ipa",[[IPAInstallerHelper ipaExtractedPath] stringByAppendingPathComponent:@"AppZipped.ipa"].stringByDeletingPathExtension] toDestination:[IPAInstallerHelper ipaExtractedPath] delegate:self];
                    });
                    
                }
            } else {
                dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), ^{
                    [SSZipArchive unzipFileAtPath:[NSString stringWithFormat:@"%@-signed.ipa",[[IPAInstallerHelper ipaExtractedPath] stringByAppendingPathComponent:@"AppZipped.ipa"].stringByDeletingPathExtension] toDestination:[IPAInstallerHelper ipaExtractedPath] delegate:self];
                });
            }
        }
        
    } else if ([sender.title isEqualToString:@"Patch"]) {
        [self createShellFile];
    } else if ([sender.title isEqualToString:@"Install"]) {
        [self instalApp];
    }
}

- (void)createShellFile {
    if (_profileTextField.stringValue.length < 1) {
        [self updateProgressLabel:@""];
        [self updateProgressLabel:@"You have to add provisioning profile"];
    } else if (certificateName.length < 1) {
        [self updateProgressLabel:@""];
        [self updateProgressLabel:@"You have to select developer certificate"];
    } else if (_ipaTextField.stringValue.length < 1) {
        [self updateProgressLabel:@""];
        [self updateProgressLabel:@"You have to select ipa file"];
    } else if (_appNameTextField.stringValue.length < 1) {
        [self updateProgressLabel:@""];
        [self updateProgressLabel:@"You have to type new App Display name"];
    } else if (_bundleIDTextField.stringValue.length < 1) {
        [self updateProgressLabel:@""];
        [self updateProgressLabel:@"You have to type same provisioning profile bundle ID"];
    } else {
        BOOL isPathExiste = [[NSFileManager defaultManager] fileExistsAtPath:[IPAInstallerHelper payloadExtractedPath]];
        
        if (isPathExiste) {
            for (NSString *appFile in [[NSFileManager defaultManager] contentsOfDirectoryAtPath:[IPAInstallerHelper payloadExtractedPath] error:nil]) {
                if ([appFile.lowercaseString containsString:@"app"]) {
                    NSString *appPath = [[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:appFile];
                    appPathStatic = appPath;
                    NSString *appBinary = [appPath stringByAppendingPathComponent:appFile.stringByDeletingPathExtension];
                    appBinaryStatic = appBinary;
                    NSString *shellContents = [NSString stringWithFormat:
                                               
                                               @"#!/bin/bash\n\n"
                                               
                                               @"cd %@\n\n"
                                               
                                               @"CERT_NAME=\"%@\"\n\n"
                                               
                                               @"ProfilePath=\"%@\"\n\n"
                                               
                                               @"DIRIPA=\"%@\"\n\n"
                                               
                                               @"DIRIPAOut=\"%@\"\n\n"
                                               
                                               @"BUNDLEID=\"%@\"\n\n"
                                               
                                               @"BUNDLENAME=\"%@\"\n\n"
                                               
                                               @"DYLIB=\"%@\"\n\n"
                                               
                                               @"APPDIR2=\"%@\"\n\n"
                                               
                                               @"######### Changing app values\n\n"
                                               
                                               @"#/usr/libexec/PlistBuddy -c \"Set :CFBundleDisplayName %@\" \"%@\""
                                               
                                               @"######### Signing tweaks\n\n"
                                               
                                               @"codesign -fs \"$CERT_NAME\" $DYLIB\n\n"
                                               
                                               @"echo '[+] Copying tweaks files'\n\n"
                                               
                                               @"cp $DYLIB $APPDIR2\n\n"
                                               
                                               @"echo '[+] Repacking the .ipa'\n\n"
                                               
                                               @"./optool uninstall -p @executable_path/%@ -t %@\n\n"
                                               
                                               @"./optool install -c load -p @executable_path/%@ -t %@\n\n"
                                               
                                               @"zip -9r \"AppZipped.ipa\" Payload/ >/dev/null 2>&1\n\n"
                                               
                                               @"echo \"Wrote AppZipped.ipa\"\n\n"
                                               
                                               @"echo \"Signing\"\n\n"
                                               
                                               @"./AppSignerCMD -f $DIRIPA -s \"$CERT_NAME\" -p \"$ProfilePath\" -i $BUNDLEID -n $BUNDLENAME -o $DIRIPAOut\n\n"
                                               
                                               ,[IPAInstallerHelper ipaExtractedPath]
                                               ,certificateName
                                               ,_profileTextField.stringValue
                                               ,[[IPAInstallerHelper ipaExtractedPath] stringByAppendingPathComponent:@"AppZipped.ipa"]
                                               ,[NSString stringWithFormat:@"%@-signed.ipa",[[IPAInstallerHelper ipaExtractedPath] stringByAppendingPathComponent:@"AppZipped.ipa"].stringByDeletingPathExtension]
                                               ,_bundleIDTextField.stringValue
                                               ,_appNameTextField.stringValue
                                               ,_dylibTextField.stringValue
                                               ,appPathStatic
                                               ,_bundleIDTextField.stringValue
                                               ,[appPathStatic stringByAppendingString:@"Info.plist"]
                                               ,_dylibTextField.stringValue.lastPathComponent
                                               ,appBinaryStatic
                                               ,_dylibTextField.stringValue.lastPathComponent
                                               ,appBinaryStatic
                                               ];
                    [IPAInstallerHelper createShellFileWithContents:shellContents];
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [_progressBarLabel setStringValue:@"Patched"];
                        _uniButton.title = @"Sign";
                        [self updateProgressLabel:@""];
                        [self updateProgressLabel:@"Click Sign now"];
                    });
                }
                
            }
            
        }
    }
}

- (void)updateProgress:(CGFloat)progress {
    [_progressBar setDoubleValue:progress];
}

- (void)updateProgressLabel:(NSString *)file {
    [[_logTextView textStorage] beginEditing];
    [[[_logTextView textStorage] mutableString] appendString:file];
    [[[_logTextView textStorage] mutableString] appendString:@"\n"];
    [[_logTextView textStorage] endEditing];
    
    NSRange range;
    range = NSMakeRange ([[_logTextView string] length], 0);
    
    [_logTextView scrollRangeToVisible: range];
}

- (void)instalApp {
    
    BOOL isPathExiste = [[NSFileManager defaultManager] fileExistsAtPath:[IPAInstallerHelper payloadExtractedPath]];
    
    if (isPathExiste) {
        for (NSString *appFile in [[NSFileManager defaultManager] contentsOfDirectoryAtPath:[IPAInstallerHelper payloadExtractedPath] error:nil]) {
            if ([appFile.lowercaseString containsString:@"app"]) {
                NSLog(@"Installing: %@", [[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:appFile]);
                [self fixAppBundleNameFromAppPath:[[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:appFile]];
                
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self updateProgressLabel:[NSString stringWithFormat:@"Installing: %@", [[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:appFile]]];
                });
                NSString *bundleNameKey = @"CFBundleName";
                NSDictionary *infoPlist = [NSDictionary dictionaryWithContentsOfFile:[[[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:appFile] stringByAppendingPathComponent:@"Info.plist"]];
                NSLog(@"**** %@", infoPlist);
                NSString *bundleName = [infoPlist objectForKey:bundleNameKey];
                NSLog(@"**** %@", bundleName);
                NSString *appPathEdited = [[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.app", bundleName]];
                STPrivilegedTask *privilegedTask = [[STPrivilegedTask alloc] init];
                [privilegedTask setLaunchPath:[[NSBundle mainBundle] pathForResource:@"ideviceinstaller" ofType:nil]];
                [privilegedTask setArguments:@[@"-i", appPathEdited]];
                
                // Setting working directory is optional, defaults to /
                NSArray * paths = NSSearchPathForDirectoriesInDomains (NSDesktopDirectory, NSUserDomainMask, YES);
                NSString * desktopPath = [paths objectAtIndex:0];
                [privilegedTask setCurrentDirectoryPath:desktopPath];
                
                // Launch it, user is prompted for password
                OSStatus err = [privilegedTask launch];
                if (err != errAuthorizationSuccess) {
                    if (err == errAuthorizationCanceled) {
                        NSLog(@"User cancelled");
                        dispatch_async(dispatch_get_main_queue(), ^{
                            [self updateProgressLabel:@"User cancelled"];
                        });
                    } else {
                        NSLog(@"Something went wrong");
                        dispatch_async(dispatch_get_main_queue(), ^{
                            [self updateProgressLabel:@"Something went wrong"];
                        });
                    }
                } else {
                    NSLog(@"Task successfully launched");
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [self updateProgressLabel:@"Task successfully launched"];
                    });
                }
                
                NSFileHandle *readHandle = [privilegedTask outputFileHandle];
                [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getInstallOutputData:) name:NSFileHandleReadCompletionNotification object:readHandle];
                [readHandle readInBackgroundAndNotify];
            }
        }
        
    }
    
}

- (BOOL)fixAppBundleNameFromAppPath:(NSString *)appPath {
    
    NSString *bundleNameKey = @"CFBundleName";
    NSDictionary *infoPlist = [NSDictionary dictionaryWithContentsOfFile:[appPath stringByAppendingPathComponent:@"Info.plist"]];
    NSString *bundleName = [infoPlist objectForKey:bundleNameKey];
    NSString *appPathEdited = [[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.app", bundleName]];
    
    return [IPAInstallerHelper ApplyCommandWithRoot:[NSString stringWithFormat:@"mv %@ %@", appPath, appPathEdited]];
}

- (BOOL)makeAppBinaryExecutable:(NSString *)appBinary {
    return [IPAInstallerHelper ApplyCommandWithRoot:[NSString stringWithFormat:@"chmod +x %@", appBinary]];
}

- (void)getInstallOutputData:(NSNotification *)aNotification {
    //get data from notification
    NSData *data = [[aNotification userInfo] objectForKey:NSFileHandleNotificationDataItem];
    
    //make sure there's actual data
    if ([data length]) {
        // do something with the data
        NSString* output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        NSLog(@"OUT: %@", output);
        dispatch_async(dispatch_get_main_queue(), ^{
            if ([output containsString:@"5"]) {
                [self updateProgress:5.0f];
            }
            if ([output containsString:@"15"]) {
                [self updateProgress:15.0f];
            }
            if ([output containsString:@"20"]) {
                [self updateProgress:20.0f];
            }
            if ([output containsString:@"30"]) {
                [self updateProgress:30.0f];
            }
            if ([output containsString:@"40"]) {
                [self updateProgress:40.0f];
            }
            if ([output containsString:@"50"]) {
                [self updateProgress:50.0f];
            }
            if ([output containsString:@"60"]) {
                [self updateProgress:60.0f];
            }
            if ([output containsString:@"70"]) {
                [self updateProgress:70.0f];
            }
            if ([output containsString:@"80"]) {
                [self updateProgress:80.0f];
            }
            if ([output containsString:@"90"]) {
                [self updateProgress:90.0f];
            }
            if ([output containsString:@"Complete"]) {
                [self updateProgress:100.0f];
                [self resetAllContents];
            }
            [self updateProgressLabel:output];
        });
        // go read more data in the background
        [[aNotification object] readInBackgroundAndNotify];
    } else {
        // do something else
    }
}

- (void)copyResourcesToTempPath {
    NSString *signerPath = [[NSBundle mainBundle] pathForResource:@"AppSignerCMD" ofType:nil];
    NSString *optoolPath = [[NSBundle mainBundle] pathForResource:@"optool" ofType:nil];
    
    [IPAInstallerHelper ApplyCommandWithRoot:[NSString stringWithFormat:@"cp %@ %@", optoolPath, [IPAInstallerHelper ipaExtractedPath]]];
    [IPAInstallerHelper ApplyCommandWithRoot:[NSString stringWithFormat:@"cp %@ %@", signerPath, [IPAInstallerHelper ipaExtractedPath]]];
}

- (void)signAppBeforeAnyProcess {
    isFirstExtract = YES;
    if (_profileTextField.stringValue.length < 1) {
        [self updateProgressLabel:@""];
        [self updateProgressLabel:@"You have to add provisioning profile"];
    } else if (certificateName.length < 1) {
        [self updateProgressLabel:@""];
        [self updateProgressLabel:@"You have to select developer certificate"];
    } else if (_ipaTextField.stringValue.length < 1) {
        [self updateProgressLabel:@""];
        [self updateProgressLabel:@"You have to select ipa file"];
    } else if (_appNameTextField.stringValue.length < 1) {
        [self updateProgressLabel:@""];
        [self updateProgressLabel:@"You have to type new App Display name"];
    } else if (_bundleIDTextField.stringValue.length < 1) {
        [self updateProgressLabel:@""];
        [self updateProgressLabel:@"You have to type same provisioning profile bundle ID"];
    } else {
        // start sign
        STPrivilegedTask *privilegedTask = [[STPrivilegedTask alloc] init];
        [privilegedTask setLaunchPath:@"/bin/sh"];
        [privilegedTask setArguments:@[[[IPAInstallerHelper ipaExtractedPath] stringByAppendingPathComponent:@"TweakApp.sh"]]];
        
        // Launch it, user is prompted for password
        OSStatus err = [privilegedTask launch];
        if (err != errAuthorizationSuccess) {
            if (err == errAuthorizationCanceled) {
                NSLog(@"User cancelled");
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self updateProgressLabel:@"User cancelled"];
                });
            } else {
                NSLog(@"Something went wrong");
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self updateProgressLabel:@"Something went wrong"];
                });
            }
        } else {
            NSLog(@"Task successfully launched");
            dispatch_async(dispatch_get_main_queue(), ^{
                [self updateProgressLabel:@"Task successfully launched"];
            });
        }
        
        NSFileHandle *readHandle = [privilegedTask outputFileHandle];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(getOutputData:) name:NSFileHandleReadCompletionNotification object:readHandle];
        [readHandle readInBackgroundAndNotify];
        
    }
}

- (void)getOutputData:(NSNotification *)aNotification {
    //get data from notification
    NSData *data = [[aNotification userInfo] objectForKey:NSFileHandleNotificationDataItem];
    
    //make sure there's actual data
    if ([data length]) {
        // do something with the data
        NSString* output = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        NSLog(@"OUT: %@", output);
        dispatch_async(dispatch_get_main_queue(), ^{
            [self updateProgressLabel:output];
        });
        if ([output containsString:@"17"]) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [_progressBarLabel setStringValue:@"Signed"];
                _uniButton.title = @"Extract";
                _ipaTextField.stringValue = [NSString stringWithFormat:@"%@-signed.ipa",_ipaTextField.stringValue.stringByDeletingPathExtension];
                [self updateProgressLabel:@""];
                [self updateProgressLabel:@"Click Extract now"];
                isFirstExtract = NO;
            });
        }
        // go read more data in the background
        [[aNotification object] readInBackgroundAndNotify];
    } else {
        // do something else
    }
}

- (void)resetAllContents {
    if ([IPAInstallerHelper deleteFileAtPath:[IPAInstallerHelper payloadExtractedPath]]) {
        _ipaTextField.stringValue = @"";
        _dylibTextField.stringValue = @"";
        _uniButton.title = @"Browse";
        _progressBarLabel.stringValue = @"unzip";
    }
}

#pragma mark - NSTableViewDelegate/DataSource

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return certsArray.count;
}

- (nullable id)tableView:(NSTableView *)tableView objectValueForTableColumn:(nullable NSTableColumn *)tableColumn row:(NSInteger)row {
    return certsArray[row];
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    NSTableView *table = [notification object];
    certificateName = [certsArray objectAtIndex:[table selectedRow]];
}
#pragma mark - SSZipArchiveDelegate

- (void)zipArchiveDidUnzipFileAtIndex:(NSInteger)fileIndex totalFiles:(NSInteger)totalFiles archivePath:(NSString *)archivePath unzippedFilePath:(NSString *)unzippedFilePath {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self updateProgressLabel:unzippedFilePath];
    });
}

- (void)zipArchiveProgressEvent:(unsigned long long)loaded total:(unsigned long long)total {
    
    dispatch_async(dispatch_get_main_queue(), ^{
        CGFloat progress = ((100.0 / total) * loaded);
        if (progress < 100) {
            [self updateProgress:progress];
        } else if (progress == 100) {
            if (isFirstExtract) {
                [self makeAppBinaryExecutable:[appPathStatic stringByAppendingPathComponent:[NSString stringWithFormat:@"lib%@", _dylibTextField.stringValue.lastPathComponent]]];
                [self makeAppBinaryExecutable:appBinaryStatic];
                [self updateProgress:100];
                [self updateProgressLabel:@"unzipped DONE"];
                [_progressBarLabel setStringValue:@"Extracted"];
                [_uniButton setTitle:@"Patch"];
                [self updateProgressLabel:@""];
                [self updateProgressLabel:@"Click Patch now"];
            } else {
                [self updateProgress:100];
                [self updateProgressLabel:@"unzipped DONE"];
                [_progressBarLabel setStringValue:@"Extracted"];
                [_uniButton setTitle:@"Install"];
                [self updateProgressLabel:@""];
                [self updateProgressLabel:@"Click Install now"];
            }
        }
        
    });
}
#pragma mark - MobileDeviceAccessListener

/// This method will be called whenever a device is connected
- (void)deviceConnected:(AMDevice *)device {
    [deviceConnectedLogoView setImage:greenImage];
    self.deviceName.stringValue = [NSString stringWithFormat:@"Device Connected: %@", device.deviceName];
}
/// This method will be called whenever a device is disconnected
- (void)deviceDisconnected:(AMDevice *)device {
    [deviceConnectedLogoView setImage:redImage];
    self.deviceName.stringValue = @"Connect Device";
}
@end
