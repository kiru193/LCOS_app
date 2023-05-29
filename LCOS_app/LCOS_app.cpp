/*
少し昔のものであるので，今のものと少し違う可能性があります，
ただ，自作関数とかはある程度同じであるためコメントをある程度書きます．
このプログラミングコードはWin32apiで検索してもらうとある程度情報が出てきます．
できればリソースファイル(.rc)とRecource.hファイル辺りの理解も必要なのでできれば以下の参考書籍を読んでほしい．
参考書籍は以下のものです（大学の図書館にもあります）
https://item.rakuten.co.jp/bookoffonline/0016869437/
猫でもわかるＷｉｎｄｏｗｓプログラミング　第４版／粂井康孝【著】 【中古】afb
ある程度上記の参考書を読めば，後はネットからの情報でこのプログラムで大切な部分は見えてくるかと…
やっていることを以下に列挙します．
・基本的なwindowsアプリケーションの作製．（ボタンを配置する方法や，上部バーへの要素の追加，ダイアログボックスの生成と作り方など）
・画像を連続してアプリケーションウィンドへ表示
・シリアル通信
また，必要のない関数や変数を削除し，ある程度効率化しています．
シャッターとステージの情報は論文に書いてあるのでユーザーマニュアルを必ず読んでください．ネットでダウンロードできます．
2023/05/29 sena yamagishi

このプログラムの問題点
かなり動作が不安定，主にシリアル通信回りの問題を抱えている．
具体的には，シリアル通信が想定通りの処理をしない，自分の想定している信号を出さない，など．
推測としては，ゆっくり一つ一つデバッグすると正しく動作するので，受け取り側と送り側の同期のようなものが取れていないと考えている
（そのため，無駄にsleep関数や繰り返し文などで時間稼ぎをしているところがたまにある．）
シャッターの動作が不安定，これはハード面なのかソフト面の問題の二つある．
ハード面としては，たまに電磁石が磁気を持ってしまい動かなくなるので手を用いて強引に開けたり閉じたりするとよい．
ソフト面としては，先のシリアル通信の想定通りの通信が問題だと思われる．なぜかシリアル通信のデータが貯まってしまい，一つ前のデータが送られるとかありがち
*/

#include "framework.h"
#include "LCOS_app.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winusb.h>
#include <process.h>
#include <iostream>

//window表示に必要な定義
#define MAX_LOADSTRING 100
#define WINDOW_WIDE 400
#define WINDOW_HIGH 200

//movieの為に必要な定義
#define MOVIE_START 2
#define MOVIE_END 0
#define MOVIE_WAIT 0
#define HPK 1

//等分数と分割数
#define Split_Image 2
#define Separate_Image 16
#define Num_Image Split_Image*Separate_Image

//シャッターを動かすための定義
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
POINT pt = { 1920, 0 };

//BITMAPに必要な変数
HDC hdc_men = 0;
HDC hdc_men_array[50];
int w = 0, h = 0;
int drawing = 0;

//ダイアログに関して
BOOL CALLBACK AlignmentDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK SpeedcontrollDlgProc(HWND, UINT, WPARAM, LPARAM);

//回転、Xステージへの送信関数
BOOL Send_Stage_Message_Serial(HWND hWnd, const char* equipment, const char* controll_num, const char* move);
BOOL Receive_Stage_Message_Serial(HWND hWnd);

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

//一つ目のウィンドに関しての情報
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

//二つ目のウィンドについての情報
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
        //弄るのはこのswitch文の中身，caseは各ボタンを押したとき，または何某らの動作が行われたときの処理である．
        // 選択されたメニューの解析:
        switch (wmId)
        {
        case IDM_ABOUT:
            //ヘルプ→バージョン確認ボタンを押したときの動作
            //ダイアログボックスの表示
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;

        case IDM_EXIT:
            //ファイル→終了ボタンを押したときの動作
            DestroyWindow(hWnd);
            break;

        case ID_ALIGNMENT:
            //ALIGNMENTボタンを押したときの動作
            //ダイアログを表示し数値を代入できるようにする
            DialogBox(hInst, MAKEINTRESOURCE(ALIGNMENT_DAILOG), hWnd, (DLGPROC)AlignmentDlgProc);
            
            CloseHandle(hShutter);
            CloseHandle(hStage);
            break;

        case ID_SHOW:
            //SHOWボタンを押したときの動作
            //HPKを表示できるCGHを表示
            hWnd = FindWindow(NULL, TEXT("BmpWindow"));
            drawing = HPK;//どの画像を表示するのか
            SendMessage(hWnd, WM_PAINT, NULL, NULL);//SendMessage関数は第二画面に対して指令をだす（この場合第二画面を更新するように命令する）
            break;
        case ID_OPEN:
            //OPENボタンを押したときの動作
            //シャッターを開けて，5秒後に閉じる動作
            Shutter_Controll(hShutter, Shutter_OPEN);
            Sleep(5000);
            Shutter_Controll(hShutter, Shutter_CLOSE);
            CloseHandle(hShutter);
            CloseHandle(hStage);
            break;
        case WRITING:
            //WRITINGボタンを押したときの動作
            //初期位置の設定（この場合"0"つまり動かさない）
            if (Send_Stage_Message_Serial(hWnd, "RPS2", "9", "0") == FALSE) {
                MessageBox(hWnd, TEXT("送信に失敗しました。"), TEXT("エラー"), MB_OK);
            }
            //ステージと画像の再生制御
            for (int a = 0; a < 4; a++) {
                //Control_stage_and_imageの3は固定値，一つ目のif文は3番目の画像からスタートする，という意味である．
                //"10000"は移動量，10000pls分の回転を行う（180000が半回転）
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
            if (Send_Stage_Message_Serial(hWnd, "RPS2", "9", "20000") == FALSE) {
                MessageBox(hWnd, TEXT("送信に失敗しました。"), TEXT("エラー"), MB_OK);
            }
            while (Receive_Stage_Message_Serial(hWnd) == FALSE);

            drawing = MOVIE_END;
            SendMessage(hWnd, WM_PAINT, NULL, NULL);
            
            CloseHandle(hShutter);
            CloseHandle(hStage);
            break;

        case ID_SPEED_CON:
            //SPEED CONボタンを押したときの動作
            //ダイアログを用いた処理，出てきたboxに適切なパラメータを投入
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
        //ボタン自体の生成
        hSend = CreateWindow(TEXT("BUTTON"), TEXT("ALIGNMENT"), WS_CHILD | WS_VISIBLE, 10, 10, 100, 30, hWnd, (HMENU)ID_ALIGNMENT, hInst, NULL);
        hShow = CreateWindow(TEXT("BUTTON"), TEXT("SHOW HPK"), WS_CHILD | WS_VISIBLE, 10, 50, 100, 30, hWnd, (HMENU)ID_SHOW, hInst, NULL);
        hShutter = CreateWindow(TEXT("BUTTON"), TEXT("OPEN"), WS_CHILD | WS_VISIBLE, 10, 90, 100, 30, hWnd, (HMENU)ID_OPEN, hInst, NULL);
        hWrintig = CreateWindow(TEXT("BUTTON"), TEXT("WRITING"), WS_CHILD | WS_VISIBLE, 130, 50, 100, 30, hWnd, (HMENU)WRITING, hInst, NULL);
        hSpeedcon = CreateWindow(TEXT("BUTTON"), TEXT("SPEED CON"), WS_CHILD | WS_VISIBLE, 130, 10, 100, 30, hWnd, (HMENU)ID_SPEED_CON, hInst, NULL);
        hReset = CreateWindow(TEXT("BUTTON"), TEXT("RESET"), WS_CHILD | WS_VISIBLE, 130, 90, 100, 30, hWnd, (HMENU)ID_RESET, hInst, NULL);
        break;
    case WM_PAINT:
        {
            //特にやっていることはない
            hdc = BeginPaint(hWnd, &ps);
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
//二つ目のウィンドウとは，CGHが実際に表示されるところのこと
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
        hBmp[0] = LoadBitmap(hInst, TEXT("WHITE"));//hBmpの0番にCGHとして全照射する"白"を格納
        hdc_men_array[0] = CreateCompatibleDC(hdc);
        SelectObject(hdc_men_array[0], hBmp[0]);

        hBmp[1] = LoadBitmap(hInst, TEXT("HPK"));//hBmpの1番にCGHとして何も照射しない"黒"を格納
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
        InvalidateRect(hWnd, NULL, TRUE);//←これ必須，透明な四角形を表示することで何某らのアクションをPC側に認識させ，画面を更新させる
        hdc = BeginPaint(hWnd, &ps);
        hdc_men = hdc_men_array[drawing];
        BitBlt(hdc, (MonitorInfoEx.rcMonitor.right - MonitorInfoEx.rcMonitor.left) / 2 - w / 2, (MonitorInfoEx.rcMonitor.bottom - MonitorInfoEx.rcMonitor.top) / 2 - h / 2, w, h, hdc_men, 0, 0, SRCCOPY);
        //↑実際に表示する画像
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
        SetDlgItemInt(hDlog, ALIGNMENT_EDIT, -16400, TRUE);//-16400は初期値，この辺の値であれば書き込みができるはず…
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
            if (Send_Stage_Message_Serial(hDlog, "RPS1", "9", numchar /*"16400"*/) == FALSE) {
                MessageBox(hDlog, TEXT("シリアル送信エラーです"), TEXT("エラー"), MB_OK);
            }
        }
        else if (LOWORD(wp) == ALIGNMENT_INI) {
            if (Send_Stage_Message_Serial(hDlog, "ORG1", "9", "0") == FALSE) {
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
//ステージのスピードや，加速度，その他のパラメータを書き込むプログラム，詳しくはKOHZUのCRUXユーザーマニュアルを参照
//これ未完成です．最初に述べたシリアル通信の動作問題で適切に書き込むことが難しいです…．（完成させてくれてもええんやで）
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
            //データ送信部
            char equipment[5] = {};
            char numchar[2], tablechar[2], data[50],receive[1];
            char* end1 = 0, * end2 = 0, * end3 = 0, * end4 = 0, * end5 = 0;
            strcat_s(equipment, "RTB");
            sprintf_s(numchar, 2, "%d", GetDlgItemInt(hDlog, SPEEDCON_NUM_EDIT, success, FALSE));
            strcat_s(equipment, numchar);
            sprintf_s(tablechar, 2, "%d", GetDlgItemInt(hDlog, SPEEDCON_TABLE_EDIT, success, FALSE));
            Send_Stage_Message_Serial(hDlog, equipment, tablechar, FALSE);
            //データ受信部
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
            //データ送信部
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
            Send_Stage_Message_Serial(hDlog,  num, tablechar, move);
            
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

//ステージのコントロールと画像の制御を行うプログラム（ここも大事）
BOOL Control_Stage_and_image(HWND hWnd, int bitmap_num, const char* move) {
    HBITMAP hBmp[200];
    TCHAR bmpname[] = TEXT("cubic3_10_10AA");
    HDC hdc;
    bool flag = TRUE;

    //シャッターのクローズ
    Shutter_Controll(hShutter, Shutter_CLOSE);

    //ステージの移動
    if (Send_Stage_Message_Serial(hWnd, "RPS2", "9", move) == FALSE) {
        MessageBox(hWnd, TEXT("ステージ移動ができません。"), TEXT("エラー"), MB_OK);
        flag = FALSE;
    }

    //bitmapの読み込み
    //bitmapを指定された配列の番号から投入していく，Split_image，Separate_imageはこのプログラムの一番最初に定義されている．
    for (int i = 0; i < Split_Image; i++) {
        for (int m = 0; m < Separate_Image; m++) {
            hdc = GetDC(hWnd);
            wsprintf(bmpname, TEXT("BIDB_BITMAP%d"), bitmap_num + m + i * Separate_Image);//TEXT("MOVIE1_%d_%d"), i, m);
            //↑bitmapの名前が数字ごとになっているため，これを名付けている．
            int imagenum = MOVIE_START + Separate_Image * i + m;
            hBmp[imagenum] = LoadBitmap(hInst, bmpname);
            hdc_men_array[imagenum] = CreateCompatibleDC(hdc);//hdc_men_arrayはグローバル変数であるので，二つ目のウィンドウのWM_PAINT内にいる物と同じ
            SelectObject(hdc_men_array[imagenum], hBmp[imagenum]);
            ReleaseDC(hWnd, hdc);
        }
    }

    while (Receive_Stage_Message_Serial(hWnd) == FALSE);

    //シャッターオープン
    Shutter_Controll(hShutter, Shutter_OPEN);
    Sleep(100);

    //ビットマップの再生
    for (int i = MOVIE_START; i < MOVIE_START + Split_Image * Separate_Image; i++) {
        hWnd = FindWindow(NULL, TEXT("BmpWindow"));
        drawing = i;//drawingはグローバル変数であるので，二つ目のウィンドウへi番目の画像を送れと命令している．
        SendMessage(hWnd, WM_PAINT, NULL, NULL);//二つ目のウィンドウのWM_PAINTを実行せよという命令
        Sleep(20);//ここで画像一枚当たりの再生速度を決定している．20ms待って次の画像を表示するため，50Hzの画像再生が行われる．
    }

    drawing = MOVIE_WAIT;
    SendMessage(hWnd, WM_PAINT, NULL, NULL);
    Sleep(100);

    return flag;
}
//シャッターのコントロールに関する関数
//ソフト的にもハード的にもかなり不安定部分
//実際に閉じているか開いているかを聞いて返す関数もあるがそれがたまに間違ったりする（っぽい）ということもあり対策を求む
BOOL Shutter_Controll(HANDLE hShutter, int status) {
    char data[50];
    DWORD dwSendSize = 10;
    bool flag = TRUE;
    int i = 0;
    bool openflag = FALSE, closeflag = FALSE;

    switch (status)
    {
    case Shutter_OPEN:
        openflag = FALSE;
        i = 0;
        //以下一方的にシャッターへ信号を送ることができるため，3回同じ信号を送っている（意味があるかは不明）
        while (i < 3) {
            WriteFile(hShutter, "\r\nOPEN:1\r\n", 12, &dwSendSize, NULL);
            i++;
        }
        break;
    case Shutter_CLOSE:
        closeflag = FALSE;
        i = 0;
        while (i < 3) {
            WriteFile(hShutter, "\r\nCLOSE:1\r\n", 13, &dwSendSize, NULL);
            i++;
        }
        break;    
    default:
        flag = FALSE;
        break;
    }
    return flag;
}

//シリアル通信で直接CRUXへ送信する関数
//CRUX内固有のコマンドを生成し送信するための関数であるため，必ずCRUXのユーザーマニュアルを見るように
BOOL Send_Stage_Message_Serial(HWND hWnd, const char* equipment, const char* controll_num, const char* move) {
    char str[50] = {};
    char crlf[4] = {};
    char receive[1];
    char data[50];
    DWORD dwSendSize = 10;
    bool flag=TRUE;
    unsigned long nn;

    if (equipment[0] == 'R' && equipment[1] == 'P' && equipment[2] == 'S') {//←多分この書き方はもう少し何とかなる．
        //RPSは相対量の移動を命令するコマンド
        //これがこのプログラムにおいて最も使われるコマンド
        strcat_s(str, "\r\n\x02");//ASCIIコードの0x02はSTX信号といい，通信開始を示すコードである．
        strcat_s(str, equipment);//動作させるステージ番号の指定
        strcat_s(str, "/");
        strcat_s(str, controll_num);//動作させるステージを動かすための速度テーブルの番号を指定
        strcat_s(str, "/");
        strcat_s(str, move);//動作量を指定
        strcat_s(str, "/");
        strcat_s(str, "0");
        strcat_s(str, "\r\n");
    }
    else if (equipment[0] == 'O' && equipment[1] == 'R' && equipment[2] == 'G') {
        //ORGは初期位置へ戻すためのコマンド
        strcat_s(str, "\r\n\x02");
        strcat_s(str, equipment);
        strcat_s(str, "/");
        strcat_s(str, controll_num);
        strcat_s(str, "/");
        strcat_s(str, "0");
        strcat_s(str, "\r\n");
    }
    else if (equipment[0] == 'R' && equipment[1] == 'T' && equipment[2] == 'B') {
        //RTBは速度テーブルの値を読み出すためのコマンド
        strcat_s(str, "\r\n\x02");
        strcat_s(str, equipment);
        strcat_s(str, "/");
        strcat_s(str, controll_num);
        strcat_s(str, "\r\n");
    }
    else if (equipment[0] == 'W' && equipment[1] == 'T' && equipment[2] == 'B') {
        //WTBは速度テーブルの値を書き込むためのコマンド
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
    
    //一番最初に送ったcrlf信号を拾っている。
    int i = 0;
    while (*receive != '\n') {
        ReadFile(hStage, receive, 1, &nn, 0);
        //シリアル通信の返信を格納している，どの返答が帰ってきているかのデバッグ及び確認用．
    }
    return flag;
}

//受け取りメッセージの解析（未完成）デバッグモードで満足したためあまり使わなかったのよ
BOOL Receive_Stage_Message_Serial(HWND hWnd) {
    char receive[1];
    unsigned long nn;

    *receive = 0;
    nn = 0;
    while (ReadFile(hStage, receive, 1, &nn, 0) == FALSE);
    while (*receive != '\n') {
       ReadFile(hStage, receive, 1, &nn, 0);
    }

    return TRUE;
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
