#ifndef WAITINGTHREAD_H
#define WAITINGTHREAD_H

#include <QThread>
#include "ftd2xx.h"
#include "Decoder.h"

class CWaitingThread : public QThread
{
	Q_OBJECT

public:
	CWaitingThread(FT_HANDLE aHandle,HANDLE ahEvent, QObject * parent = 0);
	~CWaitingThread(){};
	bool m_stop;
	unsigned char m_typeToWait;
	void setWaitForPacket(unsigned char typeToWait = 0);// { if (m_typeToWait==1) m_errorCode = -1; m_typeToWait=typeToWait; m_waitForPacket=true;};
	bool getWaitForPacket() { return m_waitForPacket; };	
	int getErrorCode() { return m_errorCode;};

signals:
	void newData(const QString& );
	void newKadr(unsigned char, unsigned short, const unsigned char*);//id len data

private:	

	void run() Q_DECL_OVERRIDE;
	HANDLE m_hEvent;
	FT_HANDLE m_handle;

	QString m_string;
	CDecoder m_dec;
	bool m_waitForPacket;
	int m_errorCode;

};
#endif // FTDIDECODER_H
