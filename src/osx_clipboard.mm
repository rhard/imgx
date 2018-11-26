#include <string>
#import <Appkit/Appkit.h>


bool GetOSXClipboard(std::string& outText) {
    NSString * contents = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
    const char * utf8 = [contents UTF8String];
    outText = std::string(utf8);
    return true;
}

bool SetOSXClipboard(const std::string& inText) {
    NSString * nss = [NSString stringWithCString:inText.c_str() encoding:[NSString defaultCStringEncoding]];
    [[NSPasteboard generalPasteboard] clearContents];
    [[NSPasteboard generalPasteboard] setString:nss forType:NSPasteboardTypeString];
    return true;
}
