#ifndef FILM_ITERATOR_H
#define FILM_ITERATOR_H

#include "FilmElement.h"
#include <vector>

enum class FilmTraversalMode { AllElements, LocationsNeedingClearance };

class FilmIterator {
public:
    FilmIterator(const FilmElement& root, FilmTraversalMode mode);
    bool hasNext() const;
    FilmElement* next();
    bool isInvalidated() const;

private:
    const FilmElement& root;
    FilmTraversalMode mode;
    std::vector<FilmElement*> snapshot;
    std::size_t position;
    std::size_t expectedVersion;
};

#endif