#include <winsock2.h>
#include <WinBase.h>
#include <Winerror.h>
#include "KendyNet.h"
#include "stdio.h"
enum
{
	IO_RECVREQUEST = 1<<1,   //Ӧ�ò��������
	IO_SENDREQUEST = 1<<3,   //Ӧ�ò㷢������
	IO_RECVFINISH =  1<<2,//�������
	IO_SENDFINISH =  1<<4,   //�������
};


enum
{
	IO_RECV = (1<<1) + (1<<2),
	IO_SEND = (1<<3) + (1<<4),
	IO_REQUEST = (1<<1) + (1<<3),
};

int32_t    InitNetSystem()
{
	int32_t nResult;
	WSADATA wsaData;
	nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (NO_ERROR != nResult)
	{
		printf("\nError occurred while executing WSAStartup().");
		return -1; //error
	}
	return 0;
}
void   CleanNetSystem()
{
	 WSACleanup();
}


HANDLE CreateNetEngine(DWORD NumberOfConcurrentThreads)
{
	HANDLE CompletePort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, NumberOfConcurrentThreads);
	if(NULL == CompletePort)
	{
		printf("\nError occurred while creating IOCP: %d.", WSAGetLastError());
	}
	return CompletePort;
}

void   CloseNetEngine(HANDLE CompletePort)
{
	CloseHandle(CompletePort);
}

/* return:
*  >  0 :bytestransfer
*  == 0 :WSA_IO_PENDING
*  <  0 :error or socket close
*/
static int32_t raw_Send(Socket_t s,struct OverLapContext *overLapped,uint32_t *err_code)
{
	uint32_t dwFlags = 0;
	DWORD dwBytes = 0;
	if(SOCKET_ERROR == WSASend(s->sock, overLapped->wbuf, overLapped->buf_count, 
		&dwBytes, dwFlags, (OVERLAPPED*)overLapped, NULL))
	{
		if(err_code)
			*err_code = WSAGetLastError();
		return IO_ERROR;
	}	
	else 
		return dwBytes;
}

static int32_t raw_Recv(Socket_t s,struct OverLapContext *overLapped,uint32_t *err_code)
{
	DWORD dwFlags = 0;
	DWORD dwBytes = 0;
	int32_t ret = 0;
	if(SOCKET_ERROR == WSARecv(s->sock, overLapped->wbuf, overLapped->buf_count, 
		&dwBytes, &dwFlags, (OVERLAPPED*)overLapped, NULL))
	{
		if(err_code)
			*err_code = WSAGetLastError();
		return IO_ERROR;
	}	
	else 
		return dwBytes;
}

typedef void (*CallBack)(struct Socket*,struct OverLapContext*,int32_t,uint32_t);
int    RunEngine(HANDLE CompletePort,DWORD timeout)
{

	DWORD bytesTransfer;
	Socket_t       socket;
	struct OverLapContext *overLapped = 0;
	uint32_t lastErrno = 0;
	BOOL bReturn;
	CallBack call_back;
	uint32_t ms;
	uint32_t tick = GetTickCount();
	uint32_t _timeout = tick + timeout;
	do
	{
		ms = _timeout - tick;
		call_back = 0;
		lastErrno = 0;
		bReturn = GetQueuedCompletionStatus(
			CompletePort,&bytesTransfer,
			reinterpret_cast<PULONG_PTR>(&socket),
			(OVERLAPPED**)&overLapped,ms);
		
		if(FALSE == bReturn && !overLapped)// || socket == NULL || overLapped == NULL)
		{
			break;
		}
		if(0 == bytesTransfer)
		{
			lastErrno = WSAGetLastError();			
			if(bReturn == TRUE && lastErrno == 0)
			{
			}
			else
			{
				if(overLapped->m_Type & IO_RECV)
					call_back = socket->RecvFinish;	
				else
					call_back = socket->SendFinish;
				if(FALSE == bReturn)
					bytesTransfer = -1;
			}

		}
		else
		{
			if(overLapped->m_Type & IO_RECVFINISH)
				call_back = socket->RecvFinish;
			else if(overLapped->m_Type & IO_SENDFINISH)
				call_back = socket->SendFinish;
			else
			{
				continue;
			}
		}

		if(call_back)
		{
			call_back(socket,overLapped,bytesTransfer,lastErrno);
		}
		tick = GetTickCount();
	}while(tick < _timeout);
	return 0;
}

int    Bind2Engine(HANDLE CompletePort,Socket_t socket)
{
	HANDLE hTemp;
	if(!socket->RecvFinish || !socket->SendFinish)
		return -1;
	hTemp = CreateIoCompletionPort((HANDLE)socket->sock, CompletePort,(ULONG_PTR)socket, 0);
	if (NULL == hTemp)
		return -1;
	socket->complete_port = CompletePort;
	return 0;
}


int32_t    WSA_Send(Socket_t socket,struct OverLapContext *OverLap,int32_t now,uint32_t *err_code)
{
	int32_t bytetransfer = 0;
	if(!socket->complete_port)
		return UNBIND2ENGINE;
	ZeroMemory(&OverLap->m_overLapped, sizeof(OVERLAPPED));
	OverLap->m_Type = IO_SENDFINISH;
	bytetransfer =  raw_Send(socket,OverLap,err_code);
	if(bytetransfer > 0)
	{
		bytetransfer = -1;
		*err_code = WSA_IO_PENDING;
	}
	return bytetransfer;
}

int32_t    WSA_Recv(Socket_t socket,struct OverLapContext *OverLap,int32_t now,uint32_t *err_code)
{
	int32_t bytetransfer = 0;
	if(!socket->complete_port)
		return UNBIND2ENGINE;
	ZeroMemory(&OverLap->m_overLapped, sizeof(OVERLAPPED));

	OverLap->m_Type = IO_RECVFINISH;
	bytetransfer = raw_Recv(socket,OverLap,err_code);
	if(bytetransfer > 0)
	{
		bytetransfer = -1;
		*err_code = WSA_IO_PENDING;
	}
	return bytetransfer;
}
