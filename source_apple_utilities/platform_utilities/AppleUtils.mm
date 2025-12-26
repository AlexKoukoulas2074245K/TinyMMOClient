///------------------------------------------------------------------------------------------------
///  AppleUtils.cpp
///  TinyMMOClient
///
///  Created by Alex Koukoulas on 20/01/2024.
///-----------------------------------------------------------------------------------------------

#include <platform_utilities/AppleUtils.h>
#import <Foundation/Foundation.h>
#import <platform_utilities/TinyMMOClientReachability.h>
#include <engine/resloading/ResourceLoadingService.h>
#include <engine/utils/Logging.h>
#include <engine/utils/PlatformMacros.h>
#include <engine/utils/StringUtils.h>
#include <codecvt>
#include <fstream>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#import <StoreKit/StoreKit.h>
#import <StoreKit/SKStoreReviewController.h>
#import <UserNotifications/UserNotifications.h>
#import <UserNotifications/UNUserNotificationCenter.h>
#if __has_include(<UIKit/UIKit.h>)
#import <UIKit/UIKit.h>
#endif

///-----------------------------------------------------------------------------------------------

#if defined(MACOS)
@interface CustomTextField : NSTextField

@end

@implementation CustomTextField

- (BOOL)performKeyEquivalent:(NSEvent *)event {
    if ((event.modifierFlags & NSEventModifierFlagDeviceIndependentFlagsMask) == NSEventModifierFlagCommand) {
        if ([event.charactersIgnoringModifiers isEqualToString:@"c"]) {
            [self copyText:self];
            return YES;
        } else if ([event.charactersIgnoringModifiers isEqualToString:@"v"]) {
            [self pasteText:self];
            return YES;
        }
    }
    return [super performKeyEquivalent:event];
}

- (IBAction)copyText:(id)sender {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    [pasteboard writeObjects:@[self.stringValue]];
}

- (IBAction)pasteText:(id)sender {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSArray *classes = @[[NSString class]];
    NSDictionary *options = @{};
    NSArray *copiedItems = [pasteboard readObjectsForClasses:classes options:options];
    if (copiedItems.count > 0) {
        self.stringValue = copiedItems.firstObject;
    }
}

@end

#else
@interface CustomTextField : UITextField

@end

@implementation CustomTextField

- (BOOL)canPerformAction:(SEL)action withSender:(id)sender {
    if (action == @selector(copy:) || action == @selector(paste:)) {
        return YES;
    }
    return [super canPerformAction:action withSender:sender];
}

@end
#endif

@interface InAppPurchaseManager : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver>

- (void) addObserverToTransactionQueue;
- (void) requestProductInformationWithProductIdentifiers:(NSSet<NSString*>*)productIdentifiers;
- (void) initiatePurchaseWithProductId:(NSString*)productId;

@end


typedef void(^TextInputCompletionHandler)(std::string);

static void InternalGetMessageBoxTextInput(TextInputCompletionHandler completionHandler)
{
#if defined(MACOS)
    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Enter Gift Code"];
    
    CustomTextField *textField = [[CustomTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 48)];
    [alert setAccessoryView:textField];
    [[alert window] setInitialFirstResponder:textField];
    
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    
    NSModalResponse response = [alert runModal];
    
    if (response == NSAlertFirstButtonReturn && completionHandler)
    {
        completionHandler(std::string([[textField stringValue] UTF8String]));
    }
    else if (completionHandler)
    {
        completionHandler(std::string());
    }
#else
    UIAlertController *alert = [UIAlertController alertControllerWithTitle:@"Gift Code"
                                                                   message:@"Enter Gift Code"
                                                            preferredStyle:UIAlertControllerStyleAlert];
    
    [alert addTextFieldWithConfigurationHandler:^(UITextField *textField) {
        textField.placeholder = @"Gift Code";
        textField.keyboardType = UIKeyboardTypeAlphabet;
        textField.autocapitalizationType = UITextAutocapitalizationTypeAllCharacters;
    }];
    
    [alert addAction:[UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction *action) {
        UITextField *textField = alert.textFields.firstObject;
        NSString *inputText = textField.text;
        if (completionHandler) {
            completionHandler(std::string([inputText UTF8String]));
        }
    }]];
    
    [alert addAction:[UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:^(UIAlertAction *action) {
        if (completionHandler) {
            completionHandler(std::string());
        }
    }]];
    
    // Present the alert
    UIViewController *rootViewController = [[[UIApplication sharedApplication] delegate] window].rootViewController;
    [rootViewController presentViewController:alert animated:YES completion:nil];
#endif
}

///-----------------------------------------------------------------------------------------------

static std::function<void(apple_utils::PurchaseResultData)> purchaseFinishedCallback = nullptr;
static InAppPurchaseManager* purchaseManager = nil;
static NSArray* products = nil;

@implementation InAppPurchaseManager

- (void)addObserverToTransactionQueue
{
    [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
}

// Method to request product information with inline block
- (void)requestProductInformationWithProductIdentifiers:(NSSet<NSString*>*)productIdentifiers
{
    SKProductsRequest *productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:productIdentifiers];
    productsRequest.delegate = self;
    [productsRequest start];
}

// Method to initiate a product purchase
- (void) initiatePurchaseWithProductId:(NSString*)productId
{
    for (SKProduct* product in products)
    {
        if ([product.productIdentifier isEqualToString:productId])
        {
            SKPayment *payment = [SKPayment paymentWithProduct:product];
            SKPaymentQueue* paymentQueue = [SKPaymentQueue defaultQueue];
            [paymentQueue addPayment:payment];
            return;
        }
    }
    
    // Product not found. Instantly confirm purchase with a transaction id of now's timestamp.
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    auto secsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    purchaseFinishedCallback({ std::to_string(secsSinceEpoch), std::string([productId UTF8String]), true});
}


// Delegate method to handle product information response
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
    products = response.products;
    for (SKProduct* product in products)
    {
        NSLog(@"Product ID: %@, Title: %@, Price: %@%@, Description: %@", product.productIdentifier, product.localizedTitle, product.priceLocale.currencySymbol, product.price, product.localizedDescription);
    }
}

// Delegate method to handle transaction updates
- (void)paymentQueue:(nonnull SKPaymentQueue *)queue updatedTransactions:(nonnull NSArray<SKPaymentTransaction *> *)transactions
{
    if (!purchaseFinishedCallback)
    {
        return;
    }
    
    for (SKPaymentTransaction* transaction in transactions)
    {
        switch (transaction.transactionState)
        {
            case SKPaymentTransactionStatePurchased:
            {
                NSLog(@"Transaction %@ at Date %@ Sucessful transaction", transaction.transactionIdentifier, transaction.transactionDate);
                purchaseFinishedCallback({ std::string([transaction.transactionIdentifier UTF8String]), std::string([transaction.payment.productIdentifier UTF8String]), true});
                [queue finishTransaction:transaction];
                
            } break;
                
            case SKPaymentTransactionStateFailed:
                NSLog(@"Transaction %@ at Date %@ Failed transaction", transaction.transactionIdentifier, transaction.transactionDate);
                purchaseFinishedCallback({ "", std::string([transaction.payment.productIdentifier UTF8String]), false});
                [queue finishTransaction:transaction];
                break;
            case SKPaymentTransactionStateRestored:
                NSLog(@"Transaction %@ at Date %@ Transaction restored", transaction.transactionIdentifier, transaction.transactionDate);
                purchaseFinishedCallback({ std::string([transaction.transactionIdentifier UTF8String]), std::string([transaction.payment.productIdentifier UTF8String]), true});
                [queue finishTransaction:transaction];
                break;
            case SKPaymentTransactionStatePurchasing:
                NSLog(@"Transaction %@ at Date %@Transaction pending purchasing", transaction.transactionIdentifier, transaction.transactionDate);
            default:
                break;
        }
    }
}

@end

///-----------------------------------------------------------------------------------------------

namespace apple_utils
{

///-----------------------------------------------------------------------------------------------

bool IsConnectedToTheInternet()
{
    return !([[TinyMMOClientReachability reachabilityForInternetConnection] currentReachabilityStatus] == NotReachable);
}

///-----------------------------------------------------------------------------------------------

std::string GetPersistentDataDirectoryPath()
{
#if defined(MACOS)
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* path = [paths objectAtIndex:0];
    return std::string([path UTF8String]) + "/";
#else
    return std::string(getenv("HOME")) + "/Documents/";
#endif
}

///-----------------------------------------------------------------------------------------------

std::string GetDeviceId()
{
#if defined(MACOS)
    NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];
    NSString* deviceId = [NSString stringWithFormat:@"%@ %ld.%ld.%ld", [[NSHost currentHost] localizedName], (long)version.majorVersion, (long)version.minorVersion, (long)version.patchVersion];
#else
    NSString* deviceId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
#endif
    
    return std::string([deviceId UTF8String]);
}

///-----------------------------------------------------------------------------------------------

std::string GetDeviceName()
{
#if defined(MACOS)
    return "iMac/MacBook";
#else
    UIDevice *device = [UIDevice currentDevice];
    return std::string([device.name UTF8String]);
#endif
    
    return "Unknown Device";
}

///-----------------------------------------------------------------------------------------------

std::string GetAppVersion()
{
    NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
    return std::string([version UTF8String]);
}

///-----------------------------------------------------------------------------------------------

void SetAssetFolder()
{
    NSString *launchPath=[NSBundle.mainBundle
                          pathForResource:@"assets"
                          ofType:nil
    ];
    
    resources::ResourceLoadingService::RES_ROOT = std::string([launchPath UTF8String]) + "/";
}

///-----------------------------------------------------------------------------------------------

bool HasLoadedProducts()
{
    return products != nil;
}

///-----------------------------------------------------------------------------------------------

void LoadStoreProducts(const std::vector<std::string>& productIdsToLoad)
{
    // Instantiate InAppPurchaseManager
    if (!purchaseManager)
    {
        purchaseManager = [[InAppPurchaseManager alloc] init];
        [purchaseManager addObserverToTransactionQueue];
    }
    
    // Request product information
    NSMutableSet<NSString*>* productIdSet = [NSMutableSet set];
    for (const auto& productId: productIdsToLoad)
    {
        NSString* nsStringProductId = [NSString stringWithUTF8String:productId.c_str()];
        [productIdSet addObject:nsStringProductId];
    }
    
    [purchaseManager requestProductInformationWithProductIdentifiers:productIdSet];
}

///-----------------------------------------------------------------------------------------------

std::string GetProductPrice(const std::string& productId)
{
    if (purchaseManager && products)
    {
        NSString* nsStringProductId = [NSString stringWithUTF8String:productId.c_str()];
        for (SKProduct* product in products)
        {
            if ([product.productIdentifier isEqualToString:nsStringProductId])
            {
                NSNumberFormatter *formatter = [NSNumberFormatter new];
                [formatter setNumberStyle:NSNumberFormatterCurrencyStyle];
                [formatter setLocale:product.priceLocale];
                NSString* cost = [formatter stringFromNumber:product.price];
                std::string cppString([cost UTF8String]);
                
                // Euro
                if (strutils::StringContains(cppString, "\xe2\x82\xac"))
                {
                    strutils::StringReplaceAllOccurences("\xe2\x82\xac", "\x80", cppString);
                }
                // ANSI currencies. Just need to strip \xc2
                else if (strutils::StringContains(cppString, "\xc2"))
                {
                    strutils::StringReplaceAllOccurences("\xc2", "", cppString);
                }
                // Strip other undisplayable unicode/hex escape codes
                else
                {
                    for (auto iter = cppString.begin(); iter != cppString.end();)
                    {
                        if (*iter < 0x20 || *iter > 0x7E)
                        {
                            iter = cppString.erase(iter);
                        }
                        else
                        {
                            iter++;
                        }
                    }
                }
                
                return cppString;
            }
        }
    }
    
    return "";
}

///-----------------------------------------------------------------------------------------------

void InitiateProductPurchase(const std::string& productId, std::function<void(PurchaseResultData)> onPurchaseFinishedCallback)
{
    if (purchaseManager && products)
    {
        NSString* nsStringProductId = [NSString stringWithUTF8String:productId.c_str()];
        purchaseFinishedCallback = onPurchaseFinishedCallback;
        [purchaseManager initiatePurchaseWithProductId:nsStringProductId];
    }
}

///-----------------------------------------------------------------------------------------------

void GetMessageBoxTextInput(std::function<void(const std::string&)> inputTextReceivedCallback)
{
    InternalGetMessageBoxTextInput(^(std::string input)
                                   {
        inputTextReceivedCallback(input);
    });
}

///-----------------------------------------------------------------------------------------------

void RequestReview()
{
#if !defined(MACOS)
    [SKStoreReviewController requestReview];
#endif
}

///-----------------------------------------------------------------------------------------------

}

///-----------------------------------------------------------------------------------------------
