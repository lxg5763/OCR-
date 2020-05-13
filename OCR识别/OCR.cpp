// weixin.cpp : 定义控制台应用程序的入口点。
//


#include "OCR.h"
#include <opencv2/opencv.hpp>
#include<iostream> 
#include <vector>
#include <string>



#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace cv;

// 唯一的应用程序对象


char SQL_server[64] = { "192.168.99.202" };
char SQL_user[64] = { "sa" };//用户名
char SQL_pwd[64] = { "123" };//密码
char SQL_database[64] = { "ocr" };//数据库名称


char s_connection[MAX_PATH] = { 0 };//读取配置文件链接数据库

Rect selecta;
bool select_flag = false;

int OPENCV ;//opencv保存 图片名字变量

vector<string>TXTfile;//保存OCR识别过的文本的容器
vector<string>YPname;// 药品名称
vector<string>YPage; // 药品规格
vector<string>Number; //数量
vector<string>value;//金额

vector<int>test;//保存计数；
Mat img, showImg;
char *filebuffer;

string  Resultpath;//保存最终识别结果的路径 

string Jpath; //用于删除截图的路径
string Bpath; // 用于删除识别结果的路径

void Connect(void)//链接数据库
{
	try{
		::CoInitialize(NULL);  //初始化COM环境
		HRESULT hr = m_pConnection.CreateInstance(__uuidof(Connection));  //创建连接对象

		//连接服务器和数据库

		std::string sqlconStr = "Provider=SQLOLEDB";
		sqlconStr += ";Data Source=";
		sqlconStr += SQL_server;
		sqlconStr += ";Initial Catalog=";
		sqlconStr += SQL_database;
		sqlconStr += ";User ID=";
		sqlconStr += SQL_user;
		sqlconStr += ";Password=";
		sqlconStr += SQL_pwd;
		m_pConnection->ConnectionTimeout = 10;


		hr = m_pConnection->Open((_bstr_t)(char*)s_connection, "", "", adModeUnknown);

		if (hr != S_OK)
		{
			cout << "Can not connect to the specified database!" << endl;
			return;
		}
		
	}
	catch (_com_error e){
		cout << e.Description() << endl;
		return;
	}
}

void ExitConnect(void)
{
	if (m_pRecordset != NULL){
		m_pRecordset->Close();
		m_pConnection->Close();
	}
	::CoUninitialize();  //释放环境
}

_RecordsetPtr& GetRecordset(_bstr_t SQL)
{
	m_pRecordset = NULL;

	try{
		if (m_pConnection == NULL)
			Connect();
		m_pRecordset.CreateInstance(__uuidof(Recordset));
		m_pRecordset->Open((_bstr_t)SQL, m_pConnection.GetInterfacePtr(), adOpenDynamic, adLockOptimistic, adCmdText);
	}
	catch (_com_error e){
		cout << e.Description() << endl;
		m_pRecordset = NULL;
		return m_pRecordset;
	}
	return m_pRecordset;
}




//利用OPENCV截图
void S_on_Mouse(int x, int y, int X1, int Y1, string bname)//画矩形框并截图  x,y 是需要截取图片左上角的点的坐标；X1,Y1是截取图片右下角点的坐标
{
	Point p1, p2;

	selecta.x = X1;
	selecta.y = Y1;
	select_flag = true;

	if (select_flag)
	{
		img.copyTo(showImg);
		p1 = Point(selecta.x, selecta.y);
		p2 = Point(x, y);
		rectangle(showImg, p1, p2, Scalar(0, 255, 0), 2);
		imshow("img", showImg);
		waitKey(5);
	}

	
//	ostringstream oss;
//	oss << OPENCV << ".bmp";

//	string str = oss.str();
//	cout << str << endl;
	

	//显示框出的ROI   
	Rect roi = Rect(Point(selecta.x, selecta.y), Point(x, y));
	if (roi.width && roi.height)
	{
		Mat roiImg = img(roi);
		imshow("roi", roiImg);
		imwrite(bname, roiImg);
		waitKey(5);
	}
	select_flag = false;
}


//windows后台截图
BOOL SaveHwndToBmpFile(HWND hWnd, LPCTSTR lpszPath)
{
	HWND hDesktop = ::GetDesktopWindow();
	ASSERT(hDesktop);
	if (NULL == hWnd)
	{
		hWnd = hDesktop;
	}
	RECT rect;
	::GetWindowRect(hWnd, &rect);

	int nWidht = rect.right - rect.left;
	int nHeight = rect.bottom - rect.top;

	HDC hSrcDC = ::GetWindowDC(hWnd);
	ASSERT(hSrcDC);
	HDC hMemDC = ::CreateCompatibleDC(hSrcDC);
	ASSERT(hMemDC);
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hSrcDC, nWidht, nHeight);
	ASSERT(hBitmap);
	HBITMAP hOldBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);
	::BitBlt(hMemDC, 0, 0, nWidht, nHeight, hSrcDC, 0, 0, SRCCOPY);

	BITMAP bitmap = { 0 };
	::GetObject(hBitmap, sizeof(BITMAP), &bitmap);
	BITMAPINFOHEADER bi = { 0 };
	BITMAPFILEHEADER bf = { 0 };

	CONST int nBitCount = 24;
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bitmap.bmWidth;
	bi.biHeight = bitmap.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = nBitCount;
	bi.biCompression = BI_RGB;
	DWORD dwSize = ((bitmap.bmWidth * nBitCount + 31) / 32) * 4 * bitmap.bmHeight;

	HANDLE hDib = GlobalAlloc(GHND, dwSize + sizeof(BITMAPINFOHEADER));
	LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDib);
	*lpbi = bi;

	::GetDIBits(hMemDC, hBitmap, 0, bitmap.bmHeight, (BYTE*)lpbi + sizeof(BITMAPINFOHEADER), (BITMAPINFO*)lpbi, DIB_RGB_COLORS);

	try
	{
		CFile file;
		file.Open(lpszPath, CFile::modeCreate | CFile::modeWrite);
		bf.bfType = 0x4d42;
		dwSize += sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		bf.bfSize = dwSize;
		bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		file.Write((BYTE*)&bf, sizeof(BITMAPFILEHEADER));
		file.Write((BYTE*)lpbi, dwSize);
		file.Close();
	}
	catch (CFileException* e)
	{
		e->ReportError();
		e->Delete();
	}

	GlobalUnlock(hDib);
	GlobalFree(hDib);

	::SelectObject(hMemDC, hOldBitmap);
	::DeleteObject(hBitmap);
	::DeleteDC(hMemDC);
	::ReleaseDC(hWnd, hSrcDC);

	return TRUE;
}

void GetFpath(char *buff)
{

	int Dlen = strlen(buff);
	if (Dlen == 0) return;
	int i = 0;
	while (Dlen > 0)
	{
		if (buff[Dlen - 1] == '\\')
		{
			buff[Dlen - 1] = '\0';
			break;
		}
		else
		{
			buff[Dlen - 1] = '\0';
			Dlen--;
			i++;
		}
	}

}
//保存截图
BOOL CaptureLolToDesktop(HWND hWnd, string str, int X, int Y, int W, int H)
{
	Sleep(1 * 1000);
	LPRECT lprc = new RECT;
	GetWindowRect(hWnd, lprc);
	CString strTime;

	// 	char chDesktopPath[MAX_PATH] = { 0 };
	// 	SHGetSpecialFolderPathA(NULL, chDesktopPath, CSIDL_DESKTOP, 0);//获取当前用户桌面路径  


	char Path[MAX_PATH] = { 0 };
	char tmpPath[MAX_PATH] = { 0 };
	DWORD len = MAX_PATH;
	GetModuleFileName(NULL, Path, len);
	GetFpath(Path);
	strTime.Format("%s\\截图%s", Path, str.c_str());

	ScreenCapture((LPSTR)(LPCTSTR)(strTime), 32, lprc, X, Y, W, H);

	delete lprc;

	return TRUE;
}

//windows前台截图
HBITMAP ScreenCapture(LPSTR filename, WORD bitCount, LPRECT lpRect, int X, int Y, int W, int H)
{
	HBITMAP hBitmap;
	HDC hScreenDC = CreateDCA("DISPLAY", NULL, NULL, NULL);   //通过指定DISPLAY来获取一个显示设备上下文环境  
	HDC hmemDC = CreateCompatibleDC(hScreenDC);            //该函数创建一个与指定设备兼容的内存设备上下文环境（DC）  
	int ScreenWidth = GetDeviceCaps(hScreenDC, HORZRES);    //获取指定设备的性能参数（此处获取屏幕宽度）  
	int ScreenHeight = GetDeviceCaps(hScreenDC, VERTRES);   //获取指定设备的性能参数（此处获取屏幕高度）  
	HBITMAP hOldBM;
	PVOID lpData;   //内存分配成功返回的指向所分配内存块的首地址指针  
	int startX;     //截图x位置  
	int startY;     //截图y位置  
	int width;      //截图宽度  
	int height;     //截图高度  
	long BitmapSize;
	DWORD BitsOffset;
	DWORD ImageSize;
	DWORD FileSize;
	BITMAPINFOHEADER bmInfo; //BITMAPINFOHEADER结构所包含的成员表明了图像的尺寸、原始设备的颜色格式、以及数据压缩方案  
	BITMAPFILEHEADER bmFileHeader;
	HANDLE bmFile, hpal, holdpal = NULL;;
	DWORD dwWritten;
	if (lpRect == NULL)
	{
		startX = startY = 0;
		width = ScreenWidth;
		height = ScreenHeight;
	}
	else
	{
		//	startX = lpRect->left;
		//	startY = lpRect->top;
		startX = X;
		startY = Y;
		//	width = lpRect->right - lpRect->left;
		width = W;
		//	height = lpRect->bottom - lpRect->top;
		height = H;
	}
	//创建一张长width，宽height的画布，用于后面绘制图形  
	hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
	//该函数选择一对象到指定的设备上下文环境中，该新对象替换先前的相同类型的对象。  
	hOldBM = (HBITMAP)SelectObject(hmemDC, hBitmap);
	//该函数对指定的源设备环境区域中的像素进行位块（bit_block）转换，以传送到目标设备环境。  
	BitBlt(hmemDC, 0, 0, width, height, hScreenDC, startX, startY, SRCCOPY);
	hBitmap = (HBITMAP)SelectObject(hmemDC, hOldBM);
	if (filename == NULL)
	{
		DeleteDC(hScreenDC);
		DeleteDC(hmemDC);
		return hBitmap;
	}
	BitmapSize = ((((width * 32) + 32) / 32) * 4)*height;
	//用来在指定的堆上分配内存，并且分配后的内存不可移动(HEAP_NO_SERIALIZE 不使用连续存取)  
	lpData = HeapAlloc(GetProcessHeap(), HEAP_NO_SERIALIZE, BitmapSize);
	ZeroMemory(lpData, BitmapSize);
	ZeroMemory(&bmInfo, sizeof(BITMAPINFOHEADER));
	bmInfo.biSize = sizeof(BITMAPINFOHEADER); //位图信息结构长度 ,必须为40  
	bmInfo.biWidth = width;                   //图像宽度 单位是像素  
	bmInfo.biHeight = height;                 //图像高度 单位是像素  
	bmInfo.biPlanes = 1;                      //必须为1  
	bmInfo.biBitCount = bitCount;             //设置图像的位数。比如8位，16位，32位位数越高分辨率越高  
	bmInfo.biCompression = BI_RGB;            //位图是否压缩 BI_RGB为不压缩  
	ZeroMemory(&bmFileHeader, sizeof(BITMAPFILEHEADER));
	BitsOffset = sizeof(BITMAPFILEHEADER) + bmInfo.biSize;
	ImageSize = ((((bmInfo.biWidth*bmInfo.biBitCount) + 31) / 32) * 4)*bmInfo.biHeight;
	FileSize = BitsOffset + ImageSize;
	bmFileHeader.bfType = 0x4d42;//'B'+('M'<<8);  
	bmFileHeader.bfSize = FileSize;
	bmFileHeader.bfOffBits = BitsOffset;
	hpal = GetStockObject(DEFAULT_PALETTE);
	if (hpal)
	{
		holdpal = SelectPalette(hmemDC, (HPALETTE)hpal, false);
		RealizePalette(hmemDC);
	}
	GetDIBits(hmemDC, hBitmap, 0, bmInfo.biHeight, lpData, (BITMAPINFO *)&bmInfo, DIB_RGB_COLORS);
	if (holdpal)
	{
		SelectPalette(hmemDC, (HPALETTE)holdpal, true);
		RealizePalette(hmemDC);
	}
	bmFile = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (bmFile == INVALID_HANDLE_VALUE)
	{
		MessageBoxA(NULL, "Create File Error!", "提示", MB_OK | MB_ICONWARNING);
	}
	WriteFile(bmFile, &bmFileHeader, sizeof(BITMAPFILEHEADER), &dwWritten, NULL);
	WriteFile(bmFile, &bmInfo, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
	WriteFile(bmFile, lpData, ImageSize, &dwWritten, NULL);
	CloseHandle(bmFile);
	HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, lpData);
	::ReleaseDC(0, hScreenDC);
	DeleteDC(hmemDC);
	printf("【截图保存成功】");
	return hBitmap;
}

//将文件全名（带路径）分解成路径名，文件名，后缀名  
void SplitPathFileName(const char *pstrPathFileName, char *pstrPath, char *pstrFileName, char *pstrExtName)
{
	if (pstrPath != NULL)
	{
		char szTemp[MAX_PATH];
		_splitpath(pstrPathFileName, pstrPath, szTemp, pstrFileName, pstrExtName);
		strcat(pstrPath, szTemp);
	}
	else
	{
		_splitpath(pstrPathFileName, NULL, NULL, pstrFileName, pstrExtName);
	}
}

//得到当前进程可执行文件的路径名，文件名，后缀名  
BOOL GetProcessPathNameAndFileName(char *pstrPath, char *pstrFileName, char *pstrExtName)
{
	char szExeFilePathFileName[MAX_PATH];
	if (GetModuleFileName(NULL, szExeFilePathFileName, MAX_PATH) == 0)
		return FALSE;

	SplitPathFileName(szExeFilePathFileName, pstrPath, pstrFileName, pstrExtName);
	return TRUE;
}

//调整进程当前目录为程序可执行文件所在目录  
BOOL AdjustProcessCurrentDirectory()
{
	char szPathName[MAX_PATH];
	GetProcessPathNameAndFileName(szPathName, NULL, NULL);
	return SetCurrentDirectory(szPathName);
}
//对比算法
int ldistance(const string source, const string target)
{
	//step 1

	int n = source.length();
	int m = target.length();
	if (m == 0) return n;
	if (n == 0) return m;
	//Construct a matrix
	typedef vector< vector<int> >  Tmatrix;
	Tmatrix matrix(n + 1);
	for (int i = 0; i <= n; i++)  matrix[i].resize(m + 1);

	//step 2 Initialize

	for (INT i = 1; i <= n; i++) matrix[i][0] = i;
	for (int i = 1; i <= m; i++) matrix[0][i] = i;

	//step 3
	for (int i = 1; i <= n; i++)
	{
		const char si = source[i - 1];
		//step 4
		for (int j = 1; j <= m; j++)
		{

			const char dj = target[j - 1];
			//step 5
			int cost;
			if (si == dj){
				cost = 0;
			}
			else{
				cost = 1;
			}
			//step 6
			const int above = matrix[i - 1][j] + 1;
			const int left = matrix[i][j - 1] + 1;
			const int diag = matrix[i - 1][j - 1] + cost;
			matrix[i][j] = min(above, min(left, diag));

		}
	}//step7
	return matrix[n][m];
}
char* UTF8ToANSI(const char* utf8_str/*, string ansi_result*/)
{
	int unicode_len;
	wchar_t * unicode_buf;
	unicode_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
	unicode_buf = (wchar_t *)malloc((unicode_len + 1) * sizeof(wchar_t));
	memset(unicode_buf, 0, (unicode_len + 1) * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, (LPWSTR)unicode_buf, unicode_len);

	char* ansi_buf;
	int ansi_len;
	ansi_len = WideCharToMultiByte(CP_ACP, 0, unicode_buf, -1, NULL, 0, NULL, NULL);
	ansi_buf = (char *)malloc((ansi_len + 1) * sizeof(char));
	memset(ansi_buf, 0, sizeof(char) * (ansi_len + 1));
	WideCharToMultiByte(CP_ACP, 0, unicode_buf, -1, ansi_buf, ansi_len, NULL, NULL);

	ansi_buf[strlen(ansi_buf)] = '\0';


	//	ansi_result += ansi_buf;
	free(unicode_buf);
	return ansi_buf;
}

//读文件
BOOL Read(string filePath)
{
	HANDLE pFile;
	DWORD fileSize, dwBytesRead;

	pFile = CreateFileA(filePath.c_str(), GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,        //打开已存在的文件 
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (pFile == INVALID_HANDLE_VALUE)
	{
		printf("open file error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	fileSize = GetFileSize(pFile, NULL);          //得到文件的大小

	char * buffer = new char[fileSize + 10];
	memset(buffer, 0, fileSize + 10);

	dwBytesRead = 0;
	if (FALSE == ReadFile(pFile, buffer, fileSize, &dwBytesRead, NULL))
	{

		INT i = GetLastError();
		CloseHandle(pFile);
	}

	filebuffer = UTF8ToANSI(buffer);
	//处理尾部
	int i, L;
	L = strlen(filebuffer);
	for (i = L - 1; i > 0; i--)
		if (filebuffer[i] != ' '&& filebuffer[i] != '\n' && filebuffer[i] != '\n') break;
		else filebuffer[i] = '\0';

		//对多行文本以\n分割	
		char*temp = strtok(filebuffer, "\n");
		while (temp)
		{

			TXTfile.push_back(temp);
	//		printf("%s ", temp);
			temp = strtok(NULL, "\n");
		}

		CloseHandle(pFile);

		return TRUE;
}

//写文件
BOOL Write(const char *buffer, DWORD contentLen)
{
	HANDLE pFile;
	const char *tmpBuf;
	DWORD dwBytesWrite, dwBytesToWrite;

	pFile = CreateFile(Resultpath.c_str(), GENERIC_WRITE,
		0,
		NULL,
		OPEN_ALWAYS,        //打开文件
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (pFile == INVALID_HANDLE_VALUE)
	{
		printf("create file error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	dwBytesToWrite = contentLen;
	dwBytesWrite = 0;

	tmpBuf = buffer;

	do{                                       //循环写文件，确保完整的文件被写入  
		SetFilePointer(pFile, 0, NULL, FILE_END);
		WriteFile(pFile, tmpBuf, dwBytesToWrite, &dwBytesWrite, NULL);

		dwBytesToWrite -= dwBytesWrite;
		tmpBuf += dwBytesWrite;

	} while (dwBytesToWrite > 0);

	CloseHandle(pFile);

	return TRUE;
}

//运行ocr识别并保存到文件
void startOCR(string path)
{
	CHAR s1[4096] = { 0 };
	const char* i1 = "1.bmp";
	const char* i2 = "1";
	memset(s1, 0, 4096);
	sprintf_s(s1, "tesseract %s %s -l chi_sim", i1, i2);
	// 运行OCR识别
	WinExec(s1, 0);
	CString strasd;

	Sleep(2000);
	string txtpath = path;
	txtpath += "1.txt";
	
	
	Read(txtpath);

	////////////////////////////////////////////////////////////
	string sql;
	_RecordsetPtr m_pRecordset;
	sql = "select * from Table_1";// 数据库表名
	_bstr_t bstr_t(sql.c_str());
	m_pRecordset = GetRecordset(bstr_t);
	_variant_t var;
	CString strName, strAge, strID;
	
	char yname[4096] = { 0 };
	char gname[4096] = { 0 };
	try
	{
		if (!m_pRecordset->BOF)
			m_pRecordset->MoveFirst();
		else
		{
			printf("表内数据为空");
			return;
		}
		
		
		for (int i = 0; i < TXTfile.size(); i++)
		{
			int disnum = 1000;
//			cout << TXTfile[i];
			m_pRecordset->MoveFirst();//数据库移动顶端
			test.clear();//清空test容器
			test.shrink_to_fit();
			while (!m_pRecordset->adoEOF)
			{
				var = m_pRecordset->GetCollect("商品名称");
				if (var.vt != VT_NULL)
					strName = (var);
				var = m_pRecordset->GetCollect("规格");
				if (var.vt != VT_NULL)
					strAge = var;

				int dist = ldistance(strName.GetBuffer(0), TXTfile[i]);//与数据库数据对比；
				cout << "dist=" << dist << endl;
				if (dist < disnum)
				{
					disnum = dist;
					memset(yname, 0, 4096);
					strcpy(yname, strName);
					memset(gname, 0, 4096);
					strcpy(gname, strAge);

				}
				test.push_back(dist);//将对比的系数放入容器test;

				m_pRecordset->MoveNext();
			}
			
			auto smallest = min_element(begin(test), end(test));
		//	cout << "min element is " << *smallest << " at position " << distance(begin(test), smallest) << endl;

			int ID = (distance(begin(test), smallest)) + 1;
			sort(test.begin(), test.end());
		//	cout << test[0] << endl;

			char ocrbuffer[4096] = { 0 };//未经对比的文本
			memset(ocrbuffer, 0, 4096);
			strcpy(ocrbuffer, TXTfile[i].c_str());
			


			YPname.push_back(yname);
			YPage.push_back(gname);

// 			Write(yname, strlen(yname));
// 			Write("     ", strlen("     "));
// 			Write(gname, strlen(gname));
// 			Write("     ", strlen("     "));

		}

		TXTfile.clear();//清空数据
		TXTfile.shrink_to_fit();
		
	}
	catch (_com_error *e)
	{
		printf(e->ErrorMessage());
	}

	
	/////////////////////////////////////////////	
}

void startOCR2(string path)
{
	CHAR s1[4096] = { 0 };
	const char* i1 = "2.bmp";
	const char* i2 = "2";
	memset(s1, 0, 4096);
	sprintf_s(s1, "tesseract %s %s -l chi_sim", i1, i2);
	// 运行OCR识别
	WinExec(s1, 0);
	Sleep(2000);
	string txtpath = path;
	txtpath += "2.txt";

	Read(txtpath);

	Write(filebuffer, strlen(filebuffer));
}
void startOCR3(string path)
{
	CHAR s1[4096] = { 0 };
	const char* i1 = "3.bmp";
	const char* i2 = "3";
	memset(s1, 0, 4096);
	sprintf_s(s1, "tesseract %s %s -l chi_sim", i1, i2);
	// 运行OCR识别
	WinExec(s1, 0);
	Sleep(2000);
	string txtpath = path;
	txtpath += "3.txt";

	Read(txtpath);

	Write(filebuffer, strlen(filebuffer));
}

void startOCR4(string path)
{
	CHAR s1[4096] = { 0 };
	const char* i1 = "4.bmp";
	const char* i2 = "4";
	memset(s1, 0, 4096);
	sprintf_s(s1, "tesseract %s %s ", i1, i2);
	// 运行OCR识别
	WinExec(s1, 0);
	Sleep(2000);
	string txtpath4 = path;
	txtpath4 += "4.txt";

	Read(txtpath4);

	for (int i = 0; i < TXTfile.size(); i++)
	{
		Number.push_back(TXTfile[i]);



	}

// 	Write(filebuffer, strlen(filebuffer));
// 	Write("     ", strlen("     "));
	if (Number.size() < 1)
	{
		char * a = "0";
		Number.push_back(a);
	}

	TXTfile.clear();//清空数据
	TXTfile.shrink_to_fit();

}
void startOCR5(string path)
{
	CHAR s1[4096] = { 0 };
	const char* i1 = "5.bmp";
	const char* i2 = "5";
	memset(s1, 0, 4096);
	sprintf_s(s1, "tesseract %s %s ", i1, i2);
	// 运行OCR识别
	WinExec(s1, 0);
	Sleep(2000);
	string txtpath5 = path;
	txtpath5 += "5.txt";
	Read(txtpath5);

	for (int i = 0; i < TXTfile.size(); i++)
	{
		value.push_back(TXTfile[i]);
	}
	if (value.size() < 1)
	{
		char * a = "0";
		value.push_back(a);
	}

	TXTfile.clear();//清空数据
	TXTfile.shrink_to_fit();
}

void trim(string &s)
{
	/*
	if( !s.empty() )
	{
	s.erase(0,s.find_first_not_of(" "));
	s.erase(s.find_last_not_of(" ") + 1);
	}
	*/
	int index = 0;
	if (!s.empty())
	{
		while ((index = s.find(' ', index)) != string::npos)
		{
			s.erase(index, 1);
		}
	}

}

// int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
// {
// 	// 	string sql;
// 	// 	
// 	// 	_RecordsetPtr m_pRecordset;
// 	// 	sql = "select * from Table_1";
// 	// 	_bstr_t bstr_t(sql.c_str());
// 	// 	m_pRecordset = GetRecordset(bstr_t);
// 	// 	m_pRecordset->AddNew();
// 	// 	m_pRecordset->PutCollect("ID", _variant_t("1"));
// 	// 	m_pRecordset->PutCollect("商品名称", _variant_t("阿莫西林克拉维酸钾咀嚼片"));
// 	// 	m_pRecordset->Update();
// 	// 	ExitConnect();
// 
// 
// 	TCHAR exeFullPath[MAX_PATH]; // MAX_PATH在WINDEF.h中定义了，等于260  
// 	memset(exeFullPath, 0, MAX_PATH);
// 
// 
// 	char szModuleFilePath[MAX_PATH];
// 	char SaveResult[MAX_PATH];
// 	int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH); //获得当前执行文件的路径  
// 	szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;//将最后一个"\\"后的字符置为0  
// 	strcpy(SaveResult, szModuleFilePath);
// 	strcat(SaveResult, "\\config.ini");//在当前路径后添加子路径  
// 
// 	GetPrivateProfileString("SETTING", "CONNTECTION_STRING", "", s_connection, MAX_PATH, SaveResult);
// 
// 
// 	int nRetCode = 0;
// 
// 	HMODULE hModule = ::GetModuleHandle(NULL);
// 
// 	if (hModule != NULL)
// 	{
// 
// 		// 初始化 MFC 并在失败时显示错误
// 		if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
// 		{
// 			// TODO:  更改错误代码以符合您的需要
// 			_tprintf(_T("错误:  MFC 初始化失败\n"));
// 			nRetCode = 1;
// 		}
// 		else
// 		{
// 			// TODO:  在此处为应用程序的行为编写代码。
// 
// 			char szCurrentDirectory[MAX_PATH];
// 			GetCurrentDirectory(MAX_PATH, szCurrentDirectory);
// 	//		printf("进程当前目录为: \n%s\n", szCurrentDirectory);
// 
// 			AdjustProcessCurrentDirectory();
// 
// 			GetCurrentDirectory(MAX_PATH, szCurrentDirectory);
// 	//		printf("\n调整后，进程当前目录为: \n%s\n", szCurrentDirectory);
// 			string txtpath1;
// 			string txtpath4;
// 			string txtpath5;
// 			txtpath1 = szCurrentDirectory;
// 			txtpath1 += "\\1.txt";
// 			txtpath4 = szCurrentDirectory;
// 			txtpath4 += "\\4.txt";
// 			txtpath5 = szCurrentDirectory;
// 			txtpath5 += "\\5.txt";
// 			DeleteFile(txtpath1.c_str());
// 			DeleteFile(txtpath4.c_str());
// 			DeleteFile(txtpath5.c_str());
// 
// 			Jpath = szCurrentDirectory;
// 			Jpath += "\\1.txt";
// 			Bpath = szCurrentDirectory;
// 			Bpath += "\\2.txt";
// 			Resultpath = szCurrentDirectory;
// 			Resultpath += "\\result.txt";
// 			HWND hDisplay;
// 			HANDLE finder;
// 			WIN32_FIND_DATA findFileData;
// 			BOOL isOK = TRUE;
// 			CString path = szCurrentDirectory;
// 			path += "\\dest\\";
// 
// 			finder = FindFirstFile(path + _T("*.jpg"), &findFileData);
// 			while ((finder != NULL) && (isOK))
// 			{
// 
// 				char szCurrentDirectory[MAX_PATH];
// 				GetCurrentDirectory(MAX_PATH, szCurrentDirectory);
// 			//	printf("进程当前目录为: \n%s\n", szCurrentDirectory);
// 
// 				AdjustProcessCurrentDirectory();
// 
// 				GetCurrentDirectory(MAX_PATH, szCurrentDirectory);
// 			//	printf("\n调整后，进程当前目录为: \n%s\n", szCurrentDirectory);
// 
// 				string path11 = path + findFileData.cFileName;//需要截取图片的路径；
// 				img = imread(path11, 1);
// 				showImg = img.clone();
// 				selecta.x = selecta.y = 0;
// 				imshow("img", showImg);
// 				waitKey(5);//不添加等待图片显示灰色；
// 
// 
// 				
// 				string shotpath = szCurrentDirectory ;// 保存用于ocr识别截图的路径
// 
// 				shotpath += "\\";
// 
// 				string shotpath1, shotpath2, shotpath3, shotpath4, shotpath5 = shotpath;
// 				shotpath1 += "1.bmp"; shotpath2 += "2.bmp"; shotpath3 += "3.bmp"; shotpath4 += "4.bmp"; shotpath5 += "5.bmp";
// 				S_on_Mouse(255, 675, 800, 1000, shotpath1);//调用opencv截图；
// 				Sleep(500);
// 				startOCR(shotpath);
// //  			Sleep(2000);
// //  			S_on_Mouse(870, 675, 1160, 1000, shotpath2);
// // 				Sleep(500);
// // 				startOCR2(shotpath);
// // 				Sleep(2000);
// // 				S_on_Mouse(1172, 675, 1313, 1000, shotpath3);
// // 				Sleep(500);
// // 				startOCR3(shotpath);
// 				Sleep(2000);
// 				S_on_Mouse(1319, 675, 1555, 1000, shotpath4);
// 				Sleep(500);
// 				startOCR4(shotpath);
// 				Sleep(2000);
// 				S_on_Mouse(1563, 675, 1795, 1000, shotpath5);
// 				Sleep(500);
// 				startOCR5(shotpath);
// 				Sleep(2000);
// 
// 				for (size_t i = 0; i < YPname.size(); i++)
// 				{
// 					string TxtResult = YPname[i];
// 					TxtResult += "    ";
// 					TxtResult += YPage[i];
// 					TxtResult += "    ";
// 					TxtResult += Number[i];
// 					TxtResult += "    ";
// 					TxtResult += value[i];
// 
// 					int a = atoi(Number[i].c_str());
// 					const	char* str = value[i].c_str();
// 
// 					string s = str;
// 					trim(s);
// 					float b = strtod(s.c_str(), NULL);
// 					float C = a*b;//计算金额
// 					char sf[20];
// 					sprintf(sf, "%0.2f", C);  // float 到 char
// 
// 					TxtResult += "    ";
// 					TxtResult += sf;
// 
// 					Write(TxtResult.c_str(), TxtResult.length());
// 					Write("\r\n", strlen("\r\n"));
// 		
// 				}
// 				YPname.clear();//清空数据
// 				YPname.shrink_to_fit();
// 				YPage.clear();//清空数据
// 				YPage.shrink_to_fit();
// 				Number.clear();//清空数据
// 				Number.shrink_to_fit();
// 				value.clear();//清空数据
// 				value.shrink_to_fit();
// 
// 				isOK = FindNextFile(finder, &findFileData);
// 				
// 			}
// 
// 			//	system("pause");
// 			return 0;
// 		}
// 	}
// 	else
// 	{
// 		// TODO:  更改错误代码以符合您的需要
// 		_tprintf(_T("错误:  GetModuleHandle 失败\n"));
// 		nRetCode = 1;
// 	}
// 
// 
// 	return 0;
// }

extern "C" __declspec(dllexport) void* Distinguish(char*path,char *result)
{
	TCHAR exeFullPath[MAX_PATH]; // MAX_PATH在WINDEF.h中定义了，等于260  
	memset(exeFullPath, 0, MAX_PATH);

	 char * txtresult="";

	char szModuleFilePath[MAX_PATH];
	char SaveResult[MAX_PATH];
	int n = GetModuleFileNameA(0, szModuleFilePath, MAX_PATH); //获得当前执行文件的路径  
	szModuleFilePath[strrchr(szModuleFilePath, '\\') - szModuleFilePath + 1] = 0;//将最后一个"\\"后的字符置为0  
	strcpy(SaveResult, szModuleFilePath);
	strcat(SaveResult, "\\config.ini");//在当前路径后添加子路径  

	GetPrivateProfileString("SETTING", "CONNTECTION_STRING", "", s_connection, MAX_PATH, SaveResult);


	int nRetCode = 0;


			// TODO:  在此处为应用程序的行为编写代码。

			char szCurrentDirectory[MAX_PATH];
			GetCurrentDirectory(MAX_PATH, szCurrentDirectory);
			//		printf("进程当前目录为: \n%s\n", szCurrentDirectory);

			AdjustProcessCurrentDirectory();

			GetCurrentDirectory(MAX_PATH, szCurrentDirectory);
			//		printf("\n调整后，进程当前目录为: \n%s\n", szCurrentDirectory);
			string txtpath1;
			string txtpath4;
			string txtpath5;
			txtpath1 = szCurrentDirectory;
			txtpath1 += "\\1.txt";
			txtpath4 = szCurrentDirectory;
			txtpath4 += "\\4.txt";
			txtpath5 = szCurrentDirectory;
			txtpath5 += "\\5.txt";
			DeleteFile(txtpath1.c_str());
			DeleteFile(txtpath4.c_str());
			DeleteFile(txtpath5.c_str());

			Jpath = szCurrentDirectory;
			Jpath += "\\1.txt";
			Bpath = szCurrentDirectory;
			Bpath += "\\2.txt";
			Resultpath = szCurrentDirectory;
			Resultpath += "\\result.txt";
			HWND hDisplay;
			HANDLE finder;
			WIN32_FIND_DATA findFileData;

				string path11 = path; //需要截取图片的路径；
				img = imread(path11, 1);
				showImg = img.clone();
				selecta.x = selecta.y = 0;
				imshow("img", showImg);
				waitKey(5);//不添加等待图片显示灰色；



				string shotpath = szCurrentDirectory;// 保存用于ocr识别截图的路径

				shotpath += "\\";

				string shotpath1, shotpath2, shotpath3, shotpath4, shotpath5 = shotpath;
				shotpath1 += "1.bmp"; shotpath2 += "2.bmp"; shotpath3 += "3.bmp"; shotpath4 += "4.bmp"; shotpath5 += "5.bmp";
				S_on_Mouse(255, 675, 800, 1000, shotpath1);//调用opencv截图；
				Sleep(500);
				startOCR(shotpath);
				//  			Sleep(2000);
				//  			S_on_Mouse(870, 675, 1160, 1000, shotpath2);
				// 				Sleep(500);
				// 				startOCR2(shotpath);
				// 				Sleep(2000);
				// 				S_on_Mouse(1172, 675, 1313, 1000, shotpath3);
				// 				Sleep(500);
				// 				startOCR3(shotpath);
				Sleep(2000);
				S_on_Mouse(1319, 675, 1555, 1000, shotpath4);
				Sleep(500);
				startOCR4(shotpath);
				Sleep(2000);
				S_on_Mouse(1563, 675, 1795, 1000, shotpath5);
				Sleep(500);
				startOCR5(shotpath);
				Sleep(2000);


					string TxtResult = YPname[0];
					TxtResult += "    ";
					TxtResult += YPage[0];
					TxtResult += "    ";
					TxtResult += Number[0];
					TxtResult += "    ";
					TxtResult += value[0];

					int a = atoi(Number[0].c_str());
					const	char* str = value[0].c_str();

					string s = str;
					trim(s);
					float b = strtod(s.c_str(), NULL);
					float C = a*b;//计算金额
					char sf[20];
					sprintf(sf, "%0.2f", C);  // float 到 char

					TxtResult += "    ";
					TxtResult += sf;

					Write(TxtResult.c_str(), TxtResult.length());
					Write("\r\n", strlen("\r\n"));
					

					
					int len = TxtResult.length();
					TxtResult.copy(result, len, 0);
				
// 				YPname.clear();//清空数据
// 				YPname.shrink_to_fit();
// 				YPage.clear();//清空数据
// 				YPage.shrink_to_fit();
// 				Number.clear();//清空数据
// 				Number.shrink_to_fit();
// 				value.clear();//清空数据
// 				value.shrink_to_fit();

				

			//	system("pause");
			return 0;
}
	

