#include "FilmIterator.h"
#include <stdexcept>

FilmIterator::FilmIterator(const FilmElement& rootElement, FilmTraversalMode traversalMode)
    : root(rootElement), mode(traversalMode), position(0), expectedVersion(0) {
    std::shared_ptr<std::size_t> token = root.versionToken();
    if (token) expectedVersion = *token;
    std::vector<FilmElement*> work;
    work.push_back(const_cast<FilmElement*>(&root));
    while (!work.empty()) {
        FilmElement* current = work.back();
        work.pop_back();
        if (mode == FilmTraversalMode::AllElements ||
            (mode == FilmTraversalMode::LocationsNeedingClearance && current->isLocation() && current->needsClearance()))
            snapshot.push_back(current);
        std::vector<FilmElement*> children;
        current->appendChildren(children);
        for (std::vector<FilmElement*>::reverse_iterator it = children.rbegin(); it != children.rend(); ++it)
            work.push_back(*it);
    }
}

bool FilmIterator::isInvalidated() const {
    std::shared_ptr<std::size_t> token = root.versionToken();
    return token && *token != expectedVersion;
}

bool FilmIterator::hasNext() const {
    if (isInvalidated()) return false;
    return position < snapshot.size();
}

FilmElement* FilmIterator::next() {
    if (isInvalidated()) throw std::runtime_error("film iterator invalidated by structural change");
    if (!hasNext()) throw std::out_of_range("film iterator exhausted");
    return snapshot[position++];
}