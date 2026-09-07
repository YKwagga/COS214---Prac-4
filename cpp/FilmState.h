#ifndef FILM_STATE_H
#define FILM_STATE_H

#include <string>

class Film;

class FilmState {
public:
    virtual ~FilmState() {}
    virtual const std::string& name() const = 0;
    virtual bool confirmAreas(Film& film) = 0;
    virtual bool startShooting(Film& film) = 0;
    virtual bool refresh(Film& film) = 0;
    virtual bool finish(Film& film) = 0;
};

#endif