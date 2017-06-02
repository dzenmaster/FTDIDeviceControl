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
	void setRawBuffer(const unsigned char* buffer, int width,int height);

protected:
	virtual void resizeEvent(QResizeEvent * event);
	virtual void showEvent(QShowEvent * event);
	virtual void mousePressEvent(QMouseEvent * event);
	virtual void mouseMoveEvent(QMouseEvent * event);
	virtual void mouseReleaseEvent(QMouseEvent * event);
	virtual void wheelEvent(QWheelEvent * event);

private:
	QImage* m_img;

	double m_pixInPix;//пикселей из jpg в пикселе экрана
	double m_maxPixInPix;
	double	m_cx; //центр
	double	m_cy; //центр
	double	m_rw;
	double	m_rh;
	double	m_arReal;
	QMutex	m_mtx;

	QPoint m_handPoint;// точка начала перемещения
	bool m_moveEventInProgress;

	void draw(bool calcPars=true);
};


#endif
