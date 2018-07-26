#include "pf_hashtable.h"

PFHashTable::PFHashTable(unsigned int capacity_) : capacity(capacity_), table(capacity_)
{

}


int PFHashTable::search(int fd, int num)
{
    int key = calcHash(fd, num);
    if (key < 0)
        return -1;
    std::list<Triple>& lst = table[key];
    std::list<Triple>::const_iterator it;
    for (it = lst.begin(); it != lst.end(); it++)
    {
        if ((it->fd == fd) && (it->num == num))
            return it->slot;
    }
    return -1;
}



bool PFHashTable::insert(int fd, int num, int slot)
{
    int key = calcHash(fd, num);
    if (key < 0)
        return false;
    std::list<Triple>& lst = table[key];
    std::list<Triple>::const_iterator it;
    for (it = lst.begin(); it != lst.end(); it++)
    {
        if ((it->fd == fd) && (it->num == num))
            return false;
    }
    lst.push_back(Triple(fd, num, slot));
    return true;
}


bool PFHashTable::remove(int fd, int num)
{
    int key = calcHash(fd, num);
    if (key < 0) return false;
    std::list<Triple>& lst = table[key];
    std::list<Triple>::const_iterator it;

    for (it = lst.begin(); it != lst.end(); it++) {
        if ((it->fd == fd) && (it->num == num)) {
            lst.erase(it);
            return true;
        }
    }
    return false;
}