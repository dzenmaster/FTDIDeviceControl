#define NOMINMAX

#include "ftdidevicecontrol.h"
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>

#include "WaitingThread.h"

QSettings g_Settings("FTDIDeviceControl","FTDIDeviceControl");

extern QString LOGPath;

FTDIDeviceControl::FTDIDeviceControl(QWidget *parent)
	: QMainWindow(parent), m_handle(0),m_waitingThread(0), m_flashID(-1),m_inputLength(0),m_readBytes(0),m_inputFile(0),m_startAddr(0)
{
	clearLogs();
	ui.setupUi(this);



	ui.lModule->hide();
	ui.cbModule->hide();
	
	ui.tabWidget->removeTab(0);
	ui.tabWidget->removeTab(3);
	ui.tabWidget->setEnabled(false);

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

	fillDeviceList();
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

bool FTDIDeviceControl::openPort(int aNum)
{
	closePort();
	FT_STATUS ftStatus = FT_OK;
	ftStatus = FT_Open(aNum, &m_handle);
	if (ftStatus!=FT_OK){
		QMessageBox::critical(this, "FT_Open error", "FT_Open error");
		return false;
	}
	ftStatus = FT_SetEventNotification(m_handle, FT_EVENT_RXCHAR, m_hEvent);
	if (ftStatus!=FT_OK){
		QMessageBox::critical(this, "FT_SetEventNotification error", "FT_SetEventNotification error");
		return false;
	}	
	ftStatus = FT_SetBaudRate(m_handle, 115200);
	//ftStatus = FT_SetBaudRate(m_handle, 57600);
	if (ftStatus!=FT_OK){
		QMessageBox::critical(this, "FT_SetBaudRate error", "FT_SetBaudRate error");
		return false;
	}
	ftStatus = FT_SetDataCharacteristics(m_handle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
	if (ftStatus!=FT_OK) {
		QMessageBox::critical(this, "FT_SetDataCharacteristics error", "FT_SetDataCharacteristics error");
		return false;
	}
	ui.teReceive->append("Opened succesful\n");
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
			ui.teReceive->append("error : " + m_lastErrorStr);		
			tResult = false;
			break;
		}		
	}	
	ui.widget_3->setEnabled(tResult);
	m_mtx.unlock();
	return tResult;
}

void FTDIDeviceControl::slNewKadr(unsigned char aType, unsigned short aLen, const unsigned char* aData)
{
	if ((!aData)||(!aLen)||(aLen>2048))
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
	if (aType==2) {
		char* tStr = new char[aLen+1];
		memcpy(tStr, aData, aLen);
		tStr[aLen] = 0;
		QString tQStr = tStr;
		ui.teModuleMessages->moveCursor (QTextCursor::End);
		ui.teModuleMessages->insertPlainText(tStr);
		QApplication::processEvents();
	}
	if (aType==3) {//read from SW
		//0xa5 0x5a 0x3 0x7 0x0 0x1 0x0 0x0 0x16 0x0 0x0 0x0
		if ((aLen==7)&&(aData[0]==1)) {			
			unsigned short swAddr = ((unsigned short)aData[1]) + ((unsigned short)aData[2])*256;
			if (swAddr==0){ // Flash ID
				m_flashID =  *((quint32*)&aData[3]);
				ui.leFlashID->setText(QString("%1").arg(m_flashID));
			}
			else if (swAddr==3){ // 
				quint32 r3 = *((quint32*)&aData[3]);				
				ui.teReceive->append(QString("r3 = %1\n").arg(r3));
			}
			else if (swAddr==2){ // len
				m_inputLength = *((quint32*)&aData[3]);				
				ui.teReceive->append(QString("r2 = %1\n").arg(m_inputLength));
			}
			else if (swAddr==4){
				m_startAddr =  *((quint32*)&aData[3]);				
				ui.teReceive->append(QString("Start Address = %1\n").arg(m_startAddr));
			}
		}
		// a5 5a 03 |07 00|01|03 00| XX XX XX XX
	}
	if (aType==4) {
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
				QMessageBox::information(this, "finished", "finished");
			}
		}
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
	m_rbfFileName  = fn;
	ui.lePathToRBF->setText(m_rbfFileName );
	if (QFile::exists(m_rbfFileName )) {
		QFileInfo fi(m_rbfFileName);
		ui.lSize->setText(QString("Size %1").arg(fi.size()));
		g_Settings.setValue("rbfFileName", m_rbfFileName);
		return true;
	}
	return false;
}

bool FTDIDeviceControl::slWriteFlash()
{
	if (!m_mtx.tryLock())
		return false;
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
		QMessageBox::critical(this, "wrong flash ID", "wrong flash ID");
		m_mtx.unlock();
		return false;	
	}
	QString fileName = ui.lePathToRBF->text();
	if (!QFile::exists(fileName)) {
		QMessageBox::critical(this, "no file", "no file");
		m_mtx.unlock();
		return false;	
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
		return false;	
	}
	buff[0] = 0xA5;
	buff[1] = 0x5A;
	buff[2] = 0x04;

	qint64 wasRW=0;
	bool tSuccsess = false;
	for(;;)	
	{
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
			QMessageBox::critical(this, "FT_Write error", "FT_Write error");			
		}
		int tWTime=0;
		int tCode=-1;
		if (waitForPacket(tWTime, tCode)==1) {		
			ui.teReceive->append("Wait timeout\n");
			ui.teReceive->append(QString("write error %1 len=%2 %3 %4\n").arg(tCode).arg(nWasRead).arg(ret).arg(wasRW));
			break;
		}
		else {
			ui.teReceive->append(QString("good Wait %1ms len=%2 %3 %4\n").arg(tWTime).arg(nWasRead).arg(ret).arg(wasRW));
			if (tCode!=0){
				ui.teReceive->append(QString("write error %1\n").arg(tCode));				
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
		ui.teReceive->append("Unsuccessful write\n");
		ui.tabWidget->setEnabled(true);
		QApplication::processEvents();	
		m_mtx.unlock();
		return false;
	}	
	ui.teReceive->append("Checking...\n");	
	if (sendPacket(PKG_TYPE_RWSW, 3, REG_RD, 3)!=0)	{
		ui.teReceive->append("error : " + m_lastErrorStr);	// ѕрочитать регистр -> addr=0x3 должно изменитьс€ значениe с 0x4 на 0х0 (NULL)	
	}

	ui.tabWidget->setEnabled(true);
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
		ui.teReceive->append("error : " + m_lastErrorStr);		
		return false;
	}

	m_mtx.unlock();
	return true;
}

bool FTDIDeviceControl::slReadStartAddress()
{
	if (!m_mtx.tryLock())
		return false;

	if (sendPacket(PKG_TYPE_RWSW, 3, REG_RD, 4)!=0)	{
		m_mtx.unlock();
		ui.teReceive->append("error : " + m_lastErrorStr);		
		return false;
	}
	Sleep(200);


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
	setEnabled(false);
	FT_STATUS ftStatus = FT_OK;
	DWORD ret;	

	for (unsigned char i = 0; i < 13; ++i) {	
		if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x01, i* 0x10000 + m_startAddr)!=0)	{
			ui.teReceive->append("error : " + m_lastErrorStr);
			break;
		}		
		if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x03, 0x03)!=0)	{
			ui.teReceive->append("error : " + m_lastErrorStr);
			break;
		}		
		QApplication::processEvents();
	}	
	//back address
	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x01, m_startAddr)!=0)	{
		ui.teReceive->append("error : " + m_lastErrorStr);	
		setEnabled(true);
		m_mtx.unlock();
		return false;
	}

	setEnabled(true);
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
		ui.teReceive->append("error : " + m_lastErrorStr);
		m_mtx.unlock();
		return false;
	}
	m_mtx.unlock();
	return true;
}

bool FTDIDeviceControl::slWriteCmdUpdateFirmware()
{
	if (!m_mtx.tryLock())
		return false;
	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x03, 0x04)!=0) { // 4. «аписать в регистр команду Update Firmware 0x4  -> addr=0x3 (ƒождатьс€ пакета OK)
		ui.teReceive->append("error : " + m_lastErrorStr);	
		m_mtx.unlock();
		return false;
	}
	m_mtx.unlock();
	return true;
}

void FTDIDeviceControl::slReadFlash()
{	
	if (!m_mtx.tryLock())
		return;
	
	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x01, 0x00)!=0) { // a5 5a 03 07 00 00 01 00 00 00 00 00 -- set start epcs addr 0
		ui.teReceive->append("error : " + m_lastErrorStr);		
	}	

	if (sendPacket(PKG_TYPE_RWSW, 3, REG_RD, 0x02)!=0)	{ // a5 5a 03 07 00 00 02 00 10 00 00 00 -- read length
		ui.teReceive->append("error : " + m_lastErrorStr);		
	}

	m_readBytes = 0;
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
		ui.teReceive->append("Open file error\n");
		QApplication::processEvents();
		m_mtx.unlock();
		return;
	}

	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x03, 0x05)!=0) { // start reading a5 5a 03 07 00 00 03 00 05 00 00 00 -- cmd read fw
		ui.teReceive->append("error : " + m_lastErrorStr);		
	}
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
	if ( (m_rbfFileName!=ui.lePathToRBF->text())||(!QFile::exists(m_rbfFileName)) ) {
		if (!slBrowseRBF()){	
			QMessageBox::critical(0,"Open RBF error","Open RBF  error");
			return;
		}
	}
	ui.statusBar->showMessage("Read Flash ID");
	if (!slReadFlashID()){	
		QMessageBox::critical(0,"ReadFlashID error","ReadFlashID error");
		return;
	}
	ui.statusBar->showMessage("Read Start Address");
	if (!slReadStartAddress()){	
		QMessageBox::critical(0,"Read Start Address error","Read Start Address error");
		return;
	}
	ui.statusBar->showMessage("Erase Flash");
	if (!slEraseFlash()){	
		QMessageBox::critical(0,"EraseFlash error","EraseFlash error");
		return;
	}
	ui.statusBar->showMessage("Write Length");
	if (!slWriteLength()){	
		QMessageBox::critical(0,"WriteLength error","WriteLength error");
		return;
	}
	ui.statusBar->showMessage("Write Cmd Update Firmware");
	if (!slWriteCmdUpdateFirmware()){	
		QMessageBox::critical(0,"WriteCmdUpdateFirmware error","WriteCmdUpdateFirmware error");
		return;
	}
	ui.statusBar->showMessage("Write Flash Firmware");
	if (!slWriteFlash()){	
		QMessageBox::critical(0,"WriteFlash error","WriteFlash error");
		return;
	}
	ui.statusBar->showMessage("Updated successful");
	QMessageBox::information(0, "Updated successful", "Updated successful");
}

void FTDIDeviceControl::slConnectToDevice()
{
	bool tResult = false;
	if (ui.cbFTDIDevice->count()>0)
		tResult = openPort(ui.cbFTDIDevice->currentIndex());
	if (!tResult){
		QMessageBox::critical(0,"Open error","Open error");
		return;
	}
	ui.tabWidget->setEnabled(true);
	if (slGetInfo()){
		ui.statusBar->showMessage("Connected successful");
		QMessageBox::information(0, "Connected successful", "Connected successful");

	}
	else{
		ui.statusBar->showMessage("Connection error");
		QMessageBox::critical(0,"Connection error","Connection error");
	}
}

void FTDIDeviceControl::clearLogs()
{	//почистим логи
	QDir logDir(LOGPath);
	QFileInfoList fiList = logDir.entryInfoList();
	QFileInfoList::iterator i = fiList.begin();
	int deletedFiles=0;
	while(i!=fiList.end()) {
		qint64 dt = (*i).lastModified().daysTo(QDateTime::currentDateTime());
		if (dt>20){ // cтарше 20 дней чистим
			if (QFile::remove((*i).absoluteFilePath()))
				deletedFiles++;
		}
		++i;
	}
	if (deletedFiles){
		ui.teJournal->addMessage("",QString("”далено %1 файлов из каталога LOG").arg(deletedFiles));
	}	
}

bool FTDIDeviceControl::slJumpToFact()
{
	if (!m_mtx.tryLock())
		return false;
		
	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x16, 0x00)!=0)	{
		ui.teReceive->append("error : " + m_lastErrorStr);
		m_mtx.unlock();
		return false;
	}

	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x17, 0x01)!=0)	{
		ui.teReceive->append("error : " + m_lastErrorStr);
		m_mtx.unlock();
		return false;
	}

	m_mtx.unlock();
	return true;
}

bool FTDIDeviceControl::slJumpToAppl()
{
	if (!m_mtx.tryLock())
		return false;

	if (sendPacket(PKG_TYPE_RWSW, 3, REG_RD, 4)!=0)	{
		m_mtx.unlock();
		ui.teReceive->append("error : " + m_lastErrorStr);		
		return false;
	}
	Sleep(200);

	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x16, m_startAddr)!=0)	{
		ui.teReceive->append("error : " + m_lastErrorStr);
		m_mtx.unlock();
		return false;
	}

	if (sendPacket(PKG_TYPE_RWSW, 7, REG_WR, 0x17, 0x01)!=0)	{
		ui.teReceive->append("error : " + m_lastErrorStr);
		m_mtx.unlock();
		return false;
	}

	m_mtx.unlock();
	return true;
}