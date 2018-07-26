#ifndef REDBASE_BITMAP_H
#define REDBASE_BITMAP_H

#include <iostream>

class bitmap {
public:
    bitmap(int numBits);
    bitmap(char * buf, int numBits); //将buff传入
    ~bitmap();

    void set(unsigned int bitNumber);//
    void set(); // 所有位都置为1
    void reset(unsigned int bitNumber);
    void reset(); // 所有位置为0
    bool test(unsigned int bitNumber) const;

    int numChars() const; // bitmap需要多少char
    int to_char_buf(char *, int len) const; //bitmap to char
    int getSize() const { return size; }
private:
    unsigned int size;
    char * buffer;
};

std::ostream& operator <<(std::ostream & os, const bitmap& b);


#endif //REDBASE_BITMAP_H
