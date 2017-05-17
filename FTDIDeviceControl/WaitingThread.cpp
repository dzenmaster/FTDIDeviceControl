#include "WaitingThread.h"

CWaitingThread::CWaitingThread(FT_HANDLE aHandle, HANDLE ahEvent, QObject * parent)
	: QThread(parent),m_handle(aHandle), m_hEvent(ahEvent), m_stop(false)
{
	m_waitForPacket=false;
}

void CWaitingThread::run()
{
	for(;;){
		if (m_stop)
			return;
		DWORD dwRead, dwRXBytes;
		unsigned char b;
		FT_STATUS status;
		//waiting for packet with sync word 
		WaitForSingleObject(m_hEvent, -1);

		if(m_handle) {
			status = FT_GetQueueStatus(m_handle, &dwRead); //  dwRead = length to read in bytes
			if(status == FT_OK) {
				m_string="";
				while(dwRead) {
					status = FT_Read(m_handle, &b, 1, &dwRXBytes);
					if(status == FT_OK) {
						m_string+=QString(" 0x%1").arg(b, 0, 16);
						if ((b==0xA)||(b==0xD))
						{
							m_string+="\n";
						}
						//нужно делать ресет по таймауту, чтобы не зависло в случае сбоя
						if (m_dec.pushByte(b))
						{//got full kadr
							emit newKadr(m_dec.getType(),m_dec.getLen(),m_dec.getData());
							m_string+="\n";
							m_waitForPacket=false;
						}
					}					
					status = FT_GetQueueStatus(m_handle, &dwRead);
				}
				if (m_string.size())
					emit newData(m_string);
			}
		}
		Sleep(16);
	}//for(;;)
}

