// LCOS_app.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "LCOS_app.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define MAX_LOADSTRING 100
#define WINDOW_WIDE 400
#define WINDOW_HIGH 200

#define MOVIE_START 2
#define MOVIE_END 0
#define MOVIE_WAIT 0
#define HPK 1

#define Split_Image 4
#define Separate_Image 16
#define Num_Image Split_Image*Separate_Image

#define X_Forward 1
#define X_Backward 2
#define Rotate_Forward 3
#define Rotate_Backward 4

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// 二つ目のウィンドウについて
LPCWSTR szWindowClass2 = L"BitMap Window";
ATOM BitMapClass(HINSTANCE hInstance);
LRESULT CALLBACK WndProc2(HWND, UINT, WPARAM, LPARAM);

//モニター情報取得関連
HMONITOR hMonitor;
MONITORINFOEX MonitorInfoEx;
POINT pt = { 1921, 0 };

//BITMAPに必要な変数
HDC hdc_men = 0;
HDC hdc_men_array[50];
int w = 0, h = 0;
int drawing = 0;

//ダイアログに関して
BOOL CALLBACK AlignmentDlgProc(HWND, UINT, WPARAM, LPARAM);

//回転、Xステージへの送信関数
BOOL Send_Stage_Message(HWND hSSM,char const* equipment,char const* controll_num,char const* move);
//ステージコントロールと画像表示の為の関数
void Control_Stage_and_image(HWND hWnd,int bitmap_num,int movement);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: ここにコードを挿入してください。

    // グローバル文字列を初期化する
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_LCOSAPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    BitMapClass(hInstance);
    
    //モニター情報の取得関連
    hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    MonitorInfoEx.cbSize = sizeof(MonitorInfoEx);
    GetMonitorInfo(hMonitor, &MonitorInfoEx);//MonitorInfoEx内に情報が格納される

    // アプリケーション初期化の実行:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LCOSAPP));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LCOSAPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_LCOSAPP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//二つ目のウィンドクラスの登録
ATOM BitMapClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc2;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_LCOSAPP));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDI_LCOSAPP);
    wcex.lpszClassName = szWindowClass2;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

   HWND hWnd = CreateWindowW(szWindowClass,
       TEXT("操作メニュー"), 
       WS_OVERLAPPEDWINDOW,
       CW_USEDEFAULT, 
       0, 
       WINDOW_WIDE, 
       WINDOW_HIGH, 
       nullptr, 
       nullptr,
       hInstance, 
       nullptr);


   HWND hWnd2 = CreateWindowW(szWindowClass2, //クラス名
       TEXT("BmpWindow"),			        //ウィンドウ名
       WS_POPUPWINDOW,					//ウィンドウスタイル
       MonitorInfoEx.rcMonitor.left,	    //X位置
       MonitorInfoEx.rcMonitor.top,			//Y位置
       MonitorInfoEx.rcMonitor.right- MonitorInfoEx.rcMonitor.left,	    //ウィンドウ幅
       MonitorInfoEx.rcMonitor.bottom- MonitorInfoEx.rcMonitor.top,	    //ウィンドウ高さ
       nullptr,								//親ウィンドウのハンドル，親を作るときはnullptr
       nullptr,								//メニューハンドル，クラスメニューを使うときはnullptr
       hInstance,							//インスタンスハンドル
       nullptr);							//ウィンドウ作成データ

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   ShowWindow(hWnd2, nCmdShow);
   UpdateWindow(hWnd2);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;
    static HWND hSend, hShow, hAllmag, hWrintig, hTestwriting, hReset;
    static HWND hShow2;

    HBITMAP hBmp[200];
    TCHAR bmpname[] = TEXT("cubic3_10_10AA");

    WCHAR szBuff[1024];
    char const *eq = "RPS1", *con = "0", *move = "10000000";

    unsigned char Set_moving_stage[] = {
        X_Forward,
        Rotate_Forward,
        X_Backward,
        Rotate_Forward
    };
    int count = 0;

    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;

            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;

            case ID_ALIGNMENT:
                //ALIGNMENTボタンを押したときの動作
                DialogBox(hInst, MAKEINTRESOURCE(ALIGNMENT_DAILOG), hWnd, (DLGPROC)AlignmentDlgProc);
                break;

            case ID_SHOW:
                //SHOWボタンを押したときの動作
                hWnd = FindWindow(NULL, TEXT("BmpWindow"));
                drawing = HPK;
                SendMessage(hWnd, WM_PAINT, NULL, NULL);
                break;
            case ALL_MAG:

                //ALL MAGボタンを押したときの動作
                hWnd = FindWindow(NULL, TEXT("Chamonix"));
                if (hWnd != 0) {
                    eq = "RPS2";//RPS"2"この2が動作させる機器の番号に対応する。（CharmonixやCRUXを見ればどちらが何番かがわかるはず。）
                    con = "0";//Charmonix内のSystemに保存されている速度テーブルの選択番号
                    //以下番号要約
                    //0：タイリング法における最適値（とりあえず）
                    //1～8:予約（必要になり次第、登録すること）
                    //9：自由に変えてよい値、テスト、試験用
                    move = "90000";//これで半回転　入力可能な値は5桁の数字まで、6桁の数字を入力すると一の位が省略された数字が入力されたと判断され動作する。
                    Send_Stage_Message(hWnd, eq, con, move);
                    while (Send_Stage_Message == 0);
                    Send_Stage_Message(hWnd, "RPS2", "9", "900");
                    while (Send_Stage_Message == 0);
                }
                else {
                    MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
                }
                break;

            case WRITING:
                //WRITINGボタンを押したときの動作
                for (int a = 0; a < 4; a++) {
                    Control_Stage_and_image(hWnd, 3, 10000);
                    Control_Stage_and_image(hWnd, 3 + Split_Image * Separate_Image, 10000);
                    Control_Stage_and_image(hWnd, 3 + Split_Image * Separate_Image * 2, 10000);
                    Control_Stage_and_image(hWnd, 3 + Split_Image * Separate_Image * 3, 10000);
                    /*                    for (int i = 0; i < Split_Image; i++) {
                                            for (int m = 0; m < Separate_Image; m++) {
                                                hdc = GetDC(hWnd);
                                                wsprintf(bmpname, TEXT("IDB_BITMAP%d"), 3 + m + i * Separate_Image);//TEXT("MOVIE1_%d_%d"), i, m);
                                                int imagenum = MOVIE_START + Separate_Image * i + m;
                                                hBmp[imagenum] = LoadBitmap(hInst, bmpname);
                                                hdc_men_array[imagenum] = CreateCompatibleDC(hdc);
                                                SelectObject(hdc_men_array[imagenum], hBmp[imagenum]);
                                                ReleaseDC(hWnd, hdc);
                                            }
                                        }
                                        Sleep(1000);

                                        for (int i = MOVIE_START; i < MOVIE_START + Split_Image * Separate_Image; i++) {
                                            hWnd = FindWindow(NULL, TEXT("BmpWindow"));
                                            drawing = i;
                                            SendMessage(hWnd, WM_PAINT, NULL, NULL);
                                            Sleep(40);
                                        }

                                        drawing = MOVIE_WAIT;
                                        SendMessage(hWnd, WM_PAINT, NULL, NULL);

                                        hWnd = FindWindow(NULL, TEXT("Chamonix"));
                                        if (hWnd != 0) {
                                            eq = "RPS2";
                                            con = "9";
                                            move = "10000";
                                            Send_Stage_Message(hWnd, eq, con, move);
                                            while (Send_Stage_Message == 0);
                                        }
                                        else {
                                            MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
                                        }


                                        for (int i = 0; i < Split_Image; i++) {
                                            for (int m = 0; m < Separate_Image; m++) {
                                                hdc = GetDC(hWnd);
                                                wsprintf(bmpname, TEXT("IDB_BITMAP%d"), 3 + m + i * Separate_Image + Separate_Image * Split_Image);
                                                int imagenum = MOVIE_START + Separate_Image * i + m;
                                                hBmp[imagenum] = LoadBitmap(hInst, bmpname);
                                                hdc_men_array[imagenum] = CreateCompatibleDC(hdc);
                                                SelectObject(hdc_men_array[imagenum], hBmp[imagenum]);
                                                ReleaseDC(hWnd, hdc);
                                            }
                                        }
                                        Sleep(1000);

                                        for (int i = MOVIE_START; i < MOVIE_START + Split_Image * Separate_Image; i++) {
                                            hWnd = FindWindow(NULL, TEXT("BmpWindow"));
                                            drawing = i;
                                            SendMessage(hWnd, WM_PAINT, NULL, NULL);
                                            Sleep(40);
                                        }

                                        drawing = MOVIE_WAIT;
                                        SendMessage(hWnd, WM_PAINT, NULL, NULL);

                                        hWnd = FindWindow(NULL, TEXT("Chamonix"));
                                        if (hWnd != 0) {
                                            eq = "RPS2";
                                            con = "9";
                                            move = "10000";
                                            Send_Stage_Message(hWnd, eq, con, move);
                                            while (Send_Stage_Message == 0);
                                        }
                                        else {
                                            MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
                                        }


                                        for (int i = 0; i < Split_Image; i++) {
                                            for (int m = 0; m < Separate_Image; m++) {
                                                hdc = GetDC(hWnd);
                                                wsprintf(bmpname, TEXT("IDB_BITMAP%d"), 3 + m + i * Separate_Image + 2 * Separate_Image * Split_Image);
                                                int imagenum = MOVIE_START + Separate_Image * i + m;
                                                hBmp[imagenum] = LoadBitmap(hInst, bmpname);
                                                hdc_men_array[imagenum] = CreateCompatibleDC(hdc);
                                                SelectObject(hdc_men_array[imagenum], hBmp[imagenum]);
                                                ReleaseDC(hWnd, hdc);
                                            }
                                        }
                                        Sleep(1000);

                                        for (int i = MOVIE_START; i < MOVIE_START + Split_Image * Separate_Image; i++) {
                                            hWnd = FindWindow(NULL, TEXT("BmpWindow"));
                                            drawing = i;
                                            SendMessage(hWnd, WM_PAINT, NULL, NULL);
                                            Sleep(40);
                                        }

                                        drawing = MOVIE_WAIT;
                                        SendMessage(hWnd, WM_PAINT, NULL, NULL);

                                        hWnd = FindWindow(NULL, TEXT("Chamonix"));
                                        if (hWnd != 0) {
                                            eq = "RPS2";
                                            con = "9";
                                            move = "10000";
                                            Send_Stage_Message(hWnd, eq, con, move);
                                            while (Send_Stage_Message == 0);
                                        }
                                        else {
                                            MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
                                        }


                                        for (int i = 0; i < Split_Image; i++) {
                                            for (int m = 0; m < Separate_Image; m++) {
                                                hdc = GetDC(hWnd);
                                                wsprintf(bmpname, TEXT("IDB_BITMAP%d"), 3 + m + i * Separate_Image + 3 * Separate_Image * Split_Image);
                                                int imagenum = MOVIE_START + Separate_Image * i + m;
                                                hBmp[imagenum] = LoadBitmap(hInst, bmpname);
                                                hdc_men_array[imagenum] = CreateCompatibleDC(hdc);
                                                SelectObject(hdc_men_array[imagenum], hBmp[imagenum]);
                                                ReleaseDC(hWnd, hdc);
                                            }
                                        }
                                        Sleep(1000);

                                        for (int i = MOVIE_START; i < MOVIE_START + Split_Image * Separate_Image; i++) {
                                            hWnd = FindWindow(NULL, TEXT("BmpWindow"));
                                            drawing = i;
                                            SendMessage(hWnd, WM_PAINT, NULL, NULL);
                                            Sleep(40);
                                        }

                                        drawing = MOVIE_WAIT;
                                        SendMessage(hWnd, WM_PAINT, NULL, NULL);

                                        hWnd = FindWindow(NULL, TEXT("Chamonix"));
                                        if (hWnd != 0) {
                                            eq = "RPS2";
                                            con = "9";
                                            move = "10000";
                                            Send_Stage_Message(hWnd, eq, con, move);
                                            while (Send_Stage_Message == 0);
                                        }
                                        else {
                                            MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
                                        }

                    */
                }
                hWnd = FindWindow(NULL, TEXT("Chamonix"));
                if (hWnd != 0) {
                    Send_Stage_Message(hWnd, "RPS2", "9", "20000");
                    while (Send_Stage_Message == 0);
                }
                else {
                    MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
                }

                drawing = MOVIE_END;
                SendMessage(hWnd, WM_PAINT, NULL, NULL);
               
                break;

            case ID_RESET:
                for (int i = 0; i < sizeof(Set_moving_stage) / sizeof(unsigned char); i++) {
                    hWnd = FindWindow(NULL, TEXT("BmpWindow"));
                    drawing = MOVIE_WAIT;
                    SendMessage(hWnd, WM_PAINT, NULL, NULL);
                    hWnd = FindWindow(NULL, TEXT("Chamonix"));
                    if (hWnd != 0) {
                        switch (Set_moving_stage[i])
                        {
                        case X_Backward:
                            Send_Stage_Message(hWnd, "RPS1", "9", "5875");
                            while (Send_Stage_Message == 0);
                            Sleep(700);
                            break;
                        case X_Forward:
                            Send_Stage_Message(hWnd, "RPS1", "9","-5875");
                            while (Send_Stage_Message == 0);
                            Sleep(700);
                            break;
                        case Rotate_Backward:
                            Send_Stage_Message(hWnd, "RPS2", "9", "-1000");
                            while (Send_Stage_Message == 0);
                            Sleep(700);
                            break;
                        case Rotate_Forward:
                            Send_Stage_Message(hWnd, "RPS2", "9","1000");
                            while (Send_Stage_Message == 0);
                            Sleep(700);
                            break;
                        default:
                            break;
                        }
                    }
                    else {
                        MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
                    }
                }
            case ID_TEST:
                hWnd = FindWindow(NULL, TEXT("BmpWindow"));
                drawing = 100;
                SendMessage(hWnd, WM_PAINT, NULL, NULL);
                break;

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_CREATE:
        hSend = CreateWindow(TEXT("BUTTON"), TEXT("ALIGNMENT"), WS_CHILD | WS_VISIBLE, 10, 10, 100, 30, hWnd, (HMENU)ID_ALIGNMENT, hInst, NULL);
        hShow = CreateWindow(TEXT("BUTTON"), TEXT("SHOW HPK"), WS_CHILD | WS_VISIBLE, 10, 50, 100, 30, hWnd, (HMENU)ID_SHOW, hInst, NULL);
        hAllmag = CreateWindow(TEXT("BUTTON"), TEXT("ALL MAG"), WS_CHILD | WS_VISIBLE, 10, 90, 100, 30, hWnd, (HMENU)ALL_MAG, hInst, NULL);
        hWrintig = CreateWindow(TEXT("BUTTON"), TEXT("WRITING"), WS_CHILD | WS_VISIBLE, 130, 50, 100, 30, hWnd, (HMENU)WRITING, hInst, NULL);
        hTestwriting = CreateWindow(TEXT("BUTTON"), TEXT("TEST"), WS_CHILD | WS_VISIBLE, 130, 10, 100, 30, hWnd, (HMENU)ID_TEST, hInst, NULL);
        hReset = CreateWindow(TEXT("BUTTON"), TEXT("RESET"), WS_CHILD | WS_VISIBLE, 130, 90, 100, 30, hWnd, (HMENU)ID_RESET, hInst, NULL);
        break;
    case WM_PAINT:
        {
            hdc = BeginPaint(hWnd, &ps);
            // TODO: HDC を使用する描画コードをここに追加してください...
            EndPaint(hWnd, &ps);
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

//二つ目のウィンドウのCallBack
LRESULT CALLBACK WndProc2(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    HBITMAP hBmp[200];
    BITMAP bmp_info;

    TCHAR bmpname[] = TEXT("cubic3_10_10");
    
    switch (message) {
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // 選択されたメニューの解析:
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_CREATE:
        //デバイスコンテキストhdc_menにビットマップリソース画像を読み込む　（注）投入する画像ファイルの名前は,～～1,～～2,…のようにしておいてください
        //for文を用いて一気にすべてのbmpを読み込む

        //START，END，HPK画像の読み込み
        hdc = GetDC(hWnd);
        hBmp[0] = LoadBitmap(hInst, TEXT("START_END"));
        hdc_men_array[0] = CreateCompatibleDC(hdc);
        SelectObject(hdc_men_array[0], hBmp[0]);

        hBmp[1] = LoadBitmap(hInst, TEXT("HPK"));
        hdc_men_array[1] = CreateCompatibleDC(hdc);
        SelectObject(hdc_men_array[1], hBmp[1]);
        ReleaseDC(hWnd, hdc);

        //bitmap画像の取得、w、h変数に画像の横幅、縦幅を入力する （注）投入する画像の縦横幅は統一しておいてください
        GetObject(hBmp[0], (int)sizeof(BITMAP), &bmp_info);
        w = bmp_info.bmWidth;
        h = bmp_info.bmHeight;
        break;
    case WM_PAINT:
        //第二画面にてビットマップを表示させるプログラム
        InvalidateRect(hWnd, NULL, TRUE);//←これ必須
        hdc = BeginPaint(hWnd, &ps);
        hdc_men = hdc_men_array[drawing];
        BitBlt(hdc, (MonitorInfoEx.rcMonitor.right - MonitorInfoEx.rcMonitor.left) / 2 - w / 2, (MonitorInfoEx.rcMonitor.bottom - MonitorInfoEx.rcMonitor.top) / 2 - h / 2, w, h, hdc_men, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        DeleteObject(hBmp);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//ダイアログボックスの中身
BOOL CALLBACK AlignmentDlgProc(HWND hDlog, UINT msg, WPARAM wp, LPARAM lp) {
    HWND hWnd;
    CHAR szBuf[64];
    //CHAR numchar[10];
    size_t mojinum;
    WCHAR numwchar[10];
    BOOL *success=0, bSigned=0;
    int num=0;
    char const* eq = "RPS1", * con = "0", * move = "10000000";
    char numchar[10];
    eq = "RPS2";
    con = "9";
    move = "-1000";
    

    switch (msg)
    {
    case WM_COMMAND:
        if (LOWORD(wp) == ALIGNMENT_CANCEL_BUTTON)
        {
            EndDialog(hDlog, LOWORD(wp));
            return (INT_PTR)TRUE;
        }
        else if(LOWORD(wp) == ALIGNMENT_OK_BUTTON){
            num = GetDlgItemInt(hDlog, ALIGNMENT_EDIT, success, bSigned);
            if (&success) {
                sprintf_s(numchar,10, "%d", num);                
                //mbstowcs_s(&mojinum, numwchar, sizeof(numwchar), numchar, _TRUNCATE);
                //MessageBox(hDlog, numwchar, TEXT("確認"), MB_OK);
                hWnd = FindWindow(NULL, TEXT("Chamonix"));
                if (hWnd != 0) {
                    eq = "RPS1";
                    con = "9";
                    move = "-90000";//numchar;
                    Send_Stage_Message(hWnd, eq, con, move);
                    while (Send_Stage_Message == 0);
                    eq = "RPS1";
                    con = "9";
                    move = "-77350"; //"-77350";//numchar;
                    Send_Stage_Message(hWnd, eq, con, move);
                    while (Send_Stage_Message == 0);
                    //eq = "RPS1";
                    //con = "9";
                    //move = "-500";// "-18500";//"-70500";//numchar;////
                    //Send_Stage_Message(hWnd, eq, con, move);
                    //while (Send_Stage_Message == 0);
                }
                else {
                    MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
                }
            }
            else {
                MessageBox(hDlog, TEXT("なんも入ってないと思う"), TEXT("確認"), MB_OK);
            }
        }
        else if (LOWORD(wp) == ALIGNMENT_INI) {
            hWnd = FindWindow(NULL, TEXT("Chamonix"));
            if (hWnd != 0) {
                eq = "ORG1";
                con = "9";
                Send_Stage_Message(hWnd, eq, con, move);
                while (Send_Stage_Message == 0);
            }
            else {
                MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
            }
        }
        break;
    default:
        break;
    }
    return FALSE;
}

//ステージコントロールと画像表示の為の関数
void Control_Stage_and_image(HWND hWnd,int bitmap_num, int movement) {
    HBITMAP hBmp[200];
    TCHAR bmpname[] = TEXT("cubic3_10_10AA");
    HDC hdc;
    char move[10];

    //bitmapの読み込み
    for (int i = 0; i < Split_Image; i++) {
        for (int m = 0; m < Separate_Image; m++) {
            hdc = GetDC(hWnd);
            wsprintf(bmpname, TEXT("IDB_BITMAP%d"), bitmap_num + m + i * Separate_Image);//TEXT("MOVIE1_%d_%d"), i, m);
            int imagenum = MOVIE_START + Separate_Image * i + m;
            hBmp[imagenum] = LoadBitmap(hInst, bmpname);
            hdc_men_array[imagenum] = CreateCompatibleDC(hdc);
            SelectObject(hdc_men_array[imagenum], hBmp[imagenum]);
            ReleaseDC(hWnd, hdc);
        }
    }
    Sleep(1000);

    //ビットマップの再生
    for (int i = MOVIE_START; i < MOVIE_START + Split_Image * Separate_Image; i++) {
        hWnd = FindWindow(NULL, TEXT("BmpWindow"));
        drawing = i;
        SendMessage(hWnd, WM_PAINT, NULL, NULL);
        Sleep(40);
    }

    drawing = MOVIE_WAIT;
    SendMessage(hWnd, WM_PAINT, NULL, NULL);

    //ステージのコントロール
    hWnd = FindWindow(NULL, TEXT("Chamonix"));
    if (hWnd != 0) {
        snprintf(move, 10, "%d", movement);
        Send_Stage_Message(hWnd, "RPS2", "9", move);
        while (Send_Stage_Message == 0);
    }
    else {
        MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
    }

}

//Chamonixへ送信する関数
BOOL Send_Stage_Message(HWND hSSM,char const *equipment,char const*controll_num,char const *move) {
    COPYDATASTRUCT* SendData = new COPYDATASTRUCT();
    WPARAM ReceveData = 0;
    char Send_Contents[50] = {};
    char Slash[] = "/";

    strcat_s(Send_Contents, equipment);
    strcat_s(Send_Contents, Slash);
    strcat_s(Send_Contents, controll_num);
    strcat_s(Send_Contents, Slash);
    strcat_s(Send_Contents, move);

    SendData->dwData = (intptr_t)0;
    SendData->cbData = (UINT)12;
    SendData->lpData = (PVOID)Send_Contents;

    SendMessage(hSSM, WM_COPYDATA, ReceveData, (LPARAM)SendData);
    return (BOOL)SendMessage;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
