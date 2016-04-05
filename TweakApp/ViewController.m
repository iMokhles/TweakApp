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
}
@end

static NSImage *redImage = nil;
static NSImage *greenImage = nil;

@implementation ViewController
@synthesize deviceConnectedLogoView;

- (void)viewDidLoad {
    [super viewDidLoad];

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
    _uniButton.title = @"Sign";
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
    } else if ([sender.title isEqualToString:@"Patch"]) {
        [self applyDylibProcess];
    } else if ([sender.title isEqualToString:@"Install"]) {
        [self instalApp];
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
                dispatch_async(dispatch_get_main_queue(), ^{
                    [self updateProgressLabel:[NSString stringWithFormat:@"Installing: %@", [[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:appFile]]];
                });
                STPrivilegedTask *privilegedTask = [[STPrivilegedTask alloc] init];
                [privilegedTask setLaunchPath:[[NSBundle mainBundle] pathForResource:@"ideviceinstaller" ofType:nil]];
                [privilegedTask setArguments:@[@"-i", [[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:appFile]]];
                
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

static NSString *outPutPath;
- (void)signAppBeforeAnyProcess {
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
        outPutPath = [_ipaTextField.stringValue stringByDeletingLastPathComponent];
        outPutPath = [outPutPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@-signed.ipa", _ipaTextField.stringValue.lastPathComponent.stringByDeletingPathExtension]];
        NSString *signerPath = [[NSBundle mainBundle] pathForResource:@"AppSignerCMD" ofType:nil];
        
        NSArray *args = @[@"-f", _ipaTextField.stringValue, @"-s", certificateName, @"-p", _profileTextField.stringValue, @"-i", _bundleIDTextField.stringValue, @"-n", _appNameTextField.stringValue, @"-o",outPutPath];
        NSLog(@"%@", args);
        STPrivilegedTask *privilegedTask = [[STPrivilegedTask alloc] init];
        [privilegedTask setLaunchPath:signerPath];
        [privilegedTask setArguments:args];
        
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
                [self updateProgressLabel:@""];
                [self updateProgressLabel:@"Waiting till the sign process finishn"];
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
        dispatch_async(dispatch_get_main_queue(), ^{
            [self updateProgressLabel:output];
        });
        
        if ([output containsString:@"17"]) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [_progressBarLabel setStringValue:@"Signed"];
                _uniButton.title = @"Extract";
                _ipaTextField.stringValue = outPutPath;
                [self updateProgressLabel:@""];
                [self updateProgressLabel:@"Click extract now"];
            });
        }
        // go read more data in the background
        [[aNotification object] readInBackgroundAndNotify];
    } else {
        // do something else
    }
}

- (void)applyDylibProcess {
    
    if (_dylibTextField.stringValue) {
        GCDTask* pingTask = [[GCDTask alloc] init];
        [pingTask setArguments:@[@"-fs", certificateName, _dylibTextField.stringValue]];
        [pingTask setLaunchPath:@"/usr/bin/codesign"];
        [pingTask launchWithOutputBlock:^(NSData *stdOutData) {
            NSString* output = [[NSString alloc] initWithData:stdOutData encoding:NSUTF8StringEncoding];
            NSLog(@"OUT: %@", output);
            dispatch_async(dispatch_get_main_queue(), ^{
                [self updateProgressLabel:output];
            });
        } andErrorBlock:^(NSData *stdErrData) {
            NSString* output = [[NSString alloc] initWithData:stdErrData encoding:NSUTF8StringEncoding];
            NSLog(@"ERR: %@", output);
            dispatch_async(dispatch_get_main_queue(), ^{
                [self updateProgressLabel:output];
            });
        } onLaunch:^{
            NSLog(@"Task has started running.");
        } onExit:^{
            NSLog(@"Task has now quit.");
        }];
    }
    BOOL isPathExiste = [[NSFileManager defaultManager] fileExistsAtPath:[IPAInstallerHelper payloadExtractedPath]];
    if (isPathExiste) {
        for (NSString *appFile in [[NSFileManager defaultManager] contentsOfDirectoryAtPath:[IPAInstallerHelper payloadExtractedPath] error:nil]) {
            if ([appFile.lowercaseString containsString:@"app"]) {
                
                NSString *appPath = [[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:appFile];
                NSString *appPathEdited = [[IPAInstallerHelper payloadExtractedPath] stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.app", _appNameTextField.stringValue]];
                [IPAInstallerHelper ApplyCommandWithRoot:[NSString stringWithFormat:@"mv %@ %@", appPath, appPathEdited]];
                [IPAInstallerHelper copyDylibFile:_dylibTextField.stringValue toPath:appPathEdited];
                NSString *appBinary = [appPathEdited stringByAppendingPathComponent:appFile.stringByDeletingPathExtension];
                [self loadFileInBinary:appBinary];
                NSLog(@"%@", appBinary);
            }
        }
    }
}

- (void)loadFileInBinary:(NSString *)binaryPath {
    
    NSString *optoolPath = [[NSBundle mainBundle] pathForResource:@"optool" ofType:nil];
    NSString *dylibName = [_dylibTextField.stringValue lastPathComponent];
    NSString *dylibNameForOptool = [NSString stringWithFormat:@"@executable_path/%@", dylibName];
    NSLog(@"***** %@", dylibNameForOptool);
    if (dylibName.length < 1) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self updateProgressLabel:@""];
            [self updateProgressLabel:@"You have to add dylib file"];
        });
        
    } else {
        GCDTask* pingTask = [[GCDTask alloc] init];
        [pingTask setLaunchPath:optoolPath];
        [pingTask setArguments:@[@"install", @"-c", @"load", @"-p", dylibNameForOptool, @"-t", binaryPath]];
        [pingTask launchWithOutputBlock:^(NSData *stdOutData) {
            NSString* output = [[NSString alloc] initWithData:stdOutData encoding:NSUTF8StringEncoding];
            // Writing executable
            NSLog(@"OUT: %@", output);
            dispatch_async(dispatch_get_main_queue(), ^{
                [self updateProgressLabel:output];
            });
            if ([output containsString:@"executable"]) {
                NSLog(@"YES");
            }
        } andErrorBlock:^(NSData *stdErrData) {
            //
            if (stdErrData != nil) {
                NSLog(@"NO");
            }
            
        } onLaunch:^{

        } onExit:^{
            dispatch_async(dispatch_get_main_queue(), ^{
                [_progressBarLabel setStringValue:@"Patched"];
                [_uniButton setTitle:@"Install"];
                [self updateProgressLabel:@""];
                [self updateProgressLabel:@"Click Install now"];
            });
        }];
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
            [self updateProgress:100];
            [self updateProgressLabel:@"unzipped DONE"];
            [_progressBarLabel setStringValue:@"Extracted"];
            [_uniButton setTitle:@"Patch"];
            [self updateProgressLabel:@""];
            [self updateProgressLabel:@"Click Patch now"];
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
