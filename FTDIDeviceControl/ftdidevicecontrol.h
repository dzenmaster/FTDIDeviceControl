#ifndef FTDIDEVICECONTROL_H
#define FTDIDEVICECONTROL_H

#include <QtWidgets/QMainWindow>
#include <QThread>
#include "ui_ftdidevicecontrol.h"
#include "ftd2xx.h"

class CDecoder
{
public:
	CDecoder();
	bool pushByte(unsigned char b); // return if full kadr
	void reset();
	unsigned char getID();
	unsigned short getLen();
	const unsigned char* getData();

private:
	int m_syncState;
	unsigned char m_kadr[8][2048];
	unsigned short m_curKadr;
	unsigned short m_lastKadr;
	unsigned short m_curPos;
	unsigned short m_length;


};

class CWaitingThread : public QThread
{
	Q_OBJECT

public:
	CWaitingThread(FT_HANDLE aHandle,HANDLE ahEvent, QObject * parent = 0);
	~CWaitingThread(){};
	bool m_stop;
	void setWaitForPacket(){m_waitForPacket=true;};
	bool getWaitForPacket(){return m_waitForPacket;};

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

};

class FTDIDeviceControl : public QMainWindow
{
	Q_OBJECT

public:
	FTDIDeviceControl(QWidget *parent = 0);
	~FTDIDeviceControl();

private:
	Ui::FTDIDeviceControlClass ui;

	FT_HANDLE m_handle;
	HANDLE m_hEvent;
	CWaitingThread* m_waitingThread; 
	

	void openPort(int aNum);
	void closePort();
	int waitForPacket(int& tt);

private slots:
	void slSend();
	void addDataToTE(const QString& str);
	void slOpen();
	void slClose();
	void slGetInfo();
	void slNewKadr(unsigned char aID, unsigned short aLen, const unsigned char* aData);
};

#endif // FTDIDEVICECONTROL_H
