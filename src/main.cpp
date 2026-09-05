#include "Film.h"
#include "CrewBasics.h"
#include "CrewDecorator.h"
#include "Location.h"
#include <iostream>
#include <memory>

static void printTraversal(FilmIterator iterator) {
    while (iterator.hasNext())
        std::cout << "  visited: " << iterator.next()->name() << "\n";
}

int main() {
    Film film("The Last Frame");

    std::unique_ptr<Area> production(new Area("Production Department"));
    std::unique_ptr<Area> studio(new Area("Studio Complex"));
    studio->add(std::unique_ptr<FilmElement>(new Location("Stage 1", true)));
    studio->add(std::unique_ptr<FilmElement>(new Location("Stage 2", false)));
    production->add(std::move(studio));
    production->add(std::unique_ptr<FilmElement>(new Location("Backlot", true)));
    film.add(std::move(production));

    std::unique_ptr<Area> post(new Area("Post Production"));
    post->add(std::unique_ptr<FilmElement>(new Location("Sound Booth", true)));
    film.add(std::move(post));

    std::unique_ptr<CrewMember> directorBase(new OnSiteCrew("Maya"));
    std::unique_ptr<CrewMember> directorRole(new RoleDecorator(std::move(directorBase), "Director", true));
    CrewMember* director = directorRole.get();
    film.crew().add(std::move(directorRole));

    std::unique_ptr<CrewMember> actorBase(new OnSiteCrew("Jon"));
    std::unique_ptr<CrewMember> actorRole(new RoleDecorator(std::move(actorBase), "Actor", false));
    std::unique_ptr<CrewMember> actorStacked(new RoleDecorator(std::move(actorRole), "Stunt performer", false));
    CrewMember* actor = actorStacked.get();
    film.crew().add(std::move(actorStacked));
    film.crew().add(std::unique_ptr<CrewMember>(new RoleDecorator(
        std::unique_ptr<CrewMember>(new OffSiteCrew("Ravi")), "Producer", true)));

    std::cout << "=== Production hierarchy ===\n";
    printTraversal(film.createIterator(FilmTraversalMode::AllElements));

    std::cout << "\n=== Area confirmation ===\n";
    film.confirmAreas();
    FilmIterator needingClearance = film.createIterator(FilmTraversalMode::LocationsNeedingClearance);
    while (needingClearance.hasNext()) needingClearance.next()->clearForShooting();
    film.confirmAreas();
    std::cout << "Current state: " << film.stateName() << "\n";

    std::cout << "\n=== Independent traversal and structural change ===\n";
    FilmIterator first = film.createIterator(FilmTraversalMode::AllElements);
    FilmIterator second = film.createIterator(FilmTraversalMode::LocationsNeedingClearance);
    std::cout << "First iterator starts at: " << first.next()->name() << "\n";
    film.add(std::unique_ptr<FilmElement>(new Location("Reshoot Stage", false)));
    std::cout << "First iterator invalidated: " << (first.isInvalidated() ? "yes" : "no") << "\n";
    std::cout << "Second iterator invalidated: " << (second.isInvalidated() ? "yes" : "no") << "\n";
    FilmIterator freshClearance = film.createIterator(FilmTraversalMode::LocationsNeedingClearance);
    while (freshClearance.hasNext()) freshClearance.next()->clearForShooting();
    film.confirmAreas();

    std::cout << "\n=== Crew availability and state recovery ===\n";
    film.startShooting();
    director->setPresent(false);
    film.refreshOperationalState();
    std::cout << "Current state: " << film.stateName() << "\n";
    director->setPresent(true);
    film.refreshOperationalState();
    std::cout << "Current state: " << film.stateName() << "\n";
    actor->setPresent(false);
    film.refreshOperationalState();
    std::cout << "Current state: " << film.stateName() << "\n";
    actor->setPresent(true);
    film.refreshOperationalState();
    std::cout << "Current state: " << film.stateName() << "\n";

    CrewIterator crew = film.crew().iterator();
    while (crew.hasNext()) {
        CrewMember* member = crew.next();
        std::cout << member->name() << " - " << member->description()
                  << " [" << (member->isPresent() ? "present" : "missing") << "]\n";
    }

    FilmIterator locations = film.createIterator(FilmTraversalMode::LocationsNeedingClearance);
    while (locations.hasNext()) locations.next()->clearForShooting();
    FilmIterator allLocations = film.createIterator(FilmTraversalMode::AllElements);
    while (allLocations.hasNext()) allLocations.next()->markFilmed();
    film.finishShooting();
    std::cout << "Final state: " << film.stateName() << "\n";
    return 0;
}