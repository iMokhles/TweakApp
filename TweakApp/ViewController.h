//
//  ViewController.h
//  TweakApp
//
//  Created by iMokhles on 04/04/16.
//  Copyright © 2016 iMokhles. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DragView.h"

@interface ViewController : NSViewController <DragViewDelegate>
@property (strong) IBOutlet NSImageView *deviceConnectedLogoView;
@property (strong) IBOutlet DragView *dragView;
@property (strong) IBOutlet NSProgressIndicator *progressBar;
@property (strong) IBOutlet NSTextField *progressBarLabel;
@property (strong) IBOutlet NSTextField *deviceName;
@property (strong) IBOutlet NSTextField *ipaTextField;
@property (strong) IBOutlet NSButton *uniButton;
@property (strong) IBOutlet NSTextView *logTextView;
@property (strong) IBOutlet NSButton *installLibraries;
- (IBAction)installLibrariesTapped:(id)sender;
- (IBAction)uniButtonTapped:(id)sender;
@property (strong) IBOutlet NSTextField *dylibTextField;
@property (strong) IBOutlet NSTableView *certsTableView;
@property (strong) IBOutlet NSTextField *profileTextField;
@property (strong) IBOutlet NSTextField *bundleIDTextField;
@property (strong) IBOutlet NSTextField *appNameTextField;


@end

