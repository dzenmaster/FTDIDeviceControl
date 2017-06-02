#include "ImageLabel.h"
#include <QWheelEvent>

CImageLabel::CImageLabel(QWidget * parent, Qt::WindowFlags f)
	: QLabel(parent, f), m_img(0), m_pixInPix(1),
	m_cx(0), m_cy(0), m_rw(0), m_rh(0), m_arReal(1),
	m_transMode(Qt::FastTransformation),
	m_xStart(0),m_yStart(0)
{	
	m_sizeStr = "0x0";
	m_positionStr = "0x0";
	m_posValueStr = "0x0";
	m_scaleStr = "100%";
}

void CImageLabel::setRawBuffer(const unsigned char* aBuf, int aWid, int aHei, QImage::Format aFmt, Qt::TransformationMode aTM)
{
	m_transMode = aTM;
	if (m_img)
		delete m_img;

	m_rh = aHei;
	m_rw = aWid;	
	m_img = new QImage(aWid, aHei, aFmt);
	int tPixLen = 1; // mono8
	if (aFmt==QImage::Format_RGB888)
		tPixLen=3;
	for(int i = 0; i < aHei; ++i)	
		memcpy(m_img->scanLine(i), &aBuf[i* aWid * tPixLen], m_img->bytesPerLine());	
	m_arReal = m_rw/m_rh;
	m_sizeStr=QString("%1x%2").arg(aWid).arg(aHei);
	draw();
}

void CImageLabel::draw(bool calcPars)
{
	if (!m_img)
		return;
	double h = size().height();
	double w = size().width();
	
	if (calcPars) {
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
	m_xStart = m_cx - ww/2.;
	m_yStart = m_cy - hw/2.;
	if (m_xStart < 0)
		m_xStart = 0;
	else if (m_xStart > m_rw - ww)
		m_xStart = m_rw - ww;
	if (m_yStart < 0)
		m_yStart = 0;
	else if (m_yStart > m_rh - hw)
		m_yStart = m_rh - hw;	
	
	QPixmap pix = QPixmap::fromImage(*m_img).copy( m_xStart, m_yStart, ww, hw).scaled(w, h, Qt::KeepAspectRatio, m_transMode);	

	setPixmap(pix);
	m_scaleStr=QString("%1%").arg(m_pixInPix*100/m_maxPixInPix,0,'f',0);
	emit  newState(m_sizeStr, m_positionStr, m_posValueStr, m_scaleStr);
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
	if (!m_mtx.tryLock())
		return;	
	QPoint tp = e->pos();
	QPoint tGap = m_handPoint-tp;
	m_handPoint = tp;
	if (e->buttons() & Qt::LeftButton) {
		double h2 = size().height() * m_pixInPix/2;
		double w2 = size().width() * m_pixInPix/2;
		m_cx = m_cx + tGap.x() * m_pixInPix; 
		if (m_cx < w2)
			m_cx = w2;
		if (m_cx > m_rw - 1 - w2)
			m_cx = m_rw - 1 - w2;
		m_cy = m_cy + tGap.y() * m_pixInPix;
		if (m_cy < h2)
			m_cy = h2;
		if (m_cy > m_rh - 1 - h2)
			m_cy = m_rh - 1 - h2;
		draw(false);
	}

	m_positionStr = QString("%1x%2").arg(getX(tp.x())).arg(getY(tp.y()));
	emit  newState(m_sizeStr, m_positionStr, m_posValueStr, m_scaleStr);
	m_mtx.unlock();
}

void CImageLabel::mouseReleaseEvent(QMouseEvent * e)
{
	m_mtx.lock();
	if (e->button() &  Qt::LeftButton) 
		setCursor(QCursor(Qt:: ArrowCursor));			
	m_mtx.unlock();
}

void CImageLabel::wheelEvent(QWheelEvent * event)
{
	if (!m_mtx.tryLock())
		return;
	if (event->angleDelta().y()<0)
		m_pixInPix*=1.3;
	else
		m_pixInPix*=0.7;
	if (m_pixInPix > m_maxPixInPix)
		m_pixInPix = m_maxPixInPix;
	else if (m_pixInPix < 0.09)
		m_pixInPix = 0.1;	
	draw(false);
	m_mtx.unlock();
}

int CImageLabel::getY(int ay)
{

	return 1;
}

int CImageLabel::getX(int ax)
{
	double h = size().height();
	double w = size().width();
	
	double arScr = w/h;
	if (m_arReal < arScr) {
		double ww = w*m_pixInPix;
		if (ww > m_rw) {
			int tX = m_xStart + ax * m_pixInPix - ((w*m_pixInPix - m_rw)/2);
			return   tX;
		}
	}	
	return  m_xStart + ax * m_pixInPix;		
}