#pragma once

//#include "resource.h"

#include "stdafx.h"

using namespace std;

_ConnectionPtr m_pConnection;
_RecordsetPtr m_pRecordset;

void Connect(void);
void ExitConnect(void);
_RecordsetPtr& GetRecordset(_bstr_t SQL);


HBITMAP ScreenCapture(LPSTR filename, WORD bitCount, LPRECT lpRect,int X,int Y,int W ,int H);

extern "C" __declspec(dllexport) void* Distinguish(char * path,char *result);
