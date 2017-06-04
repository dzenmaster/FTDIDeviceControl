// Окно ввода пароля
//-----------------------
#ifndef PASSWORDDLG_H
#define PASSWORDDLG_H

#include <QDialog>
#include "ui_PswDlg.h"

class CPswDlg : public QDialog
{
	Q_OBJECT

public:
	CPswDlg(bool a_changeMode,QWidget *parent = 0, Qt::WindowFlags flags = 0);
	~CPswDlg(){};	

private:
	Ui::PswDlg ui;
	bool m_changeMode;
	QByteArray m_password;
	QByteArray m_passwordCR; //зашифрованный
	QString m_passwordPath;

	void cryptPass();
	void encryptPass();

private slots:	
	virtual void accept();
	void changeMode();
};

#endif 



