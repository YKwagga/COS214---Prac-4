#ifndef CREW_ITERATOR_H
#define CREW_ITERATOR_H

#include "CrewMember.h"
#include <vector>

class CrewIterator {
public:
    explicit CrewIterator(const std::vector<CrewMember*>& members);
    bool hasNext() const;
    CrewMember* next();
private:
    std::vector<CrewMember*> members;
    std::size_t position;
};

#endif