#pragma once

// clang-format off
// Kept aligned by hand: the point of this header is that the two branches read
// as one table, and the formatter would reflow them into two unrelated lists.
#ifdef __i386__
    #define AFW_ENGINE_CALL         __attribute__((fastcall))
    #define AFW_CDECL               __attribute__((cdecl))
    #define AFW_STDCALL             __attribute__((stdcall))
    #define AFW_THISCALL            __attribute__((thiscall))
    #define AFW_VECTORCALL          __attribute__((vectorcall))
#else
    #define AFW_ENGINE_CALL
    #define AFW_CDECL
    #define AFW_STDCALL
    #define AFW_THISCALL
    #define AFW_VECTORCALL
#endif
// clang-format on
