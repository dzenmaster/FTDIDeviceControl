#ifndef FTDIDEVICECONTROL_H
#define FTDIDEVICECONTROL_H

#include <QtWidgets/QMainWindow>

#include "ui_ftdidevicecontrol.h"
#include "ftd2xx.h"

class CWaitingThread;

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
	QByteArray hexStringToByteArray(QString& aStr);
	void fillDeviceList();

private slots:
	void slSend();
	void addDataToTE(const QString& str);
	void slOpen();
	void slClose();
	void slGetInfo();
	void slNewKadr(unsigned char aID, unsigned short aLen, const unsigned char* aData);
	void slBrowseRBF();
	void slWriteFlash();
};

#endif // FTDIDEVICECONTROL_H
