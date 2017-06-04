#include <QFile>
#include <QMessageBox>
#include <QDateTime>
#include "PswDlg.h"

extern QString g_basePath; // каталог exe файла
QString GFTDIPswFileName = "ftdidc.psw";

CPswDlg::CPswDlg(bool a_changeMode, QWidget *parent, Qt::WindowFlags flags)
	: QDialog(parent,flags)
{
	m_passwordCR.resize(4096);
	m_changeMode=a_changeMode;
	ui.setupUi(this);
	
	ui.lOld->setVisible(m_changeMode);
	ui.lNew->setVisible(m_changeMode);
	ui.leNewPsw->setVisible(m_changeMode);
	ui.pbChange->setVisible(!m_changeMode);//
	connect(ui.pbChange,SIGNAL(clicked()),SLOT(changeMode()));
	m_passwordPath = g_basePath + GFTDIPswFileName;
	QFile f1(m_passwordPath);
	if (f1.open(QIODevice::ReadOnly)){//вычитаем пароль из файла
		m_passwordCR = f1.readAll();
		encryptPass();
		f1.close();
	}
	else{
		ui.leOldPsw->setEnabled(false);
		ui.lOld->setEnabled(false);
	}
}

void CPswDlg::accept()
{		
	if (!m_changeMode) { // обычный режим
		QByteArray ba = ui.leOldPsw->text().toLocal8Bit();
		if (ba == m_password) {//совпал
			QDialog::accept();
			return;
		}		
		QMessageBox::critical(this, " ", "Пароль неверный");	
		return;
	}
	else {//режим изменения пароля
		QByteArray ba = ui.leOldPsw->text().toLocal8Bit();
		if (ba != m_password) {			
			QMessageBox::critical(this, " ", "Cтарый пароль неверный");	
			return;
		}
		ba = ui.leNewPsw->text().toLocal8Bit();//новый
		m_password = ba;
		QFile f1(m_passwordPath);
		if (f1.open(QIODevice::WriteOnly)) { //запишем новый пароль в файл
			cryptPass();
			f1.write(m_passwordCR);
			f1.close();
			QDialog::accept();
			return;
		}
	}
	QDialog::reject();
}

void CPswDlg::changeMode()
{
	m_changeMode = true;
	ui.lOld->show();
	ui.lNew->show();
	ui.leNewPsw->show();
	ui.pbChange->hide();
}

void CPswDlg::cryptPass()
{	
	QDateTime dt;
	srand( dt.toTime_t() );

	// 0) зашумляем буфер
	for(int i = 0; i < 4096; i++)	{
		m_passwordCR[i] = (unsigned char)(rand() * 256 / RAND_MAX);
	}

	// 1) В 7-й позиции будет лежать длина пароля
#ifdef Q_OS_WIN	
	unsigned char nLen = m_passwordCR[7] = unsigned char(m_password.length());
#else	
	char nLen = m_passwordCR[7] = char(m_password.length());
#endif

	// 2) В 11-й позиции будет лежать начало цепочки (20..100)
#ifdef Q_OS_WIN
	unsigned char start = m_passwordCR[11] = unsigned char(20 + rand() * 80 / RAND_MAX);
#else
	char start = m_passwordCR[11] = char(20 + rand() * 80 / RAND_MAX);
#endif
	// 3) В 17-й шаг разброса букв
#ifdef Q_OS_WIN
	unsigned char step = m_passwordCR[17] = unsigned char(int(4096-100) / (nLen+1));
#else
	char step = m_passwordCR[17] = char(int(4096-100) / (nLen+1));
#endif
	// 4) записываем пароль
	for(int j=0; j<nLen; j++) {
		int ind = int(start)+int(step)*j;
		m_passwordCR[ind] = m_password[j]; 
	}	
}

void CPswDlg::encryptPass()
{
	// 1) В 7-й позиции будет лежать длина пароля
#ifdef Q_OS_WIN	
	unsigned char nLen = m_passwordCR[7];
#else
	char nLen = m_passwordCR[7];
#endif
	// 2) В 11-й позиции будет лежать начало цепочки (20..100)
#ifdef Q_OS_WIN
	unsigned char start = m_passwordCR[11];
#else
	char start = m_passwordCR[11];
#endif
	// 3)В 17-й шаг разброса букв
#ifdef Q_OS_WIN
	unsigned char step = m_passwordCR[17];
#else
	char step = m_passwordCR[17];
#endif
	//  
	m_password.clear();
	for(int i=0; i<nLen; i++) {
		int ind = int(start)+int(step)*i;
		m_password += m_passwordCR[ind]; 
	}	
}
