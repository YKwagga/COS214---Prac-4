#ifndef CREW_ROSTER_H
#define CREW_ROSTER_H

#include "CrewIterator.h"
#include <memory>
#include <vector>

class CrewRoster {
public:
    void add(std::unique_ptr<CrewMember> member);
    CrewIterator iterator() const;
    CrewMember* find(const std::string& memberName) const;
    bool replace(const std::string& memberName, std::unique_ptr<CrewMember> replacement);
    bool allPresent() const;
    bool missingVip() const;
    bool missingOther() const;
    bool contains(const std::string& memberName) const;
private:
    std::vector<std::unique_ptr<CrewMember> > members;
};

#endif