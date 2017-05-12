#include "ftdidevicecontrol.h"
#include <QMessageBox>


CWaitingThread::CWaitingThread(FT_HANDLE aHandle, HANDLE ahEvent, QObject * parent)
	: QThread(parent),m_handle(aHandle), m_hEvent(ahEvent), m_stop(false)
{

}

void CWaitingThread::run()
{
	for(;;){
		if (m_stop)
			return;
		DWORD dwRead, dwRXBytes;
		unsigned char b;
		FT_STATUS status;
		WaitForSingleObject(m_hEvent, -1);

		if(m_handle) {
			status = FT_GetQueueStatus(m_handle, &dwRead); //  dwRead = length to read in bytes
			if(status == FT_OK) {
				m_string="";
				while(dwRead) {
					status = FT_Read(m_handle, &b, 1, &dwRXBytes);
					if(status == FT_OK) {
						m_string+=b;
					}
					status = FT_GetQueueStatus(m_handle, &dwRead);
				}
				if (m_string.size())
					emit newData(m_string);
			}
		}
	}//for(;;)
}


FTDIDeviceControl::FTDIDeviceControl(QWidget *parent)
	: QMainWindow(parent), m_handle(0),m_waitingThread(0)
{
	ui.setupUi(this);
	connect(ui.pbSend, SIGNAL(clicked()),SLOT(slSend()));
	connect(ui.pbOpen, SIGNAL(clicked()),SLOT(slOpen()));
	connect(ui.pbClose, SIGNAL(clicked()),SLOT(slClose()));

	FT_STATUS ftStatus;
	unsigned numDevs=0;
	void * p1;			

	p1 = (void*)&numDevs;
	ftStatus = FT_ListDevices(p1, NULL, FT_LIST_NUMBER_ONLY);	
	if ((ftStatus == 0)&&(numDevs>0)) 
	{
		if (numDevs>9)
			numDevs=9;
		char *BufPtrs[10];   // pointer to array of 10 pointers 
		for(int i = 0; i < numDevs; ++i) {
			BufPtrs[i]=new char[64];
		}
		BufPtrs[numDevs] = NULL;

		ftStatus = FT_ListDevices(BufPtrs,&numDevs,FT_LIST_ALL|FT_OPEN_BY_DESCRIPTION); 
		if (ftStatus == FT_OK) { 
			ui.cbFTDIDevice->clear();
			for(int i = 0; i < numDevs; ++i) {
				ui.cbFTDIDevice->addItem(BufPtrs[i]);
			}
		} 				
	}
	else
	{
		QMessageBox::critical(this,"no devs","no devices connected");
		return;
	}
}

FTDIDeviceControl::~FTDIDeviceControl()
{
	closePort();
}


void FTDIDeviceControl::openPort(int aNum)
{
	closePort();
	FT_STATUS ftStatus = FT_OK;
	ftStatus = FT_Open(aNum, &m_handle);
	ftStatus = FT_SetEventNotification(m_handle, FT_EVENT_RXCHAR, m_hEvent);
	ftStatus = FT_SetBaudRate(m_handle, 9600);
	m_waitingThread = new CWaitingThread(m_handle, m_hEvent);
	connect(m_waitingThread,SIGNAL(newData(const QString& )), SLOT(addDataToTE(const QString& )));
	m_waitingThread->start();

}

void FTDIDeviceControl::closePort()
{
	if (m_waitingThread){
		m_waitingThread->disconnect();
		m_waitingThread->m_stop=true;
		Sleep(200);
		delete m_waitingThread;	
		m_waitingThread=0;
	}

	if (m_handle) {
		FT_Close(m_handle);
		m_handle = NULL;
	}
}

void FTDIDeviceControl::slSend()
{
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;

	QString str = ui.leSend->text();
	QByteArray ba = str.toLocal8Bit();		

	if(m_handle == NULL) {
		QMessageBox::critical(this,"closed","need to open device");
		return;
	}
	else
	{
		ftStatus = FT_Write(m_handle, ba.data(), ba.size(), &ret);		
	}

}

void FTDIDeviceControl::addDataToTE(const QString& str)
{
	ui.teReceive->append(str);
	QApplication::processEvents();
}

void FTDIDeviceControl::slOpen()
{
	if (ui.cbFTDIDevice->count()>0)
		openPort(ui.cbFTDIDevice->currentIndex());
}

void FTDIDeviceControl::slClose()
{
	closePort();
}