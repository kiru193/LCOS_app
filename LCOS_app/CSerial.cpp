#include "CSerial.h"
#include <process.h>

unsigned __stdcall ThreadFunc(void* p)
{
    CSerial* pp = (CSerial*)p;

    return pp->RecvData();
}

CSerial::CSerial(void)
    : mHComPort(NULL), mRecvDataBuff(NULL), mRecvDataBuffLen(0)
{
    mMutex = CreateMutex(NULL, TRUE, NULL);
}

CSerial::~CSerial(void)
{
    CloseHandle(mMutex);
    if (mRecvDataBuff != NULL) {
        delete mRecvDataBuff;
        mRecvDataBuff = NULL;
    }
    mMutex = NULL;
}

void CSerial::Start(LPCSTR comPortName, DCB* portConfig)
{
    bool    nBret;

    // �w��|�[�g���J��
    mHComPort = CreateFile(
        TEXT("COM8"),
        GENERIC_READ | GENERIC_WRITE,               // �ǂݏ������w��
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (mHComPort != INVALID_HANDLE_VALUE)
    {
        // �|�[�g�̃{�[���[�g�A�p���e�B����ݒ�
        nBret = SetCommState(mHComPort, portConfig);
    }
    
    if (nBret != FALSE)
    {
        mThreadId = (HANDLE)_beginthreadex(
            NULL,
            0,
            ThreadFunc,
            this,
            0,
            NULL);
    }
    
}

void CSerial::End()
{
    if (mHComPort != NULL)
    {
        CloseHandle(mHComPort);
    }
}

DWORD CSerial::RecvData(void)
{
    BYTE buffer[1024];
    DWORD toReadBytes = 1024;
    DWORD readBytes;

    while (ReadFile(mHComPort, buffer, toReadBytes, &readBytes, NULL))
    {
        if (readBytes > 0)
        {
            WaitForSingleObject(mMutex, 0);

            // ��M�����f�[�^�́AmRecvDataBuff �Ɏ�M�ς݂Ŏ��o����Ă��Ȃ�
            // �f�[�^�ɑ����ĕێ����Ă���
            BYTE* tmpBuf = new BYTE[mRecvDataBuffLen + readBytes];
            if (mRecvDataBuff != NULL)
            {
                memcpy(tmpBuf, mRecvDataBuff, mRecvDataBuffLen);
                delete[] mRecvDataBuff;
            }
            memcpy(tmpBuf, buffer, readBytes);
            mRecvDataBuffLen += readBytes;
            mRecvDataBuff = tmpBuf;

            ReleaseMutex(mMutex);
        }
    }

    return S_OK;
}

DWORD CSerial::GetRecvData(LPVOID buffer)
{
    DWORD length;

    WaitForSingleObject(mMutex, 0);

    buffer = new BYTE[mRecvDataBuffLen];
    memcpy(buffer, mRecvDataBuff, mRecvDataBuffLen);
    length = this->mRecvDataBuffLen;

    ReleaseMutex(mMutex);

    return length;
}

DWORD CSerial::SendData(HWND hWnd, LPVOID buffer, int toWriteBytes)
{
    DWORD writeBytes;
    DWORD index = 0;
    

    // �w�肳�ꂽ�f�[�^��S�ď������ވׂɃ��[�v����
    while (toWriteBytes > 0)
    {
        if (WriteFile(
            mHComPort,
            "OPEN:1\r\n",
            toWriteBytes,
            &writeBytes,
            NULL) == FALSE) {
            MessageBox(hWnd, TEXT("ERROR"), TEXT("ERROR"), MB_OK);
        }
        index += writeBytes;
        toWriteBytes -= writeBytes;
    }

    return writeBytes;
}