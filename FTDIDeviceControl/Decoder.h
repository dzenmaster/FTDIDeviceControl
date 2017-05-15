#ifndef FTDIDECODER_H
#define FTDIDECODER_H

class CDecoder
{
public:
	CDecoder();
	bool pushByte(unsigned char b); // return if full kadr
	void reset();
	unsigned char getType();
	unsigned short getLen();
	const unsigned char* getData();

private:
	int m_syncState;
	unsigned char m_kadr[8][2048];
	unsigned short m_curKadr;
	unsigned short m_lastKadr;
	unsigned short m_curPos;
	unsigned short m_length;
};

#endif // FTDIDECODER_H
