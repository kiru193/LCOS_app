#include <Windows.h>
#include  "Winbase.h"

#pragma once

unsigned __stdcall ThreadFunc(void* p);

class CSerial
{
public:
    CSerial(void);
    ~CSerial(void);

    // 指定ポートをオープンし受信開始
    void Start(LPCSTR comPortName, DCB* portConfig);
    // COMポートを閉じ処理終了
    void End(void);

    DWORD RecvData(void);

    // 受信済みのデータを buffer に格納し buffer に格納したデータのバイト数を返す
    DWORD GetRecvData(LPVOID buffer);

    // buffer に格納されたデータを length バイト送信する
    DWORD SendData(LPVOID buffer, int toWriteBytes);

private:
    HANDLE mHComPort;           // COM ポート用ハンドル
    HANDLE mThreadId;           // スレッド ID
    HANDLE mMutex;              // mRecvDataBuff,mRecvDataBuffLen を排他するためのMutex
    BYTE* mRecvDataBuff;        // 受信用内部データバッファ
    DWORD mRecvDataBuffLen;     // mRecvDataBuff に格納されている受信データのバイト数
};