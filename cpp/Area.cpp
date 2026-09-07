#include "Area.h"
#include <iostream>

Area::Area(const std::string& name, const std::shared_ptr<std::size_t>& versionToken)
    : areaName(name), version(versionToken ? versionToken : std::make_shared<std::size_t>(0)) {}

void Area::add(std::unique_ptr<FilmElement> child) {
    children.push_back(std::move(child));
    ++(*version);
}

bool Area::remove(const std::string& childName) {
    for (std::vector<std::unique_ptr<FilmElement> >::iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->name() == childName) {
            children.erase(it);
            ++(*version);
            return true;
        }
    }
    return false;
}

const std::string& Area::name() const { return areaName; }
bool Area::isLocation() const { return false; }

bool Area::needsClearance() const {
    for (std::vector<std::unique_ptr<FilmElement> >::const_iterator it = children.begin(); it != children.end(); ++it)
        if ((*it)->needsClearance()) return true;
    return false;
}

void Area::clearForShooting() {
    for (std::vector<std::unique_ptr<FilmElement> >::iterator it = children.begin(); it != children.end(); ++it)
        (*it)->clearForShooting();
}

void Area::markFilmed() {
    for (std::vector<std::unique_ptr<FilmElement> >::iterator it = children.begin(); it != children.end(); ++it)
        (*it)->markFilmed();
}

bool Area::isFilmed() const {
    if (children.empty()) return false;
    for (std::vector<std::unique_ptr<FilmElement> >::const_iterator it = children.begin(); it != children.end(); ++it)
        if (!(*it)->isFilmed()) return false;
    return true;
}

void Area::print(std::size_t depth) const {
    std::cout << std::string(depth * 2, ' ') << "Area: " << areaName << "\n";
    for (std::vector<std::unique_ptr<FilmElement> >::const_iterator it = children.begin(); it != children.end(); ++it)
        (*it)->print(depth + 1);
}

std::shared_ptr<std::size_t> Area::versionToken() const { return version; }

void Area::appendChildren(std::vector<FilmElement*>& output) const {
    for (std::vector<std::unique_ptr<FilmElement> >::const_iterator it = children.begin(); it != children.end(); ++it)
        output.push_back(it->get());
}