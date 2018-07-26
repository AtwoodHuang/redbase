#include "bitmap.h"
#include <cstring>

bitmap::bitmap(int numBits): size(numBits)
{
    buffer = new char[this->numChars()];
    //所有位置为0
    memset((void*)buffer, 0, this->numChars());
    this->reset();
}

bitmap::bitmap(char * buf, int numBits): size(numBits)
{
    buffer = new char[this->numChars()];
    memcpy(buffer, buf, this->numChars());
}

int bitmap::to_char_buf(char * b, int len) const //将bitmap给b
{
    memcpy((void*)b, buffer, len);
    return 0;
}

bitmap::~bitmap()
{
    delete [] buffer;
}

//返回bitmap所需char数
int bitmap::numChars() const
{
    int numChars = (size / 8);
    if((size % 8) != 0)
        numChars++;
    return numChars;
}

void bitmap::reset()
{
    for( unsigned int i = 0; i < size; i++) {
        bitmap::reset(i);
    }
}

//设为0
void bitmap::reset(unsigned int bitNumber)
{
    int byte = bitNumber/8;
    int offset = bitNumber%8;

    buffer[byte] &= ~(1 << offset); //从0开始，所以要左移一位
}

//设为1
void bitmap::set(unsigned int bitNumber)
{
    int byte = bitNumber/8;
    int offset = bitNumber%8;

    buffer[byte] |= (1 << offset);
}

void bitmap::set()
{
    for( unsigned int i = 0; i < size; i++) {
        bitmap::set(i);
    }
}

//某位是否为1
bool bitmap::test(unsigned int bitNumber) const
{
    int byte = bitNumber/8;
    int offset = bitNumber%8;

    return buffer[byte] & (1 << offset);
}


std::ostream& operator <<(std::ostream & os, const bitmap& b)
{
    os << "[";
    for(int i=0; i < b.getSize(); i++)
    {
        if( i % 8 == 0 && i != 0 )
            os << ".";
        os << (b.test(i) ? 1 : 0);
    }
    os << "]";
    return os;
}
