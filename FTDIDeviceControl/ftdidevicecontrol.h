#ifndef FTDIDEVICECONTROL_H
#define FTDIDEVICECONTROL_H

#include <QtWidgets/QMainWindow>
#include <QMutex>

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
	quint32 m_flashID;
	QMutex m_mtx;


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
	void slNewKadr(unsigned char aType, unsigned short aLen, const unsigned char* aData);
	void slBrowseRBF();
	void slWriteFlash();
	void slReadFlashID();
	void slEraseFlash();
	void slWriteLength();
	void slUpdateFirmware();
};

#endif // FTDIDEVICECONTROL_H
