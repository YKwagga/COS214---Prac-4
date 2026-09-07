#ifndef CREW_ROSTER_H
#define CREW_ROSTER_H

#include "CrewIterator.h"
#include <memory>
#include <vector>

class CrewRoster {
public:
    void add(std::unique_ptr<CrewMember> member);
    CrewIterator iterator() const;
    bool allPresent() const;
    bool missingVip() const;
    bool missingOther() const;
private:
    std::vector<std::unique_ptr<CrewMember> > members;
};

#endif