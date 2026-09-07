#include "CrewRoster.h"

void CrewRoster::add(std::unique_ptr<CrewMember> member) { members.push_back(std::move(member)); }

CrewIterator CrewRoster::iterator() const {
    std::vector<CrewMember*> result;
    for (std::vector<std::unique_ptr<CrewMember> >::const_iterator it = members.begin(); it != members.end(); ++it)
        result.push_back(it->get());
    return CrewIterator(result);
}

CrewMember* CrewRoster::find(const std::string& memberName) const {
    CrewIterator it = iterator();
    while (it.hasNext()) {
        CrewMember* member = it.next();
        if (member->name() == memberName) return member;
    }
    return nullptr;
}

bool CrewRoster::replace(const std::string& memberName,
                         std::unique_ptr<CrewMember> replacement) {
    if (!replacement) return false;
    for (std::vector<std::unique_ptr<CrewMember> >::iterator it = members.begin();
         it != members.end(); ++it) {
        if ((*it)->name() == memberName) {
            *it = std::move(replacement);
            return true;
        }
    }
    return false;
}

bool CrewRoster::allPresent() const {
    CrewIterator it = iterator();
    while (it.hasNext()) {
        CrewMember* member = it.next();
        if (member->requiredOnSite() && !member->isPresent()) return false;
    }
    return true;
}

bool CrewRoster::missingVip() const {
    CrewIterator it = iterator();
    while (it.hasNext()) {
        CrewMember* member = it.next();
        if (member->isVip() && !member->isPresent()) return true;
    }
    return false;
}

bool CrewRoster::missingOther() const {
    CrewIterator it = iterator();
    while (it.hasNext()) {
        CrewMember* member = it.next();
        if (!member->isVip() && member->requiredOnSite() && !member->isPresent()) return true;
    }
    return false;
}

bool CrewRoster::contains(const std::string& memberName) const {
    CrewIterator it = iterator();
    while (it.hasNext()) if (it.next()->name() == memberName) return true;
    return false;
}