//	журнал логов
//-----------------------------------------------------------

#include <QDir>
#include <QDate>
#include <QTextCodec>
#include <QTime>
#include <QKeyEvent>
#include "Journal.h"

extern QString LOGPath;

CJournal::CJournal(QWidget *parent)
	: QTextEdit(parent), m_clrErr(Qt::red), m_clrInf(Qt::black)
{
	m_mtx.lock();	
	QDir dir(LOGPath);
	if (!dir.exists())
		dir.mkpath(LOGPath);
	QTime tTime = QTime::currentTime();
	QDate tDate = QDate::currentDate();  
	m_logFile = new QFile(LOGPath + tDate.toString("dd-MM-yyyy_") + tTime.toString("hh'h'mm'm'ss's'")+".log"); 

    m_codec = QTextCodec::codecForName("UTF-8");


	QTextDocument *	 tDoc = document();
	if (tDoc) {
		tDoc->setMaximumBlockCount(80);
	}
	connect(this,SIGNAL(newData(QString, QString, bool )),SLOT(safeAddMessage(QString, QString, bool)) );
	m_mtx.unlock();
	addMessage("", "Программа FTDI Device Control запущена"); 
	clearLogs();
}

CJournal::~CJournal()
{
	addMessage("", "Программа FTDI Device Control завершена"); 
}

void CJournal::addMessage(const QString& a_module, const QString& a_text, bool isError)
{
	//m_mtx.lock();
	emit newData(a_module,a_text, isError);
	//m_mtx.unlock();
}

void CJournal::safeAddMessage(QString a_module, QString a_text, bool isError)
{
	m_mtx.lock();	
	QTime tTime = QTime::currentTime();
	QString str = tTime.toString("hh:mm:ss.zzz ");
	if (a_module.isEmpty())
		str += a_text;
	else
		str += a_module + " : " + a_text;
	m_logFile->open( QIODevice::Text | QIODevice::Append);
	m_logFile->write(m_codec->fromUnicode(str+"\n"));
	m_logFile->close();
	if (isError)
		setTextColor(m_clrErr);
	else
		setTextColor(m_clrInf);
	append(str);	
	QTextCursor c = textCursor();//курсор в конец	
	c.movePosition(QTextCursor::End);
	setTextCursor(c);
	m_mtx.unlock();
}

void CJournal::clearLogs()
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
		addMessage("",QString("Удалено %1 файлов из каталога LOG").arg(deletedFiles));
	}	
}