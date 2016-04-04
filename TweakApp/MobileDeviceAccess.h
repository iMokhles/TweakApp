/*! \mainpage MobileDeviceAccess
 *
 * This module is intended to provide access to the iPhone and iPod Touch file systems.
 * It achieves this via the same mechanism that iTunes uses, relying on direct entry
 * points into an undocumented private framework provided by Apple.
 *
 * It will be necessary to include /System/Library/PrivateFrameworks/MobileDevice.framework
 * in any project using this library
 *
 * \author Jeff Laing
 * <br>
 * Copyright 2010 Tristero Computer Systems. All rights reserved.
 * \section intro_sec Typical Usage
 *
 * The application delegate will usually register itself as a listener to the MobileDeviceAccess
 * singleton.  It will then be called back for every iPhone and iPod Touch that connects.  To access
 * files on the AMDevice, create one of the subclasses AFCDirectoryAccess using the 
 * corresponding \p -newAFCxxxDirectory: method.
 *
 * \section refs_sec References
 *
 * This is based on information extracted from around the Internet, by people
 * who wanted to look a lot further under the covers than I did.  I've deliberately
 * pushed all Apple's datatypes back to being opaque, since there is nothing
 * to be gained by looking inside them, and everything to lose.
 *
 * This library doesn't contain any of the 'recovery-mode' or other 'jailbreak-related' stuff
 *
 * Some of the places I looked include:
 * - http://iphonesvn.halifrag.com/svn/iPhone/
 * - http://www.theiphonewiki.com/wiki/index.php?title=MobileDevice_Library
 * - http://code.google.com/p/iphonebrowser/issues/detail?id=52
 * - http://www.koders.com/csharp/fidE79340F4674D47FFF3EFB6F949A1589D942798F3.aspx
 * - http://iphone-docs.org/doku.php?id=docs:protocols:screenshot
 * I can't guarantee that they're still there.
 */
#pragma once

// sometimes pragma once seems to get confused so lets go belt and braces
#if !defined(_MobileDeviceAccess_h_)
#define _MobileDeviceAccess_h_

#import <Foundation/Foundation.h>
#import <Foundation/Foundation.h>
#import <CoreGraphics/CoreGraphics.h>
#include <AppKit/AppKit.h>

#ifdef __cplusplus
extern "C" {
#endif

// Apple's opaque types
typedef uint32_t afc_error_t;
typedef uint64_t afc_file_ref;

// some functions use different size longs.
#if __x86_64__
typedef uint64_t	afc_long;
#else
typedef uint32_t	afc_long;
#endif

/* opaque structures */
typedef struct _am_device				*am_device;
typedef struct _afc_connection			*afc_connection;
typedef struct _am_device_notification	*am_device_notification;

// on OSX, this is a raw file descriptor, not a pointer - it ends up being
// passed directly to send()
//typedef struct _am_service				*am_service;
typedef int								am_service;

/// This class represents a service running on the mobile device.  To create
/// an instance of this class, send the \p -startService: message to an instance
/// of AMDevice.
///
/// To determine the names of all services, look in \p /System/Library/Lockdown/Services.plist - some have details
/// on their implementing binaries, others don't.  For those, look in \p /System/Library/LaunchDaemons in the appropriate
/// plist file (named for the service).
///
/// Some are only present on "developer" systems - look in /Developer/Library/Lockdown/ServiceAgents/* - this may only
/// be available if XCode is connected (?)
///
/// On a jailbroken 3.1.2 iPod Touch, the \p Services.plist lists the following as
/// valid service names:
/// - \p "com.apple.afc" - see AFCMediaDirectory
/// - \p "com.apple.crashreportcopymobile" - see AFCCrashLogDirectory
/// - \p "com.apple.mobile.house_arrest" - see AFCApplicationDirectory
/// - \p "com.apple.mobile.installation_proxy" - see AMInstallationProxy
/// - \p "com.apple.syslog_relay" - see AMSyslogRelay
/// - \p "com.apple.mobile.file_relay" - see AMFileRelay
/// - \p "com.apple.springboardservices" - see AMSpringboardServices
/// - \p "com.apple.mobile.notification_proxy" - see AMNotificationProxy
/// - \p "com.apple.mobilesync" - see AMMobileSync
///			(implemented as /usr/libexec/SyncAgent --lockdown --oneshot -v)
/// - \p "com.apple.crashreportcopy"
///			(implemented as /usr/libexec/CrashReportCopyAgent --lockdown --oneshot)
/// - \p "com.apple.crashreportcopymover"
///			(renamed to com.apple.crashreportmover at 3.1.2)
///			(implemented as /usr/libexec/crash_mover --lockdown)
/// - \p "com.apple.misagent"
///			(implemented as /usr/libexec/misagent)
/// - \p "com.apple.debug_image_mount"
///			(renamed to com.apple.mobile.debug_image_mount at 3.1.2)
///			(implemented as /usr/libexec/debug_image_mount)
/// - \p "com.apple.mobile.integrity_relay"
///			(implemented as /usr/libexec/mobile_integrity_relay)
/// - \p "com.apple.mobile.MCInstall"
///			(implemented as /usr/libexec/mc_mobile_tunnel)
/// - \p "com.apple.mobile.mobile_image_mounter"
///			(implemented as /usr/libexec/mobile_image_mounter)
///			(see also http://iphone-docs.org/doku.php?id=docs:protocols:mobile_image_mounter)
/// - \p "com.apple.mobilebackup"
///			(implemented as /usr/libexec/BackupAgent --lockdown)
///
/// If you use pwnage you get these as well - blackra1n doesn't
/// set them up
/// - \p "com.apple.afc2" - see AFCRootDirectory
/// - \p "org.devteam.utility"
///			(implemented as ??????)
///
/// The following are mentioned in \p Services.plist but the corresponding binaries
/// do not appear to be installed.
///
/// - \p "com.apple.purpletestr"
///			(implemented as /usr/libexec/PurpleTestr --lockdown --oneshot)
/// - \p "com.apple.mobile.diagnostics_relay"
///			(implemented as /usr/libexec/mobile_diagnostics_relay)
/// - \p "com.apple.mobile.factory_proxy"
///			(implemented as /usr/libexec/mobile_factory_proxy)
/// - \p "com.apple.mobile.software_update"
///			(implemented as /usr/libexec/software_update)
/// - \p "com.apple.mobile.system_profiler"
///			(implemented as /usr/sbin/system_profiler)
///
/// The Internet suggests that the following existed in the past:
/// - \p "com.apple.screenshotr"
/// or that its available IFF you are have the Developer disk image
/// mounted
///
/// ios7.0.4 installed the following:
///
/// - \p "com.apple.afc"
///		(implemented as /usr/libexec/afcd --xpc -d /private/var/mobile/Media)
///		(username=mobile)
/// - \p "com.apple.ait.aitd"
///		(implemented as /usr/libexec/aitd)
///		(username=mobile)
/// - \p "com.apple.atc"
///		(implemented as /usr/libexec/atc)
///		(username=mobile)
/// - \p "com.apple.crashreportcopymobile"
///		(implemented as /usr/libexec/afcd --xpc--service-name com.apple.crashreportcopymobile -d /private/var/mobile/Library/Logs/CrashReporter)
///		(username=mobile)
/// - \p "com.apple.crashreportmover"
///		(implemented as /usr/libexec/crash_mover)
/// - \p "com.apple.hpd.mobile"
///		(implemented as /usr/libexec//usr/libexec/hpd  --lockdown -d /var/mobile/Media/HighlandPark -u mobile)
///		(username=mobile)
/// - \p "com.apple.iosdiagnostics.relay"
///		(implemented as /usr/libexec/ios_diagnostics_relay)
///		(username=mobile)
/// - \p "com.apple.misagent"
///		(implemented as /usr/libexec/misagent)
///		(username=mobile)
/// - \p "com.apple.mobile.MCInstall"
///		(implemented as /usr/libexec/mc_mobile_tunnel)
///		(username=mobile)
/// - \p "com.apple.mobile.MDMService"
///		(implemented as /usr/libexec/MDMService)
/// - \p "com.apple.mobile.assertion_agent"
///		(implemented as /usr/libexec/mobile_assertion_agent)
///		(username=mobile)
/// - \p "com.apple.mobile.debug_image_mount"
///		(implemented as /usr/libexec/debug_image_mount)
/// - \p "com.apple.mobile.diagnostics_relay"
///		(implemented as /usr/libexec/mobile_diagnostics_relay)
/// - \p "com.apple.mobile.file_relay"
///		(implemented as /usr/libexec/mobile_file_relay)
/// - \p "com.apple.mobile.heartbeat"
///		(implemented as /usr/libexec/lockdownd)
///		(username=mobile)
/// - \p "com.apple.mobile.house_arrest"
///		(implemented as /usr/libexec/mobile_house_arrest)
///		(username=mobile)
/// - \p "com.apple.mobile.insecure_notification_proxy"
///		(implemented as /usr/libexec/notification_proxy -i)
///		(username=mobile)
/// - \p "com.apple.mobile.installation_proxy"
///		(implemented as /usr/libexec/mobile_installation_proxy)
///		(username=mobile)
/// - \p "com.apple.mobile.mobile_image_mounter"
///		(implemented as /usr/libexec/mobile_storage_proxy)
/// - \p "com.apple.mobile.notification_proxy"
///		(implemented as /usr/libexec/notification_proxy)
///		(username=mobile)
/// - \p "com.apple.mobilebackup"
///		(implemented as /usr/libexec/BackupAgent --lockdown)
/// - \p "com.apple.mobilebackup2"
///		(implemented as /usr/libexec/BackupAgent2 --lockdown)
/// - \p "com.apple.mobilesync"
///		(implemented as /usr/libexec/SyncAgent --lockdown --oneshot -v)
///		(username=mobile)
/// - \p "com.apple.pcapd"
///		(implemented as /usr/libexec/pcapd)
/// - \p "com.apple.purpletestr"
///		(implemented as /usr/libexec/PurpleTestr --lockdown --oneshot)
/// - \p "com.apple.radios.wirelesstester.mobile"
///		(implemented as /usr/local/bin/WirelessTester  -l 1 -o /var/mobile/WirelessTester_mobile.log)
///		(username=mobile)
/// - \p "com.apple.radios.wirelesstester.root"
///		(implemented as /usr/local/bin/WirelessTester  -l 1 -o /var/mobile/WirelessTester_mobile.log)
///		(username=root)
/// - \p "com.apple.rasd"
///		(implemented as /usr/libexec/rasd)
///		(username=mobile)
/// - \p "com.apple.springboardservices"
///		(implemented as /usr/libexec/springboardservicesrelay)
///		(username=mobile)
/// - \p "com.apple.syslog_relay"
///		(implemented as /usr/libexec/syslog_relay)
///		(username=mobile)
/// - \p "com.apple.thermalmonitor.thermtgraphrelay"
///		(implemented as /usr/libexec/thermtgraphrelay)
///		(username=mobile)
/// - \p "com.apple.webinspector"
///		(implemented as /usr/libexec/webinspectord)
///		(username=mobile)

@interface AMService : NSObject {
@protected
	am_service _service;
	NSString *_lasterror;
	id _delegate;
}

/// The last error that occurred on this service
///
/// The object remembers the last error that occurred, which allows most other api's
/// to return YES/NO as their failure condition.  If no error occurred,
/// this property will be nil.
@property (readonly) NSString *lasterror;

/// The delegate for this service.  Whilst AMService does not use it directly,
/// some of the subclasses (like AMInstallationProxy) do.  The delegate is
/// *not* retained - it is the callers responsibility to ensure it remains
/// valid for the life of the service.
@property (assign) id delegate;

@end

/// This class represents a service on the device that uses the DeviceLink protocol.
/// Mostly it exists to add a few helper methods to its subclasses
@interface AMDLService : AMService {
}
@end

/// This class represents an installed application on the device.  To retrieve
/// the list of installed applications on a device use [AMDevice installedApplications]
/// or one of the methods on AMInstallationProxy.
///
/// Information about an application is derived from and maintained in an internal
/// dictionary similar to the following:
/// <PRE>
///    ApplicationType = System;
///    CFBundleDevelopmentRegion = English;
///    CFBundleExecutable = MobileSafari;
///    CFBundleIdentifier = "com.apple.mobilesafari";
///    CFBundleInfoDictionaryVersion = "6.0";
///    CFBundlePackageType = APPL;
///    CFBundleResourceSpecification = "ResourceRules.plist";
///    CFBundleSignature = "????";
///    CFBundleSupportedPlatforms = ( iPhoneOS );
///    CFBundleURLTypes = (
///        {
///            CFBundleURLName = "Web site URL";
///            CFBundleURLSchemes = ( http, https );
///        }, {
///            CFBundleURLName = "Radar URL";
///            CFBundleURLSchemes = ( rdar, radar );
///        }, {
///            CFBundleURLName = "FTP URL";
///            CFBundleURLSchemes = ( ftp );
///        }, {
///            CFBundleURLName = "RSS URL";
///            CFBundleURLSchemes = ( feed, feeds );
///        }
///    );
///    CFBundleVersion = "528.16";
///    DTPlatformName = iphoneos;
///    DTSDKName = "iphoneos3.1.2.internal";
///    LSRequiresIPhoneOS = 1;
///    MinimumOSVersion = "3.1.2";
///    Path = "/Applications/MobileSafari.app";
///    PrivateURLSchemes = ( webclip );
///    SBIsRevealable = 1;
///    SBUsesNetwork = 3;
///    SafariProductVersion = "4.0";
///    UIHasPrefs = 1;
///    UIJetsamPriority = 75;
/// </PRE>
/// The contents of the dictionary are key-value coded in a manner that allows
/// NSPredicate to be used to filter applications. 
/// For example, to locate all applications which use networking, you could use
/// <PRE>
/// [NSPredicate predicateWithFormat:@"SBUsesNetwork != nil"]
/// </PRE>
/// To locate hidden applications, you could use:
/// <PRE>
/// [NSPredicate predicateWithFormat:@"SBAppTags contains 'hidden'" ];
/// </PRE>
@interface AMApplication : NSObject {
@private
	NSDictionary *_info;
	NSString *_appname;
	NSString *_bundleid;
}

/// Return the internal dictionary that contains all our information.  Note that
/// the application may be incomplete since it is filtered by the attributes listed
/// in the applicationAttributes member of the AMDevice object.
- (NSDictionary*) info;

/// Return the name (usually) visible in the Springboard.  To get the actual name
/// being displayed, use AMSpringboardServices getIconState method and search
/// using the AMApplication's bundleid.
- (NSString*) appname;

/// Return the CFBundleID value from
/// the applications Info.plist.
- (NSString*) bundleid;

/// Return the full pathname of the directory that
/// the .app file is installed in
- (NSString*) appdir;

@end


/// This class represents the com.apple.mobile.notification_proxy service
/// running on the device.  It allows programs on the Mac to send simple
/// notifications to programs running on the device.
///
/// To create one, send the \p -newAMNotificationProxy message to an instance of AMDevice.
///
/// To receive notifications on the mobile device, add an observer to the
/// Darwin Notification Center as follows:
/// <pre>
/// static void gotNotification(
///    CFNotificationCenterRef center,
///    void                    *observer,
///    CFStringRef             name,
///    const void              *alwaysZero1,
///    CFDictionaryRef         alwaysZero2)
/// {
///     ...
/// }
/// ...
///    CFNotificationCenterAddObserver(
///        CFNotificationCenterGetDarwinNotifyCenter(),
///        observer,
///        &gotNotification,
///        name,			// eg, CFSTR("com.myapp.notification")
///        NULL,
///        0 );
/// </pre>
///
/// Alternately, use the AMNotificationCenter class defined in
/// "MobileDeviceAccessIPhone.h"
///
/// Under the covers, it is implemented as a service called \p "com.apple.mobile.notification_proxy" which
/// executes the following command on the device:
/// <PRE>
///	/usr/libexec/notification_proxy
/// </PRE>
#if 0
https://gist.github.com/149443/6a40bf5cb9e47abe8a4b406c6396940e8a30dc7a suggests

com.apple.language.changed
com.apple.AddressBook.PreferenceChanged
com.apple.mobile.data_sync.domain_changed
com.apple.mobile.lockdown.device_name_changed
com.apple.mobile.developer_image_mounted
com.apple.mobile.lockdown.trusted_host_attached
com.apple.mobile.lockdown.host_detached
com.apple.mobile.lockdown.host_attached
com.apple.mobile.lockdown.phone_number_changed
com.apple.mobile.lockdown.registration_failed
com.apple.mobile.lockdown.activation_state
com.apple.mobile.lockdown.brick_state
com.apple.itunes-client.syncCancelRequest
com.apple.itunes-client.syncSuspendRequest
com.apple.itunes-client.syncResumeRequest
com.apple.springboard.attemptactivation
com.apple.mobile.application_installed
com.apple.mobile.application_uninstalled
#endif

@interface AMNotificationProxy : AMService {
@private
	NSMutableDictionary *_messages;
}

/// Send the named notification to the
/// Darwin Notification Center on the device.  Note that there is no
/// possibility to send any information with the notification.
/// @param notification
- (void)postNotification:(NSString*)notification;

/// Add an observer for a specific message.  Whenever this message is
/// recieved by the proxy, it will be passed to all observers who
/// are registered, in an indeterminate order.
/// @param notificationObserver
/// @param notificationSelector
/// @param notificationName
- (void)addObserver:(id)notificationObserver
           selector:(SEL)notificationSelector
               name:(NSString *)notificationName;

/// Remove an observer for a specific message.  Once this message
/// is processed, the \p notificationObserver object will no longer
/// recieve notifications.
/// @param notificationObserver
/// @param notificationName
- (void)removeObserver:(id)notificationObserver
                  name:(NSString *)notificationName;

/// Remove an observer for all messages.
/// @param notificationObserver
- (void)removeObserver:(id)notificationObserver;

@end

/// This class allows certain bits of information to be retrieved
/// from the springboard.
/// 
/// Under the covers, it is implemented as a service called \p "com.apple.springboardservices" which
/// executes the following command on the device:
/// <PRE>
///	/usr/libexec/springboardservicesrelay
/// </PRE>
@interface AMSpringboardServices : AMService {
}

/// This method seems to return an NSArray which contains one entry
/// per "page" on the iPod, though the first page appears to be for
/// the icons displayed in the dock.
///
/// Each page appears to be an NSArray of entries, each of which
/// describes an icon position on the page.  Each icon is represented
/// by an NSDictionary containing the following keys
/// -               bundleIdentifier = "com.apple.mobileipod";
/// -               displayIdentifier = "com.apple.mobileipod-AudioPlayer";
/// -               displayName = Music;
/// -               iconModDate = 2009-09-26 20:45:29 +1000;
///
/// If a position is unoccupied, the entry will be an NSInteger (0)
/// The array for each page appears to be padded to a multiple of
/// 4, rather than reserving a full 16 entries per page.
- (id)getIconState;

/// This method returns an NSDictionary containing a single entry with
/// the key "pngData", which contains an NSData holding the .png data
/// for the requested application.
///
/// The key required appears to be the displayIdentifier rather than
/// the bundleIdentifier.
- (id)getIconPNGData:(NSString*)displayIdentifier;

/// This method returns an NSImage holding the icon .png data
/// for the requested application.
///
/// The key required appears to be the displayIdentifier rather than
/// the bundleIdentifier.
- (NSImage*)getIcon:(NSString*)displayIdentifier;

@end

/// This protocol describes the messages that will be sent by AMInstallationProxy
/// to its delegate.
@protocol AMInstallationProxyDelegate
@optional

/// A new current operation is beginning.
-(void)operationStarted:(NSDictionary*)info;

/// The current operation is continuing.
-(void)operationContinues:(NSDictionary*)info;

/// The current operation finished (one way or the other)
-(void)operationCompleted:(NSDictionary*)info;

@end

/// This class communicates with the mobile_installation_proxy.  It can be used
/// to retrieve information about installed applications on the device (as well as other
/// installation operations that are not supported by this framework).
///
/// Under the covers, it is implemented as a service called \p "com.apple.mobile.installation_proxy" which
/// executes the following command on the device:
/// <PRE>
///	/usr/libexec/mobile_installation_proxy
/// </PRE>
/// That binary seems to vet the incoming requests, then pass them to the installd process
/// for execution.
///
/// The protocol also seems to be a one-shot.  That is, you need to establish multiple connections
/// if you want to perform multiple operations.
///
/// See also: http://iphone-docs.org/doku.php?id=docs:protocols:installation_proxy
@interface AMInstallationProxy : AMService

/// Return an array of all installed applications (see AMApplication) matching the input type.
/// @param type may be "User", "System" or "Internal".  If specified as \p nil, it is ignored
/// and all application types are returned.
///
/// This is used indirectly by [AMDevice installedApplications] which may be a more convenient
/// interface.
- (NSArray *)browse:(NSString*)type;

/// Returns an array of all installed applications (see AMApplication) that match the input predicate.
/// @param filter defines the conditions for accepting an application.
- (NSArray *)browseFiltered:(NSPredicate*)filter;

/// Return a dictionary (indexed by bundleid) of all installed applications (see AMApplication) matching the input type,
/// and optionally filtering those that have a specific attribute in their Info.plist.
/// @param type may be "User", "System", "Internal" or "Any"
/// @param attr can be any attribute (like CFBundlePackageType, etc).  Note, however that
/// you can't filter on the value of the attribute, just its existance.
///
/// You probably don't want to use lookupType:withAttribute - see browseFiltered: instead.
- (NSDictionary*)lookupType:(NSString*)type
			  withAttribute:(NSString*)attr;

/// Ask the installation daemon on the device to create an archive of a specific
/// application.  Once finished a corresponding
/// zip file will be present in the Media/ApplicationArchives directory where it could
/// be retrieved via AFCMediaDirectory.
/// The contents of the archive depends on the values of the container: and payload:
/// arguments.
/// If the uninstall: argument is YES, the application will also be uninstalled.
- (BOOL)archive:(NSString*)bundleid
	  container:(BOOL)container
		payload:(BOOL)payload
	  uninstall:(BOOL)uninstall;

/// Ask the installation daemon on the device to restore a previously archived application.
/// The .zip file must be placed in the Media/ApplicationArchives directory.
- (BOOL)restore:(NSString*)bundleid;

/// Ask the installation daemon on the device what application archives are
/// available.  Return an array of bundle ids which can be passed to functions
/// like restore:
- (NSArray*)archivedAppBundleIds;

/// Ask the installation daemon on the device what application archives are
/// available.  Return a dictionary keyed by application bundle id.
///
/// This seems to be reading the file /var/mobile/Library/MobileInstallation/ArchivedApplications.plist
/// which is not otherwise accessible (except using AFCRootDirectory on jailbreaked devices)
- (NSDictionary*)archivedAppInfo;

/// Remove the archive for a given bundle id.  Note that this is more than just removing
/// the .zip file from the Media/ApplicationArchives directory - if you do that, the
/// archive remains "known to" the installation daemon and future requests to archive
/// this bundle will fail.  Sadly, explicit requests to removeArchive: will file if the
/// .zip file has been removed as well. (The simplest fix for this scenario is to create
/// a dummy file before calling removeArchive:)
- (BOOL)removeArchive:(NSString*)bundleid;

/// Ask the installation daemon on the device to install an application.  pathname
/// must be the name of a directory located in /var/mobile/Media and must contain
/// a pre-expanded .app which must not already exist on the device
- (BOOL)install:(NSString*)pathname;

/// Ask the installation daemon on the device to upgrade an application.  pathname
/// must be the name of a directory located in /var/mobile/Media and must contain
/// a pre-expanded .app which already exists on the device
- (BOOL)upgrade:(NSString*)bundleId from:(NSString*)pathname;

@end

/// This class communicates with the MobileSync service.  There is a fairly complicated protocol
/// required.
///
/// Under the covers, it is implemented as a service called \p "com.apple.mobilesync" which
/// executes the following command on the device:
/// <PRE>
///	/usr/libexec/SyncAgent --lockdown --oneshot -v
/// </PRE>
@interface AMMobileSync : AMService
- (id)getContactData;
@end

/// This class communicates with the AMService to remotely request and download device screenshots.
///
/// Under the covers, it is implemented as a service called \p "com.apple.screenshotr" - this seems to only
/// be present on devices that are enabled for development, and is implemented by
/// <PRE>
///	/Developer/usr/bin/ScreenShotr
/// </PRE>
@interface AMScreenshotService : AMDLService

/// Return a raw NSImage of the current screen
- (NSImage *)getScreenshot;

/// Create an image file of the current screen in an PNG format
- (BOOL)getPNGScreenshot:(NSString*)path;

/// Create an image file of the current screen in an JPEG format
- (BOOL)getJPEGScreenshot:(NSString*)path;

/// Create an image file of the current screen in an arbitrary file format
- (BOOL)getScreenshot:(NSString*)path
			   ofType:(NSBitmapImageFileType)fileType
		   properties:(NSDictionary*)properties;
@end
    
/// This class communicates with the syslog_relay.
///
/// To create one, send \p -newAMSyslogRelay:\p message: to an instance of AMDevice.
///
/// The message must conform to the prototype
/// <PRE>
/// -(void)syslogMessageRead:(NSString*)line
/// </PRE>
///
/// Under the covers, it is implemented as a service called \p "com.apple.syslog_relay" which
/// executes the following command on the device:
/// <PRE>
///	/usr/libexec/syslog_relay --lockdown
/// </PRE>
@interface AMSyslogRelay : AMService {
	CFReadStreamRef _readstream;
	id _listener;
	SEL _message;
}
@end

/// This class copies back specific files or sets of files from
/// the device, in CPIO file format.  The file format is non-negotiable and individual files
/// cannot be requested.  Instead, the caller specifies one or more "fileset names" from the
/// following table.
/// <TABLE>
///		<TR><TH>Set Name</TH><TH>File/Directory</TH></TR>
///		<TR><TD>AppleSupport</TD><TD>/private/var/logs/AppleSupport</TD></TR>
///		<TR><TD>AppleTV</TD><TD>???</TD></TR>
///		<TR><TD>Bluetooth</TD><TD>???</TD></TR>
///		<TR><TD>Caches</TD><TD>/private/var/mobile/Library/Caches</TD></TR>
///		<TR><TD>CoreLocation</TD><TD>???</TD></TR>
///		<TR><TD>CrashReporter</TD><TD>/Library/Logs/CrashReporter<BR>/private/var/mobile/Library/Logs/CrashReporter</TD></TR>
///		<TR><TD>demod</TD><TD>???</TD></TR>
///		<TR><TD>keyboard</TD><TD>???</TD></TR>
///		<TR><TD>Baseband</TD><TD>???</TD></TR>
///		<TR><TD>MobileWirelessSync</TD><TD>/private/var/mobile/Library/Logs/MobileWirelessSync</TD></TR>
///		<TR><TD>Lockdown</TD><TD>/private/var/root/Library/Lockdown/activation_records
/// <BR>/private/var/root/Library/Lockdown/data_ark.plist
/// <BR>/private/var/root/Library/Lockdown/pair_records
/// <BR>/Library/Logs/lockdownd.log</TD></TR>
///		<TR><TD>MobileBackup</TD><TD>/var/root/Library/Logs/MobileBackup</TD></TR>
///		<TR><TD>MobileInstallation</TD><TD>/var/mobile/Library/Logs/MobileInstallation
/// <BR>/var/mobile/Library/Caches/com.apple.mobile.installation.plist
/// <BR>/var/mobile/Library/MobileInstallation/ArchivedApplications.plist
/// <BR>/var/mobile/Library/MobileInstallation/ApplicationAttributes.plist
/// <BR>/var/mobile/Library/MobileInstallation/SafeHarbor.plist</TD></TR>
///		<TR><TD>MobileMusicPlayer</TD><TD>???</TD></TR>
///		<TR><TD>Network</TD><TD>/private/var/log/ppp</TD></TR>
/// <BR>/private/var/log/racoon.log
/// <BR>/var/log/eapolclient.en0.log</TD></TR>
///		<TR><TD>Photos</TD><TD>???</TD></TR>
///		<TR><TD>SafeHarbor</TD><TD>/var/mobile/Library/SafeHarbor</TD></TR>
///		<TR><TD>SystemConfiguration</TD><TD>/Library/Preferences/SystemConfiguration</TD></TR>
///		<TR><TD>Ubiquity</TD><TD>???</TD></TR>
///		<TR><TD>UserDatabases</TD><TD>/private/var/mobile/Library/AddressBook</TD></TR>
/// <BR>/private/var/mobile/Library/Calendar
/// <BR>/private/var/mobile/Library/CallHistory
/// <BR>/private/var/mobile/Library/Mail/Envelope Index
/// <BR>/private/var/mobile/Library/SMS</TD></TR>
///		<TR><TD>AppSupport</TD><TD>???</TD></TR>
///		<TR><TD>Voicemail</TD><TD>???</TD></TR>
///		<TR><TD>WirelessAutomation</TD><TD>???</TD></TR>
///		<TR><TD>MapsLogs</TD><TD>???</TD></TR>
///		<TR><TD>MobileAsset</TD><TD>???</TD></TR>
///		<TR><TD>GameKitLogs</TD><TD>???</TD></TR>
///		<TR><TD>Device-O-Matic</TD><TD>???</TD></TR>
///		<TR><TD>MobileDelete</TD><TD>???</TD></TR>
///		<TR><TD>Accounts</TD><TD>???</TD></TR>
///		<TR><TD>AddressBook</TD><TD>???</TD></TR>
///		<TR><TD>FindMyiPhone</TD><TD>???</TD></TR>
///		<TR><TD>DataAccess</TD><TD>???</TD></TR>
///		<TR><TD>DataMigrator</TD><TD>???</TD></TR>
///		<TR><TD>EmbeddedSocial</TD><TD>???</TD></TR>
///		<TR><TD>MobileCal</TD><TD>???</TD></TR>
///		<TR><TD>MobileNotes</TD><TD>???</TD></TR>
///		<TR><TD>VPN</TD><TD>/private/var/log/racoon.log</TD></TR>
///		<TR><TD>WiFi</TD><TD>/var/log/wifimanager.log
/// <BR>/var/log/eapolclient.en0.log</TD></TR>
///		<TR><TD>tmp</TD><TD>/private/var/tmp</TD></TR>
/// </TABLE>
/// The special fileset name "All" includes the files from all the other sets.
///
/// Due to the protocol used by the "com.apple.mobile.file_relay" service, the AMFileRelay
/// can only be used once and must be released afterward.
///
/// Under the covers, it is implemented as a service called \p "com.apple.mobile.file_relay" which
/// executes the following command on the device:
/// <PRE>
///	/usr/libexec/mobile_file_relay
/// </PRE>
@interface AMFileRelay : AMService {
	bool _used;
}

/// Gets one or more filesets and writes the results to the nominated stream.
/// If a problem occurs during the request, the method returns NO and
/// lasterror will be set to an appropriate
/// error code / message.
/// - AlreadyUsed - AMFileRelay object has already been used
/// - InvalidSource	- fileset name is invalid
/// - StagingEmpty - fileset contains no files to transfer
/// - CreateStagingPathFailed - failed to create temporary work directory on device
/// - CopierCreationFailed - BOMCopierNew() failed (on device)
/// - PopulationFailed - BOMCopierCopy() failed (on device)
- (bool)getFileSets:(NSArray*)set into:(NSOutputStream*)output;

/// Gets a single fileset and writes the result to the nominated stream.  This is
/// a convenience wrapper around \p -getFileSets:
- (bool)getFileSet:(NSString*)name into:(NSOutputStream*)output;
@end

/// Under the covers, it is implemented as a service called \p "com.apple.iosdiagnostics.relay" which
/// executes the following command on the device:
/// <PRE>
///	/usr/libexec/ios_diagnostics_relay
/// </PRE>
@interface AMIOSDiagnosticsRelay : AMService {
}

/// Retrieve a dictionary containing all the diagnostics information available on the device.
/// Each entry in the dictionary appears to be a subdictionary containing more specific information.
/// Keys that have been observed in the top-level dictionary include:
/// <PRE>basic</PRE>
/// <PRE>battery</PRE>
/// <PRE>icloud</PRE>
/// <PRE>aggd</PRE>
- (NSDictionary *)getDiagnostics;

@end

/// Under the covers, it is implemented as a service called \p "com.apple.mobile.diagnostics_relay" which
/// executes the following command on the device:
/// <PRE>
///	/usr/libexec/diagnostics_relay
/// </PRE>
@interface AMMobileDiagnosticsRelay : AMService {
}

- (NSDictionary *)getWiFiDiagnostics;
- (NSDictionary *)getHDMIDiagnostics;
- (NSDictionary *)getGasGaugeDiagnostics;
- (NSDictionary *)getNANDDiagnostics;
- (NSDictionary *)getMobileGestaltValues:(NSArray*)keys;
- (id)getMobileGestaltValue:(NSString*)key;

- (BOOL)sleep;
- (NSDictionary *)test;

@end

/// This class represents an open file on the device.
/// The caller can read from or write to the file depending on the
/// file open mode.
@interface AFCFileReference : NSObject
{
@private
	afc_file_ref _ref;
	afc_connection _afc;
	NSString *_lasterror;
}

/// The last error that occurred on this file
///
/// The object remembers the last error that occurred, which allows most other api's
/// to return YES/NO as their failure condition.  If no previous error occurred,
/// this property will be nil.
@property (readonly) NSString *lasterror;

/// Close the file.  
/// Any outstanding writes are flushed to disk.
- (bool)closeFile;

/// Change the current position within the file.
/// @param offset is the number of bytes to move by
/// @param mode must be one of the following:
/// - \p SEEK_SET (0) - offset is relative to the start of the file
/// - \p SEEK_CUR (1) - offset is relative to the current position
/// - \p SEEK_END (2) - offset is relative to the end of the file
- (bool)seek:(int64_t)offset mode:(int)mode;

/// Return the current position within the file.
/// The position is suitable to be passed to \p seek: \p mode:SEEK_SET
- (bool)tell:(uint64_t*)offset;

/// Read \p n
/// bytes from the file into the nominated buffer (which must be at
/// least \p n bytes long).  Returns the number of bytes actually read.
- (afc_long)readN:(afc_long)n bytes:(char *)buff;

/// Write \p n bytes to the file.  Returns \p true if the write was
/// successful and \p false otherwise.
- (bool)writeN:(afc_long)n bytes:(const char *)buff;

/// Write the contents of an NSData to the file.  Returns \p true if the
/// write was successful and \p false otherwise
- (bool)writeNSData:(NSData*)data;

/// Set the size of the file
///
/// Truncates the file to the specified size.
- (bool)setFileSize:(uint64_t)size;

@end

/// This object manages a single file server connection to the connected device.
/// Using it, you can open files for reading or writing.  It also provides higher-order
/// functions such as directory scanning, directory creation and file copying.
///
/// You should not use these directly.  Instead, see AFCMediaDirectory, AFCApplicationDirectory
/// and AFCRootDirectory
@interface AFCDirectoryAccess : AMService
{
@protected
	afc_connection _afc;						///< the low-level connection
}

/**
 * Return a dictionary containing information about the connected device.
 *
 * Keys in the result dictionary include:
 *	- \p "Model"
 *	- \p "FSFreeBytes"
 *	- \p "FSBlockSize"
 *	- \p "FSTotalBytes"
 */
- (NSDictionary*)deviceInfo;

/**
 * Return a dictionary containing information about the specified file.
 * @param path Full pathname to the file to retrieve information for
 *
 * Keys in the result dictionary include:
 *	- \p "st_ifmt" - file type
 *		- \p "S_IFREG" - regular file
 *		- \p "S_IFDIR" - directory
 *		- \p "S_IFCHR" - character device
 *		- \p "S_IFBLK" - block device
 *		- \p "S_IFLNK" - symbolic link (see LinkTarget)
 *
 *	- \p "st_blocks" - number of disk blocks occupied by file
 *
 *	- \p "st_nlink" - number of "links" occupied by file
 *
 *	- \p "st_size" - number of "bytes" in file
 *
 *	- \p "LinkTarget" - target of symbolic link (only if st_ifmt="S_IFLNK")
 *
 *	- \p "st_birthtime" - time that file was created
 *
 *	- \p "st_mtime" - time that file was modified
 */
- (NSDictionary*)getFileInfo:(NSString*)path;

/**
 * Return YES if the specified file/directory exists on the device.
 * @param path Full pathname to file/directory to check
 */
- (BOOL)fileExistsAtPath:(NSString *)path;

/**
 * Return a array containing a list of simple filenames found
 * in the specified directory.  The entries for "." and ".." are
 * not included.
 * @param path Full pathname to the directory to scan
 */
- (NSArray*)directoryContents:(NSString*)path;

/**
 * Return a array containing a list of full pathnames found
 * in the specified directory, and all subordinate directories.
 * Entries for directories will end in "/"
 * The entries for "." and ".." are not included.
 * @param path Full pathname to the directory to scan
 */
- (NSArray*)recursiveDirectoryContents:(NSString*)path;

/**
 * Open a file for reading.
 * @param path Full pathname to the file to open
 */
- (AFCFileReference*)openForRead:(NSString*)path;

/**
 * Open a file for writing.
 * @param path Full pathname to the file to open
 */
- (AFCFileReference*)openForWrite:(NSString*)path;

/**
 * Open a file for reading or writing.
 * @param path Full pathname to the file to open
 */
- (AFCFileReference*)openForReadWrite:(NSString*)path;

/**
 * Create a new directory on the device.
 * @param path Full pathname of the directory to create
 */
- (BOOL)mkdir:(NSString*)path;

/**
 * Unlink a file or directory on the device.  If a directory is specified, it must
 * be empty.
 * @param path Full pathname of the directory to delete
 */
- (BOOL)unlink:(NSString*)path;

/**
 * Rename a file or directory on the device.
 * @param oldpath Full pathname of file or directory to rename
 * @param newpath Full pathname to rename file or directory to
 */
- (BOOL)rename:(NSString*)oldpath to:(NSString*)newpath;

/**
 * Create a hard link on the device.
 * @param linkname Full pathname of link to create
 * @param target Target of link
 */
- (BOOL)link:(NSString*)linkname to:(NSString*)target;

/**
 * Create a symbolic link on the device.
 * @param linkname Full pathname of symbolic link to create
 * @param target Target of symbolic link
 */
- (BOOL)symlink:(NSString*)linkname to:(NSString*)target;

/**
 * Set file modification time for a file on the device
 * @param pathname Full pathname of file to modify
 * @param timestamp Time to set it's modification time to
 */
- (BOOL)setmodtime:(NSString*)pathname to:(NSDate*)timestamp;

/**
 * Copy the contents of a local file or directory (on the Mac) to
 * a directory on the device.  The copy is recursive (for directories)
 * and will copy symbolic links as links.
 * @param frompath Full pathname of the local file/directory
 * @param topath Full pathname of the device directory to copy into
 */
- (BOOL)copyLocalFile:(NSString*)frompath toRemoteDir:(NSString*)topath;

/**
 * Copy the contents of a local file (on the Mac) to the device.
 * @param frompath Full pathname of the local file
 * @param topath Full pathname of the device file to copy into
 */
- (BOOL)copyLocalFile:(NSString*)frompath  toRemoteFile:(NSString*)topath;

/**
 * Copy the contents of a device file to a file on the Mac.
 * @param frompath Full pathname of the device file
 * @param topath Full pathname of the local file to copy into
 */
- (BOOL)copyRemoteFile:(NSString*)frompath toLocalFile:(NSString*)topath;

/**
 * Close this connection.  From this point on, none of the other functions
 * will run correctly.
 */
- (void)close;

@end

/// This class represents an AFC connection that is rooted to the devices
/// Media directory (/var/mobile/Media).
///
/// To create one, send the \p -newAFCMediaDirectory message to an instance of AMDevice.
///
/// Under the covers, it is implemented as a service called \p "com.apple.afc" which
/// executes the following command on the device:
/// <PRE>
///	/usr/libexec/afcd --lockdown -d /var/mobile/Media -u mobile
/// </PRE>
///
/// Common subdirectories to access within the Media directory are:
/// - DCIM
/// - ApplicationArchives
/// - PublicStaging
@interface AFCMediaDirectory : AFCDirectoryAccess {
}
@end

/// This class represents an AFC connection that is rooted to the devices
/// crash-log directory (/var/mobile/Library/Logs/CrashReporter).
///
/// To create one, send the \p -newAFCCrashLogDirectory message to an instance of AMDevice.
///
/// Under the covers, it is implemented as a service called \p "com.apple.crashreportcopymobile" which
/// executes the following command on the device:
/// <PRE>
/// /usr/libexec/afcd --lockdown -d /var/mobile/Library/Logs/CrashReporter -u mobile
/// </PRE>
@interface AFCCrashLogDirectory : AFCDirectoryAccess {
}

@end

/// This class represents an AFC connection on a jail-broken device.  It has
/// full access to the devices filesystem.
///
/// To create one, send the \p -newAFCRootDirectory message to an instance of AMDevice.
///
/// Note, instances of this class will only operate correctly on devices that are
/// running the \p "com.apple.afc2" service.  If your device was jailbroken with
/// blackra1n this service may be missing in which case it can be installed via
/// Cydia.
///
/// Under the covers, it is implemented as a service called \p "com.apple.afc2" which
/// executes the following command on the device:
/// <PRE>
/// /usr/libexec/afcd --lockdown -d /
/// </PRE>
@interface AFCRootDirectory : AFCDirectoryAccess {
}
@end

/// This class represents an AFC connection that is rooted to a single
/// application's sandbox.  You must know the \p CFBundleIdentifier value
/// from the application's \p Info.plist
///
/// To create one, send the \p -newAFCApplicationDirectory: message to an instance of AMDevice.
///
/// The current user will be 'mobile' and will only be able to
/// access files within the sandbox.  The root directory will appear to contain
/// - the application
/// - Documents
/// - Library
/// - tmp
///
/// Under the covers, it is implemented as a service called \p "com.apple.mobile.house_arrest" which
/// executes the following command on the device:
/// <PRE>
///	/usr/libexec/mobile_house_arrest
/// </PRE>
@interface AFCApplicationDirectory : AFCDirectoryAccess {
}
@end

/// This class represents a connected device
/// (iPhone or iPod Touch).
@interface AMDevice : NSObject {
@private
	am_device _device;
	NSString *_lasterror;
	NSString *_deviceName;
	NSString *_udid;
	NSArray *_applAttributes;

	bool _connected, _insession;
}

/// The last error that occurred on this device
///
/// The object remembers the last error that occurred, which allows most other api's
/// to return YES/NO as their failure condition.  If no previous error occurred,
/// this property will be nil.
@property (readonly) NSString *lasterror;

/// Returns the device name (the name visible in the Devices section
/// in iTunes and labelled as Name: in the Summary pane in iTunes).
///
/// The same value may be retrieved by passing \p "DeviceName" to \p -deviceValueForKey:
@property (readonly) NSString *deviceName;		// configured name

/// Returns the 40 character UDID (the field labeled as Identifier: in the Summary pane
/// in iTunes - if its not visible, click the Serial Number: label).
///
/// The same value may be retrieved by passing \p "UniqueDeviceID" to \p -deviceValueForKey:
@property (readonly) NSString *udid;			// "ed9896a213aa2341274928472234127492847211"

/// Specific class of device.  eg, "iPod1,1"
///
/// The same value may be retrieved by passing
/// \p "ProductType" to \p -deviceValueForKey:
@property (readonly) NSString *productType;		// eg, "iPod1,1"

/// General class of device.  eg, "iPod"
///
/// The same value may be retrieved by passing
/// \p "DeviceClass" to \p -deviceValueForKey:
@property (readonly) NSString *deviceClass;

/// Returns the serial number (the field labeled as Serial Number: in the Summary pane
/// in iTunes - if its not visible, click the Identifier: label)
///
/// The same value may be retrieved by passing \p "SerialNumber" to \p -deviceValueForKey:
@property (readonly) NSString *serialNumber;	// "5984999T14P"

/// Returns the array of attributes that will be requested for when using the
/// installedApplications method. If nil, we use a "reasonable set".
@property (retain) NSArray *applicationAttributes;

/// Create a file service connection which can access the media directory.
/// This uses the service \p "com.apple.afc" which is present on all devices
/// and only allows access to the \p "/var/mobile/Media" directory structure
/// as the user 'mobile'
///
/// Fails on iOS8.1 due to security restrictions
- (AFCMediaDirectory*)newAFCMediaDirectory;

/// Create a file service connection which can access the crash log directory.
/// This uses the service \p "com.apple.crashreportcopymobile" which is present on all devices
/// and only allows access to the \p "/var/mobile/Library/Logs/CrashReporter" directory structure
/// as the user 'mobile'
///
/// Fails on iOS8.1 due to security restrictions
- (AFCCrashLogDirectory*)newAFCCrashLogDirectory;

/// Create a file service connection rooted in the sandbox for the nominated
/// application.  This uses the service \p "com.apple.mobile.house_arrest" which is
/// present on all devices
/// and only allows access to the applications directory structure
/// as the user 'mobile'
/// @param bundleId This is the identifier value for the application.
- (AFCApplicationDirectory*)newAFCApplicationDirectory:(NSString*)bundleId;

/// Create a file service connection which can access the entire file system.
/// This uses the service \p "com.apple.afc2" which is only present on
/// jailbroken devices (and may need to be added manually with Cydia if you used
/// blackra1n to jailbreak).
- (AFCRootDirectory*)newAFCRootDirectory;

/// Create a notification proxy service.
/// This allows notifications to be posted on the device.
- (AMNotificationProxy*)newAMNotificationProxy;

/// Create a springboard services relay.
/// This allows info about icons and png data to be retrieved
- (AMSpringboardServices*)newAMSpringboardServices;

/// Create an installation proxy relay.
/// If an object is passed for the delegate, it will be notified during some of the
/// operations performed by the proxy.
- (AMInstallationProxy*)newAMInstallationProxyWithDelegate:(id<AMInstallationProxyDelegate>)delegate;

/// Create a fileset relay.  This can be used to en-masse extract certain groups
/// of information from the device. For more information, see AMFileRelay.
///
/// Fails on iOS8.1 due to security restrictions
- (AMFileRelay*)newAMFileRelay;

- (AMIOSDiagnosticsRelay*)newAMIOSDiagnosticsRelay;
- (AMMobileDiagnosticsRelay*)newAMMobileDiagnosticsRelay;

/// Create an instance of AMSyslogRelay which will relay us message
/// from the syslog daemon on the device.
/// @param listener This object will be notified for every message
/// recieved by the relay service from the syslog daemon
/// @param message This is the message sent to the \p listener object.
- (AMSyslogRelay*)newAMSyslogRelay:(id)listener message:(SEL)message;

/// Create a mobile sync relay.  Allows synchronisation of information
/// with the device. For more information, see AMMobileSync.
- (AMMobileSync*)newAMMobileSync;

/// Create a mobile screenshot service.  Allows screensnaps to be captured from the
/// device.  This only works on development devices (ie, devices that have been
/// marked as "Used for Development" in XCode.  Otherwise the required
/// service is not present.
- (AMScreenshotService*)newAMScreenshotService;

/// Returns an informational value from the device's root domain.
/// @param key can apparently be any value from
/// the following:
/// <TABLE>
///     <TR><TD><TT>BasebandBootloaderVersion</TT></TD><TD>"5.8_M3S2"</TD></TR>
///     <TR><TD><TT>BasebandVersion</TT></TD><TD>"01.45.00"</TD></TR>
///     <TR><TD><TT>BluetoothAddress</TT></TD><TD>?</TD></TR>
///     <TR><TD><TT>DeviceCertificate</TT></TD><TD>lots of bytes</TD></TR>
///     <TR><TD><TT>IntegratedCircuitCardIdentity</TT></TD><TD>?</TD></TR>
///     <TR><TD><TT>InternationalMobileEquipmentIdentity</TT></TD><TD>?</TD></TR>
///     <TR><TD><TT>InternationalMobileSubscriberIdentity</TT></TD><TD>?</TD></TR>
///     <TR><TD><TT>PhoneNumber</TT></TD><TD>?</TD></TR>
///     <TR><TD><TT>SomebodySetTimeZone</TT></TD><TD>1</TD></TR>
///     <TR><TD><TT>iTunesHasConnected</TT></TD><TD>1</TD></TR>
/// </TABLE>
- (id)deviceValueForKey:(NSString*)key;

/// Same as deviceValueForKey: but queries the specified domain.  The list of domains does not
/// appear to be documented anywhere, but see "MobileDeviceAccess.md" for values that
/// have been known to return useful information.  Note that some domains are superceded
/// by others as versions change.
- (id)deviceValueForKey:(NSString*)key inDomain:(NSString*)domain;

/// Returns a dictionary of "most" informational values for the device.  If called
/// with a nil domain value, the keys
/// correspond to those shown in the table for \ref deviceValueForKey: but it appears
/// that it doesn't always return *all* values.
- (id)allDeviceValuesForDomain:(NSString*)domain;

/// Return a array of applications, each of which is represented by an instance
/// of AMApplication.  Note that this only returns details for applications installed
/// by iTunes.  For other (system) applications, use NSInstallationProxy to browse.
- (NSArray*)installedApplications;

/// Check whether the specified bundleId corresponds to an application
/// installed on the device.  If so, return an appropriate AMApplication.
/// Otherwise return nil.
- (AMApplication*)installedApplicationWithId:(NSString*)bundleId;

@end

/// An object must implement this protocol if it is to be passed as a listener
/// to the MobileDeviceAccess singleton object.
@protocol MobileDeviceAccessListener
@optional
/// This method will be called whenever a device is connected
- (void)deviceConnected:(AMDevice*)device;
/// This method will be called whenever a device is disconnected
- (void)deviceDisconnected:(AMDevice*)device;
@end

/// This class provides the high-level interface onto the MobileDevice
/// framework.  Instances of this class should not be created directly;
/// instead, use \ref singleton "+singleton"
@interface MobileDeviceAccess : NSObject {
@private
	id _listener;
	BOOL _subscribed;
	am_device_notification _notification;
	NSMutableArray *_devices;
	BOOL _waitingInRunLoop;
}

/// Returns an array of AMDevice objects representing the currently
/// connected devices.
@property (readonly) NSArray *devices;

/// Returns the one true instance of \c MobileDeviceAccess
+ (MobileDeviceAccess*)singleton;

/// Nominate the entity that will recieve notifications about device
/// connections and disconnections.  The listener object must implement
/// the MobileDeviceAccessListener protocol.
- (bool)setListener:(id<MobileDeviceAccessListener>)listener;

/// \deprecated
///
/// This method allows the caller to wait till a connection has been
/// made.  It sits in a run loop and does not return till a device
/// connects.
- (bool)waitForConnection;

/// Call this method to treat the nominated device as "disconnected".  Note,
/// this does not disconnect the device from Mac OS X - only from the
/// MobileDeviceAccess singleton
/// @param device The device to disconnect.
- (void)detachDevice:(AMDevice*)device;

/// Returns the client-library version string.  On my MacOSX machine, the
/// value returned was "@(#)PROGRAM:afc  PROJECT:afc-84"
- (NSString*)clientVersion;

@end

#ifdef __cplusplus
}
#endif

#endif // _MobileDeviceAccess.h
