// ArchiveExports.cpp

#include "StdAfx.h"

#include "../../../C/7zVersion.h"

#include "../../Common/ComTry.h"

#include "../../Windows/PropVariant.h"

#include "../Common/RegisterArc.h"

static const unsigned kNumArcsMax = 64;
static unsigned g_NumArcs = 0;
static unsigned g_DefaultArcIndex = 0;
static const CArcInfo *g_Arcs[kNumArcsMax];

void RegisterArc(const CArcInfo *arcInfo) throw()
{
  if (g_NumArcs < kNumArcsMax)
  {
    const char *p = arcInfo->Name;
    if (p[0] == '7' && p[1] == 'z' && p[2] == 0)
      g_DefaultArcIndex = g_NumArcs;
    g_Arcs[g_NumArcs++] = arcInfo;
  }
}

DEFINE_GUID(CLSID_CArchiveHandler,
    k_7zip_GUID_Data1,
    k_7zip_GUID_Data2,
    k_7zip_GUID_Data3_Common,
    0x10, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00);

#define CLS_ARC_ID_ITEM(cls) ((cls).Data4[5])

static inline HRESULT SetPropStrFromBin(const char *s, unsigned size, PROPVARIANT *value)
{
  if ((value->bstrVal = ::SysAllocStringByteLen(s, size)) != 0)
    value->vt = VT_BSTR;
  return S_OK;
}

static inline HRESULT SetPropGUID(const GUID &guid, PROPVARIANT *value)
{
  return SetPropStrFromBin((const char *)&guid, sizeof(guid), value);
}

int FindFormatCalssId(const GUID *clsid)
{
  GUID cls = *clsid;
  CLS_ARC_ID_ITEM(cls) = 0;
  if (cls != CLSID_CArchiveHandler)
    return -1;
  Byte id = CLS_ARC_ID_ITEM(*clsid);
  for (unsigned i = 0; i < g_NumArcs; i++)
    if (g_Arcs[i]->Id == id)
      return (int)i;
  return -1;
}

STDAPI CreateArchiver(const GUID *clsid, const GUID *iid, void **outObject)
{
  COM_TRY_BEGIN
  {
    int needIn = (*iid == IID_IInArchive);
    int needOut = (*iid == IID_IOutArchive);
    if (!needIn && !needOut)
      return E_NOINTERFACE;
    int formatIndex = FindFormatCalssId(clsid);
    if (formatIndex < 0)
      return CLASS_E_CLASSNOTAVAILABLE;
    
    const CArcInfo &arc = *g_Arcs[formatIndex];
    if (needIn)
    {
      *outObject = arc.CreateInArchive();
      ((IInArchive *)*outObject)->AddRef();
    }
    else
    {
      if (!arc.CreateOutArchive)
        return CLASS_E_CLASSNOTAVAILABLE;
      *outObject = arc.CreateOutArchive();
      ((IOutArchive *)*outObject)->AddRef();
    }
  }
  COM_TRY_END
  return S_OK;
}

STDAPI GetNumberOfFormats(UINT32 *numFormats)
{
  *numFormats = g_NumArcs;
  return S_OK;
}

STDAPI GetIsArc(UInt32 formatIndex, Func_IsArc *isArc)
{
  *isArc = NULL;
  if (formatIndex >= g_NumArcs)
    return E_INVALIDARG;
  *isArc = g_Arcs[formatIndex]->IsArc;
  return S_OK;
}

STDAPI CreateInArchiver(UInt32 formatIndex, void **outObject)
{
    if (formatIndex >= g_NumArcs) {
        *outObject = NULL;
        return CLASS_E_CLASSNOTAVAILABLE;
    }
    const CArcInfo &arc = *g_Arcs[formatIndex];
    *outObject = arc.CreateInArchive();
    ((IInArchive *)*outObject)->AddRef();
    return S_OK;
}
