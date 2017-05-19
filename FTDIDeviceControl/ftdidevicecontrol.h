#ifndef FTDIDEVICECONTROL_H
#define FTDIDEVICECONTROL_H

#include <QtWidgets/QMainWindow>
#include <QMutex>

#include "ui_ftdidevicecontrol.h"
#include "ftd2xx.h"

class CWaitingThread;
class QFile;

enum
{
	PKG_TYPE_INFO=0,
	PKG_TYPE_ERRORMES=1,
	PKG_TYPE_ASCIIMES=2,
	PKG_TYPE_RWSW=3,
	PKG_TYPE_RWPACKET=4
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
	quint32 m_flashID;
	QMutex m_mtx;
	quint64 m_readBytes;
	quint64 m_inputLength;
	QFile* m_inputFile;
	char m_buff[2048];


	void openPort(int aNum);
	void closePort();
	int waitForPacket(int& tt, int& aCode);
	QByteArray hexStringToByteArray(QString& aStr);
	void fillDeviceList();
	int sendPacket(unsigned char aType, unsigned short aLen, char* data);

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
	void slReadFlash();
};

#endif // FTDIDEVICECONTROL_H
