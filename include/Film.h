#ifndef FILM_H
#define FILM_H

#include "Area.h"
#include "CrewRoster.h"
#include "FilmIterator.h"
#include "FilmState.h"
#include <memory>

class Film : public Area {
public:
    explicit Film(const std::string& title);
    virtual ~Film() {}

    void add(std::unique_ptr<FilmElement> child);
    bool confirmAreas();
    bool startShooting();
    bool refreshOperationalState();
    bool finishShooting();
    const std::string& stateName() const;
    FilmIterator createIterator(FilmTraversalMode mode) const;
    CrewRoster& crew();
    const CrewRoster& crew() const;
    void setState(std::unique_ptr<FilmState> state);

private:
    CrewRoster roster;
    std::unique_ptr<FilmState> state;
};

#endif