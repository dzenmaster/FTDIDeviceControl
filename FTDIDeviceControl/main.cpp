#include "ftdidevicecontrol.h"
#include <QtWidgets/QApplication>
#include <QSystemSemaphore>
#include <QSharedMemory>
#include <QMessageBox>

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

	QApplication a(argc, argv);

	if(isRunning) {
		QMessageBox::critical(0,"", "re-run FTDI Device Control");
		return 0;
	}

	FTDIDeviceControl w;
	w.show();
	return a.exec();
}
