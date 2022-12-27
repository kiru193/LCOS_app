// LCOS_app.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "LCOS_app.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winusb.h>
#include <process.h>
#include <iostream>

#define MAX_LOADSTRING 100
#define WINDOW_WIDE 400
#define WINDOW_HIGH 200

#define MOVIE_START 2
#define MOVIE_END 0
#define MOVIE_WAIT 0
#define HPK 1

#define Split_Image 1
#define Separate_Image 16
#define Num_Image Split_Image*Separate_Image

#define X_Forward 1
#define X_Backward 2
#define Rotate_Forward 3
#define Rotate_Backward 4

#define Shutter_OPEN 1
#define Shutter_CLOSE 0

using namespace std;
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
BOOL CALLBACK SpeedcontrollDlgProc(HWND, UINT, WPARAM, LPARAM);

//回転、Xステージへの送信関数
BOOL Send_Stage_Message(HWND hSSM,char const* equipment,char const* controll_num,char const* move);
BOOL Send_Stage_Message_Serial(HWND hWnd, DCB dcb, HANDLE hPort, const char* equipment, const char* controll_num, const char* move);

//ステージコントロールと画像表示の為の関数
void Control_Stage_and_image(HWND hWnd, HANDLE hPort, int bitmap_num, int movement);
BOOL Control_Stage_and_image(HWND hWnd,int bitmap_num, const char* move);

//シャッターの制御をおこなう関数
BOOL Shutter_Controll(HANDLE hShutter,int status);

//シリアル通信の変数について
HANDLE hShutter, hStage;
DCB dcbShutter, dcbStage;

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
    static HWND hSend, hShow, hWrintig, hSpeedcon, hReset;
    static HWND hShow2;

    HBITMAP hBmp[200];
    TCHAR bmpname[] = TEXT("cubic3_10_10AA");

    WCHAR szBuff[1024];
    char const *eq = "RPS1", *con = "0", *move = "10000000";

    const char* str = "OPEN:1\r\n";
    DWORD dwSendSize = 10;
    COMSTAT Comstat;

    bool start = TRUE;

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
        //シャッターのシリアル通信設定
        hShutter = CreateFile(L"COM8", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hShutter == INVALID_HANDLE_VALUE) {
            MessageBox(hWnd, TEXT("シャッターのシリアル接続エラーです。"), TEXT("エラー"), MB_OK);
            CloseHandle(hShutter);
        }

        dcbShutter.BaudRate = 9600; // 速度
        dcbShutter.ByteSize = 8; // データ長
        dcbShutter.Parity = NOPARITY; // パリティ
        dcbShutter.StopBits = ONESTOPBIT; // ストップビット長
        dcbShutter.fOutxCtsFlow = FALSE; // 送信時CTSフロー
        dcbShutter.fRtsControl = RTS_CONTROL_ENABLE; // RTSフロー
        dcbShutter.EofChar = 0x03;
        dcbShutter.EvtChar = 0x02;

        SetCommState(hShutter, &dcbShutter);


        //ステージのシリアル通信設定
        hStage = CreateFile(L"COM5", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hStage == INVALID_HANDLE_VALUE) {
            MessageBox(hWnd, TEXT("ステージのシリアル接続エラーです。"), TEXT("エラー"), MB_OK);
            CloseHandle(hStage);
        }

        dcbStage.BaudRate = 9600; // 速度
        dcbStage.ByteSize = 8; // データ長
        dcbStage.Parity = NOPARITY; // パリティ
        dcbStage.StopBits = ONESTOPBIT; // ストップビット長
        dcbStage.fOutxCtsFlow = FALSE; // 送信時CTSフロー
        dcbStage.fRtsControl = RTS_CONTROL_ENABLE; // RTSフロー
        dcbStage.EofChar = 0x03;
        dcbStage.EvtChar = 0x02;

        SetCommState(hStage, &dcbStage);

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
            //Dialogでの処理
            DialogBox(hInst, MAKEINTRESOURCE(ALIGNMENT_DAILOG), hWnd, (DLGPROC)AlignmentDlgProc);
            
            CloseHandle(hShutter);
            CloseHandle(hStage);
            break;

        case ID_SHOW:
            //SHOWボタンを押したときの動作
            hWnd = FindWindow(NULL, TEXT("BmpWindow"));
            drawing = HPK;
            SendMessage(hWnd, WM_PAINT, NULL, NULL);
            break;
        case ID_OPEN:
            //OPENボタンを押したときの動作
            if (Control_Stage_and_image(hWnd, 3, "10000") == FALSE) {
                MessageBox(hWnd, TEXT("コントロールステージエラーです。"), TEXT("エラー"), MB_OK);
            }
            Sleep(500);
            if (Send_Stage_Message_Serial(hWnd, dcbStage, hStage, "RPS2", "9", "80000") == FALSE) {
                MessageBox(hWnd, TEXT("送信に失敗しました。"), TEXT("エラー"), MB_OK);
            }
            CloseHandle(hShutter);
            CloseHandle(hStage);
            break;
        case WRITING:
            //WRITINGボタンを押したときの動作
            
            if (Send_Stage_Message_Serial(hWnd, dcbStage, hStage, "RPS2", "9", "0") == FALSE) {//180000で一回転
                MessageBox(hWnd, TEXT("送信に失敗しました。"), TEXT("エラー"), MB_OK);
            }
            for (int a = 0; a < 4; a++) {
                if (Control_Stage_and_image(hWnd, 3, "10000") == FALSE) {
                    MessageBox(hWnd, TEXT("コントロールステージエラーです。"), TEXT("エラー"), MB_OK);
                }
                if (Control_Stage_and_image(hWnd, 3 + Split_Image * Separate_Image, "10000") == FALSE) {
                    MessageBox(hWnd, TEXT("コントロールステージエラーです。"), TEXT("エラー"), MB_OK);
                }
                if (Control_Stage_and_image(hWnd, 3 + Split_Image * Separate_Image * 2, "10000") == FALSE) {
                    MessageBox(hWnd, TEXT("コントロールステージエラーです。"), TEXT("エラー"), MB_OK);
                }
                if (Control_Stage_and_image(hWnd, 3 + Split_Image * Separate_Image * 3, "10000") == FALSE) {
                    MessageBox(hWnd, TEXT("コントロールステージエラーです。"), TEXT("エラー"), MB_OK);
                }
            }            
            Sleep(500);
            if (Send_Stage_Message_Serial(hWnd, dcbStage, hStage, "RPS2", "9", "20000") == FALSE) {
                MessageBox(hWnd, TEXT("送信に失敗しました。"), TEXT("エラー"), MB_OK);
            }

            drawing = MOVIE_END;
            SendMessage(hWnd, WM_PAINT, NULL, NULL);
            
            CloseHandle(hShutter);
            CloseHandle(hStage);
            break;

        case ID_RESET:
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
                        Send_Stage_Message(hWnd, "RPS1", "9", "-5875");
                        while (Send_Stage_Message == 0);
                        Sleep(700);
                        break;
                    case Rotate_Backward:
                        Send_Stage_Message(hWnd, "RPS2", "9", "-1000");
                        while (Send_Stage_Message == 0);
                        Sleep(700);
                        break;
                    case Rotate_Forward:
                        Send_Stage_Message(hWnd, "RPS2", "9", "1000");
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
        case ID_SPEED_CON:
            //Dialogでの処理
            DialogBox(hInst, MAKEINTRESOURCE(SPEEDCON_DIALOG), hWnd, (DLGPROC)SpeedcontrollDlgProc);

            CloseHandle(hShutter);
            CloseHandle(hStage);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
    
        break;
    case WM_CREATE:
        hSend = CreateWindow(TEXT("BUTTON"), TEXT("ALIGNMENT"), WS_CHILD | WS_VISIBLE, 10, 10, 100, 30, hWnd, (HMENU)ID_ALIGNMENT, hInst, NULL);
        hShow = CreateWindow(TEXT("BUTTON"), TEXT("SHOW HPK"), WS_CHILD | WS_VISIBLE, 10, 50, 100, 30, hWnd, (HMENU)ID_SHOW, hInst, NULL);
        hShutter = CreateWindow(TEXT("BUTTON"), TEXT("OPEN"), WS_CHILD | WS_VISIBLE, 10, 90, 100, 30, hWnd, (HMENU)ID_OPEN, hInst, NULL);
        hWrintig = CreateWindow(TEXT("BUTTON"), TEXT("WRITING"), WS_CHILD | WS_VISIBLE, 130, 50, 100, 30, hWnd, (HMENU)WRITING, hInst, NULL);
        hSpeedcon = CreateWindow(TEXT("BUTTON"), TEXT("SPEED CON"), WS_CHILD | WS_VISIBLE, 130, 10, 100, 30, hWnd, (HMENU)ID_SPEED_CON, hInst, NULL);
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
        hBmp[0] = LoadBitmap(hInst, TEXT("CHECKER2"));
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

//アライメントダイアログボックスの中身
BOOL CALLBACK AlignmentDlgProc(HWND hDlog, UINT msg, WPARAM wp, LPARAM lp) {
    BOOL *success=0, bSigned=0;
    int num = 0;
    char numchar[] = "100000";
    static HWND hParent;

    switch (msg)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlog, ALIGNMENT_EDIT, -16400, TRUE);
        break;
    case WM_COMMAND:
        if (LOWORD(wp) == ALIGNMENT_CANCEL_BUTTON)
        {
            EndDialog(hDlog, LOWORD(wp));
            return (INT_PTR)TRUE;
        }
        else if(LOWORD(wp) == ALIGNMENT_OK_BUTTON){
            num = GetDlgItemInt(hDlog, ALIGNMENT_EDIT, success, TRUE);
            sprintf_s(numchar, strlen(numchar) + 1, "%d", num);
            if (Send_Stage_Message_Serial(hDlog, dcbStage, hStage, "RPS1", "9", numchar /*"16400"*/) == FALSE) {
                MessageBox(hDlog, TEXT("シリアル送信エラーです"), TEXT("エラー"), MB_OK);
            }
        }
        else if (LOWORD(wp) == ALIGNMENT_INI) {
            if (Send_Stage_Message_Serial(hDlog, dcbStage, hStage, "ORG1", "9", "0") == FALSE) {
                MessageBox(hDlog, TEXT("シリアル送信エラーです"), TEXT("エラー"), MB_OK);
            }
        }
        break;
    case WM_CLOSE:
        EndDialog(hDlog, LOWORD(wp));
        return (INT_PTR)TRUE;
    default:
        break;
    }
    return FALSE;
}

//スピードコントロールダイアログボックス
BOOL CALLBACK SpeedcontrollDlgProc(HWND hDlog, UINT msg, WPARAM wp, LPARAM lp) {
    BOOL* success = 0;
    unsigned long nn;
    static HWND hParent;
    bool flag = FALSE;

    switch (msg)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlog, SPEEDCON_NUM_EDIT, 2, FALSE);
        SetDlgItemInt(hDlog, SPEEDCON_TABLE_EDIT, 9, FALSE);
        break;
    case WM_COMMAND:
        if (LOWORD(wp) == SPEEDCON_CANCEL_BUTTON)
        {
            EndDialog(hDlog, LOWORD(wp));
            return (INT_PTR)TRUE;
        }
        else if (LOWORD(wp) == SPEEDCON_READING_BUTTON) {
            int j = 0;
            char equipment[5] = {};
            char numchar[2], tablechar[2], data[50],receive[1];
            char* end1 = 0, * end2 = 0, * end3 = 0, * end4 = 0, * end5 = 0;
            strcat_s(equipment, "RTB");
            sprintf_s(numchar, 2, "%d", GetDlgItemInt(hDlog, SPEEDCON_NUM_EDIT, success, FALSE));
            strcat_s(equipment, numchar);
            sprintf_s(tablechar, 2, "%d", GetDlgItemInt(hDlog, SPEEDCON_TABLE_EDIT, success, FALSE));
            Send_Stage_Message_Serial(hDlog, dcbStage, hStage, equipment, tablechar, FALSE);
            while (1) {
                ReadFile(hStage, receive, 1, &nn, 0);
                if (*receive == 'C') {
                    while (!(*receive >= '0' && *receive <= '9')) {
                        ReadFile(hStage, receive, 1, &nn, 0);
                        if (*receive == '\n') {
                            break;
                        }
                    }
                    while (*receive != '\n') {
                        data[j] = *receive;
                        j++;
                        ReadFile(hStage, receive, 1, &nn, 0);
                    }
                }
                else if (*receive == 'E') {
                    MessageBox(hDlog, TEXT("取得エラーです"), TEXT("エラー"), MB_OK);
                    SetDlgItemInt(hDlog, SPEEDCON_NUM_EDIT, 2, FALSE);
                    SetDlgItemInt(hDlog, SPEEDCON_TABLE_EDIT, 9, FALSE);
                    flag = TRUE;
                    break;
                }else if (*receive == '\n') {
                    break;
                }
            }
            if (flag == TRUE) {
                flag = FALSE;
                break;
            }
            //ダイアログボックス内のエディターボックスに数値を入力する、strtolはdata内の数値のみを取り出し、何かしらの区切り記号を用いている場合以下のようにして各ボックスに入力できる。
            SetDlgItemInt(hDlog, SPEEDCON_NUM_EDIT, strtol(data, &end1, 10), FALSE);
            SetDlgItemInt(hDlog, SPEEDCON_TABLE_EDIT, strtol(end1, &end2, 10), FALSE);
            SetDlgItemInt(hDlog, SPEEDCON_START_EDIT, strtol(end2,&end3,10), FALSE);
            SetDlgItemInt(hDlog, SPEEDCON_MAX_EDIT, strtol(end3, &end4, 10), FALSE);
            SetDlgItemInt(hDlog, SPEEDCON_ACCELLTIME_EDIT, strtol(end4, &end5, 10), FALSE);
            SetDlgItemInt(hDlog, SPEEDCON_ACCELLMODE_EDIT, strtol(end5, &end1, 10), FALSE);
            break;
        }
        else if (LOWORD(wp) == SPEEDCON_WRITING_BUTTON) {
            char num[5] = {};
            char move[50] = {};
            char numchar[2], tablechar[2],startchar[7],maxchar[7],accelltimechar[3],accellmodechar[2], receive[1];
            strcat_s(num, "WTB");
            sprintf_s(numchar, 2, "%d", GetDlgItemInt(hDlog, SPEEDCON_NUM_EDIT, success, FALSE));
            strcat_s(num, numchar);
            sprintf_s(tablechar, 2, "%d", GetDlgItemInt(hDlog, SPEEDCON_TABLE_EDIT, success, FALSE));
            sprintf_s(startchar, 7, "%d", GetDlgItemInt(hDlog, SPEEDCON_START_EDIT, success, FALSE));
            strcat_s(move, startchar);
            strcat_s(move, "/");
            sprintf_s(maxchar, 7, "%d", GetDlgItemInt(hDlog, SPEEDCON_MAX_EDIT, success, FALSE));
            strcat_s(move, maxchar);
            strcat_s(move, "/");
            sprintf_s(accelltimechar, 3, "%d", GetDlgItemInt(hDlog, SPEEDCON_ACCELLTIME_EDIT, success, FALSE));
            strcat_s(move, accelltimechar);
            strcat_s(move, "/");
            sprintf_s(accellmodechar, 2, "%d", GetDlgItemInt(hDlog, SPEEDCON_ACCELLMODE_EDIT, success, FALSE));
            strcat_s(move, accellmodechar);
            Send_Stage_Message_Serial(hDlog, dcbStage, hStage, num, tablechar, move);
            
            while (1) {
                ReadFile(hStage, receive, 1, &nn, 0);
                if (*receive == 'C') {
                    MessageBox(hDlog, TEXT("書き込みに成功しました"), TEXT("確認"), MB_OK);
                    break;
                }
                else if (*receive == 'E') {
                    MessageBox(hDlog, TEXT("書き込みに失敗しました"), TEXT("エラー"), MB_OK);
                    break;
                }
            }
        }
        break;
    case WM_CLOSE:
        EndDialog(hDlog, LOWORD(wp));
        return (INT_PTR)TRUE;
    default:
        break;
    }
    return FALSE;
}

//ステージコントロールと画像表示の為の関数
void Control_Stage_and_image(HWND hWnd, HANDLE hPort,int bitmap_num, int movement) {
    HBITMAP hBmp[200];
    TCHAR bmpname[] = TEXT("cubic3_10_10AA");
    HDC hdc;
    char move[10];
    BOOL Ret;
    LPCSTR str = "OPEN:1\r\n";
    DWORD dwSendSize = 10;

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

    //シャッターのクローズ
    str = "\r\nCLOSE:1\r\n";
    Ret = WriteFile(hPort, str, strlen(str)+1, &dwSendSize, NULL);

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
    
    //シャッターオープン
    Sleep(2000);
    str = "\r\nOPEN:1\r\n";
    Ret = WriteFile(hPort, str, strlen(str)+1, &dwSendSize, NULL);

}

BOOL Control_Stage_and_image(HWND hWnd, int bitmap_num, const char* move) {
    HBITMAP hBmp[200];
    TCHAR bmpname[] = TEXT("cubic3_10_10AA");
    HDC hdc;
    LPCSTR str = "OPEN:1\r\n";
    DWORD dwSendSize = 10;
    bool flag = TRUE;
    unsigned long nn;
    char receive[1];
    char data[100];

    //bitmapの読み込み
    for (int i = 0; i < Split_Image; i++) {
        for (int m = 0; m < Separate_Image; m++) {
            hdc = GetDC(hWnd);
            wsprintf(bmpname, TEXT("AIDB_BITMAP%d"), bitmap_num + m + i * Separate_Image);//TEXT("MOVIE1_%d_%d"), i, m);
            int imagenum = MOVIE_START + Separate_Image * i + m;
            hBmp[imagenum] = LoadBitmap(hInst, bmpname);
            hdc_men_array[imagenum] = CreateCompatibleDC(hdc);
            SelectObject(hdc_men_array[imagenum], hBmp[imagenum]);
            ReleaseDC(hWnd, hdc);
        }
    }
    //Sleep(1000);

    //ビットマップの再生
    for (int i = MOVIE_START; i < MOVIE_START + Split_Image * Separate_Image; i++) {
        hWnd = FindWindow(NULL, TEXT("BmpWindow"));
        drawing = i;
        SendMessage(hWnd, WM_PAINT, NULL, NULL);
        Sleep(40);
    }

    drawing = MOVIE_WAIT;
    SendMessage(hWnd, WM_PAINT, NULL, NULL);
    
    //シャッターのクローズ
    Shutter_Controll(hShutter, Shutter_CLOSE);

    //ステージの移動
    if (Send_Stage_Message_Serial(hWnd, dcbStage, hStage, "RPS2", "9", move) == FALSE) {
        MessageBox(hWnd, TEXT("ステージ移動ができません。"), TEXT("エラー"), MB_OK);
        flag = FALSE;
    }
    
//    Sleep(200);
    //シャッターオープン
    Shutter_Controll(hShutter, Shutter_OPEN);
    flag = FALSE;
    flag = TRUE;

    return flag;
}
//シャッターのコントロールに関する関数
BOOL Shutter_Controll(HANDLE hShutter, int status) {
    char receive[1];
    char data[50];
    DWORD dwSendSize = 10;
    bool flag = TRUE;
    unsigned long nn;
    int i = 0;
    char str[50] = {};
    bool openflag = FALSE, closeflag = FALSE;

    switch (status)
    {
    case Shutter_OPEN:
        openflag = FALSE;
        i = 0;
        while (i < 3) {
            WriteFile(hShutter, "\r\nOPEN:1\r\n", 12, &dwSendSize, NULL);
            Sleep(1);
            i++;
/*
            while (1) {
                ReadFile(hShutter, receive, 1, &nn, 0);
                data[i] = *receive;
                i++;
                //CRLFの受け取り確認はこれだと思う
                if (*receive == '\r') {
                    ReadFile(hShutter, receive, 1, &nn, 0);
                    if (*receive == '\n') {
                        break;
                    }
                }
                else if (*receive == 'S') {
                    while (1) {
                        ReadFile(hShutter, receive, 1, &nn, 0);
                        if (*receive == 'O') {
                            openflag = TRUE;
                            break;
                        }
                        else if (*receive == 'C') {
                            break;
                        }
                    }
                }
                if (openflag == TRUE) {
                    break;
                }
            }
            if (openflag == TRUE) {
                break;
            }
            WriteFile(hShutter, "\r\nOPEN:1\r\n", 12, &dwSendSize, NULL);
                    
*/
        }
        break;
    case Shutter_CLOSE:
        closeflag = FALSE;
        i = 0;
        while (i < 3) {
            WriteFile(hShutter, "\r\nCLOSE:1\r\n", 13, &dwSendSize, NULL);
            Sleep(1);
            i++;
/*
            while (1) {
                ReadFile(hShutter, receive, 1, &nn, 0);
                data[i] = *receive;
                i++;
                //CRLFの受け取り確認はこれだと思う
                if (*receive == '\r') {
                    ReadFile(hShutter, receive, 1, &nn, 0);
                    if (*receive == '\n') {
                        break;
                    }
                }
                else if (*receive == 'S') {
                    while (1) {
                        ReadFile(hShutter, receive, 1, &nn, 0);
                        if (*receive == 'C') {
                            closeflag = TRUE;
                            break;
                        }
                        else if (*receive == 'O') {
                            break;
                        }
                    }
                }
                if (closeflag == TRUE) {
                    break;
                }
            }
            if (closeflag == TRUE) {
                break;
            }
            WriteFile(hShutter, "\r\nCLOSE:1\r\n", 13, &dwSendSize, NULL);
           */ 
        }
        break;    
    default:
        flag = FALSE;
        break;
    }
    printf("%s", data);

    return flag;
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

//シリアル通信で直接CRUXへ送信する関数
BOOL Send_Stage_Message_Serial(HWND hWnd, DCB dcb, HANDLE hPort, const char* equipment, const char* controll_num, const char* move) {
    char str[50] = {};
    char crlf[4] = {};
    char receive[1];
    char data[50];
    DWORD dwSendSize = 10;
    bool flag=TRUE;
    unsigned long nn;

    if (equipment[0] == 'R' && equipment[1] == 'P' && equipment[2] == 'S') {
        strcat_s(str, "\r\n\x02");
        strcat_s(str, equipment);
        strcat_s(str, "/");
        strcat_s(str, controll_num);
        strcat_s(str, "/");
        strcat_s(str, move);
        strcat_s(str, "/");
        strcat_s(str, "0");
        strcat_s(str, "\r\n");
    }
    else if (equipment[0] == 'O' && equipment[1] == 'R' && equipment[2] == 'G') {
        strcat_s(str, "\r\n\x02");
        strcat_s(str, equipment);
        strcat_s(str, "/");
        strcat_s(str, controll_num);
        strcat_s(str, "/");
        strcat_s(str, "0");
        strcat_s(str, "\r\n");
    }
    else if (equipment[0] == 'R' && equipment[1] == 'T' && equipment[2] == 'B') {
        strcat_s(str, "\r\n\x02");
        strcat_s(str, equipment);
        strcat_s(str, "/");
        strcat_s(str, controll_num);
        strcat_s(str, "\r\n");
    }
    else if (equipment[0] == 'W' && equipment[1] == 'T' && equipment[2] == 'B') {
        strcat_s(str, "\r\n\x02");
        strcat_s(str, equipment);
        strcat_s(str, "/");
        strcat_s(str, controll_num);
        strcat_s(str, "/");
        strcat_s(str, move);
        strcat_s(str, "\r\n");
    }

    if (WriteFile(hStage, str, strlen(str) + 1, &dwSendSize, NULL) == FALSE) {
        MessageBox(hWnd, TEXT("SENDに失敗しました。"), TEXT("エラー"), MB_OK);
        CloseHandle(hStage);
        flag = FALSE;
    }    
    
    int i = 0;
    while (*receive != '\n') {
        ReadFile(hStage, receive, 1, &nn, 0);
        //data[i] = *receive;
        //i++;
    }
    *receive = 0;
    nn = 0;
    while (ReadFile(hStage, receive, 1, &nn, 0) == FALSE);
    while (*receive != '\n') {
        ReadFile(hStage, receive, 1, &nn, 0);
    }
    
    //ReadFile(hStage, receive, 1, &nn, 0);    
    //ReadFile(hStage, receive, 1, &nn, 0);

    return flag;
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
