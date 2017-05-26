#define NOMINMAX

#include "ftdidevicecontrol.h"
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>

#include "WaitingThread.h"

QSettings g_Settings("FTDIDeviceControl","FTDIDeviceControl");

extern QString g_basePath;

FTDIDeviceControl::FTDIDeviceControl(QWidget *parent)
	: QMainWindow(parent), m_handle(0),m_waitingThread(0), m_flashID(-1),
	m_inputLength(0),m_readBytes(0),m_inputFile(0),m_startAddr(0),m_fileType(FILE_RBF),
	m_img(384, 288, QImage::Format_Indexed8), m_timer4WaitFrame(0),m_frameCnt(0)
{
	ui.setupUi(this);
	m_framesPath = g_basePath+"Frames";
	if (!QFile::exists(m_framesPath)) {
		QDir td;
		if (!td.mkpath(m_framesPath)) {
			ui.teJournal->addMessage("cant make path",m_framesPath,1);		
		}
	}
	updateFramesList();
//	slDrawPicture("framelink22.raw");

	ui.lModule->hide();
	ui.cbModule->hide();
	
	ui.tabWidget->removeTab(0);
	ui.tabWidget->removeTab(3);

	//for test
//	ui.tabWidget->setEnabled(false);

	setRbfFileName(g_Settings.value("rbfFileName", "").toString());

	m_buff[0]=0xA5;
	m_buff[1]=0x5A;
	memset(&m_buff[2], 0, 2046);

	connect(ui.cbShowTerminal, SIGNAL(toggled(bool)), SLOT(slShowTerminal(bool)));	
	connect(ui.pbSend, SIGNAL(clicked()), SLOT(slSend()));
	connect(ui.pbOpen, SIGNAL(clicked()), SLOT(slOpen()));
	connect(ui.pbClose, SIGNAL(clicked()), SLOT(slClose()));
	connect(ui.pbGetInfo, SIGNAL(clicked()), SLOT(slGetInfo()));
	connect(ui.pbClear, SIGNAL(clicked()), ui.teReceive, SLOT(clear()));
	connect(ui.pbClear, SIGNAL(clicked()),  ui.teModuleMessages, SLOT(clear()));
	connect(ui.pbBrowseRBF, SIGNAL(clicked()), SLOT(slBrowseRBF()));
	connect(ui.pbWriteFlash, SIGNAL(clicked()), SLOT(slWriteFlash()));
	connect(ui.pbReadFlashID, SIGNAL(clicked()), SLOT(slReadFlashID()));
	connect(ui.pbReadStartAddress, SIGNAL(clicked()), SLOT(slReadStartAddress()));
	connect(ui.pbEraseFlash, SIGNAL(clicked()), SLOT(slEraseFlash()));
	connect(ui.pbWriteCmdUpdateFirmware, SIGNAL(clicked()), SLOT(slWriteCmdUpdateFirmware()));
	connect(ui.pbWriteLength, SIGNAL(clicked()), SLOT(slWriteLength()));
	connect(ui.pbReadFlash, SIGNAL(clicked()), SLOT(slReadFlash()));
	connect(ui.pbUpdateFirmware, SIGNAL(clicked()), SLOT(slUpdateFirmware()));
	connect(ui.pbConnectToDevice, SIGNAL(clicked()), SLOT(slConnectToDevice()));
	connect(ui.pbJumpToFact, SIGNAL(clicked()), SLOT(slJumpToFact()));
	connect(ui.pbJumpToAppl, SIGNAL(clicked()), SLOT(slJumpToAppl()));
	connect(ui.pbCancelUpdate,SIGNAL(clicked()), SLOT(slCancelUpdate()));

	connect(ui.pbFCView,SIGNAL(clicked()), SLOT(slViewRaw()));
	connect(this,SIGNAL(newRAWFrame(const QString&)),SLOT(slDrawPicture(const QString&)));
	connect(ui.listWFrames, SIGNAL(itemClicked(QListWidgetItem *)),SLOT(slSelectedFrame(QListWidgetItem *)));

	fillDeviceList();	
}

void FTDIDeviceControl::updateFramesList()
{
	ui.listWFrames->clear();
	QDir frDir(m_framesPath);
	QFileInfoList fiList = frDir.entryInfoList();
	QFileInfoList::iterator i = fiList.begin();	

	while(i!=fiList.end()) {
		QString fn = (*i).fileName();
		if ((fn!=".")&&(fn!=".."))
			ui.listWFrames->addItem((*i).fileName());
		++i;
	}
}

FTDIDeviceControl::~FTDIDeviceControl()
{	
	closePort();
	ui.wTerm->close();
}

	
void	FTDIDeviceControl::closeEvent(QCloseEvent * event)
{
	ui.wTerm->close();
}


void FTDIDeviceControl::slShowTerminal(bool st)
{
	if (st){
		ui.wTerm->setParent(0);
		ui.wTerm->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowMaximizeButtonHint);
		ui.wTerm->setEnabled(true);
		ui.wTerm->show();
	}
	else {
		ui.wTerm->hide();
	}
}

void FTDIDeviceControl::fillDeviceList()
{
	FT_STATUS ftStatus = FT_OK;
	unsigned numDevs = 0;
	void * p1 = (void*)&numDevs;				
	ftStatus = FT_ListDevices(p1, NULL, FT_LIST_NUMBER_ONLY);	
	if ((ftStatus == FT_OK)&&(numDevs>0)) {
		ui.teJournal->addMessage("fillDeviceList",QString("Found %1 devices").arg(numDevs));
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
		ui.teJournal->addMessage("fillDeviceList","No connected devices", 1);
		QMessageBox::critical(this,"No connected devices","No connected devices");			
	}
}

bool FTDIDeviceControl::openPort(int aNum)
{
	closePort();
	FT_STATUS ftStatus = FT_OK;
	ftStatus = FT_Open(aNum, &m_handle);
	if (ftStatus!=FT_OK){
		ui.teJournal->addMessage("openPort","FT_Open error", 1);
		QMessageBox::critical(this, "FT_Open error", "FT_Open error");
		return false;
	}
	ftStatus = FT_SetEventNotification(m_handle, FT_EVENT_RXCHAR, m_hEvent);
	if (ftStatus!=FT_OK){
		ui.teJournal->addMessage("openPort","FT_SetEventNotification error", 1);
		QMessageBox::critical(this, "FT_SetEventNotification error", "FT_SetEventNotification error");
		return false;
	}	
	ftStatus = FT_SetBaudRate(m_handle, 115200);
	//ftStatus = FT_SetBaudRate(m_handle, 57600);
	if (ftStatus!=FT_OK){
		ui.teJournal->addMessage("openPort","FT_SetBaudRate error", 1);
		QMessageBox::critical(this, "FT_SetBaudRate error", "FT_SetBaudRate error");
		return false;
	}
	ftStatus = FT_SetDataCharacteristics(m_handle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
	if (ftStatus!=FT_OK) {
		ui.teJournal->addMessage("openPort","FT_SetDataCharacteristics error", 1);
		QMessageBox::critical(this, "FT_SetDataCharacteristics error", "FT_SetDataCharacteristics error");
		return false;
	}
	ui.teJournal->addMessage("openPort","Opened succesful");	
	m_waitingThread = new CWaitingThread(m_handle, m_hEvent);
	connect(m_waitingThread,SIGNAL(newData(const QString& )), SLOT(addDataToTE(const QString& )));
	connect(m_waitingThread,SIGNAL(newKadr(unsigned char, unsigned short, const unsigned char*)), SLOT(slNewKadr(unsigned char, unsigned short, const unsigned char*)));
	
	m_waitingThread->start();
	return true;
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
			ui.teJournal->addMessage("closePort", "FT_Close error", 1);
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

bool FTDIDeviceControl::slGetInfo()
{
	if (!m_mtx.tryLock())
		return false;

	bool tResult = true;
	for (unsigned char nc=0; nc < 4; ++nc){
		if (sendPacket(PKG_TYPE_INFO, 1, REG_RD, nc)!=0)	{
			ui.teJournal->addMessage("slGetInfo", "error : " + m_lastErrorStr, 1);			
			tResult = false;
			break;
		}		
	}	
	ui.teJournal->addMessage("slGetInfo", QString("Result: %1").arg(tResult), tResult ? 0 : 1);
	ui.widget_3->setEnabled(tResult);
	m_mtx.unlock();
	return tResult;
}

void FTDIDeviceControl::slNewKadr(unsigned char aType, unsigned short aLen, const unsigned char* aData)
{
	if ((!aData)||(!aLen)||(aLen>2048)){
		ui.teJournal->addMessage("slNewKadr", "Wrong new kadr", 1);
		return;
	}

	switch (aType)
	{
	case 0: {
				unsigned char tID = aData[0];
				switch (tID) {
				case 0:
					ui.leModuleType->setText(QString("%1").arg(aData[1]));	
					ui.teJournal->addMessage("slNewKadr", QString("ModuleType %1").arg(aData[1]));
					break;
				case 1:
					ui.leSerialNumber->setText(QString("%1").arg(aData[1]));	
					ui.teJournal->addMessage("slNewKadr", QString("SerialNumber %1").arg(aData[1]));
					break;
				case 2:
					ui.leFirmwareVersion->setText(QString("%1").arg(aData[1]));	
					ui.teJournal->addMessage("slNewKadr", QString("FirmwareVersion %1").arg(aData[1]));
					break;
				case 3:
					ui.leSoftwareVersion->setText(QString("%1").arg(aData[1]));	
					ui.teJournal->addMessage("slNewKadr", QString("SoftwareVersion %1").arg(aData[1]));
					break;
			}
			break;
			}
	case 1: {//error
				if (aData[0]!=0)
					ui.teJournal->addMessage("slNewKadr", QString("Got Error Kadr %1").arg(aData[0]), 1);
			}
			break;
	case 2: {
				char* tStr = new char[aLen+1];
				memcpy(tStr, aData, aLen);
				tStr[aLen] = 0;
				QString tQStr = tStr;
				ui.teModuleMessages->moveCursor (QTextCursor::End);
				ui.teModuleMessages->insertPlainText(tQStr);
				ui.teJournal->addMessage("slNewKadr", QString("Ascii message from module : ").arg(aLen) + tQStr);
				QApplication::processEvents();
			}
			break;
	case 3:  {//read from SW
				//0xa5 0x5a 0x3 0x7 0x0 0x1 0x0 0x0 0x16 0x0 0x0 0x0
				if ((aLen==7)&&(aData[0]==1)) {			
					unsigned short swAddr = ((unsigned short)aData[1]) + ((unsigned short)aData[2])*256;
					if (swAddr==0){ // Flash ID
						m_flashID =  *((quint32*)&aData[3]);
						//ui.leFlashID->setText(QString("%1").arg(m_flashID));
						ui.teJournal->addMessage("slNewKadr", QString("Flash ID %1").arg(m_flashID));
					}
					else if (swAddr==3){ // 
						m_R3 = *((quint32*)&aData[3]);				
						//ui.teReceive->append(QString("r3 = %1\n").arg(r3));
						ui.teJournal->addMessage("slNewKadr", QString("R3 = %1").arg(m_R3));
					}
					else if (swAddr==2){ // len
						m_inputLength = *((quint32*)&aData[3]);				
						//ui.teReceive->append(QString("r2 = %1\n").arg(m_inputLength));
						ui.teJournal->addMessage("slNewKadr", QString("R2 inputLength = %1").arg(m_inputLength));
					}
					else if (swAddr==4){
						m_startAddr =  *((quint32*)&aData[3]);				
						//ui.teReceive->append(QString("Start Address = %1\n").arg(m_startAddr));
						ui.teJournal->addMessage("slNewKadr", QString("Start Address = %1\n").arg(m_startAddr));
					}
				}
				// a5 5a 03 |07 00|01|03 00| XX XX XX XX
			 }
			 break;
	case 4: {
				if (m_inputFile) {								
					m_inputFile->write((const char*)aData, aLen);
					m_readBytes+=aLen;
					ui.progressBarRBF->setValue((m_readBytes*100)/m_inputLength);	
					QApplication::processEvents();
					if (m_inputLength<=m_readBytes) {
						if (m_inputFile->isOpen())
							m_inputFile->close();
						delete m_inputFile;
						m_inputFile = 0;
						if (m_fileType==FILE_RBF) {			
							ui.teJournal->addMessage("slNewKadr", "Flash Reading is finished");
							QMessageBox::information(this, "Flash Reading is finished", "Flash Reading is finished");
						}
						else if (m_fileType==FILE_RAW) {
							ui.teJournal->addMessage("slNewKadr", "got RAW");
							emit newRAWFrame(m_fileName);
						}
					}
				}				
			}
			break;
	case 5: {
			/*	if (m_inputFrameFile) {			
					m_inputFrameFile->write((const char*)aData, aLen);
					m_readFrameBytes+=aLen;
					ui.progressFrameBar->setValue((m_readBytes*100)/m_inputLength);	
					QApplication::processEvents();
					if (221184<=m_readBytes) {
						if (m_inputFrameFile->isOpen())
							m_inputFrameFile->close();
						delete m_inputFrameFile;
						m_inputFrameFile = 0;
						ui.teJournal->addMessage("slNewKadr", "Frame is got");
						emit newFrame;
					}
				}*/
			}
			break;
	default:
		break;
	}
}

int FTDIDeviceControl::waitForPacket(int& tt , int& aCode)
{
	for(int i = 0; i < 500; ++i) {
		Sleep(16);
		if (m_waitingThread->getWaitForPacket()==false){
			tt = 16*i;
			aCode = m_waitingThread->getErrorCode();
			return 0;
		}
	}
	aCode = m_waitingThread->getErrorCode();
	return 1; // timeout;
}

bool FTDIDeviceControl::slBrowseRBF()
{	
	return setRbfFileName(QFileDialog::getOpenFileName(this,"Open RBF","", "RBF Files (*.rbf)"));
}

bool FTDIDeviceControl::setRbfFileName(const QString& fn)
{
	m_fileName  = fn;
	ui.lePathToRBF->setText(m_fileName );
	if (QFile::exists(m_fileName )) {
		QFileInfo fi(m_fileName);
		ui.lSize->setText(QString("Size %1").arg(fi.size()));
		g_Settings.setValue("rbfFileName", m_fileName);
		ui.teJournal->addMessage("setRbfFileName", m_fileName);
		return true;
	}
	return false;
}

bool FTDIDeviceControl::slWriteFlash()
{
	if (!m_mtx.tryLock())
		return false;
	m_cancel = false;
	//5. ƒалее просто посылать пакеты (type = 0x04) по 1024 байта(последний пакет будет меньшего размера); после каждого пакета должен приходить пакет OK. 
	//	a5 5a 04 |00 04|00 00...........
	//	| LEN |data.................. 

	//ответ от модул€  0xa5 0x5a 0x1 0x1 0x0 0x0 (последний байт код ршибки) - 0x00 = PASS	
	if(m_handle == NULL) {		
		QMessageBox::critical(this, "closed", "need to open device");
		m_mtx.unlock();
		return false;
	}
	if (m_flashID!=0x16){
		ui.teJournal->addMessage("slWriteFlash", QString("wrong flash ID %1").arg(m_flashID), 1);
		QMessageBox::critical(this, "wrong flash ID", "wrong flash ID");
		m_mtx.unlock();
		return false;	
	}
	QString fileName = ui.lePathToRBF->text();
	if (!QFile::exists(fileName)) {
		ui.teJournal->addMessage("slWriteFlash", QString("file %1 doesn't exist").arg(fileName), 1);
		QMessageBox::critical(this, "no file", "no file");
		m_mtx.unlock();
		return false;	
	}

	ui.wUpdate->setEnabled(false);
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;
	char buff[2048];

	QFileInfo fi(fileName);
	qint64 szFile = fi.size();

	QFile f1(fileName);
	if (!f1.open(QIODevice::ReadOnly))	{
		ui.teJournal->addMessage("slWriteFlash", QString("file open error %1").arg(fileName), 1);
		QMessageBox::critical(this, "open err", "open err");
		ui.wUpdate->setEnabled(true);
		m_mtx.unlock();
		return false;	
	}
	buff[0] = 0xA5;
	buff[1] = 0x5A;
	buff[2] = 0x04;

	qint64 wasRW=0;
	bool tSuccsess = false;
	for(;;)	
	{
		if (m_cancel){//pushed cancel
			if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x03, 0x00)!=0)  // stop write
				ui.teJournal->addMessage("slWriteFlash", QString("error : ") + m_lastErrorStr, 1);					
			else
				ui.teJournal->addMessage("slWriteFlash", "stopped",1);		
			tSuccsess = false;
			break;
		}
		qint64 nWasRead = f1.read(&buff[5], 1024);		
		//qint64 nWasRead = f1.read(&buff[5], 128);		
		if (nWasRead < 1)
			break;
		wasRW+=nWasRead;
		quint16 tLen = nWasRead;
		memcpy(&buff[3],&tLen,2); 
		m_waitingThread->setWaitForPacket(PKG_TYPE_ERRORMES);	
		Sleep(100);//костыль
		ftStatus = FT_Write(m_handle, buff, nWasRead+5, &ret);
		//ftStatus = FT_Write(m_handle, buff, 133, &ret);
		//Sleep(200);//костыль
		if (ftStatus!=FT_OK) {
			ui.teJournal->addMessage("slWriteFlash", "FT_Write error", 1);
			QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
		}
		int tWTime=0;
		int tCode=-1;
		if (waitForPacket(tWTime, tCode)==1) {		
			//ui.teReceive->append("Wait timeout\n");
			//ui.teReceive->append(QString("write error %1 len=%2 %3 %4\n").arg(tCode).arg(nWasRead).arg(ret).arg(wasRW));
			ui.teJournal->addMessage("slWriteFlash", "Wait timeout", 1);
			ui.teJournal->addMessage("slWriteFlash", QString("write error %1 len=%2 %3 %4\n").arg(tCode).arg(nWasRead).arg(ret).arg(wasRW), 1);
			break;
		}
		else {
			ui.teJournal->addMessage("slWriteFlash", QString("good Wait %1ms len=%2 %3 %4\n").arg(tWTime).arg(nWasRead).arg(ret).arg(wasRW));
			//ui.teReceive->append(QString("good Wait %1ms len=%2 %3 %4\n").arg(tWTime).arg(nWasRead).arg(ret).arg(wasRW));
			if (tCode!=0){
				ui.teJournal->addMessage("slWriteFlash", QString("write error %1\n").arg(tCode), 1);
				//ui.teReceive->append(QString("write error %1\n").arg(tCode));				
				break;
			}
		}		
		if (nWasRead<1024){
			if (wasRW==szFile)
				tSuccsess = true;
			break;
		}
		ui.progressBarRBF->setValue((wasRW*100)/szFile);	
		
		QApplication::processEvents();	
	}
	
	f1.close();
	ui.progressBarRBF->setValue(100);
	if (!tSuccsess)
	{
		ui.teJournal->addMessage("slWriteFlash", "Unsuccessful write", 1);
		//ui.teReceive->append("Unsuccessful write\n");
		ui.wUpdate->setEnabled(true);
		QApplication::processEvents();	
		m_mtx.unlock();
		return false;
	}	
	//ui.teReceive->append("Checking...\n");	
	m_R3=1;
	if (sendPacket(PKG_TYPE_RWSW, 3, REG_RD, 3)!=0)	{
		ui.teJournal->addMessage("slWriteFlash", "error : " + m_lastErrorStr, 1);
		ui.teReceive->append("error : " + m_lastErrorStr);	// ѕрочитать регистр -> addr=0x3 должно изменитьс€ значениe с 0x4 на 0х0 (NULL)	
	}
	Sleep(200);
	if (m_R3==0){
		ui.teJournal->addMessage("slWriteFlash", "Successful write");
	}
	else{
		ui.teJournal->addMessage("slWriteFlash", "Unsuccessful write R3!=0", 1);
	}
	ui.wUpdate->setEnabled(true);
	QApplication::processEvents();	
	m_mtx.unlock();
	return true;
}

void FTDIDeviceControl::slSend()
{
	if (!m_mtx.tryLock())
		return;
	FT_STATUS ftStatus = FT_OK;	
	QString str = ui.leSend->text();
	QByteArray ba = hexStringToByteArray(str);
	if(m_handle == NULL) {
		QMessageBox::critical(this,"closed","need to open device");		
	}
	else {
		DWORD ret;
		ftStatus = FT_Write(m_handle, ba.data(), ba.size(), &ret);	
		if (ftStatus!=FT_OK) {
			ui.teJournal->addMessage("slSend", "FT_Write error", 1);
			QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
		}
	}
	m_mtx.unlock();
}


bool FTDIDeviceControl::slReadFlashID()
{
	if (!m_mtx.tryLock())
		return false;
	
	if (sendPacket(PKG_TYPE_RWSW, 3, REG_RD, 0)!=0)	{
		m_mtx.unlock();
		ui.teJournal->addMessage("slReadFlashID", QString("error : ") + m_lastErrorStr, 1);
		//ui.teReceive->append("error : " + m_lastErrorStr);		
		return false;
	}
	ui.teJournal->addMessage("slReadFlashID", "success ");
	m_mtx.unlock();
	return true;
}

bool FTDIDeviceControl::slReadStartAddress()
{
	if (!m_mtx.tryLock())
		return false;

	if (sendPacket(PKG_TYPE_RWSW, 3, REG_RD, 4)!=0)	{
		m_mtx.unlock();
		ui.teJournal->addMessage("slReadStartAddress", QString("error : ") + m_lastErrorStr, 1);
		//ui.teReceive->append("error : " + m_lastErrorStr);		
		return false;
	}
	Sleep(200);
	ui.teJournal->addMessage("slReadStartAddress", "success ");
	m_mtx.unlock();
	return true;
}

bool FTDIDeviceControl::slEraseFlash()
{
	// a5 5a 03 03 00 01 00 00

	//	 0xa5 0x5a 0x3 0x7 0x0 0x1 0x0 0x0 0x16 0x0 0x0 0x0


/*	a5 5a 03 |07 00|00|01 00|00 00 00 00| -- set start epcs addr sector 0 
		| LEN |wr|RgAdr|data	    |	

		ответ от модул€  0xa5 0x5a 0x1 0x1 0x0 0x0 (последний байт код ршибки) - 0x00 = PASS						

		2.2 «аписать команду 0x3 (erase sector) -> addr = 0x3
		a5 5a 03 |07 00|00|03 00|03 00 00 00| -- erase sector (адрес сектора устанолен в п.2.1)
		| LEN |wr|RgAdr|data	    |

		ответ от модул€  0xa5 0x5a 0x1 0x1 0x0 0x0 (последний байт код ршибки) - 0x00 = PASS		 

		2.3 проделать данную операцию дл€ остальных секторов sector 1 = 1*0x010000; sector 2 = 2*0x010000; sector 12 = 12*0x010000 (см п.2.1)
*/
	if (!m_mtx.tryLock())
		return false;
	m_cancel = false;
	if(m_handle == NULL) {
		QMessageBox::critical(this,"closed","need to open device");
		m_mtx.unlock();
		return false;
	}
	if (m_flashID!=0x16){
		QMessageBox::critical(this,"wrong flash ID","wrong flash ID");
		m_mtx.unlock();
		return false;	
	}
	ui.wUpdate->setEnabled(false);
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;	

	for (unsigned char i = 0; i < 13; ++i) {
		if (m_cancel){//pushed cancel
			m_mtx.unlock();
			return false;
		}
		if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x01, i* 0x10000 + m_startAddr)!=0)	{
			//ui.teReceive->append("error : " + m_lastErrorStr);
			ui.teJournal->addMessage("slEraseFlash", QString("error 1 : ") + m_lastErrorStr, 1);
			break;
		}		
		if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x03, 0x03)!=0)	{
			//ui.teReceive->append("error : " + m_lastErrorStr);
			ui.teJournal->addMessage("slEraseFlash", QString("error 3: ") + m_lastErrorStr, 1);
			break;
		}	
		ui.progressBarRBF->setValue(i*100/12);
		QApplication::processEvents();
	}	
	//back address
	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x01, m_startAddr)!=0)	{
		//ui.teReceive->append("error : " + m_lastErrorStr);	
		ui.teJournal->addMessage("slEraseFlash", QString("error back 1: ") + m_lastErrorStr, 1);
		ui.wUpdate->setEnabled(true);
		m_mtx.unlock();
		return false;
	}
	ui.teJournal->addMessage("slEraseFlash", "success ");
	ui.wUpdate->setEnabled(true);
	m_mtx.unlock();
	return true;
}

bool FTDIDeviceControl::slWriteLength()
{
	if (!m_mtx.tryLock())
		return false;

	quint32 sz = 0;
	QString fileName = ui.lePathToRBF->text();
	if (QFile::exists(fileName)) {
		QFileInfo fi(fileName);
		sz = fi.size();
	}
	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x02, sz)!=0)	{//«аписать полную длину файла
		//ui.teReceive->append("error : " + m_lastErrorStr);
		ui.teJournal->addMessage("slWriteLength", QString("error back 1: ") + m_lastErrorStr, 1);
		m_mtx.unlock();
		return false;
	}
	ui.teJournal->addMessage("slWriteLength", "success ");
	m_mtx.unlock();
	return true;
}

bool FTDIDeviceControl::slWriteCmdUpdateFirmware()
{
	if (!m_mtx.tryLock())
		return false;
	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x03, 0x04)!=0) { // 4. «аписать в регистр команду Update Firmware 0x4  -> addr=0x3 (ƒождатьс€ пакета OK)
		//ui.teReceive->append("error : " + m_lastErrorStr);	
		ui.teJournal->addMessage("slWriteCmdUpdateFirmware", QString("error : ") + m_lastErrorStr, 1);
		m_mtx.unlock();
		return false;
	}
	ui.teJournal->addMessage("slWriteCmdUpdateFirmware", "success ");
	m_mtx.unlock();
	return true;
}

void FTDIDeviceControl::slReadFlash()
{	
	if (!m_mtx.tryLock())
		return;

	if (sendPacket(PKG_TYPE_RWSW, 3, REG_RD, 4)!=0)	{
		m_mtx.unlock();
		ui.teJournal->addMessage("slReadFlash", QString("error : ") + m_lastErrorStr, 1);
		//ui.teReceive->append("error : " + m_lastErrorStr);		
		return;
	}
	Sleep(200);


	
	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x01, m_startAddr)!=0) { // a5 5a 03 07 00 00 01 00 00 00 00 00 -- set start epcs addr 0
		//ui.teReceive->append("error : " + m_lastErrorStr);	
		ui.teJournal->addMessage("slReadFlash", QString("error 1: ") + m_lastErrorStr, 1);
		m_mtx.unlock();
		return;
	}	

	if (sendPacket(PKG_TYPE_RWSW, 3, REG_RD, 0x02)!=0)	{ // a5 5a 03 07 00 00 02 00 10 00 00 00 -- read length
		//ui.teReceive->append("error : " + m_lastErrorStr);		
		ui.teJournal->addMessage("slReadFlash", QString("error 2: ") + m_lastErrorStr, 1);
		m_mtx.unlock();
		return;
	}

	m_readBytes = 0;
	m_fileType=FILE_RBF;
	if (m_inputFile) {
		if (m_inputFile->isOpen())
			m_inputFile->close();
		delete m_inputFile;
		m_inputFile = 0;
	}
	
	QString tFileName = QFileDialog::getSaveFileName(this, "save", "", "RBF Files (*.rbf)");
	if (tFileName.isEmpty())
		tFileName = "a.rbf";
	m_inputFile = new QFile(tFileName);
	if (!m_inputFile->open(QIODevice::WriteOnly)) {
		m_inputFile = 0;
		//ui.teReceive->append("Open file error\n");
		ui.teJournal->addMessage("slReadFlash", "Open file error", 1);
		QApplication::processEvents();
		m_mtx.unlock();
		return;
	}

	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x03, 0x05)!=0) { // start reading a5 5a 03 07 00 00 03 00 05 00 00 00 -- cmd read fw
		//ui.teReceive->append("error : " + m_lastErrorStr);	
		ui.teJournal->addMessage("slReadFlash", QString("error 3: ") + m_lastErrorStr, 1);
	}
	ui.teJournal->addMessage("slReadFlash", "Reading is beginning ");
	//		ff придти данные
	m_mtx.unlock();
}

//common wrap
int FTDIDeviceControl::sendPacket(unsigned char aType, quint16 aLen, unsigned char aRdWr,quint16 aAddr, quint32 aData)
{
	if ( m_handle == NULL ) {		
		m_lastErrorStr = "need to open device";
		return -1;
	}
	if ((aLen < 1)||(aLen>2043)) {		
		m_lastErrorStr = "wrong par";
		return -1;
	}	

	Sleep(100); // костыль

	m_buff[2] = aType;
	quint32 fullLen = aLen + 5;
	memcpy(&m_buff[3], &aLen, 2);
	if (aType==PKG_TYPE_RWSW) { //regRead
		m_buff[5] = aRdWr;
		memcpy(&m_buff[6], &aAddr, 2);
		if (aRdWr==REG_RD) {
			fullLen = 8;
		}
		else{ // write
			memcpy(&m_buff[8], &aData, 4);
			fullLen = 12;
		}
	}
	else if (aType ==PKG_TYPE_INFO) {		
		m_buff[5] = (unsigned char)(aAddr&0xFF);
		fullLen = 6;
	}
	else {
		return -1;
	}	
	
	unsigned char typeToWait = PKG_TYPE_ERRORMES;
	if ((aType==PKG_TYPE_RWSW)&&(aRdWr==REG_RD)) // if read register
		typeToWait=PKG_TYPE_RWSW;
	if (aType ==PKG_TYPE_INFO)
		typeToWait=PKG_TYPE_INFO;

	DWORD ret;
	m_waitingThread->setWaitForPacket(typeToWait);	
	FT_STATUS ftStatus = FT_Write(m_handle, m_buff, fullLen, &ret);

	if (ftStatus!=FT_OK) {		
		m_lastErrorStr = "FT_Write error";
		return -1;
	}

	int tWTime=0;
	int tCode=-1;
	if (waitForPacket(tWTime, tCode)==1) {		
		m_lastErrorStr = "Wait timeout\n";		
		return -1;
	}
	else {
		ui.teReceive->append(QString("good Wait %1ms\n").arg(tWTime));	
		//ui.teJournal->addMessage("slReadFlash", QString("error 3: ") + m_lastErrorStr, 1);
		if (typeToWait==PKG_TYPE_ERRORMES){
			if (tCode!=0) {
				m_lastErrorStr = QString("Error code %1\n").arg(tCode);	
				return -1;
			}
		}
	}
	QApplication::processEvents();
	return 0;
}

void FTDIDeviceControl::slUpdateFirmware()
{	
	if ( (m_fileName!=ui.lePathToRBF->text())||(!QFile::exists(m_fileName)) ) {
		if (!slBrowseRBF()){
			ui.teJournal->addMessage("slUpdateFirmware", "Open RBF  error", 1);
			QMessageBox::critical(0,"Open RBF error","Open RBF  error");
			return;
		}
	}
	ui.statusBar->showMessage("Read Flash ID");
	if (!slReadFlashID()){	
		ui.teJournal->addMessage("slUpdateFirmware", "ReadFlashID error", 1);
		QMessageBox::critical(0,"ReadFlashID error","ReadFlashID error");
		return;
	}
	ui.statusBar->showMessage("Read Start Address");
	if (!slReadStartAddress()){	
		ui.teJournal->addMessage("slUpdateFirmware", "Read Start Address error", 1);
		QMessageBox::critical(0,"Read Start Address error","Read Start Address error");
		return;
	}
	ui.statusBar->showMessage("Erase Flash");
	if (!slEraseFlash()){
		ui.teJournal->addMessage("slUpdateFirmware", "EraseFlash error", 1);
		QMessageBox::critical(0,"EraseFlash error","EraseFlash error");
		return;
	}
	ui.statusBar->showMessage("Write Length");
	if (!slWriteLength()){	
		ui.teJournal->addMessage("slUpdateFirmware", "WriteLength error", 1);
		QMessageBox::critical(0,"WriteLength error","WriteLength error");
		return;
	}
	ui.statusBar->showMessage("Write Cmd Update Firmware");
	if (!slWriteCmdUpdateFirmware()){
		ui.teJournal->addMessage("slUpdateFirmware", "WriteCmdUpdateFirmware error", 1);
		QMessageBox::critical(0,"WriteCmdUpdateFirmware error","WriteCmdUpdateFirmware error");
		return;
	}
	ui.statusBar->showMessage("Write Flash Firmware");
	if (!slWriteFlash()){	
		ui.teJournal->addMessage("slUpdateFirmware", "WriteFlash error", 1);
		QMessageBox::critical(0,"WriteFlash error","WriteFlash error");
		return;
	}
	ui.teJournal->addMessage("slUpdateFirmware", "Updated successful");
	ui.statusBar->showMessage("Updated successful");
	QMessageBox::information(0, "Updated successful", "Updated successful");
}

void FTDIDeviceControl::slConnectToDevice()
{
	bool tResult = false;
	if (ui.cbFTDIDevice->count()>0)
		tResult = openPort(ui.cbFTDIDevice->currentIndex());
	if (!tResult){
		ui.teJournal->addMessage("slConnectToDevice", "Opening error",1);
		QMessageBox::critical(0,"Opening error","Opening error");
		return;
	}
	ui.tabWidget->setEnabled(true);
	if (slGetInfo()){
		ui.teJournal->addMessage("slConnectToDevice", "Connected successful");
		ui.statusBar->showMessage("Connected successful");
		QMessageBox::information(0, "Connected successful", "Connected successful");

	}
	else{
		ui.teJournal->addMessage("slConnectToDevice", "Connection error",1);
		ui.statusBar->showMessage("Connection error");
		QMessageBox::critical(0,"Connection error","Connection error");
	}
}


bool FTDIDeviceControl::slJumpToFact()
{
	if (!m_mtx.tryLock())
		return false;
		
	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x10, 0x00)!=0)	{
		//ui.teReceive->append("error : " + m_lastErrorStr);
		ui.teJournal->addMessage("slJumpToFact", QString("error 10 : ") + m_lastErrorStr,1);
		m_mtx.unlock();
		return false;
	}

	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x11, 0x01)!=0)	{
		//ui.teReceive->append("error : " + m_lastErrorStr);
		ui.teJournal->addMessage("slJumpToFact", QString("error 11: ") + m_lastErrorStr,1);
		m_mtx.unlock();
		return false;
	}
	ui.teJournal->addMessage("slJumpToFact", "Jump Appl successful ");
	m_mtx.unlock();
	return true;
}

bool FTDIDeviceControl::slJumpToAppl()
{
	if (!m_mtx.tryLock())
		return false;

	if (sendPacket(PKG_TYPE_RWSW, 3, REG_RD, 4)!=0)	{
		m_mtx.unlock();
		ui.teJournal->addMessage("slJumpToAppl", QString("error 4: ") + m_lastErrorStr,1);
		//ui.teReceive->append("error : " + m_lastErrorStr);		
		return false;
	}
	Sleep(200);

	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x10, m_startAddr)!=0)	{
		ui.teJournal->addMessage("slJumpToAppl", QString("error 10: ") + m_lastErrorStr,1);
		//ui.teReceive->append("error : " + m_lastErrorStr);
		m_mtx.unlock();
		return false;
	}

	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x11, 0x01)!=0)	{
		ui.teJournal->addMessage("slJumpToAppl", QString("error 11: ") + m_lastErrorStr,1);
		//ui.teReceive->append("error : " + m_lastErrorStr);
		m_mtx.unlock();
		return false;
	}
	ui.teJournal->addMessage("slJumpToAppl", "Jump Appl successful ");
	m_mtx.unlock();
	return true;
}

void FTDIDeviceControl::slCancelUpdate()
{
	m_cancel=true;
}



//for test
void FTDIDeviceControl::slViewRaw()
{
	if (!m_mtx.tryLock())
		return;

	m_readBytes = 0;	
	m_fileType=FILE_RAW;
	if (m_inputFile) {
		if (m_inputFile->isOpen())
			m_inputFile->close();
		delete m_inputFile;
		m_inputFile = 0;
	}
	QDateTime dt = QDateTime::currentDateTime();
	m_fileName = m_framesPath + "/fr" + QString::number(++m_frameCnt) + dt.toString("_dd-MM-yyyy")+"_"+dt.toString("hh'h'mm'm'ss's'")+".raw" ; //QFileDialog::getSaveFileName(this, "save", "", "RBF Files (*.rbf)");
	m_inputLength = 384*288*2;
	m_inputFile = new QFile(m_fileName);
	if (!m_inputFile->open(QIODevice::WriteOnly)) {
		delete m_inputFile;
		m_inputFile = 0;		
		ui.teJournal->addMessage("slViewRaw", "Open file error", 1);
		QApplication::processEvents();
		m_mtx.unlock();
		return;
	}		
	if (m_timer4WaitFrame) {
		m_timer4WaitFrame->stop();
		m_timer4WaitFrame->disconnect();
		delete m_timer4WaitFrame;
	}

	m_timer4WaitFrame = new QTimer;
	connect(m_timer4WaitFrame,SIGNAL(timeout()), SLOT(slWaitFrameFinished()));
	m_timer4WaitFrame->start(50000);
	m_time.start();
	m_gettingFile=true;
	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x15, 0x01)!=0) { 
		ui.teJournal->addMessage("slReadRaw", QString("error 15: ") + m_lastErrorStr, 1);		
		m_mtx.unlock();
		return;
	}	

	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x16, 0x01)!=0) { 
		ui.teJournal->addMessage("slReadRaw", QString("error 16: ") + m_lastErrorStr, 1);
		m_mtx.unlock();
		return;
	}	

	ui.teJournal->addMessage("slReadRaw", "Reading is beginning ");
	ui.tabWidget->setEnabled(false);
	m_mtx.unlock();
}

void FTDIDeviceControl::slDrawPicture(const QString& fileName)
{
	m_mtx.lock();
	ui.tabWidget->setEnabled(true);
	if (m_timer4WaitFrame) {
		m_timer4WaitFrame->stop();
		m_timer4WaitFrame->disconnect();
		delete m_timer4WaitFrame;
		m_timer4WaitFrame=0;
	}

	QFileInfo fi(fileName);
	qint64 sz = fi.size();
	ui.teJournal->addMessage("slReadRaw", QString("size %1").arg(sz));
	if (sz!=221184){
		ui.teJournal->addMessage("slReadRaw", "wrong size",1);
		m_mtx.unlock();
		return;
	}
	QFile f1(fileName);
	if (!f1.open(QIODevice::ReadOnly)){
		ui.teJournal->addMessage("slReadRaw", "open error",1);
		m_mtx.unlock();
		return;
	}
	
	unsigned short tUS=0;
	for(int i = 0; i < 288; ++i)
	{
		for(int j = 0; j < 384; ++j){
			f1.read((char*)(&tUS), 2);
			m_rawDataMono8[i][j]=(unsigned char)(tUS>>8);
		}
	}
	f1.close();
	ui.teJournal->addMessage("slReadRaw", "read file finished");
	//QImage m_img(384, 288, QImage::Format_Indexed8); //640,480 size picture.	
	for(int i = 0; i < m_img.height(); ++i)
	{
		 memcpy(m_img.scanLine(i), m_rawDataMono8[i], m_img.bytesPerLine());
	}
	//ui.lView->setScaledContents(true);
	//ui.lView->setPixmap(QPixmap::fromImage(m_img).scaled(ui.lView->size(),Qt::KeepAspectRatio));
	ui.lView->setPixmap(QPixmap::fromImage(m_img).scaled(ui.lView->size().width()-2, ui.lView->size().height()-2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	m_gettingFile = false;	

	int et = m_time.elapsed();
	ui.teJournal->addMessage("slReadRaw", QString("time of getting new frame %1").arg(et));

	m_mtx.unlock();
}

void FTDIDeviceControl::slWaitFrameFinished() // timeout in waiting frame
{
	m_mtx.lock();
	if (m_timer4WaitFrame){
		m_timer4WaitFrame->stop();
		m_timer4WaitFrame->disconnect();
		delete m_timer4WaitFrame;
		m_timer4WaitFrame=0;
	}

	if (m_inputFile) {
		if (m_inputFile->isOpen())
			m_inputFile->close();
		delete m_inputFile;
		m_inputFile = 0;
	}

	if (!m_gettingFile)
		return;
	ui.tabWidget->setEnabled(true);
	QMessageBox::critical(this,"slWaitFrameFinished","Timeout");	
	m_mtx.unlock();
}

void	FTDIDeviceControl::resizeEvent(QResizeEvent * event)
{	
	ui.lView->setPixmap(QPixmap::fromImage(m_img).scaled(ui.lView->size().width()-2, ui.lView->size().height()-2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void	FTDIDeviceControl::showEvent(QShowEvent * event)
{
	ui.lView->setPixmap(QPixmap::fromImage(m_img).scaled(ui.lView->size().width()-2, ui.lView->size().height()-2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void	FTDIDeviceControl::slSelectedFrame(QListWidgetItem * aItem)
{
	if (!aItem)
		return;	
	slDrawPicture(m_framesPath+"/"+aItem->text());
}