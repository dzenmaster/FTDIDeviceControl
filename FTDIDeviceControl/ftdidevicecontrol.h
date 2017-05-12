#ifndef FTDIDEVICECONTROL_H
#define FTDIDEVICECONTROL_H

#include <QtWidgets/QMainWindow>
#include <QThread>
#include "ui_ftdidevicecontrol.h"
#include "ftd2xx.h"

class CWaitingThread : public QThread
{
	Q_OBJECT

public:
	CWaitingThread(FT_HANDLE aHandle,HANDLE ahEvent, QObject * parent = 0);
	~CWaitingThread(){};
	bool m_stop;

signals:
	void newData(const QString& );

private:	

	void run() Q_DECL_OVERRIDE;
	HANDLE m_hEvent;
	FT_HANDLE m_handle;

	QString m_string;

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

private slots:
	void slSend();
	void addDataToTE(const QString& str);
	void slOpen();
	void slClose();
};

#endif // FTDIDEVICECONTROL_H
