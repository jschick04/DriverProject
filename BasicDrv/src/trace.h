#pragma once

#define WPP_CONTROL_GUIDS                                               \
    WPP_DEFINE_CONTROL_GUID(                                            \
        myBasicDrvTraceGuid, (a5532548,1c1a,43dd,9ced,52708bae781e),    \
        WPP_DEFINE_BIT(LEVEL_ERROR)                                     \
        WPP_DEFINE_BIT(LEVEL_WARNING)                                   \
        WPP_DEFINE_BIT(LEVEL_INFO)                                      \
    )
