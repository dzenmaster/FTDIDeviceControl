#include "ImageLabel.h"

#include <QMouseEvent>
#include <QWheelEvent>

CImageLabel::CImageLabel(QWidget * parent, Qt::WindowFlags f)
	: QLabel(parent,f),m_img(0)
{
	m_moveEventInProgress = false;
	m_pixInPix = 1;
	m_cx = m_cy = m_rw = m_rh= 0; 
	m_arReal = 1;
}

void CImageLabel::setRawBuffer(const unsigned char* buffer, int width,int height, QImage::Format fmt)
{
	if (m_img)
		delete m_img;
	m_rh = height;
	m_rw = width;	
	m_img = new QImage(width, height, fmt);
	int tPixLen = 1;
	if (fmt==QImage::Format_RGB888)
		tPixLen=3;
	for(int i = 0; i < m_img->height(); ++i)
	{
		memcpy(m_img->scanLine(i), &buffer[i* width * tPixLen], m_img->bytesPerLine());
	}

	m_arReal = m_rw/m_rh;

	draw();
}

void CImageLabel::draw(bool calcPars)
{
	if(!m_img)
		return;
	double h = size().height();
	double w = size().width();

	if (calcPars){
		double arScr = w/h;

		if (m_arReal<arScr)
			m_pixInPix = m_rh/h;
		else
			m_pixInPix = m_rw/w;
		m_maxPixInPix = m_pixInPix;
		m_cx = m_rw/2;
		m_cy = m_rh/2;
	}
	double ww = w*m_pixInPix;
	double hw = h*m_pixInPix;
	double xStart = m_cx - ww/2.;
	double yStart = m_cy - hw/2.;
	if (xStart < 0)
		xStart = 0;
	if (xStart > m_rw-ww)
		xStart = m_rw-ww;
	if (yStart < 0)
		yStart = 0;
	if (yStart > m_rh-hw)
		yStart = m_rh-hw;	

	QPixmap pix = QPixmap::fromImage(*m_img).copy( xStart, yStart, ww, hw).scaled(w, h, Qt::KeepAspectRatio, Qt::FastTransformation/*SmoothTransformation*/);

	setPixmap(pix);
}

void	CImageLabel::resizeEvent(QResizeEvent * event)
{	
	draw();
}

void	CImageLabel::showEvent(QShowEvent * event)
{
	draw();
}

void CImageLabel::mousePressEvent(QMouseEvent * e)
{	
	m_mtx.lock();
	if (e->button() &  Qt::LeftButton) {
		setCursor(QCursor(Qt::ClosedHandCursor));
		m_handPoint = e->pos();		
	}

	m_mtx.unlock();
}

void CImageLabel::mouseMoveEvent(QMouseEvent * e)
{
	if (!m_mtx.tryLock()){
		return;
	}
	if (m_moveEventInProgress)
		return;
	m_moveEventInProgress=true;
	QPoint tp = e->pos();
	QPoint tGap = m_handPoint-tp;
	m_handPoint = tp;
	if (e->buttons() & Qt::LeftButton) {
		double h2 = size().height()*m_pixInPix/2;
		double w2 = size().width()*m_pixInPix/2;
		m_cx = m_cx + tGap.x() * m_pixInPix; 
		if (m_cx < w2)
			m_cx = w2;
		if (m_cx > m_rw-1-w2)
			m_cx = m_rw-1-w2;
		m_cy = m_cy + tGap.y() * m_pixInPix;
		if (m_cy < h2)
			m_cy = h2;
		if (m_cy > m_rh-1-h2)
			m_cy = m_rh-1-h2;
		draw(false);
	}	
			
	m_moveEventInProgress=false;
	m_mtx.unlock();
}

void CImageLabel::mouseReleaseEvent(QMouseEvent * e)
{
	m_mtx.lock();
	if (e->button() &  Qt::LeftButton) {
		setCursor(QCursor(Qt:: ArrowCursor));		
	}
	m_mtx.unlock();
}

void CImageLabel::wheelEvent(QWheelEvent * event)
{
	if (!m_mtx.tryLock())
		return;
	double h = size().height();
	double w = size().width();

	double px=event->pos().x() - w/2;
	double py=event->pos().y() - h/2;

	double prevCX=m_cx;
	double prevCY=m_cy;

	m_cx = m_cx + px * m_pixInPix; 
	if (m_cx < 0)
		m_cx = 0;
	if (m_cx > m_rw-1)
		m_cx = m_rw-1;
	m_cy = m_cy + py * m_pixInPix;
	if (m_cy < 0)
		m_cy = 0;
	if (m_cy > m_rh-1)
		m_cy = m_rh-1;

	QPoint ad= event->angleDelta();
	
	if (ad.y()<0)
		m_pixInPix*=1.3;
	else
		m_pixInPix*=0.7;
	if (m_pixInPix > m_maxPixInPix)
		m_pixInPix = m_maxPixInPix;
	if (m_pixInPix < 0.09){
		m_pixInPix = 0.1;
		m_cx = prevCX;
		m_cy = prevCY;
	}
	draw(false);
	m_mtx.unlock();
}