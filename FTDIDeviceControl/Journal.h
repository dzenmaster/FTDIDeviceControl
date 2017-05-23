//журнал логов
//-----------------------------------------------------------

#ifndef JOURNAL_H
#define JOURNAL_H

#include <QTextEdit>
#include <QMutex>

class QFile;
class QTextCodec;

class CJournal : public QTextEdit
{
	Q_OBJECT

public:
	CJournal(QWidget *parent = 0);
	~CJournal();

	void addMessage(const QString& a_module, const QString& a_text, bool isError = false);

signals:
	void newData(QString a_module, QString a_text, bool isError);

private:
	QMutex m_mtx;	
	QFile* m_logFile;
	QTextCodec* m_codec;
	QColor m_clrErr;
	QColor m_clrInf;

	void clearLogs();

private slots:
	void safeAddMessage(QString a_module, QString a_text, bool isError);

};

#endif // JOURNAL_H
