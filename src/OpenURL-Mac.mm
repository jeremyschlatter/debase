#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>
#import "OpenURL.h"

void OpenURL(const char* url) {
    @autoreleasepool {
        LSOpenCFURLRef((__bridge CFURLRef)[NSURL URLWithString:@(url)], nil);
    }
}
