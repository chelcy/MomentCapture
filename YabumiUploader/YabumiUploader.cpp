// YabumiUploader.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "MomentCapture.h"

// グローバル変数:
HINSTANCE hInst;// 現在のインターフェイス
TCHAR *szTitle        = _T("MomentCapture Lite");// タイトル バーのテキスト
TCHAR *szWindowClass  = _T("YABUMIUPLOADER");// メイン ウィンドウ クラス名
TCHAR *szWindowClassL = _T("YABUMIUPLOADERL");// レイヤー ウィンドウ クラス名
TCHAR *szFileDir;
HWND hLayerWnd;

int ofX, ofY;// 画面オフセット

// プロトタイプ宣言
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	LayerWndProc(HWND, UINT, WPARAM, LPARAM);

int					GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

BOOL				isPng(LPCTSTR fileName);
BOOL				isGif(LPCTSTR fileName);
BOOL				isJpg(LPCTSTR fileName);
BOOL				isSvg(LPCTSTR fileName);
BOOL				isPdf(LPCTSTR fileName);
BOOL				isPsd(LPCTSTR fileName);
VOID				drawRubberband(HDC hdc, LPRECT newRect, BOOL erase);
VOID				execUrl(const char* str);
VOID				setClipBoardText(const char* str);
BOOL				convertPNG(LPCTSTR destFile, LPCTSTR srcFile);
BOOL				savePNG(LPCTSTR fileName, HBITMAP newBMP);
BOOL				uploadFile(HWND hwnd, LPCTSTR fileName);
std::string			getId();
BOOL				saveId(const WCHAR* str);
void				LastErrorMessageBox(HWND hwnd, LPTSTR lpszError);


// エントリーポイント
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;

	TCHAR	szThisPath[MAX_PATH];
	DWORD   sLen;

	// 自身のディレクトリを取得する
	sLen = GetModuleFileName(NULL, szThisPath, MAX_PATH);
	for(unsigned int i = sLen; i >= 0; i--) {
		if(szThisPath[i] == _T('\\')) {
			szThisPath[i] = _T('\0');
			break;
		}
	}

	szFileDir = szThisPath;

	// カレントディレクトリを exe と同じ場所に設定
	SetCurrentDirectory(szThisPath);

	// 引数にファイルが指定されていたら
	if ( 2 == __argc )
	{
		// ファイルをアップロードして終了
		if (isPng(__targv[1]) || isJpg(__targv[1]) || isGif(__targv[1]) || isSvg(__targv[1]) || isPdf(__targv[1]) || isPsd(__targv[1])) {
			// PNG はそのままupload
			uploadFile(NULL, __targv[1]);
		} else {
			// PNG 形式に変換
			TCHAR tmpDir[MAX_PATH], tmpFile[MAX_PATH];
			GetTempPath(MAX_PATH, tmpDir);
			GetTempFileName(tmpDir, _T("gya"), 0, tmpFile);
			
			if (convertPNG(tmpFile, __targv[1])) {
				//アップロード
				uploadFile(NULL, tmpFile);
			} else {
				// PNGに変換できなかった...
				MessageBox(NULL, _T("Cannot convert this image"), szTitle, 
					MB_OK | MB_ICONERROR);
			}
			DeleteFile(tmpFile);
		}
		return TRUE;
	}

	// ウィンドウクラスを登録
	MyRegisterClass(hInstance);

	// アプリケーションの初期化を実行します:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}
	
	// メイン メッセージ ループ:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

// ヘッダを見て PNG 画像かどうか(一応)チェック
BOOL isPng(LPCTSTR fileName)
{
	unsigned char pngHead[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	unsigned char readHead[8];
	
	FILE *fp = NULL;
	
	if (0 != _tfopen_s(&fp, fileName, _T("rb")) ||
		8 != fread(readHead, 1, 8, fp)) {
		// ファイルが読めない	
		return FALSE;
	}
	fclose(fp);
	
	// compare
	for(unsigned int i=0;i<8;i++)
		if(pngHead[i] != readHead[i]) return FALSE;

	return TRUE;

}

// ヘッダを見て GIF 画像かどうか(一応)チェック
BOOL isGif(LPCTSTR fileName)
{
	unsigned char gifHead[] = { 0x47, 0x49, 0x46 };
	unsigned char readHead[3];

	FILE *fp = NULL;

	if (0 != _tfopen_s(&fp, fileName, _T("rb")) ||
		3 != fread(readHead, 1, 3, fp)) {
		// ファイルが読めない	
		return FALSE;
	}
	fclose(fp);

	// compare
	for (unsigned int i = 0; i<3; i++)
	if (gifHead[i] != readHead[i]) return FALSE;

	return TRUE;

}

// ヘッダを見て JPG 画像かどうか(一応)チェック
BOOL isJpg(LPCTSTR fileName)
{
	unsigned char jpgHead[] = { 0xff, 0xd8 };
	unsigned char readHead[2];

	FILE *fp = NULL;

	if (0 != _tfopen_s(&fp, fileName, _T("rb")) ||
		2 != fread(readHead, 1, 2, fp)) {
		// ファイルが読めない	
		return FALSE;
	}
	fclose(fp);

	// compare
	for (unsigned int i = 0; i<2; i++)
	if (jpgHead[i] != readHead[i]) return FALSE;

	return TRUE;

}

// ヘッダを見て SVG かどうか(一応)チェック
BOOL isSvg(LPCTSTR fileName)
{
	unsigned char svgHead[] = { 0x3c };
	unsigned char readHead[1];

	FILE *fp = NULL;

	if (0 != _tfopen_s(&fp, fileName, _T("rb")) ||
		1 != fread(readHead, 1, 1, fp)) {
		// ファイルが読めない	
		return FALSE;
	}
	fclose(fp);

	// compare
	for (unsigned int i = 0; i<1; i++)
	if (svgHead[i] != readHead[i]) return FALSE;

	return TRUE;

}

// ヘッダを見て PDF かどうか(一応)チェック
BOOL isPdf(LPCTSTR fileName)
{
	unsigned char pdfHead[] = { 0x25, 0x50, 0x44, 0x46, 0x2d, 0x31, 0x2e };
	unsigned char readHead[7];

	FILE *fp = NULL;

	if (0 != _tfopen_s(&fp, fileName, _T("rb")) ||
		7 != fread(readHead, 1, 7, fp)) {
		// ファイルが読めない	
		return FALSE;
	}
	fclose(fp);

	// compare
	for (unsigned int i = 0; i<7; i++)
		if (pdfHead[i] != readHead[i]) return FALSE;

	return TRUE;

}

// ヘッダを見て PSD かどうか(一応)チェック
BOOL isPsd(LPCTSTR fileName)
{
	unsigned char psdHead[] = { 0x38, 0x42, 0x50, 0x53, 0x00, 0x01, 0x00 };
	unsigned char readHead[7];

	FILE *fp = NULL;

	if (0 != _tfopen_s(&fp, fileName, _T("rb")) ||
		7 != fread(readHead, 1, 7, fp)) {
		// ファイルが読めない	
		return FALSE;
	}
	fclose(fp);

	// compare
	for (unsigned int i = 0; i<7; i++)
		if (psdHead[i] != readHead[i]) return FALSE;

	return TRUE;

}

// ウィンドウクラスを登録
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASS wc;

	// メインウィンドウ
	wc.style         = 0;							// WM_PAINT を送らない
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GYAZOWIN));
	wc.hCursor       = LoadCursor(NULL, IDC_CROSS);	// + のカーソル
	wc.hbrBackground = 0;							// 背景も設定しない
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClass;

	RegisterClass(&wc);

	// レイヤーウィンドウ
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = LayerWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GYAZOWIN));
	wc.hCursor       = LoadCursor(NULL, IDC_CROSS);	// + のカーソル
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = szWindowClassL;

	return RegisterClass(&wc);
}


// インスタンスの初期化（全画面をウィンドウで覆う）
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
//	HWND hLayerWnd;
	hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

	int x, y, w, h;

	// 仮想スクリーン全体をカバー
	x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

	// x, y のオフセット値を覚えておく
	ofX = x; ofY = y;

	// 完全に透過したウィンドウを作る
	hWnd = CreateWindowEx(
		WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST
#if(_WIN32_WINNT >= 0x0500)
		| WS_EX_NOACTIVATE
#endif
		,
		szWindowClass, NULL, WS_POPUP,
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);

	// 作れなかった...?
	if (!hWnd) return FALSE;
	
	// 全画面を覆う
	MoveWindow(hWnd, x, y, w, h, FALSE);
	
	// nCmdShow を無視 (SW_MAXIMIZE とかされると困る)
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	// ESCキー検知タイマー
	SetTimer(hWnd, 1, 100, NULL);


	// レイヤーウィンドウの作成
	hLayerWnd = CreateWindowEx(
	 WS_EX_TOOLWINDOW
#if(_WIN32_WINNT >= 0x0500)
		| WS_EX_LAYERED | WS_EX_NOACTIVATE
#endif
		,
		szWindowClassL, NULL, WS_POPUP,
		100, 100, 300, 300,
		hWnd, NULL, hInstance, NULL);

    SetLayeredWindowAttributes(hLayerWnd, RGB(255, 0, 0), 100, LWA_COLORKEY|LWA_ALPHA);

	return TRUE;
}

// 指定されたフォーマットに対応する Encoder の CLSID を取得する
// Cited from MSDN Library: Retrieving the Class Identifier for an Encoder
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;// number of image encoders
   UINT  size = 0;// size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;// Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;// Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
	   if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
	   {
		   *pClsid = pImageCodecInfo[j].Clsid;
		   free(pImageCodecInfo);
		   return j;// Success
	   }
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}

// ラバーバンドを描画.
VOID drawRubberband(HDC hdc, LPRECT newRect, BOOL erase)
{
	
	static BOOL firstDraw = TRUE;// 1 回目は前のバンドの消去を行わない
	static RECT lastRect  = {0};// 最後に描画したバンド
	static RECT clipRect  = {0};// 最後に描画したバンド
	
	if(firstDraw) {
		// レイヤーウィンドウを表示
		ShowWindow(hLayerWnd, SW_SHOW);
		UpdateWindow(hLayerWnd);

		firstDraw = FALSE;
	}

	if (erase) {
		// レイヤーウィンドウを隠す
		ShowWindow(hLayerWnd, SW_HIDE);
	}

	// 座標チェック
	clipRect = *newRect;
	if ( clipRect.right < clipRect.left ) {
		int tmp = clipRect.left;
		clipRect.left   = clipRect.right;
		clipRect.right  = tmp;
	}
	if ( clipRect.bottom < clipRect.top  ) {
		int tmp = clipRect.top;
		clipRect.top    = clipRect.bottom;
		clipRect.bottom = tmp;
	}
	MoveWindow(
		hLayerWnd,
		clipRect.left,
		clipRect.top,
		clipRect.right - clipRect.left + 1,
		clipRect.bottom - clipRect.top + 1,
		true
	);

	
	return;
}

// PNG 形式に変換
BOOL convertPNG(LPCTSTR destFile, LPCTSTR srcFile)
{
	BOOL				res = FALSE;

	GdiplusStartupInput	gdiplusStartupInput;
	ULONG_PTR			gdiplusToken;
	CLSID				clsidEncoder;

	// GDI+ の初期化
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	Image *b = new Image(srcFile, 0);

	if (0 == b->GetLastStatus()) {
		if (GetEncoderClsid(L"image/png", &clsidEncoder)) {
			// save!
			if (0 == b->Save(destFile, &clsidEncoder, 0) ) {
					// 保存できた
					res = TRUE;
			}
		}
	}

	// 後始末
	delete b;
	GdiplusShutdown(gdiplusToken);

	return res;
}

// PNG 形式で保存 (GDI+ 使用)
BOOL savePNG(LPCTSTR fileName, HBITMAP newBMP)
{
	BOOL				res = FALSE;

	GdiplusStartupInput	gdiplusStartupInput;
	ULONG_PTR			gdiplusToken;
	CLSID				clsidEncoder;

	// GDI+ の初期化
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	
	// HBITMAP から Bitmap を作成
	Bitmap *b = new Bitmap(newBMP, NULL);
	
	if (GetEncoderClsid(L"image/png", &clsidEncoder)) {
		// save!
		if (0 == b->Save(fileName, &clsidEncoder, 0) ) {
				// 保存できた
				res = TRUE;
		}
	}
	
	// 後始末
	delete b;
	GdiplusShutdown(gdiplusToken);

	return res;
}

// レイヤーウィンドウプロシージャ
LRESULT CALLBACK LayerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	RECT clipRect	= {0, 0, 500, 500};
	HBRUSH hBrush;
	HPEN hPen;
	HFONT hFont;

	switch (message)
	{
	case WM_ERASEBKGND:
		 GetClientRect(hWnd, &clipRect);
		
		hdc = GetDC(hWnd);
        hBrush = CreateSolidBrush(RGB(100,100,100));
        SelectObject(hdc, hBrush);
		hPen = CreatePen(PS_DASH,1,RGB(255,255,255));
		SelectObject(hdc, hPen);
		Rectangle(hdc,0,0,clipRect.right,clipRect.bottom);

		//矩形のサイズを出力
		int fHeight;
		fHeight = -MulDiv(8, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		hFont = CreateFont(fHeight,//フォント高さ
			0,//文字幅
			0,//テキストの角度
			0,//ベースラインとｘ軸との角度
			FW_REGULAR,//フォントの重さ（太さ）
			FALSE,//イタリック体
			FALSE,//アンダーライン
			FALSE,//打ち消し線
			ANSI_CHARSET,//文字セット
			OUT_DEFAULT_PRECIS,//出力精度
			CLIP_DEFAULT_PRECIS,//クリッピング精度
			PROOF_QUALITY,//出力品質
			FIXED_PITCH | FF_MODERN,//ピッチとファミリー
			L"Tahoma");//書体名

		SelectObject(hdc, hFont);
		// show size
		int iWidth, iHeight;
		iWidth  = clipRect.right  - clipRect.left;
		iHeight = clipRect.bottom - clipRect.top;

		wchar_t sWidth[200], sHeight[200];
		swprintf_s(sWidth, L"%d", iWidth);
		swprintf_s(sHeight, L"%d", iHeight);

		int w,h,h2;
		w = -fHeight * 2.5 + 8;
		h = -fHeight * 2 + 8;
		h2 = h + fHeight;

		SetBkMode(hdc,TRANSPARENT);
		SetTextColor(hdc,RGB(0,0,0));
		TextOut(hdc, clipRect.right-w+1,clipRect.bottom-h+1,(LPCWSTR)sWidth,wcslen(sWidth));
		TextOut(hdc, clipRect.right-w+1,clipRect.bottom-h2+1,(LPCWSTR)sHeight,wcslen(sHeight));
		SetTextColor(hdc,RGB(255,255,255));
		TextOut(hdc, clipRect.right-w,clipRect.bottom-h,(LPCWSTR)sWidth,wcslen(sWidth));
		TextOut(hdc, clipRect.right-w,clipRect.bottom-h2,(LPCWSTR)sHeight,wcslen(sHeight));

		DeleteObject(hPen);
		DeleteObject(hBrush);
		DeleteObject(hFont);
		ReleaseDC(hWnd, hdc);

		return TRUE;

        break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	
	static BOOL onClip		= FALSE;
	static BOOL firstDraw	= TRUE;
	static RECT clipRect	= {0, 0, 0, 0};
	
	switch (message)
	{
	case WM_RBUTTONDOWN:
		// キャンセル
		DestroyWindow(hWnd);
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;

	case WM_TIMER:
		// ESCキー押下の検知
		if (GetKeyState(VK_ESCAPE) & 0x8000){
			DestroyWindow(hWnd);
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_MOUSEMOVE:
		if (onClip) {
			// 新しい座標をセット
			clipRect.right  = LOWORD(lParam) + ofX;
			clipRect.bottom = HIWORD(lParam) + ofY;
			
			hdc = GetDC(NULL);
			drawRubberband(hdc, &clipRect, FALSE);

			ReleaseDC(NULL, hdc);
		}
		break;
	

	case WM_LBUTTONDOWN:
		{
			// クリップ開始
			onClip = TRUE;
			
			// 初期位置をセット
			clipRect.left = LOWORD(lParam) + ofX;
			clipRect.top  = HIWORD(lParam) + ofY;
			


			// マウスをキャプチャ
			SetCapture(hWnd);
		}
		break;

	case WM_LBUTTONUP:
		{
			// クリップ終了
			onClip = FALSE;
			
			// マウスのキャプチャを解除
			ReleaseCapture();
		
			// 新しい座標をセット
			clipRect.right  = LOWORD(lParam) + ofX;
			clipRect.bottom = HIWORD(lParam) + ofY;

			// 画面に直接描画，って形
			HDC hdc = GetDC(NULL);

			// 線を消す
			drawRubberband(hdc, &clipRect, TRUE);
			DestroyWindow(hLayerWnd);

			// 座標チェック
			if (clipRect.right < clipRect.left) {
				int tmp = clipRect.left;
				clipRect.left   = clipRect.right;
				clipRect.right  = tmp;
			}
			if (clipRect.bottom < clipRect.top) {
				int tmp = clipRect.top;
				clipRect.top    = clipRect.bottom;
				clipRect.bottom = tmp;
			}
			
			// 画像のキャプチャ
			int iWidth, iHeight;
			iWidth  = clipRect.right  - clipRect.left + 1;
			iHeight = clipRect.bottom - clipRect.top  + 1;

			if(iWidth == 0 || iHeight == 0) {
				// 画像になってない, なにもしない
				ReleaseDC(NULL, hdc);
				DestroyWindow(hWnd);
				break;
			}

			// ビットマップバッファを作成
			HBITMAP newBMP = CreateCompatibleBitmap(hdc, iWidth, iHeight);
			HDC	    newDC  = CreateCompatibleDC(hdc);
			
			// 関連づけ
			SelectObject(newDC, newBMP);

			// 画像を取得
			BitBlt(newDC, 0, 0, iWidth, iHeight, 
				hdc, clipRect.left, clipRect.top, SRCCOPY);
			
			// ウィンドウを隠す!
			ShowWindow(hWnd, SW_HIDE);
			
			// テンポラリファイル名を決定
			TCHAR tmpDir[MAX_PATH], tmpFile[MAX_PATH];
			GetTempPath(MAX_PATH, tmpDir);
			GetTempFileName(tmpDir, _T("gya"), 0, tmpFile);
			
			if (savePNG(tmpFile, newBMP)) {
				// うｐ
				uploadFile(hWnd, tmpFile);
			} else {
				// PNG保存失敗...
				MessageBox(hWnd, _T("Cannot save png image"), szTitle, MB_OK | MB_ICONERROR);
			}

			// 後始末
			DeleteFile(tmpFile);
			
			DeleteDC(newDC);
			DeleteObject(newBMP);

			ReleaseDC(NULL, hdc);
			DestroyWindow(hWnd);
			PostQuitMessage(0);
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


void LastErrorMessageBox(HWND hwnd, LPTSTR lpszError) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_FROM_HMODULE |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        GetModuleHandle(_T("wininet.dll")),
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message
    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszError) + 40) * sizeof(TCHAR)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s\n\nError %d: %s"), 
        lpszError, dw, lpMsgBuf); 
    MessageBox(hwnd, (LPCTSTR)lpDisplayBuf, szTitle, MB_OK | MB_ICONERROR); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

//デバッグファイル
/*
void db(const char* c) {
	std::ofstream wfile("debug.txt");
	wfile << c;
	wfile.close();
}
*/

char const *GetExportName(char*);

//パス読み込み
char* GetTargetPath() {
	//pathファイルが存在しなければ生成
	//std::ofstream ofile;
	//ofile.open("path.txt", std::ios::app);
	//存在するとき
	std::ifstream rfile;
	std::string rbuf;
	char cdir[255];
	rfile.open("path.txt", std::ios::in);
	GetCurrentDirectory(255, (LPWSTR)cdir);
	//読み込みエラーなら
	if (!rfile) {
		return NULL;
	}
	std::getline(rfile, rbuf);
	//1行目が空白なら
	if (rbuf == "") {
		return NULL;
	}
	//中身があったら
	char* cstr = new char[rbuf.size() + 1];
	std::strcpy(cstr, rbuf.c_str());
	return cstr;
}

// PNG ファイルを保存
BOOL uploadFile(HWND hwnd, LPCTSTR fileName)
{
	//キャッシュファイルを用意
	std::ifstream iim(fileName, std::ios_base::binary);
	//出力先指定
	std::ofstream oim(GetExportName(GetTargetPath()), std::ios_base::binary);
	//出力
	oim << iim.rdbuf();
	//ディレクトリをexeがある場所へ
	SetCurrentDirectory(szFileDir);
	//コピーでエラったら
	if (!(iim && oim)) {
		return FALSE;
	}
	return TRUE;
}

//ケタ埋め
/*
char const *fillZero(int num, int deg) {
	std::ostringstream sout;
	sout << std::setfill('0') << std::setw(deg) << num;
	std::string s = sout.str();
	return s.c_str();
}*/

//パスとファイル名決定
char const *GetExportName(char* dir) {
	//テキストバッファ用意
	std::ostringstream buf;
	//時間取得
	time_t now = time(NULL);
	struct tm *pnow = localtime(&now);
	//もしディレクトリの指定があったら記述
	if (dir != NULL) {
		buf << dir;
		buf << "\\";
	}
	//指定がなかったらデスクトップ
	else {
		//ディレクトリをデスクトップへ
		TCHAR szPath[MAX_PATH + 1];
		SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOP, FALSE);
		SetCurrentDirectory(szPath);
	}
	buf << pnow->tm_year + 1900;
	buf << "-";
	buf << pnow->tm_mon + 1;
	buf << "-";
	buf << pnow->tm_mday;
	buf << "_";
	buf << pnow->tm_hour;
	buf << "-";
	buf << pnow->tm_min;
	buf << "-";
	buf << pnow->tm_sec;
	buf << ".png";

	return buf.str().c_str();
}
