#ifndef REDBASE_PF_HASHTABLE_H
#define REDBASE_PF_HASHTABLE_H

#include <list>
#include <vector>
#include "pf_error.h"

struct Triple {
    Triple(int fd, int num, int slot) : fd(fd), num(num), slot(slot) {}
    int fd ;
    int num ;
    int slot ;
};

class PFHashTable {
public:
    PFHashTable(unsigned int capacity);
    ~PFHashTable() {}
public:
    int search(int fd, int num);
    bool insert(int fd, int num, int slot);
    bool remove(int fd, int num);
private:
    int calcHash(int fd, int num)
    {
        return (fd + num) % capacity;
    }
private:
    unsigned int capacity;
    std::vector<std::list<Triple>> table;
};;


#endif //REDBASE_PF_HASHTABLE_H
