#define CAML_NAME_SPACE
#import <Cocoa/Cocoa.h>
#import <caml/alloc.h>
#import <caml/callback.h>
#import <caml/memory.h>

// Enter OCaml runtime and obtain the semaphore
void brisk_caml_call(void (^block)());

// Enter OCaml runtime and obtain the semaphore, after that run flush
// This could be potentially implemented on the OCaml side. Not sure.
void brisk_caml_call_and_flush(void (^block)());

// Call during app launch before any OCaml code is called
void brisk_init();

// Manual memory management of NSViews. Maybe there's a way to remove it and
// somehow let ARC do the job even when values cross the boundary. I doubt it
// though.
void retainView(NSView *view);
void releaseView(NSView *view);
