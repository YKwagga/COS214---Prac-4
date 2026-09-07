#ifndef FILM_STATE_H
#define FILM_STATE_H

#include <string>
#include <memory>

class Film;
class CrewMember;

class FilmState {
public:
    virtual ~FilmState() {}
    virtual const std::string& name() const = 0;
    virtual bool confirmAreas(Film& film) = 0;
    virtual bool startShooting(Film& film) = 0;
    virtual bool refresh(Film& film) = 0;
    virtual bool finish(Film& film) = 0;
    virtual bool contactAuthorities(Film& film) = 0;
    virtual bool reportInjury(Film& film, const std::string& memberName) = 0;
    virtual bool replaceCrewMember(Film& film, const std::string& memberName,
                                   std::unique_ptr<CrewMember> replacement) = 0;
    virtual bool resumeShooting(Film& film) = 0;
};

#endif