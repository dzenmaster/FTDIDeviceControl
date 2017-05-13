#include "ftdidevicecontrol.h"
#include <QMessageBox>

CDecoder::CDecoder()
{
	reset();
	m_curKadr = 0;
	m_lastKadr = 0;
}
void CDecoder::reset()
{
	m_curPos=0;
	m_syncState = 0;
	m_curPos = 0;

}

bool CDecoder::pushByte(unsigned char b)
{
	switch(m_syncState)
	{
	case 0:		
		if (b==0xA5){
			m_syncState = 1;
			m_kadr[m_curKadr][0] = 0xA5;
			m_curPos=1;
		}
		break;		
	case 1:

		if (b==0x5A) {
			m_syncState = 2;
			m_kadr[m_curKadr][0] = 0x5A;
			m_curPos=2;
		}
		else {
			m_syncState=0;
		}
		break;
	case 2:
		m_kadr[m_curKadr][m_curPos] = b;
		if (m_curPos==4)
			m_length = getLen();		
		m_curPos = ((m_curPos+1)&2047);
		if ((m_curPos>5)&&(m_curPos>=m_length+5)) {
			reset();
			m_lastKadr = m_curKadr;
			m_curKadr = ((m_curKadr+1)&7);
			return true;
		}
		break;
	}
	return false;
}

unsigned char CDecoder::getID()
{
	return m_kadr[m_lastKadr][5];
}

unsigned short CDecoder::getLen()
{
	return ((unsigned short)m_kadr[m_lastKadr][3]) + ((unsigned short)m_kadr[m_lastKadr][4])*256;
}

const unsigned char* CDecoder::getData()
{
	return &m_kadr[m_lastKadr][6];
}



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
						m_string+=QString("0x%1").arg(b, 0, 16);
						//нужно делать ресет по таймауту, чтобы не зависло в случае сбоя
						if (m_dec.pushByte(b))
						{//got full kadr
							emit newKadr(m_dec.getID(),m_dec.getLen(),m_dec.getData());
							m_waitForPacket=false;
						}
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
	connect(ui.pbGetInfo, SIGNAL(clicked()),SLOT(slGetInfo()));

	FT_STATUS ftStatus = FT_OK;
	unsigned numDevs=0;
	void * p1;			

	p1 = (void*)&numDevs;
	ftStatus = FT_ListDevices(p1, NULL, FT_LIST_NUMBER_ONLY);	
	if ((ftStatus == FT_OK)&&(numDevs>0)) 
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
	if (ftStatus!=FT_OK){
		QMessageBox::critical(this, "FT_Open error", "FT_Open error");
		return;
	}
	ftStatus = FT_SetEventNotification(m_handle, FT_EVENT_RXCHAR, m_hEvent);
	if (ftStatus!=FT_OK){
		QMessageBox::critical(this, "FT_SetEventNotification error", "FT_SetEventNotification error");
		return;
	}
	//ftStatus = FT_SetBaudRate(m_handle, 9600);
	ftStatus = FT_SetBaudRate(m_handle, 115200);
	if (ftStatus!=FT_OK){
		QMessageBox::critical(this, "FT_SetBaudRate error", "FT_SetBaudRate error");
		return;
	}
	ftStatus = FT_SetDataCharacteristics(m_handle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
	if (ftStatus!=FT_OK) {
		QMessageBox::critical(this, "FT_SetDataCharacteristics error", "FT_SetDataCharacteristics error");
		return;
	}
	m_waitingThread = new CWaitingThread(m_handle, m_hEvent);
	connect(m_waitingThread,SIGNAL(newData(const QString& )), SLOT(addDataToTE(const QString& )));
	connect(m_waitingThread,SIGNAL(newKadr(unsigned char, unsigned short, const unsigned char*)), SLOT(slNewKadr(unsigned char, unsigned short, const unsigned char*)));
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
		FT_STATUS ftStatus = FT_Close(m_handle);
		if (ftStatus!=FT_OK) {
			QMessageBox::critical(this, "FT_Close error", "FT_Close error");			
		}
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
		if (ftStatus!=FT_OK) {
			QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
		}
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

void FTDIDeviceControl::slGetInfo()
{
	if(m_handle == NULL) {
		QMessageBox::critical(this,"closed","need to open device");
		return;
	}
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;
	char buff[] = {0xA5, 0x5A, 0x00, 0x01,0x00, 0x00 , 0x00}; // get module type

	for (unsigned char nc=0;nc<4;++nc){
		buff[5] = nc; // get type sn firmware software

		m_waitingThread->setWaitForPacket();
		ftStatus = FT_Write(m_handle, buff, 7, &ret);
		if (ftStatus!=FT_OK) {
			QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
		}
		if (waitForPacket()==1)
			QMessageBox::critical(this, "Wait timeout", "Wait timeout");

	}
	QMessageBox::information(this,"ok","ok");
}

void FTDIDeviceControl::slNewKadr(unsigned char aID, unsigned short aLen, const unsigned char* aData)
{
	if ((!aData)||(!aLen))
		return;
	switch (aID){
		case 0:
			ui.leModuleType->setText(QString("%1").arg(aData[0]));			
			break;
		case 1:
			ui.leSerialNumber->setText(QString("%1").arg(aData[0]));			
			break;
		case 2:
			ui.leFirmwareVersion->setText(QString("%1").arg(aData[0]));			
			break;
		case 3:
			ui.leSoftwareVersion->setText(QString("%1").arg(aData[0]));			
			break;
	}
}

int FTDIDeviceControl::waitForPacket()
{
	for(int i=0;i<500;++i)
	{
		Sleep(16);
		if (m_waitingThread->getWaitForPacket()==false)
			return 0;
	}
	return 1;//need timeout;
}