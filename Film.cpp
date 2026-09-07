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
    bool contactAuthorities(Film& film);
    bool reportInjury(Film& film, const std::string& memberName);
    bool replaceCrewMember(Film& film, const std::string& memberName,
                           std::unique_ptr<CrewMember> replacement);
    bool resumeShooting(Film&) { std::cout << "Cannot resume: shooting has not started.\n"; return false; }
};

class ReadyForShootState : public FilmState {
public:
    const std::string& name() const { static const std::string value = "ReadyForShoot"; return value; }
    bool confirmAreas(Film&) { std::cout << "Areas are already confirmed.\n"; return true; }
    bool startShooting(Film& film);
    bool refresh(Film&) { return true; }
    bool finish(Film&) { std::cout << "Cannot finish: shooting has not started.\n"; return false; }
    bool contactAuthorities(Film& film);
    bool reportInjury(Film& film, const std::string& memberName);
    bool replaceCrewMember(Film& film, const std::string& memberName,
                           std::unique_ptr<CrewMember> replacement);
    bool resumeShooting(Film& film);
};

class ShootingState : public FilmState {
public:
    const std::string& name() const { static const std::string value = "Shooting"; return value; }
    bool confirmAreas(Film&) { std::cout << "Areas are already confirmed.\n"; return true; }
    bool startShooting(Film&) { std::cout << "Shooting is already in progress.\n"; return false; }
    bool refresh(Film& film);
    bool finish(Film& film);
    bool contactAuthorities(Film&) { std::cout << "Cannot contact authorities after shooting has started.\n"; return false; }
    bool reportInjury(Film& film, const std::string& memberName);
    bool replaceCrewMember(Film&, const std::string&, std::unique_ptr<CrewMember>) {
        std::cout << "Report the injury before selecting a replacement.\n"; return false;
    }
    bool resumeShooting(Film&) { std::cout << "Shooting is already in progress.\n"; return false; }
};

class OtherMissingState : public FilmState {
public:
    const std::string& name() const { static const std::string value = "OtherMissing"; return value; }
    bool confirmAreas(Film&) { return false; }
    bool startShooting(Film&) { std::cout << "Cannot shoot: required crew is missing.\n"; return false; }
    bool refresh(Film& film);
    bool finish(Film&) { std::cout << "Cannot finish: required crew is missing.\n"; return false; }
    bool contactAuthorities(Film&) { std::cout << "Resolve the missing crew before contacting authorities.\n"; return false; }
    bool reportInjury(Film& film, const std::string& memberName);
    bool replaceCrewMember(Film& film, const std::string& memberName,
                           std::unique_ptr<CrewMember> replacement);
    bool resumeShooting(Film& film);
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
    bool contactAuthorities(Film&) { std::cout << "Cannot contact authorities: the shoot was cancelled.\n"; return false; }
    bool reportInjury(Film&, const std::string&) { std::cout << "Cannot report injury: the shoot was cancelled.\n"; return false; }
    bool replaceCrewMember(Film&, const std::string&, std::unique_ptr<CrewMember>) { std::cout << "Cannot replace crew: the shoot was cancelled.\n"; return false; }
    bool resumeShooting(Film&) { std::cout << "Cannot resume: the shoot was cancelled.\n"; return false; }
};

class FinishedState : public FilmState {
public:
    const std::string& name() const { static const std::string value = "Finished"; return value; }
    bool confirmAreas(Film&) { std::cout << "Film is finished.\n"; return false; }
    bool startShooting(Film&) { std::cout << "Film is finished.\n"; return false; }
    bool refresh(Film&) { return true; }
    bool finish(Film&) { std::cout << "Film is already finished.\n"; return false; }
    bool contactAuthorities(Film&) { std::cout << "Film is finished.\n"; return false; }
    bool reportInjury(Film&, const std::string&) { std::cout << "Film is finished.\n"; return false; }
    bool replaceCrewMember(Film&, const std::string&, std::unique_ptr<CrewMember>) { std::cout << "Film is finished.\n"; return false; }
    bool resumeShooting(Film&) { std::cout << "Film is finished.\n"; return false; }
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

bool NotShootingState::contactAuthorities(Film& film) {
    FilmIterator locations = film.createIterator(FilmTraversalMode::LocationsNeedingClearance);
    bool cleared = false;
    while (locations.hasNext()) {
        locations.next()->clearForShooting();
        cleared = true;
    }
    if (!cleared) std::cout << "No outstanding area clearance was found.\n";
    return refresh(film);
}

bool NotShootingState::reportInjury(Film& film, const std::string& memberName) {
    CrewMember* member = film.crew().find(memberName);
    if (!member) { std::cout << "Crew member not found.\n"; return false; }
    member->setPresent(false);
    std::cout << memberName << " has been marked unavailable.\n";
    return refresh(film);
}

bool NotShootingState::replaceCrewMember(Film& film, const std::string& memberName,
                                         std::unique_ptr<CrewMember> replacement) {
    if (!film.crew().replace(memberName, std::move(replacement))) {
        std::cout << "Crew member replacement failed.\n"; return false;
    }
    return refresh(film);
}

bool ReadyForShootState::startShooting(Film& film) {
    film.setState(std::unique_ptr<FilmState>(new ShootingState()));
    std::cout << "Shooting started.\n";
    return true;
}

bool ReadyForShootState::contactAuthorities(Film& film) { return film.confirmAreas(); }
bool ReadyForShootState::reportInjury(Film& film, const std::string& memberName) {
    CrewMember* member = film.crew().find(memberName);
    if (!member) { std::cout << "Crew member not found.\n"; return false; }
    member->setPresent(false);
    return film.refreshOperationalState();
}
bool ReadyForShootState::replaceCrewMember(Film& film, const std::string& memberName,
                                           std::unique_ptr<CrewMember> replacement) {
    if (!film.crew().replace(memberName, std::move(replacement))) return false;
    return film.refreshOperationalState();
}
bool ReadyForShootState::resumeShooting(Film& film) { return startShooting(film); }

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

bool ShootingState::reportInjury(Film& film, const std::string& memberName) {
    CrewMember* member = film.crew().find(memberName);
    if (!member) { std::cout << "Crew member not found.\n"; return false; }
    member->setPresent(false);
    return refresh(film);
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

bool OtherMissingState::reportInjury(Film& film, const std::string& memberName) {
    CrewMember* member = film.crew().find(memberName);
    if (!member) { std::cout << "Crew member not found.\n"; return false; }
    member->setPresent(false);
    return false;
}

bool OtherMissingState::replaceCrewMember(Film& film, const std::string& memberName,
                                          std::unique_ptr<CrewMember> replacement) {
    if (!film.crew().replace(memberName, std::move(replacement))) {
        std::cout << "Crew member replacement failed.\n"; return false;
    }
    return refresh(film);
}

bool OtherMissingState::resumeShooting(Film& film) { return refresh(film); }
}

Film::Film(const std::string& title)
    : Area(title), state(new NotShootingState()) {
    std::shared_ptr<std::function<void()> > mutationObserver(new std::function<void()>());
    *mutationObserver = [this]() {
        const std::string& current = stateName();
        if (current == "ReadyForShoot" || current == "Shooting" || current == "OtherMissing")
            state.reset(new NotShootingState());
    };
    setMutationContext(versionToken(), mutationObserver);
}

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
bool Film::contactAuthorities() { return state->contactAuthorities(*this); }
bool Film::reportInjury(const std::string& memberName) { return state->reportInjury(*this, memberName); }
bool Film::replaceCrewMember(const std::string& memberName,
                             std::unique_ptr<CrewMember> replacement) {
    return state->replaceCrewMember(*this, memberName, std::move(replacement));
}
bool Film::resumeShooting() { return state->resumeShooting(*this); }
const std::string& Film::stateName() const { return state->name(); }
FilmIterator Film::createIterator(FilmTraversalMode mode) const { return FilmIterator(*this, mode); }
CrewRoster& Film::crew() { return roster; }
const CrewRoster& Film::crew() const { return roster; }
void Film::setState(std::unique_ptr<FilmState> nextState) { state = std::move(nextState); }
