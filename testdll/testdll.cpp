// testdll.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <windows.h>

#include <iostream>
#include <vector>
#pragma comment(lib,"crypt32")

using namespace std;

int main()
{

	HMODULE hModule = NULL;

	typedef void* (__cdecl*pendd)(char* a,char *b);



	hModule = LoadLibrary("OCR.DLL");



	pendd padd = pendd(GetProcAddress(hModule, "Distinguish"));

	char sss[256] = {"123456"};

	padd("C:\\Users\\lxg\\Desktop\\dest\\1.jpg",sss);

    return 0;
}

