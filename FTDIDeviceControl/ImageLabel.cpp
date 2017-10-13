#include "ImageLabel.h"
#include <QWheelEvent>
#include <QImage>
#include <QMenu>
#include <QFileDialog>

CImageLabel::CImageLabel(QWidget * parent, Qt::WindowFlags f)
	: QLabel(parent, f), m_img(0), m_pixInPix(1),
	m_cx(0), m_cy(0), m_rw(0), m_rh(0), m_arReal(1),
	m_transMode(Qt::FastTransformation),
	m_xStart(0),m_yStart(0),m_maxPixInPix(1)
{	
	m_sizeStr = "0x0";
	m_positionStr = "0x0";
	m_posValueStr = "0x0";
	m_scaleStr = "100%";
	m_xEnd = 0;
	m_yEnd = 0;
	m_destH = 0;
	m_destW = 0;
	m_origBuf = 0;

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showContextMenuForFrame(const QPoint&)));

}

void CImageLabel::setRawBuffer(const unsigned char* aBuf, const unsigned short* aOrigBuf, int aWid, int aHei, QImage::Format aFmt, Qt::TransformationMode aTM)
{
	if (m_origBuf){
		delete[] m_origBuf;
		m_origBuf = 0;
	}
	if (aOrigBuf){
		m_origBuf = new unsigned short[aWid*aHei];
		memcpy(m_origBuf,aOrigBuf,aWid*aHei*sizeof(unsigned short));
	}

	m_transMode = aTM;
//	if (m_img)
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
	double arScr = w/h;
	if (calcPars) {		
		if (m_arReal<arScr)
			m_pixInPix = m_rh/h;
		else
			m_pixInPix = m_rw/w;
		m_maxPixInPix = m_pixInPix;
		m_cx = m_rw/2;
		m_cy = m_rh/2;	
	}
	int ww = w*m_pixInPix;
	int hw = h*m_pixInPix;
	m_xStart = m_cx - ww/2.;
	m_yStart = m_cy - hw/2.;
	if (m_xStart < 0)
		m_xStart = 0;
	else if (m_xStart > m_rw - ww - 0.5)
		m_xStart = m_rw - ww;
	if (m_yStart < 0)
		m_yStart = 0;
	else if (m_yStart > m_rh - hw - 0.5)
		m_yStart = m_rh - hw;	

	QPixmap pix = QPixmap::fromImage(*m_img).copy( m_xStart, m_yStart, ww, hw);
	
	m_destH = h;
	m_destW = w;

	if (m_arReal<arScr)
		m_destW = pix.width() / m_pixInPix + 0.5;	
	else
		m_destH = pix.height() / m_pixInPix + 0.5;

	m_xEnd = m_xStart + pix.width() - 1;
	m_yEnd = m_yStart + pix.height() - 1;

	if (ww<m_rw)//precise secure
		m_destW = w;
	if (hw<m_rh)//precise secure
		m_destH = h;

	QPixmap pix2 = pix.scaled(m_destW, m_destH, Qt::IgnoreAspectRatio, m_transMode);	

	setPixmap(pix2);
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
	if (!m_img)
		return;
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
		if (m_cx > m_rw - 0.5 - w2)
			m_cx = m_rw - w2;
		m_cy = m_cy + tGap.y() * m_pixInPix;
		if (m_cy < h2)
			m_cy = h2;
		if (m_cy > m_rh - 0.5 - h2)
			m_cy = m_rh - h2;
		draw(false);
	}

	int tCurX = getX(tp.x());
	int tCurY = getY(tp.y());
	
	//const uchar* b1 = m_img->bits();
	int tInd=tCurX+tCurY*m_img->width();
	if ((m_origBuf)&&(tInd>-1)&&(tInd<m_img->width()*m_img->height()))
		m_posValueStr = QString("%1 0x%2").arg(m_origBuf[tInd]).arg(m_origBuf[tInd],0, 16);


	m_positionStr = QString("%1x%2").arg(tCurX + 1).arg(tCurY + 1);	
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
	double h = size().height();

	double startP = (h - m_destH)/2;
	double endP = h - (h - m_destH)/2 - 1;

	if (ay<startP)
		return m_yStart;
	if (ay>endP)
		return m_yEnd;
	int tY = m_yStart + (ay-startP) * m_pixInPix;
	if (tY>m_yEnd)
		return m_yEnd;
	return   tY;
}

int CImageLabel::getX(int ax)
{
	double w = size().width();

	double startP = (w - m_destW)/2;
	double endP = w - (w - m_destW)/2 - 1;

	if (ax<startP)
		return m_xStart;
	if (ax>endP)
		return m_xEnd;
	int tX = m_xStart + (ax-startP) * m_pixInPix;
	if (tX>m_xEnd)
		return m_xEnd;
	return   tX;
}

void  CImageLabel::showContextMenuForFrame(const QPoint& aPos)
{
	if (!m_img)
		return;
	QPoint globalPos = mapToGlobal(aPos);
	QMenu myMenu;
	myMenu.addAction("Экспорт", this, SLOT(exportFrame()));
	myMenu.exec(globalPos);
}

void  CImageLabel::exportFrame()//JPEG BMP
{
	if (!m_img)
		return;
	QString tFileName = QFileDialog::getSaveFileName(this, "Сохранить", "", "BMP (*.bmp);;JPEG (*.jpg)");
	if (tFileName.isEmpty())
		return;
	QImage tImage = m_img->convertToFormat(QImage::Format_RGB888);
	if (tFileName.right(3).compare("JPG",Qt::CaseInsensitive)==0)
		bool res = tImage.save(tFileName,"JPG",85);	
	else
		bool res = tImage.save(tFileName,"BMP");
}