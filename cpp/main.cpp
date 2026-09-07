#include "test_framework.h"
#include "Film.h"
#include "CrewBasics.h"
#include "CrewDecorator.h"
#include "CrewIterator.h"
#include "CrewRoster.h"
#include "Area.h"
#include "Location.h"
#include "FilmIterator.h"
#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <string>

// ============================================================================
// ORIGINAL INTEGRATION SCENARIOS
// ============================================================================

static void printTraversal(FilmIterator iterator) {
    while (iterator.hasNext())
        std::cout << "  visited: " << iterator.next()->name() << "\n";
}

static void runHappyPath() {
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

    std::cout << "\n=== Crew availability and state recovery (non-VIP, recoverable) ===\n";
    film.startShooting();
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
}

static void runVipCancellationScenario() {
    std::cout << "\n=== VIP missing: shoot is cancelled permanently ===\n";
    Film film("Second Unit");
    film.add(std::unique_ptr<FilmElement>(new Location("Rooftop", true)));

    std::unique_ptr<CrewMember> directorRole(new RoleDecorator(
        std::unique_ptr<CrewMember>(new OnSiteCrew("Maya")), "Director", true));
    CrewMember* director = directorRole.get();
    film.crew().add(std::move(directorRole));

    film.confirmAreas();
    film.startShooting();
    std::cout << "Current state: " << film.stateName() << "\n";

    director->setPresent(false);
    film.refreshOperationalState();
    std::cout << "Current state: " << film.stateName() << "\n";

    // Even though the VIP comes back, cancellation is terminal: no resume path.
    director->setPresent(true);
    film.refreshOperationalState();
    std::cout << "Current state after VIP returns: " << film.stateName() << "\n";
    std::cout << "Can we finish? " << (film.finishShooting() ? "yes" : "no") << "\n";
}

// ============================================================================
// COMPOSITE TESTS (from test_composite.cpp)
// ============================================================================

TEST("Location: reports itself as a leaf with clearance state") {
    Location loc("Stage 1", false);
    CHECK(loc.name() == "Stage 1");
    CHECK(loc.isLocation() == true);
    CHECK(loc.needsClearance() == true);
    loc.clearForShooting();
    CHECK(loc.needsClearance() == false);
}

TEST("Location: filming only registers once cleared") {
    Location notCleared("Alley", false);
    notCleared.markFilmed();
    CHECK(notCleared.isFilmed() == false);

    Location cleared("Backlot", true);
    cleared.markFilmed();
    CHECK(cleared.isFilmed() == true);
}

TEST("Location: has no children and no version token") {
    Location loc("Rooftop", true);
    std::vector<FilmElement*> out;
    loc.appendChildren(out);
    CHECK(out.empty());
    CHECK(loc.versionToken() == nullptr);
}

TEST("Area: aggregates children and reports clearance/filming recursively") {
    Area area("Studio Complex");
    area.add(std::unique_ptr<FilmElement>(new Location("Stage 1", true)));
    area.add(std::unique_ptr<FilmElement>(new Location("Stage 2", false)));

    CHECK(area.name() == "Studio Complex");
    CHECK(area.isLocation() == false);
    CHECK(area.needsClearance() == true); 

    std::vector<FilmElement*> children;
    area.appendChildren(children);
    REQUIRE(children.size() == 2);

    area.clearForShooting();
    CHECK(area.needsClearance() == false);
}

TEST("Area: isFilmed is false for an empty area and true only once every child is filmed") {
    Area empty("Empty Area");
    CHECK(empty.isFilmed() == false);

    Area area("Post Production");
    area.add(std::unique_ptr<FilmElement>(new Location("Sound Booth", true)));
    CHECK(area.isFilmed() == false);
    area.markFilmed();
    CHECK(area.isFilmed() == true);
}

TEST("Area: remove() finds and removes a named child, bumping the version token") {
    Area area("Production Department");
    area.add(std::unique_ptr<FilmElement>(new Location("Backlot", true)));
    std::size_t before = *area.versionToken();

    CHECK(area.remove("Backlot") == true);
    std::size_t after = *area.versionToken();
    CHECK(after != before);

    CHECK(area.remove("Does Not Exist") == false);
}

TEST("Area: nested areas satisfy the 3-level nesting requirement") {
    Area root("Root");
    std::unique_ptr<Area> mid(new Area("Mid"));
    mid->add(std::unique_ptr<FilmElement>(new Location("Leaf", true)));
    root.add(std::move(mid));

    std::vector<FilmElement*> topChildren;
    root.appendChildren(topChildren);
    REQUIRE(topChildren.size() == 1);
    std::vector<FilmElement*> leafChildren;
    topChildren[0]->appendChildren(leafChildren);
    REQUIRE(leafChildren.size() == 1);
    CHECK(leafChildren[0]->name() == "Leaf");
}

// ============================================================================
// DECORATOR TESTS (from test_decorator.cpp)
// ============================================================================

TEST("OnSiteCrew / OffSiteCrew: base behaviour and presence toggling") {
    OnSiteCrew onSite("Maya");
    CHECK(onSite.name() == "Maya");
    CHECK(onSite.isPresent() == true);
    CHECK(onSite.isVip() == false);
    CHECK(onSite.requiredOnSite() == true);
    CHECK(onSite.description() == "On-site crew");
    onSite.setPresent(false);
    CHECK(onSite.isPresent() == false);

    OffSiteCrew offSite("Ravi");
    CHECK(offSite.name() == "Ravi");
    CHECK(offSite.requiredOnSite() == false);
    CHECK(offSite.description() == "Off-site crew");
}

TEST("RoleDecorator: adds role text and can mark VIP without changing wrapped presence") {
    std::unique_ptr<CrewMember> base(new OnSiteCrew("Jon"));
    RoleDecorator role(std::move(base), "Actor", false);

    CHECK(role.name() == "Jon");
    CHECK(role.description() == "On-site crew, role: Actor");
    CHECK(role.isVip() == false);
    role.setPresent(false);
    CHECK(role.isPresent() == false);
}

TEST("RoleDecorator: isVip is true if this role or the wrapped component is VIP") {
    std::unique_ptr<CrewMember> vipBase(new OnSiteCrew("Maya"));
    RoleDecorator vipRole(std::move(vipBase), "Director", true);
    CHECK(vipRole.isVip() == true);

    std::unique_ptr<CrewMember> innerVip(new OnSiteCrew("Producer Base"));
    std::unique_ptr<CrewMember> vipLayer(new RoleDecorator(std::move(innerVip), "Producer", true));
    RoleDecorator outerLayer(std::move(vipLayer), "Executive Producer", false);
    CHECK(outerLayer.isVip() == true);
}

TEST("Decorators stack: description accumulates every layer in order") {
    std::unique_ptr<CrewMember> base(new OnSiteCrew("Jon"));
    std::unique_ptr<CrewMember> actor(new RoleDecorator(std::move(base), "Actor", false));
    RoleDecorator stunt(std::move(actor), "Stunt performer", false);

    CHECK(stunt.description() == "On-site crew, role: Actor, role: Stunt performer");
    CHECK(stunt.isVip() == false);
    CHECK(stunt.requiredOnSite() == true); 
}

TEST("Bare CrewDecorator (no role) passes every call straight through to its component") {
    CrewDecorator plain(std::unique_ptr<CrewMember>(new OnSiteCrew("Extra")));
    CHECK(plain.name() == "Extra");
    CHECK(plain.isVip() == false);
    CHECK(plain.description() == "On-site crew");
    CHECK(plain.requiredOnSite() == true);
    plain.setPresent(false);
    CHECK(plain.isPresent() == false);
}

TEST("Decorated member remains usable through the plain CrewMember interface") {
    std::unique_ptr<CrewMember> decorated(new RoleDecorator(
        std::unique_ptr<CrewMember>(new OffSiteCrew("Ravi")), "Producer", true));
    CrewMember* asBase = decorated.get(); 
    CHECK(asBase->isVip() == true);
    CHECK(asBase->requiredOnSite() == false);
    asBase->setPresent(false);
    CHECK(asBase->isPresent() == false);
}

// ============================================================================
// ITERATOR TESTS (from test_iterator.cpp)
// ============================================================================

TEST("CrewIterator: walks members in order and reports exhaustion") {
    OnSiteCrew a("A");
    OnSiteCrew b("B");
    std::vector<CrewMember*> members;
    members.push_back(&a);
    members.push_back(&b);

    CrewIterator it(members);
    REQUIRE(it.hasNext());
    CHECK(it.next()->name() == "A");
    REQUIRE(it.hasNext());
    CHECK(it.next()->name() == "B");
    CHECK(it.hasNext() == false);
}

TEST("CrewIterator: throws when advanced past the end") {
    std::vector<CrewMember*> empty;
    CrewIterator it(empty);
    bool threw = false;
    try {
        it.next();
    } catch (const std::out_of_range&) {
        threw = true;
    }
    CHECK(threw);
}

TEST("CrewRoster: allPresent/missingVip/missingOther reflect roster state") {
    CrewRoster roster;
    roster.add(std::unique_ptr<CrewMember>(new RoleDecorator(
        std::unique_ptr<CrewMember>(new OnSiteCrew("Maya")), "Director", true)));
    roster.add(std::unique_ptr<CrewMember>(new RoleDecorator(
        std::unique_ptr<CrewMember>(new OnSiteCrew("Jon")), "Actor", false)));

    CHECK(roster.allPresent() == true);
    CHECK(roster.missingVip() == false);
    CHECK(roster.missingOther() == false);

    CrewIterator it = roster.iterator();
    while (it.hasNext()) {
        CrewMember* member = it.next();
        if (!member->isVip()) member->setPresent(false);
    }
    CHECK(roster.allPresent() == false);
    CHECK(roster.missingVip() == false);
    CHECK(roster.missingOther() == true);
}

TEST("CrewRoster: missingVip is only true when an actual VIP is absent") {
    CrewRoster roster;
    roster.add(std::unique_ptr<CrewMember>(new RoleDecorator(
        std::unique_ptr<CrewMember>(new OnSiteCrew("Maya")), "Director", true)));

    CrewIterator it = roster.iterator();
    it.next()->setPresent(false);
    CHECK(roster.missingVip() == true);
}

TEST("CrewRoster: off-site crew missing does not count as missingOther") {
    CrewRoster roster;
    roster.add(std::unique_ptr<CrewMember>(new RoleDecorator(
        std::unique_ptr<CrewMember>(new OffSiteCrew("Ravi")), "Producer", false)));
    CrewIterator it = roster.iterator();
    it.next()->setPresent(false);
    CHECK(roster.missingOther() == false);
}

TEST("FilmIterator (AllElements): visits the whole hierarchy") {
    Area root("Root");
    std::unique_ptr<Area> mid(new Area("Mid"));
    mid->add(std::unique_ptr<FilmElement>(new Location("Leaf A", true)));
    mid->add(std::unique_ptr<FilmElement>(new Location("Leaf B", false)));
    root.add(std::move(mid));
    root.add(std::unique_ptr<FilmElement>(new Location("Leaf C", true)));

    FilmIterator it(root, FilmTraversalMode::AllElements);
    int count = 0;
    while (it.hasNext()) { it.next(); ++count; }
    CHECK(count == 5); 
}

TEST("FilmIterator (LocationsNeedingClearance): only yields uncleared locations") {
    Area root("Root");
    root.add(std::unique_ptr<FilmElement>(new Location("Cleared", true)));
    root.add(std::unique_ptr<FilmElement>(new Location("Needs clearance", false)));

    FilmIterator it(root, FilmTraversalMode::LocationsNeedingClearance);
    REQUIRE(it.hasNext());
    CHECK(it.next()->name() == "Needs clearance");
    CHECK(it.hasNext() == false);
}

TEST("FilmIterator: two independent iterators over the same structure do not interfere") {
    Area root("Root");
    root.add(std::unique_ptr<FilmElement>(new Location("A", true)));
    root.add(std::unique_ptr<FilmElement>(new Location("B", true)));

    FilmIterator first(root, FilmTraversalMode::AllElements);
    FilmIterator second(root, FilmTraversalMode::AllElements);
    CHECK(first.next()->name() == "Root");
    CHECK(second.next()->name() == "Root"); 
    CHECK(second.next()->name() == "A");
    CHECK(first.next()->name() == "A"); 
}

TEST("FilmIterator: is invalidated by a structural change and throws on next()") {
    Area root("Root");
    root.add(std::unique_ptr<FilmElement>(new Location("A", true)));

    FilmIterator it(root, FilmTraversalMode::AllElements);
    CHECK(it.isInvalidated() == false);
    root.add(std::unique_ptr<FilmElement>(new Location("B", true)));
    CHECK(it.isInvalidated() == true);
    CHECK(it.hasNext() == false);

    bool threw = false;
    try {
        it.next();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
}

// ============================================================================
// STATE TESTS (from test_state.cpp)
// ============================================================================

static std::unique_ptr<CrewMember> vip(const std::string& n) {
    return std::unique_ptr<CrewMember>(new RoleDecorator(
        std::unique_ptr<CrewMember>(new OnSiteCrew(n)), "Director", true));
}
static std::unique_ptr<CrewMember> nonVip(const std::string& n) {
    return std::unique_ptr<CrewMember>(new RoleDecorator(
        std::unique_ptr<CrewMember>(new OnSiteCrew(n)), "Actor", false));
}

TEST("Film starts in NotShooting and rejects premature actions") {
    Film film("Debut");
    CHECK(film.stateName() == "NotShooting");
    CHECK(film.startShooting() == false);
    CHECK(film.finishShooting() == false);
}

TEST("Film: confirmAreas is blocked until every location is cleared and crew present") {
    Film film("Debut");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", false)));
    film.crew().add(nonVip("Jon"));

    CHECK(film.confirmAreas() == false);
    CHECK(film.stateName() == "NotShooting");

    FilmIterator needing = film.createIterator(FilmTraversalMode::LocationsNeedingClearance);
    while (needing.hasNext()) needing.next()->clearForShooting();

    CHECK(film.confirmAreas() == true);
    CHECK(film.stateName() == "ReadyForShoot");
}

TEST("Film: confirmAreas is also blocked while required crew is absent") {
    Film film("Debut");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    std::unique_ptr<CrewMember> actor = nonVip("Jon");
    CrewMember* actorPtr = actor.get();
    film.crew().add(std::move(actor));
    actorPtr->setPresent(false);

    CHECK(film.confirmAreas() == false);
    CHECK(film.stateName() == "NotShooting");
}

TEST("Film: ReadyForShoot -> Shooting via startShooting, rejects double-confirm/double-start") {
    Film film("Debut");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    REQUIRE(film.confirmAreas() == true);

    CHECK(film.confirmAreas() == true); 
    CHECK(film.startShooting() == true);
    CHECK(film.stateName() == "Shooting");
    CHECK(film.startShooting() == false); 
}

TEST("Film: non-VIP going missing is a recoverable OtherMissing state") {
    Film film("Debut");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    std::unique_ptr<CrewMember> actor = nonVip("Jon");
    CrewMember* actorPtr = actor.get();
    film.crew().add(std::move(actor));
    REQUIRE(film.confirmAreas() == true);
    REQUIRE(film.startShooting() == true);

    actorPtr->setPresent(false);
    CHECK(film.refreshOperationalState() == false);
    CHECK(film.stateName() == "OtherMissing");
    CHECK(film.startShooting() == false);
    CHECK(film.finishShooting() == false);

    actorPtr->setPresent(true);
    CHECK(film.refreshOperationalState() == true);
    CHECK(film.stateName() == "Shooting");
}

TEST("Film: a VIP going missing cancels the shoot permanently, even after they return") {
    Film film("Debut");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    std::unique_ptr<CrewMember> director = vip("Maya");
    CrewMember* directorPtr = director.get();
    film.crew().add(std::move(director));
    REQUIRE(film.confirmAreas() == true);
    REQUIRE(film.startShooting() == true);

    directorPtr->setPresent(false);
    CHECK(film.refreshOperationalState() == false);
    CHECK(film.stateName() == "Cancelled");

    directorPtr->setPresent(true);
    CHECK(film.refreshOperationalState() == false);
    CHECK(film.stateName() == "Cancelled");
    CHECK(film.confirmAreas() == false);
    CHECK(film.startShooting() == false);
    CHECK(film.finishShooting() == false);
}

TEST("Film: OtherMissing escalates to Cancelled if the missing member turns out to be the VIP") {
    Film film("Debut");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    std::unique_ptr<CrewMember> actor = nonVip("Jon");
    std::unique_ptr<CrewMember> director = vip("Maya");
    CrewMember* actorPtr = actor.get();
    CrewMember* directorPtr = director.get();
    film.crew().add(std::move(actor));
    film.crew().add(std::move(director));
    REQUIRE(film.confirmAreas() == true);
    REQUIRE(film.startShooting() == true);

    actorPtr->setPresent(false);
    REQUIRE(film.refreshOperationalState() == false);
    REQUIRE(film.stateName() == "OtherMissing");

    directorPtr->setPresent(false);
    CHECK(film.refreshOperationalState() == false);
    CHECK(film.stateName() == "Cancelled");
}

TEST("Film: finishShooting requires every location to be filmed") {
    Film film("Debut");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    REQUIRE(film.confirmAreas() == true);
    REQUIRE(film.startShooting() == true);

    CHECK(film.finishShooting() == false);

    FilmIterator all = film.createIterator(FilmTraversalMode::AllElements);
    while (all.hasNext()) all.next()->markFilmed();

    CHECK(film.finishShooting() == true);
    CHECK(film.stateName() == "Finished");
    CHECK(film.confirmAreas() == false);
    CHECK(film.startShooting() == false);
    CHECK(film.finishShooting() == false);
}

TEST("Film: adding a new element mid-shoot forces re-confirmation of areas") {
    Film film("Debut");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    REQUIRE(film.confirmAreas() == true);
    REQUIRE(film.startShooting() == true);
    REQUIRE(film.stateName() == "Shooting");

    film.add(std::unique_ptr<FilmElement>(new Location("New Set", false)));
    CHECK(film.stateName() == "NotShooting");
}

TEST("Film: no-op / already-satisfied calls in every state are handled sensibly") {
    Film film("Debut");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    REQUIRE(film.confirmAreas() == true);

    CHECK(film.refreshOperationalState() == true);
    CHECK(film.finishShooting() == false);
    CHECK(film.stateName() == "ReadyForShoot");

    REQUIRE(film.startShooting() == true);
    CHECK(film.confirmAreas() == true);

    std::unique_ptr<CrewMember> actor = nonVip("Jon");
    CrewMember* actorPtr = actor.get();
    film.crew().add(std::move(actor));
    actorPtr->setPresent(false);
    REQUIRE(film.refreshOperationalState() == false);
    CHECK(film.confirmAreas() == false);
    actorPtr->setPresent(true);
    REQUIRE(film.refreshOperationalState() == true);

    FilmIterator all = film.createIterator(FilmTraversalMode::AllElements);
    while (all.hasNext()) all.next()->markFilmed();
    REQUIRE(film.finishShooting() == true);
    CHECK(film.refreshOperationalState() == true);
}

TEST("Film: crew() exposes the same roster through its const overload") {
    Film film("Debut");
    film.crew().add(nonVip("Jon"));
    const Film& constFilm = film;
    CHECK(constFilm.crew().allPresent() == true);
}

TEST("Area/Location: print() renders without throwing, for any depth or clearance state") {
    Area area("Studio Complex");
    area.add(std::unique_ptr<FilmElement>(new Location("Stage 1", true)));
    area.add(std::unique_ptr<FilmElement>(new Location("Stage 2", false)));

    std::ostringstream captured;
    std::streambuf* original = std::cout.rdbuf(captured.rdbuf());
    area.print(0);
    std::cout.rdbuf(original);

    std::string text = captured.str();
    CHECK(text.find("Area: Studio Complex") != std::string::npos);
    CHECK(text.find("Location: Stage 1") != std::string::npos);
    CHECK(text.find("cleared") != std::string::npos);
    CHECK(text.find("needs clearance") != std::string::npos);
}

TEST("Film: adding elements after Finished does not resurrect the film") {
    Film film("Debut");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    REQUIRE(film.confirmAreas() == true);
    REQUIRE(film.startShooting() == true);
    FilmIterator all = film.createIterator(FilmTraversalMode::AllElements);
    while (all.hasNext()) all.next()->markFilmed();
    REQUIRE(film.finishShooting() == true);

    film.add(std::unique_ptr<FilmElement>(new Location("Epilogue Scene", true)));
    CHECK(film.stateName() == "Finished");
}

// ============================================================================
// MAIN EXECUTION
// ============================================================================

int main() {
    std::cout << "\n>>> RUNNING ORIGINAL SCENARIOS <<<\n";
    runHappyPath();
    runVipCancellationScenario();

    std::cout << "\n\n>>> RUNNING UNIT TESTS <<<\n\n";
    return testfw::run_all_tests();
}