#include "Area.h"
#include <iostream>

Area::Area(const std::string& name,
           const std::shared_ptr<std::size_t>& versionToken,
           const std::shared_ptr<std::function<void()> >& mutationObserver)
    : areaName(name),
      version(versionToken ? versionToken : std::make_shared<std::size_t>(0)),
      observer(mutationObserver ? mutationObserver :
               std::make_shared<std::function<void()> >()) {}

void Area::add(std::unique_ptr<FilmElement> child) {
    Area* nestedArea = dynamic_cast<Area*>(child.get());
    if (nestedArea) nestedArea->setMutationContext(version, observer);
    children.push_back(std::move(child));
    ++(*version);
    if (observer && *observer) (*observer)();
}

bool Area::remove(const std::string& childName) {
    for (std::vector<std::unique_ptr<FilmElement> >::iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->name() == childName) {
            children.erase(it);
            ++(*version);
            if (observer && *observer) (*observer)();
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

void Area::setMutationContext(
    const std::shared_ptr<std::size_t>& versionToken,
    const std::shared_ptr<std::function<void()> >& mutationObserver) {
    version = versionToken ? versionToken : std::make_shared<std::size_t>(0);
    observer = mutationObserver ? mutationObserver : std::make_shared<std::function<void()> >();
    for (std::vector<std::unique_ptr<FilmElement> >::iterator it = children.begin();
         it != children.end(); ++it) {
        Area* nestedArea = dynamic_cast<Area*>(it->get());
        if (nestedArea) nestedArea->setMutationContext(version, observer);
    }
}