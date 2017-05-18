#include "Decoder.h"

CDecoder::CDecoder()
{
	reset();
	m_curKadr = 0;
	m_lastKadr = 0;
}
void CDecoder::reset()
{
	m_curPos=0;
	m_syncState = 0;
	m_curPos = 0;
}

bool CDecoder::pushByte(unsigned char b)
{
	switch(m_syncState)
	{
	case 0:		
		if (b==0xA5){
			m_syncState = 1;
			m_kadr[m_curKadr][0] = 0xA5;
			m_curPos=1;
		}
		break;		
	case 1:

		if (b==0x5A) {
			m_syncState = 2;
			m_kadr[m_curKadr][0] = 0x5A;
			m_curPos=2;
		}
		else {
			m_syncState=0;
		}
		break;
	case 2:
		m_kadr[m_curKadr][m_curPos] = b;
		if (m_curPos==4)
			m_length = ((unsigned short)m_kadr[m_curKadr][3]) + ((unsigned short)m_kadr[m_curKadr][4])*256;;		
		m_curPos = ((m_curPos+1)&2047);
		if ((m_curPos>5)&&(m_curPos>=m_length+5)) {
			reset();
			m_lastKadr = m_curKadr;
			m_curKadr = ((m_curKadr+1)&7);
			return true;
		}
		break;
	}
	return false;
}

unsigned char CDecoder::getType()
{
	return m_kadr[m_lastKadr][2];
}

unsigned short CDecoder::getLen()
{
	return ((unsigned short)m_kadr[m_lastKadr][3]) + ((unsigned short)m_kadr[m_lastKadr][4])*256;
}

const unsigned char* CDecoder::getData()
{
	return &m_kadr[m_lastKadr][5];
}
