/*
    MobileDevice.h
    Header for the MobileDevice framework used by iTunes

    Created by John Heaton(Gojohnnyboi) on Sunday, April 3, 2011
    Contact: gojohnnyboi@me.com
 
    Contributions are welcome.
*/

#include <CoreFoundation/CoreFoundation.h>

// Statuses for functions -- more will be added
typedef enum {
    kAMStatusSuccess = 0,
    kAMStatusFailure = 1
} AMStatus;

// Possible messages in the AMDeviceNotificationRef sent to a AMDeviceConnectionCallback function
typedef enum {
    kAMDeviceNotificationMessageConnected = 1,
    kAMDeviceNotificationMessageDisconnected = 2,
    kAMDeviceNotificationMessageUnsubscribed = 3
} AMDeviceNotificationMessage;

// List of known services on the device

// MobileDevice types. Struct names based off of CoreFoundation
// NOTE: You may cast from dfu to a recovery device 
typedef struct __AMDevice *AMDeviceRef;
typedef struct __AMRecoveryModeDevice *AMRecoveryModeDeviceRef;
typedef struct __AMDFUModeDevice *AMDFUModeDeviceRef;
typedef struct __AMRestoreModeDevice *AMRestoreModeDeviceRef;

typedef unsigned char *AMDeviceSubscriptionRef;

// It is necessary for this object's struct to be public. There are no functions to properly obtain its elements
typedef struct {
    AMDeviceRef                     device; // can be cast to AMRestoreModeDeviceRef if notification is restore device
    AMDeviceNotificationMessage     message;
    AMDeviceSubscriptionRef         subscription;
} *AMDeviceNotificationRef;

// Callback declarations
typedef void (* AMRecoveryModeDeviceConnectionCallback)(AMRecoveryModeDeviceRef device);
typedef void (* AMDFUModeDeviceConnectionCallback)(AMDFUModeDeviceRef device);
typedef void (* AMDeviceConnectionCallback)(AMDeviceNotificationRef notification);

// Connection subscription functions. These will allow you to set up callback functions that are called upon connection/disconnection of a device.
AMStatus AMRestoreRegisterForDeviceNotifications(AMDFUModeDeviceConnectionCallback DFUConnectCallback, AMRecoveryModeDeviceConnectionCallback recoveryConnectCallback, AMDFUModeDeviceConnectionCallback DFUDisconnectCallback, AMRecoveryModeDeviceConnectionCallback recoveryDisconnectCallback, unsigned int alwaysZero, void *userInfo);

AMStatus AMDeviceNotificationSubscribe(AMDeviceConnectionCallback callback, int alwaysZero_1, int alwaysZero_2, int alwaysZero_3, AMDeviceSubscriptionRef *subscription);


// Functions for use with AMDeviceRef objects(normal interface)
AMStatus AMDeviceConnect(AMDeviceRef device);
AMStatus AMDeviceDisconnect(AMDeviceRef device);
int AMDeviceGetConnectionID(AMDeviceRef device);

Boolean AMDeviceIsPaired(AMDeviceRef device);
AMStatus AMDevicePair(AMDeviceRef device);
AMStatus AMDeviceValidatePairing(AMDeviceRef device);

CFStringRef AMDeviceCopyDeviceIdentifier(AMDeviceRef device);

AMStatus AMDeviceEnterRecovery(AMDeviceRef device);

AMStatus AMDeviceStartSession(AMDeviceRef device);
AMStatus AMDeviceStopSession(AMDeviceRef device);

CFStringRef AMDeviceCopyValue(AMDeviceRef device, CFStringRef domain, CFStringRef valueName);

AMStatus AMDeviceStartService(AMDeviceRef device, CFStringRef serviceName, int *socketDescriptor);

AMStatus AMDPostNotification(int socket, CFStringRef notification, CFStringRef userinfo);

uint16_t AMDeviceUSBProductID(AMDeviceRef device);

void AMDeviceRelease(AMDeviceRef device);
void AMDeviceRetain(AMDeviceRef device);

// Functions for use with AMRecoveryModeDeviceRef objects(recovery interface)
CFStringRef AMRecoveryModeDeviceCopyEnvironmentVariableFromDevice(AMRecoveryModeDeviceRef device, CFStringRef variable);

AMStatus AMRecoveryModeDeviceSendCommandToDevice(AMRecoveryModeDeviceRef device, CFStringRef command);
AMStatus AMRecoveryModeDeviceSendBlindCommandToDevice(AMRecoveryModeDeviceRef device, CFStringRef command);

AMStatus AMRecoveryModeDeviceSendFileToDevice(AMRecoveryModeDeviceRef device, CFStringRef filename);

AMStatus AMRecoveryModeDeviceSetAutoBoot(AMRecoveryModeDeviceRef device, Boolean autoBoot);
AMStatus AMRecoveryModeDeviceReboot(AMRecoveryModeDeviceRef device);

uint16_t AMRecoveryModeDeviceGetProductID(AMRecoveryModeDeviceRef device);
uint32_t AMRecoveryModeDeviceGetProductType(AMRecoveryModeDeviceRef device);

CFTypeID AMRecoveryModeDeviceGetTypeID(AMRecoveryModeDeviceRef device);

AMStatus AMRecoveryModeDeviceCopyAuthInstallPreflightOptions(AMRecoveryModeDeviceRef device, CFDictionaryRef inputOptions, CFDictionaryRef *newRestoreOptions);
AMStatus AMRestorePerformRecoveryModeRestore(AMRecoveryModeDeviceRef device, CFDictionaryRef restoreOptions, void *callback, void *userInfo);

// Functions for use with AMRestoreModeDeviceRef objects(restore interface)

AMRestoreModeDeviceRef AMRestoreModeDeviceCreate(int alwaysZero_1, int connectionID, int alwaysZero_2);

CFDictionaryRef AMRestoreCreateDefaultOptions(CFAllocatorRef allocator); // may also be used for recovery/dfu restores
AMStatus AMRestorePerformRestoreModeRestore(AMRecoveryModeDeviceRef device, CFDictionaryRef restoreOptions, void *callback, void *userInfo);

// Functions for use with AMDFUModeDeviceRef objects(DFU/WTF interface)
uint16_t AMDFUModeDeviceGetProductID(AMDFUModeDeviceRef device);
uint32_t AMDFUModeDeviceGetProductType(AMDFUModeDeviceRef device);