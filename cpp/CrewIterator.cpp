#include "CrewIterator.h"
#include <stdexcept>

CrewIterator::CrewIterator(const std::vector<CrewMember*>& source)
    : members(source), position(0) {}

bool CrewIterator::hasNext() const { return position < members.size(); }

CrewMember* CrewIterator::next() {
    if (!hasNext()) throw std::out_of_range("crew iterator exhausted");
    return members[position++];
}