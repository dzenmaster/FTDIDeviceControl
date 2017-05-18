#include "ftdidevicecontrol.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>

#include "WaitingThread.h"

FTDIDeviceControl::FTDIDeviceControl(QWidget *parent)
	: QMainWindow(parent), m_handle(0),m_waitingThread(0), m_flashID(-1)
{
	ui.setupUi(this);
	connect(ui.pbSend, SIGNAL(clicked()), SLOT(slSend()));
	connect(ui.pbOpen, SIGNAL(clicked()), SLOT(slOpen()));
	connect(ui.pbClose, SIGNAL(clicked()), SLOT(slClose()));
	connect(ui.pbGetInfo, SIGNAL(clicked()), SLOT(slGetInfo()));
	connect(ui.pbClear, SIGNAL(clicked()), ui.teReceive, SLOT(clear()));
	connect(ui.pbBrowseRBF, SIGNAL(clicked()), SLOT(slBrowseRBF()));
	connect(ui.pbWriteFlash, SIGNAL(clicked()), SLOT(slWriteFlash()));
	connect(ui.pbReadFlashID, SIGNAL(clicked()), SLOT(slReadFlashID()));
	connect(ui.pbEraseFlash, SIGNAL(clicked()), SLOT(slEraseFlash()));
	connect(ui.pbUpdateFirmware, SIGNAL(clicked()), SLOT(slUpdateFirmware()));
	connect(ui.pbWriteLength, SIGNAL(clicked()), SLOT(slWriteLength()));

	fillDeviceList();
}


FTDIDeviceControl::~FTDIDeviceControl()
{
	closePort();
}

void FTDIDeviceControl::fillDeviceList()
{
	FT_STATUS ftStatus = FT_OK;
	unsigned numDevs = 0;
	void * p1 = (void*)&numDevs;				
	ftStatus = FT_ListDevices(p1, NULL, FT_LIST_NUMBER_ONLY);	
	if ((ftStatus == FT_OK)&&(numDevs>0)) {
		if (numDevs>9)
			numDevs = 9;
		char *BufPtrs[10];   // pointer to array of 10 pointers 
		for(int i = 0; i < numDevs; ++i) 
			BufPtrs[i]=new char[64];
		
		BufPtrs[numDevs] = NULL;

		ftStatus = FT_ListDevices(BufPtrs, &numDevs, FT_LIST_ALL | FT_OPEN_BY_DESCRIPTION); 
		if (ftStatus == FT_OK) { 
			ui.cbFTDIDevice->clear();
			for(int i = 0; i < numDevs; ++i) {
				ui.cbFTDIDevice->addItem(BufPtrs[i]);
			}
		}
		for(int i = 0; i < numDevs; ++i) 
			delete BufPtrs[i];
	}
	else {
		QMessageBox::critical(this,"no devs","no devices connected");	
	}
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
	ui.teReceive->append("Opened succesful\n");
	m_waitingThread = new CWaitingThread(m_handle, m_hEvent);
	connect(m_waitingThread,SIGNAL(newData(const QString& )), SLOT(addDataToTE(const QString& )));
	connect(m_waitingThread,SIGNAL(newKadr(unsigned char, unsigned short, const unsigned char*)), SLOT(slNewKadr(unsigned char, unsigned short, const unsigned char*)));
	
	m_waitingThread->start();
}

void FTDIDeviceControl::closePort()
{
	if (m_waitingThread) {
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

//0x01 0x02 0xFF ...
QByteArray FTDIDeviceControl::hexStringToByteArray(QString& aStr)
{
	QByteArray ba;
	while (aStr.size() > 0){//0x01 0x02 0xFF
		bool res;
		QString ts = aStr.left(3);
		aStr.remove(0, 3);
		//ts.remove(0, 2);
		unsigned char cc = ts.toInt(&res, 16);
		if (!res)
			break;
		ba+=cc;
	}
	return ba;
}

void FTDIDeviceControl::slSend()
{
	FT_STATUS ftStatus = FT_OK;	
	QString str = ui.leSend->text();
	QByteArray ba = hexStringToByteArray(str);
	//QByteArray ba = str.toLocal8Bit();		

	if(m_handle == NULL) {
		QMessageBox::critical(this,"closed","need to open device");
		return;
	}
	else {
		DWORD ret;
		ftStatus = FT_Write(m_handle, ba.data(), ba.size(), &ret);	
		if (ftStatus!=FT_OK) {
			QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
		}
	}

}

void FTDIDeviceControl::addDataToTE(const QString& str)
{
	ui.teReceive->moveCursor (QTextCursor::End);
	ui.teReceive->insertPlainText (str);
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
	if (!m_mtx.tryLock())
		return;

	if(m_handle == NULL) {
		QMessageBox::critical(this,"closed","need to open device");
		m_mtx.unlock();
		return;
	}
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;
	char buff[] = {0xA5, 0x5A, 0x00, 0x01, 0x00, 0x00}; // get module type

	bool tResult = true;
	for (unsigned char nc=0; nc < 4; ++nc){
		buff[5] = nc; // get type sn firmware software

		m_waitingThread->setWaitForPacket(0x00);
		ftStatus = FT_Write(m_handle, buff, 6, &ret);
		if (ftStatus!=FT_OK) {
			QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
		}
		int tWTime=0;
		if (waitForPacket(tWTime)==1){
			//QMessageBox::critical(this, "Wait timeout", "Wait timeout");
			ui.teReceive->append("Wait timeout\n");
			tResult = false;
		}
		else{
			ui.teReceive->append(QString("good Wait %1ms\n").arg(tWTime));
		}
		QApplication::processEvents();
	}
	
	ui.widget_3->setEnabled(tResult);
	
//	QMessageBox::information(this,"ok","ok");
	m_mtx.unlock();
}
void FTDIDeviceControl::slNewKadr(unsigned char aType, unsigned short aLen, const unsigned char* aData)
{
	if ((!aData)||(!aLen))
		return;
	if (aType==0) {
		unsigned char tID = aData[0];
		switch (tID) {
		case 0:
			ui.leModuleType->setText(QString("%1").arg(aData[1]));			
			break;
		case 1:
			ui.leSerialNumber->setText(QString("%1").arg(aData[1]));			
			break;
		case 2:
			ui.leFirmwareVersion->setText(QString("%1").arg(aData[1]));			
			break;
		case 3:
			ui.leSoftwareVersion->setText(QString("%1").arg(aData[1]));			
			break;
		}
	}
	if (aType==1) {
		//error
	}
	if (aType==3) {//read from SW
		//0xa5 0x5a 0x3 0x7 0x0 0x1 0x0 0x0 0x16 0x0 0x0 0x0
		if ((aLen==7)&&(aData[0]==1)) {
			unsigned short swAddr =  ((unsigned short)aData[1]) + ((unsigned short)aData[2])*256;
			if (swAddr==0){ // Flash ID
				m_flashID =  *((quint32*)&aData[3]);
				ui.leFlashID->setText(QString("%1").arg(m_flashID));
			}
		}
	}
}

int FTDIDeviceControl::waitForPacket(int& tt )
{
	for(int i = 0; i < 100; ++i)
	{
		Sleep(16);
		if (m_waitingThread->getWaitForPacket()==false){
			tt = 16*i;
			return 0;
		}
	}
	return 1;//need timeout;
}

void FTDIDeviceControl::slBrowseRBF()
{
	QString fileName = QFileDialog::getOpenFileName(this,"Open RBF","", tr("RBF Files (*.rbf)"));
	ui.lePathToRBF->setText(fileName);
	if (QFile::exists(fileName)) {
		QFileInfo fi(fileName);
		ui.lSize->setText(QString("Size %1").arg(fi.size()));
	}
}

void FTDIDeviceControl::slWriteFlash()
{
	if (!m_mtx.tryLock())
		return;
	//5. ����� ������ �������� ������ (type = 0x04) �� 1024 �����(��������� ����� ����� �������� �������); ����� ������� ������ ������ ��������� ����� OK. 
	//	a5 5a 04 |00 04|00 00...........
	//	| LEN |data.................. 

	//����� �� ������  0xa5 0x5a 0x1 0x1 0x0 0x0 (��������� ���� ��� ������) - 0x00 = PASS	
	if(m_handle == NULL) {
		QMessageBox::critical(this, "closed", "need to open device");
		m_mtx.unlock();
		return;
	}
	if (m_flashID!=0x16){
		QMessageBox::critical(this, "wrong flash ID", "wrong flash ID");
		m_mtx.unlock();
		return;	
	}
	QString fileName = ui.lePathToRBF->text();
	if (!QFile::exists(fileName)) {
		QMessageBox::critical(this, "no file", "no file");
		m_mtx.unlock();
		return;	
	}

	ui.tabWidget->setEnabled(false);
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;
	char buff[2048];

	QFileInfo fi(fileName);
	qint64 szFile = fi.size();

	QFile f1(fileName);
	if (!f1.open(QIODevice::ReadOnly))	{
		QMessageBox::critical(this, "open err", "open err");
		ui.tabWidget->setEnabled(true);
		m_mtx.unlock();
		return;	
	}
	buff[0] = 0xA5;
	buff[1] = 0x5A;
	buff[2] = 0x04;

	qint64 wasRW=0;
	for(;;)	
	{
		qint64 nWasRead = f1.read(&buff[5], 1024);		
		if (nWasRead < 1)
			break;
		wasRW+=nWasRead;
		quint16 tLen = nWasRead;
		memcpy(&buff[3],&tLen,2); 
		m_waitingThread->setWaitForPacket(0x01);
		ftStatus = FT_Write(m_handle, buff, 1029, &ret);
		if (ftStatus!=FT_OK) {
			QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
		}
		int tWTime=0;
		if (waitForPacket(tWTime)==1) {		
			ui.teReceive->append("\nWait timeout\n");	
		}
		else{
			ui.teReceive->append(QString("\ngood Wait %1ms\n").arg(tWTime));
		}
		if (nWasRead<1024)
			break;
		ui.progressBarRBF->setValue((wasRW*100)/szFile);	
		QApplication::processEvents();	
	}
	f1.close();
	ui.progressBarRBF->setValue(100);
	//check
	//6. ��������� ������� -> addr=0x3 ������ ���������� �������e � 0x4 �� 0�0 (NULL)
	//	a5 5a 03 |03 00|01|03 00| 
	//	| LEN |rd|RgAdr|
	//		����� �� ������ a5 5a 03 |07 00|01|03 00| XX XX XX XX
	//	| LEN |rd|RgAdr|data	    | 

	ui.tabWidget->setEnabled(true);
	QApplication::processEvents();	
	m_mtx.unlock();
}

void FTDIDeviceControl::slReadFlashID()
{
	if (!m_mtx.tryLock())
		return;

	if(m_handle == NULL) {
		QMessageBox::critical(this,"closed","need to open device");
		m_mtx.unlock();
		return;
	}
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;
	char buff[] = { 0xA5, 0x5A, 0x03, 0x03, 0x00, 0x01, 0x00, 0x00 }; // get flash ID
		
	m_waitingThread->setWaitForPacket(0x03);
	ftStatus = FT_Write(m_handle, buff, 8, &ret);
	if (ftStatus!=FT_OK) {
		QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
	}
	int tWTime=0;
	if (waitForPacket(tWTime)==1) {		
		ui.teReceive->append("\nWait timeout\n");	
	}
	else{
		ui.teReceive->append(QString("\ngood Wait %1ms\n").arg(tWTime));
	}
	QApplication::processEvents();
	m_mtx.unlock();
}

void FTDIDeviceControl::slEraseFlash()
{
	// a5 5a 03 03 00 01 00 00

	//	 0xa5 0x5a 0x3 0x7 0x0 0x1 0x0 0x0 0x16 0x0 0x0 0x0


/*	a5 5a 03 |07 00|00|01 00|00 00 00 00| -- set start epcs addr sector 0 
		| LEN |wr|RgAdr|data	    |	

		����� �� ������  0xa5 0x5a 0x1 0x1 0x0 0x0 (��������� ���� ��� ������) - 0x00 = PASS						

		2.2 �������� ������� 0x3 (erase sector) -> addr = 0x3
		a5 5a 03 |07 00|00|03 00|03 00 00 00| -- erase sector (����� ������� ��������� � �.2.1)
		| LEN |wr|RgAdr|data	    |

		����� �� ������  0xa5 0x5a 0x1 0x1 0x0 0x0 (��������� ���� ��� ������) - 0x00 = PASS		 

		2.3 ��������� ������ �������� ��� ��������� �������� sector 1 = 1*0x010000; sector 2 = 2*0x010000; sector 12 = 12*0x010000 (�� �.2.1)
*/
	if (!m_mtx.tryLock())
		return;

	if(m_handle == NULL) {
		QMessageBox::critical(this,"closed","need to open device");
		m_mtx.unlock();
		return;
	}
	if (m_flashID!=0x16){
		QMessageBox::critical(this,"wrong flash ID","wrong flash ID");
		m_mtx.unlock();
		return;	
	}
	setEnabled(false);
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;	

	for (unsigned char i = 0; i < 13; ++i) {
		char buff[] = {0xA5, 0x5A, 0x03, 0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, i, 0x00 }; // set start epcs addr sector 0 
		m_waitingThread->setWaitForPacket(0x01);
		ftStatus = FT_Write(m_handle, buff, 12, &ret);
		if (ftStatus!=FT_OK) {
			QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
		}
		int tWTime=0;
		if (waitForPacket(tWTime)==1){		
			ui.teReceive->append("\nWait timeout\n");	
		}
		else{
			ui.teReceive->append(QString("\ngood Wait %1ms\n").arg(tWTime));
		}
		char buff2[] = { 0xA5, 0x5A, 0x03, 0x07, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00};
		m_waitingThread->setWaitForPacket(0x01);
		ftStatus = FT_Write(m_handle, buff2, 12, &ret);
		if (ftStatus!=FT_OK) {
			QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
		}
		tWTime=0;
		if (waitForPacket(tWTime)==1){		
			ui.teReceive->append("\nWait timeout\n");	
		}
		else{
			ui.teReceive->append(QString("\ngood Wait %1ms\n").arg(tWTime));
		}
		QApplication::processEvents();
	}	
	setEnabled(true);
	m_mtx.unlock();
}

void FTDIDeviceControl::slWriteLength()
{
	if (!m_mtx.tryLock())
		return;
	//3. �������� ������ ����� ����� (��� �������� 827854 ����) -> addr=0x2 
	//	a5 5a 03 |07 00|00|02 00|CE A1 0� 00| -- write file length (� �������� ������ ���� ������� ����� ����������� �� ���-�� ��������� ����)
	//	         | LEN |wr|RgAdr|data	    |  

	//	����� �� ������  0xa5 0x5a 0x1 0x1 0x0 0x0 (��������� ���� ��� ������) - 0x00 = PASS	
	if(m_handle == NULL) {
		QMessageBox::critical(this, "closed", "need to open device");
		m_mtx.unlock();
		return;
	}
	if (m_flashID!=0x16){
		QMessageBox::critical(this, "wrong flash ID", "wrong flash ID");
		m_mtx.unlock();
		return;	
	}
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;
	char buff[] = { 0xA5, 0x5A, 0x03, 0x07, 0x00, 0x00, 0x02, 0x00, 0x00,0x00,0x00,0x00 }; 
	quint32 sz = 0;
	QString fileName = ui.lePathToRBF->text();
	if (QFile::exists(fileName)) {
		QFileInfo fi(fileName);
		sz = fi.size();
	}
	memcpy(&buff[8], &sz, 4); 

	m_waitingThread->setWaitForPacket(0x01);
	ftStatus = FT_Write(m_handle, buff, 12, &ret);
	if (ftStatus!=FT_OK) {
		QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
	}
	int tWTime=0;
	if (waitForPacket(tWTime)==1) {		
		ui.teReceive->append("\nWait timeout\n");	
	}
	else{
		ui.teReceive->append(QString("\ngood Wait %1ms\n").arg(tWTime));
	}
	QApplication::processEvents();
	m_mtx.unlock();
}

void FTDIDeviceControl::slUpdateFirmware()
{
	if (!m_mtx.tryLock())
		return;
//	4. �������� � ������� ������� Update Firmware 0x4  -> addr=0x3 (��������� ������ OK)
//		a5 5a 03 |07 00|00|03 00|04 00 00 00| 
//		| LEN |wr|RgAdr|data	    | 

//		����� �� ������  0xa5 0x5a 0x1 0x1 0x0 0x0 (��������� ���� ��� ������) - 0x00 = PASS
	if(m_handle == NULL) {
		QMessageBox::critical(this, "closed", "need to open device");
		m_mtx.unlock();
		return;
	}
	if (m_flashID!=0x16){
		QMessageBox::critical(this, "wrong flash ID", "wrong flash ID");
		m_mtx.unlock();
		return;	
	}
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;
	char buff[] = { 0xA5, 0x5A, 0x03, 0x07, 0x00, 0x00, 0x03, 0x00, 0x04,0x00,0x00,0x00 };

	m_waitingThread->setWaitForPacket(0x01);
	ftStatus = FT_Write(m_handle, buff, 12, &ret);
	if (ftStatus!=FT_OK) {
		QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
	}
	int tWTime=0;
	if (waitForPacket(tWTime)==1) {		
		ui.teReceive->append("\nWait timeout\n");	
	}
	else{
		ui.teReceive->append(QString("\ngood Wait %1ms\n").arg(tWTime));
	}
	QApplication::processEvents();
	m_mtx.unlock();
}