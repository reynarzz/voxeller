// DropHoverEvents.mm
#import <Cocoa/Cocoa.h>
#include "DropHoverEvents.h"
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

DropHoverEvents::DropCallback DropHoverEvents::dropCallback = nullptr;
DropHoverEvents::HoverCallback DropHoverEvents::hoverCallback = nullptr;

@interface DropHoverHelper : NSView <NSDraggingDestination>
@end

static NSInteger lastX = -1;
static NSInteger lastY = -1;

@implementation DropHoverHelper

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    // Initial entry position
    NSPoint point = [sender draggingLocation];
    if (DropHoverEvents::hoverCallback) {
        DropHoverEvents::hoverCallback({(int)point.x, (int)point.y});
    }
    return NSDragOperationCopy;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    NSPoint point = [sender draggingLocation];
    NSInteger x = (NSInteger)point.x;
    NSInteger y = (NSInteger)point.y;

    // Only fire callback when the position changes
    if (x != lastX || y != lastY) {
        lastX = x;
        lastY = y;

        if (DropHoverEvents::hoverCallback) {
            DropHoverEvents::hoverCallback({(int)x, (int)y});
        }
    }

    return NSDragOperationCopy;
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender {
    return YES;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    NSPasteboard* pb = [sender draggingPasteboard];
    NSArray<NSURL*>* files = [pb readObjectsForClasses:@[[NSURL class]] options:nil];
    NSPoint point = [sender draggingLocation];

    if (files && DropHoverEvents::dropCallback) {
        std::vector<std::string> paths;
        for (NSURL* url in files) {
            if ([url isFileURL]) {
                paths.emplace_back([[url path] UTF8String]);
            }
        }
        DropHoverEvents::dropCallback({ paths, (int)point.x, (int)point.y });
    }
    return YES;
}
@end


static DropHoverHelper* gHoverHelper = nil;

void DropHoverEvents::Initialize(void* glfwWin) 
{
    NSWindow* window = glfwGetCocoaWindow(static_cast<GLFWwindow*>(glfwWin));
    NSView* mainView = [window contentView];

    if (!gHoverHelper) {
       gHoverHelper = [[DropHoverHelper alloc] initWithFrame:[mainView bounds]];


        // Add gHoverHelper as a transparent overlay view
        [mainView addSubview:gHoverHelper positioned:NSWindowAbove relativeTo:nil];
        [gHoverHelper setFrame:[mainView bounds]];
        [gHoverHelper setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [gHoverHelper registerForDraggedTypes:@[NSPasteboardTypeFileURL]];
    }
}
void DropHoverEvents::Shutdown() {
    gHoverHelper = nil;
}

void DropHoverEvents::SetDropCallback(DropCallback cb) {
    dropCallback = cb;
}

void DropHoverEvents::SetHoverCallback(HoverCallback cb) {
    hoverCallback = cb;
}
