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
	REG_WR=0,
	REG_RD=1
};

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

protected:
	virtual void	closeEvent(QCloseEvent * event);

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
	unsigned char m_buff[2048];
	QString m_rbfFileName;
	QString m_lastErrorStr;
	quint32 m_startAddr;


	bool openPort(int aNum);
	void closePort();
	int waitForPacket(int& tt, int& aCode);
	QByteArray hexStringToByteArray(QString& aStr);
	void fillDeviceList();
	int sendPacket(unsigned char aType, quint16 aLen, unsigned char aRdWr,quint16 aAddr, quint32 aData = 0);
	bool setRbfFileName(const QString&);
	void toLog(const QString& logStr);

private slots:
	void slShowTerminal(bool);

	void slSend();
	void addDataToTE(const QString& str);
	void slOpen();
	void slClose();
	bool slGetInfo();
	void slNewKadr(unsigned char aType, unsigned short aLen, const unsigned char* aData);
	bool slBrowseRBF();
	bool slWriteFlash();
	bool slReadFlashID();
	bool slReadStartAddress();
	bool slEraseFlash();
	bool slWriteLength();
	bool slWriteCmdUpdateFirmware();
	void slReadFlash();
	void slUpdateFirmware();
	void slConnectToDevice();
};

#endif // FTDIDEVICECONTROL_H
