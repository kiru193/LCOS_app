#include <Windows.h>
#include  "Winbase.h"

#pragma once

unsigned __stdcall ThreadFunc(void* p);

class CSerial
{
public:
    CSerial(void);
    ~CSerial(void);

    // �w��|�[�g���I�[�v������M�J�n
    void Start(LPCSTR comPortName, DCB* portConfig);
    // COM�|�[�g��������I��
    void End(void);

    DWORD RecvData(void);

    // ��M�ς݂̃f�[�^�� buffer �Ɋi�[�� buffer �Ɋi�[�����f�[�^�̃o�C�g����Ԃ�
    DWORD GetRecvData(LPVOID buffer);

    // buffer �Ɋi�[���ꂽ�f�[�^�� length �o�C�g���M����
    DWORD SendData(LPVOID buffer, int toWriteBytes);

private:
    HANDLE mHComPort;           // COM �|�[�g�p�n���h��
    HANDLE mThreadId;           // �X���b�h ID
    HANDLE mMutex;              // mRecvDataBuff,mRecvDataBuffLen ��r�����邽�߂�Mutex
    BYTE* mRecvDataBuff;        // ��M�p�����f�[�^�o�b�t�@
    DWORD mRecvDataBuffLen;     // mRecvDataBuff �Ɋi�[����Ă����M�f�[�^�̃o�C�g��
};