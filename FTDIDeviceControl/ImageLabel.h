#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QLabel>
#include <QImage>
#include <QMutex>
#include <QPoint>

class QWheelEvent;

class CImageLabel : public QLabel
{
    Q_OBJECT

public:
	CImageLabel(QWidget * parent = 0, Qt::WindowFlags f = 0);
	void setRawBuffer(const unsigned char* aBuf, const unsigned short* aOrigBuf, int aWid,int aHei, QImage::Format aFmt, Qt::TransformationMode aTM = Qt::FastTransformation);

protected:
	virtual void resizeEvent(QResizeEvent*);
	virtual void showEvent(QShowEvent*);
	virtual void mousePressEvent(QMouseEvent*);
	virtual void mouseMoveEvent(QMouseEvent*);
	virtual void mouseReleaseEvent(QMouseEvent*);
	virtual void wheelEvent(QWheelEvent*);

private:
	QMutex	m_mtx;
	QImage* m_img;
	unsigned short* m_origBuf;
	Qt::TransformationMode m_transMode;

	double m_pixInPix; // пикселей из jpg в пикселе экрана
	double m_maxPixInPix;
	double	m_cx; //центр
	double	m_cy; //центр
	double	m_rw;
	double	m_rh;
	double	m_arReal; // aspect ratio real
	double m_xStart;
	double m_yStart;	
	int m_xEnd;
	int m_yEnd;
	int m_destH;
	int m_destW;
	QPoint m_handPoint; // точка начала перемещения
	QString m_sizeStr;
	QString m_positionStr;
	QString m_posValueStr;
	QString m_scaleStr;


	void draw(bool calcPars=true);
	int getX(int ax);
	int getY(int ay);

private slots:
	void showContextMenuForFrame(const QPoint& aPos);
	void exportFrame();

signals:
	void newState(const QString& aSz, const QString& aPos, const QString& aPosVal, const QString& aScale);
};
#endif
