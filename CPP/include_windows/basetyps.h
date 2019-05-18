#ifndef _BASETYPS_H
#define _BASETYPS_H

#ifdef ENV_HAVE_GCCVISIBILITYPATCH
  #define DLLEXPORT __attribute__ ((visibility("default")))
#else
  #define DLLEXPORT
#endif

#ifdef __cplusplus
#define STDAPI extern "C" DLLEXPORT HRESULT
#else
#define STDAPI extern DLLEXPORT HRESULT
#endif  /* __cplusplus */ 

typedef GUID IID;
typedef GUID CLSID;

#define CLASS_E_CLASSNOTAVAILABLE        ((HRESULT)0x80040111L)
typedef void *HINSTANCE;
#define DLL_PROCESS_ATTACH 1
#define HIWORD(l)              ((WORD)((DWORD_PTR)(l) >> 16))
#define LOWORD(l)              ((WORD)((DWORD_PTR)(l) & 0xFFFF))

#endif

