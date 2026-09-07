#include "Film.h"
#include <iostream>

// State names below are kept identical to StateDPrac4.drawio:
//   NotShooting -> ReadyForShoot -> Shooting -> OtherMissing -> Shooting
//   Shooting -> Cancelled (terminal, when a VIP goes missing/is injured)
// "Finished" is the normal completion path; it is not shown separately in
// the state diagram but does not conflict with any labelled state there.
namespace {
class NotShootingState : public FilmState {
public:
    const std::string& name() const { static const std::string value = "NotShooting"; return value; }
    bool confirmAreas(Film& film) { return refresh(film); }
    bool startShooting(Film&) { std::cout << "Cannot start: areas are not confirmed.\n"; return false; }
    bool refresh(Film& film);
    bool finish(Film&) { std::cout << "Cannot finish: shooting has not started.\n"; return false; }
};

class ReadyForShootState : public FilmState {
public:
    const std::string& name() const { static const std::string value = "ReadyForShoot"; return value; }
    bool confirmAreas(Film&) { std::cout << "Areas are already confirmed.\n"; return true; }
    bool startShooting(Film& film);
    bool refresh(Film&) { return true; }
    bool finish(Film&) { std::cout << "Cannot finish: shooting has not started.\n"; return false; }
};

class ShootingState : public FilmState {
public:
    const std::string& name() const { static const std::string value = "Shooting"; return value; }
    bool confirmAreas(Film&) { std::cout << "Areas are already confirmed.\n"; return true; }
    bool startShooting(Film&) { std::cout << "Shooting is already in progress.\n"; return false; }
    bool refresh(Film& film);
    bool finish(Film& film);
};

class OtherMissingState : public FilmState {
public:
    const std::string& name() const { static const std::string value = "OtherMissing"; return value; }
    bool confirmAreas(Film&) { return false; }
    bool startShooting(Film&) { std::cout << "Cannot shoot: required crew is missing.\n"; return false; }
    bool refresh(Film& film);
    bool finish(Film&) { std::cout << "Cannot finish: required crew is missing.\n"; return false; }
};

// Terminal state: reached when a VIP crew member goes missing/is injured
// during shooting. Matches ActDiagram3.drawio, where a VIP injury routes
// straight to "Cancel shoot" and an end node -- there is no resume path.
class CancelledState : public FilmState {
public:
    const std::string& name() const { static const std::string value = "Cancelled"; return value; }
    bool confirmAreas(Film&) { std::cout << "Cannot confirm areas: the shoot was cancelled.\n"; return false; }
    bool startShooting(Film&) { std::cout << "Cannot start: the shoot was cancelled.\n"; return false; }
    bool refresh(Film&) { return false; }
    bool finish(Film&) { std::cout << "Cannot finish: the shoot was cancelled.\n"; return false; }
};

class FinishedState : public FilmState {
public:
    const std::string& name() const { static const std::string value = "Finished"; return value; }
    bool confirmAreas(Film&) { std::cout << "Film is finished.\n"; return false; }
    bool startShooting(Film&) { std::cout << "Film is finished.\n"; return false; }
    bool refresh(Film&) { return true; }
    bool finish(Film&) { std::cout << "Film is already finished.\n"; return false; }
};

bool NotShootingState::refresh(Film& film) {
    FilmIterator locations = film.createIterator(FilmTraversalMode::LocationsNeedingClearance);
    if (locations.hasNext()) {
        std::cout << "Area confirmation pending: " << locations.next()->name() << " needs clearance.\n";
        return false;
    }
    if (!film.crew().allPresent()) {
        std::cout << "Area confirmation pending: required crew is absent.\n";
        return false;
    }
    film.setState(std::unique_ptr<FilmState>(new ReadyForShootState()));
    std::cout << "All areas confirmed. Film is ready for shooting.\n";
    return true;
}

bool ReadyForShootState::startShooting(Film& film) {
    film.setState(std::unique_ptr<FilmState>(new ShootingState()));
    std::cout << "Shooting started.\n";
    return true;
}

bool ShootingState::refresh(Film& film) {
    if (film.crew().missingVip()) {
        film.setState(std::unique_ptr<FilmState>(new CancelledState()));
        std::cout << "Shoot cancelled: a VIP crew member is missing.\n";
        return false;
    }
    if (film.crew().missingOther()) {
        film.setState(std::unique_ptr<FilmState>(new OtherMissingState()));
        std::cout << "State changed: other required crew missing.\n";
        return false;
    }
    return true;
}

bool ShootingState::finish(Film& film) {
    if (!refresh(film)) return false;
    if (!film.isFilmed()) {
        std::cout << "Cannot finish: not every location has been filmed.\n";
        return false;
    }
    film.setState(std::unique_ptr<FilmState>(new FinishedState()));
    std::cout << "Film finished.\n";
    return true;
}

bool OtherMissingState::refresh(Film& film) {
    if (film.crew().missingVip()) {
        film.setState(std::unique_ptr<FilmState>(new CancelledState()));
        std::cout << "Shoot cancelled: a VIP crew member is missing.\n";
        return false;
    }
    if (film.crew().missingOther()) return false;
    film.setState(std::unique_ptr<FilmState>(new ShootingState()));
    std::cout << "Required crew returned. Shooting can continue.\n";
    return true;
}
}

Film::Film(const std::string& title)
    : Area(title), state(new NotShootingState()) {}

void Film::add(std::unique_ptr<FilmElement> child) {
    Area::add(std::move(child));
    const std::string& current = stateName();
    if (current == "ReadyForShoot" || current == "Shooting" || current == "OtherMissing")
        state.reset(new NotShootingState());
}

bool Film::confirmAreas() { return state->confirmAreas(*this); }
bool Film::startShooting() { return state->startShooting(*this); }
bool Film::refreshOperationalState() { return state->refresh(*this); }
bool Film::finishShooting() { return state->finish(*this); }
const std::string& Film::stateName() const { return state->name(); }
FilmIterator Film::createIterator(FilmTraversalMode mode) const { return FilmIterator(*this, mode); }
CrewRoster& Film::crew() { return roster; }
const CrewRoster& Film::crew() const { return roster; }
void Film::setState(std::unique_ptr<FilmState> nextState) { state = std::move(nextState); }
