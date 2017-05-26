#include "ftdidevicecontrol.h"
#include <QtWidgets/QApplication>
#include <QSystemSemaphore>
#include <QSharedMemory>
#include <QMessageBox>
#include <QTextCodec>

QString LOGPath;
QString g_basePath;

int main(int argc, char *argv[])
{
	//check re-run of application
	QSystemSemaphore sema("FTDIDeviceControl_1",1);
	bool isRunning;
	sema.acquire();
	{
		QSharedMemory shmem("FTDIDeviceControl_2");
		shmem.attach();
	}
	QSharedMemory shmem("FTDIDeviceControl_2");
	if(shmem.attach()) {
		isRunning = true;
	}
	else{
		shmem.create(1);
		isRunning = false;
	}
	sema.release();	

	char* basePathChar = new char[2048];
	qstrncpy(basePathChar,argv[0],2047);
	char* pp = strrchr(basePathChar,'\\');
	(pp) ?	*(pp+1)=0 :	basePathChar[0]=0;
	QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
	g_basePath = codec->toUnicode(basePathChar);
	LOGPath = g_basePath +"LOG\\";

	QApplication a(argc, argv);

	if(isRunning) {
		QMessageBox::critical(0,"", "re-run FTDI Device Control");
		return 0;
	}

	FTDIDeviceControl w;
	w.show();
	return a.exec();
}
