#include "Location.h"
#include <iostream>

Location::Location(const std::string& name, bool initiallyCleared)
    : locationName(name), cleared(initiallyCleared), filmed(false) {}

const std::string& Location::name() const { return locationName; }
bool Location::isLocation() const { return true; }
bool Location::needsClearance() const { return !cleared; }
void Location::clearForShooting() { cleared = true; }
void Location::markFilmed() { if (cleared) filmed = true; }
bool Location::isFilmed() const { return filmed; }

void Location::print(std::size_t depth) const {
    std::cout << std::string(depth * 2, ' ') << "Location: " << locationName
              << " [" << (cleared ? "cleared" : "needs clearance") << "]\n";
}

std::shared_ptr<std::size_t> Location::versionToken() const {
    return std::shared_ptr<std::size_t>();
}

void Location::appendChildren(std::vector<FilmElement*>&) const {}