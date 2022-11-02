﻿// LCOS_app.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "LCOS_app.h"
#include <stdio.h>
#include <string.h>

#define MAX_LOADSTRING 100
#define WINDOW_WIDE 400
#define WINDOW_HIGH 200

#define MOVIE_START 2
#define MOVIE_END 0
#define MOVIE_WAIT 0
#define HPK 1

#define Split_Image 12
#define Separate_Image 16
#define Num_Image Split_Image*Separate_Image

#define X_Forward 0b01
#define X_Backward 0b10
#define Rotate_Forward 0b1010
#define Rotate_Backward 0b0101

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

//回転、Xステージへの送信関数
BOOL Send_Stage_Message(HWND hSSM,char const* equipment,char const* controll_num,char const* move);

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

    WCHAR szBuff[1024];
    char const *eq = "RPS1", *con = "0", *move = "10000000";

    int Set_moving_stage[] = {
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

            case ID_SEND:
                //SENDボタンを押したときの動作
                hWnd = FindWindow(NULL, TEXT("Chamonix"));
                if (hWnd != 0) {
                    eq = "RPS1";
                    con = "9";
                    move = "10000";

                    Send_Stage_Message(hWnd, eq, con, move);
                    if (SendMessage != 0) {
                    
                        MessageBox(hWnd, szBuff, TEXT("Chamonixからの返答"), MB_OK);
                    }
                    else {
                        MessageBox(hWnd, TEXT("False Sending message"), TEXT("Chamonixからの返答"), MB_OK);
                    }
                }
                else {
                    MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
                }
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
                    eq = "RPS2";
                    con = "9";
                    move = "900";
                    Send_Stage_Message(hWnd, eq, con, move);
                    while (Send_Stage_Message == 0);
                }
                else {
                    MessageBox(hWnd, TEXT("Chamonixが開かれていません"), TEXT("エラー"), MB_OK);
                }
                break;

            case WRITING:
                //WRITINGボタンを押したときの動作
                for (int i = MOVIE_START; i < MOVIE_START + Num_Image; i++) {
                    hWnd = FindWindow(NULL, TEXT("BmpWindow"));
                    drawing = i;
                    SendMessage(hWnd, WM_PAINT, NULL, NULL);
                    Sleep(50);
                    if (i == 65 || i == 129 || i==161 || i==193) {
                        drawing = MOVIE_WAIT;
                        SendMessage(hWnd, WM_PAINT, NULL, NULL);
                        hWnd = FindWindow(NULL, TEXT("Chamonix"));
                        if (hWnd != 0) {
                            switch (Set_moving_stage[count])
                            {
                            case X_Forward:
                                eq = "RPS2";
                                con = "9";
                                move = "5875";
                                Send_Stage_Message(hWnd, eq, con, move);
                                while (Send_Stage_Message == 0);
                                Sleep(700);
                                break;
                            case X_Backward:
                                eq = "RPS2";
                                con = "9";
                                move = "-5875";
                                Send_Stage_Message(hWnd, eq, con, move);
                                while (Send_Stage_Message == 0);
                                Sleep(700);
                                break;
                            case Rotate_Forward:
                                eq = "RPS1";
                                con = "9";
                                move = "4781";
                                Send_Stage_Message(hWnd, eq, con, move);
                                while (Send_Stage_Message == 0);
                                Sleep(700);
                                break;
                            case Rotate_Backward:
                                eq = "RPS1";
                                con = "9";
                                move = "-4781";
                                Send_Stage_Message(hWnd, eq, con, move);
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
                        count++;
                    }
                }                
                drawing = 0;
                SendMessage(hWnd, WM_PAINT, NULL, NULL);
               
                break;

            case ID_RESET:
                for (int i = 0; i < sizeof(Set_moving_stage); i++) {
                    drawing = MOVIE_WAIT;
                    SendMessage(hWnd, WM_PAINT, NULL, NULL);
                    hWnd = FindWindow(NULL, TEXT("Chamonix"));
                    if (hWnd != 0) {
                        switch (~Set_moving_stage[count])
                        {
                        case X_Forward:
                            eq = "RPS2";
                            con = "9";
                            move = "5875";
                            Send_Stage_Message(hWnd, eq, con, move);
                            while (Send_Stage_Message == 0);
                            Sleep(700);
                            break;
                        case X_Backward:
                            eq = "RPS2";
                            con = "9";
                            move = "-5875";
                            Send_Stage_Message(hWnd, eq, con, move);
                            while (Send_Stage_Message == 0);
                            Sleep(700);
                            break;
                        case Rotate_Forward:
                            eq = "RPS1";
                            con = "9";
                            move = "4781";
                            Send_Stage_Message(hWnd, eq, con, move);
                            while (Send_Stage_Message == 0);
                            Sleep(700);
                            break;
                        case Rotate_Backward:
                            eq = "RPS1";
                            con = "9";
                            move = "-4781";
                            Send_Stage_Message(hWnd, eq, con, move);
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
                    break;
                }

            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_CREATE:
        hSend = CreateWindow(TEXT("BUTTON"), TEXT("SEND"), WS_CHILD | WS_VISIBLE, 10, 10, 100, 30, hWnd, (HMENU)ID_SEND, hInst, NULL);
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

    TCHAR bmpname[] = TEXT("cubic10_10");
    
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

        for (int i = 0; i < Split_Image; i++) {
            for (int m = 0; m < Separate_Image; m++) {
                hdc = GetDC(hWnd);
                wsprintf(bmpname, TEXT("cubic%d_%d"), i, m);
                hBmp[MOVIE_START + Separate_Image * i + m] = LoadBitmap(hInst, bmpname);
                hdc_men_array[MOVIE_START + 16 * i + m] = CreateCompatibleDC(hdc);
                SelectObject(hdc_men_array[MOVIE_START + Separate_Image * i + m], hBmp[MOVIE_START + Separate_Image * i + m]);
                ReleaseDC(hWnd, hdc);
            }
        }

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
