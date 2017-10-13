#include "WaitingThread.h"

CWaitingThread::CWaitingThread(FT_HANDLE aHandle, HANDLE ahEvent, QObject * parent)
	: QThread(parent),m_handle(aHandle), m_hEvent(ahEvent), m_stop(false)
{
	m_waitForPacket=false;
	m_typeToWait = 0;
	m_errorCode = -1;
}


void CWaitingThread::setWaitForPacket(unsigned char typeToWait)
{
	if (m_typeToWait==1) 
		m_errorCode = -1; 
	m_typeToWait = typeToWait; 
	m_waitForPacket = true;
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
				m_string.clear();
				while(dwRead) {
					status = FT_Read(m_handle, &b, 1, &dwRXBytes);
					if(status == FT_OK) {
						m_string+=QString(" 0x%1").arg(b, 0, 16);
					//	if ((b==0xA)||(b==0xD))
					//	{
					//		m_string+="\n";
					//	}
						//нужно делать ресет по таймауту, чтобы не зависло в случае сбоя
						if (m_dec.pushByte(b))
						{//got full kadr
							unsigned char tType = m_dec.getType();							
							if (tType==1) {
								if (b==0) 
									m_string += " <- OK\n";
								else
									m_string += " <- error msg\n";
								m_errorCode = b;
							}
							else{
								m_string+="\n";
							}
							if (m_typeToWait==tType)
								m_waitForPacket=false;
							//Sleep(100);
							emit newKadr(tType, m_dec.getLen(), m_dec.getData());
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

