// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by OptiScaler.rc
//
// Modifications Copyright (c) 2026 3zwr1 (AMDNR)
#ifdef _DEBUG
#define VER_BUILD_DATE "Debug Build"
#define VER_BUILD_COMMIT "Debug"
#else
#include "resource_build_date.h"
#include "resource_build_commit.h"
#endif // !_DEBUG

#define VS_VERSION_INFO 1

// Next default values for new objects
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE 101
#define _APS_NEXT_COMMAND_VALUE 40001
#define _APS_NEXT_CONTROL_VALUE 1001
#define _APS_NEXT_SYMED_VALUE 101
#endif
#endif

#define STRINGIZE_(s) #s
#define STRINGIZE(s) STRINGIZE_(s)

#define VER_MAJOR_VERSION 11
#define VER_MINOR_VERSION 0
#define VER_HOTFIX_VERSION 0
#define VER_BUILD_NUMBER 1
// The AMD neural-rendering build's own version, shown first in the menu title and the log
// header: "AMD-NR v0.3.0 / OptiScaler v11.0.0 (build stamp)". The stamp stays: it is how a
// tester's screenshot is matched to a build (there is no git here).
#define AMDNR_VERSION_STR "0.3.4.2"

#define VER_DEV_RELEASE
// #define VER_PRE_RELEASE

#define VER_FILE_VERSION VER_MAJOR_VERSION, VER_MINOR_VERSION, VER_HOTFIX_VERSION, VER_BUILD_NUMBER
#define VER_FILE_VERSION_STR                                                                                           \
    STRINGIZE(VER_MAJOR_VERSION) "." STRINGIZE(VER_MINOR_VERSION) "." STRINGIZE(VER_HOTFIX_VERSION) "." STRINGIZE(VER_BUILD_NUMBER)
#define OPTI_VERSION STRINGIZE(VER_MAJOR_VERSION) "." STRINGIZE(VER_MINOR_VERSION) "." STRINGIZE(VER_HOTFIX_VERSION)

#define VER_PRODUCT_VERSION VER_FILE_VERSION

#ifdef VER_DEV_RELEASE
#define VER_PRODUCT_VERSION_STR                                                                                        \
    STRINGIZE(VER_MAJOR_VERSION) "." STRINGIZE(VER_MINOR_VERSION) "." STRINGIZE(VER_HOTFIX_VERSION) "-dev (" VER_BUILD_COMMIT ") (" VER_BUILD_DATE ")"
#elif VER_PRE_RELEASE
#define VER_PRODUCT_VERSION_STR                                                                                        \
    STRINGIZE(VER_MAJOR_VERSION) "." STRINGIZE(VER_MINOR_VERSION) "." STRINGIZE(VER_HOTFIX_VERSION) "-pre" STRINGIZE(VER_BUILD_NUMBER) " (" VER_BUILD_COMMIT ") (" VER_BUILD_DATE ")"
#else
#define VER_PRODUCT_VERSION_STR                                                                                        \
    STRINGIZE(VER_MAJOR_VERSION) "." STRINGIZE(VER_MINOR_VERSION) "." STRINGIZE(VER_HOTFIX_VERSION) "-final (" VER_BUILD_COMMIT ")"
#endif // VER_PRE_RELEASE

#define VER_PRODUCT_NAME "AMD-NR v" AMDNR_VERSION_STR " / OptiScaler v" OPTI_VERSION " (" VER_BUILD_DATE ")"
