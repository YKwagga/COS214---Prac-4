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

void runHappyPath() {
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

void runVipCancellationScenario() {
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

TEST("Film: nested structural changes invalidate film iterators and reset readiness") {
    Film film("Nested mutation");
    std::unique_ptr<Area> studio(new Area("Studio"));
    Area* studioPtr = studio.get();
    studio->add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    film.add(std::move(studio));

    REQUIRE(film.confirmAreas() == true);
    REQUIRE(film.startShooting() == true);
    FilmIterator traversal = film.createIterator(FilmTraversalMode::AllElements);
    studioPtr->add(std::unique_ptr<FilmElement>(new Location("Pickup", true)));

    CHECK(traversal.isInvalidated() == true);
    CHECK(film.stateName() == "NotShooting");
}

TEST("Film: absent off-site crew does not block area confirmation") {
    Film film("Remote producer");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    std::unique_ptr<CrewMember> producer(new RoleDecorator(
        std::unique_ptr<CrewMember>(new OffSiteCrew("Ravi")), "Producer", true));
    producer->setPresent(false);
    film.crew().add(std::move(producer));

    CHECK(film.confirmAreas() == true);
    CHECK(film.stateName() == "ReadyForShoot");
}

TEST("Film: authorities clear remaining locations") {
    Film film("Authority clearance");
    film.add(std::unique_ptr<FilmElement>(new Location("Cleared", true)));
    film.add(std::unique_ptr<FilmElement>(new Location("Pending", false)));

    CHECK(film.confirmAreas() == false);
    CHECK(film.contactAuthorities() == true);
    CHECK(film.stateName() == "ReadyForShoot");
}

TEST("Film: non-VIP injury can be resolved with a replacement") {
    Film film("Replacement recovery");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    film.crew().add(nonVip("Jon"));
    REQUIRE(film.confirmAreas() == true);
    REQUIRE(film.startShooting() == true);

    CHECK(film.reportInjury("Jon") == false);
    CHECK(film.stateName() == "OtherMissing");
    CHECK(film.replaceCrewMember("Jon", nonVip("Alex")) == true);
    CHECK(film.stateName() == "Shooting");
}

TEST("Film: VIP injury cancels the shoot permanently") {
    Film film("VIP injury");
    film.add(std::unique_ptr<FilmElement>(new Location("Stage", true)));
    film.crew().add(vip("Maya"));
    REQUIRE(film.confirmAreas() == true);
    REQUIRE(film.startShooting() == true);

    CHECK(film.reportInjury("Maya") == false);
    CHECK(film.stateName() == "Cancelled");
    CHECK(film.replaceCrewMember("Maya", vip("Alex")) == false);
    CHECK(film.resumeShooting() == false);
}

// ============================================================================
// MAIN EXECUTION
// ============================================================================

/*
int main() {
    std::cout << "\n>>> RUNNING ORIGINAL SCENARIOS <<<\n";
    runHappyPath();
    runVipCancellationScenario();

    std::cout << "\n\n>>> RUNNING UNIT TESTS <<<\n\n";
    return testfw::run_all_tests();
}
*/

namespace {
const char* const COLOR_RESET = "\033[0m";
const char* const COLOR_TITLE = "\033[1;36m";
const char* const COLOR_OPTION = "\033[1;33m";
const char* const COLOR_SUCCESS = "\033[1;32m";
const char* const COLOR_ERROR = "\033[1;31m";
const char* const COLOR_MUTED = "\033[0;90m";

void title(const std::string& text) {
    std::cout << "\n" << COLOR_TITLE << "==== " << text << " ====\n" << COLOR_RESET;
}

std::string readText(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

int readChoice(const std::string& prompt, int minimum, int maximum) {
    while (true) {
        std::string input = readText(prompt);
        std::stringstream stream(input);
        int choice = 0;
        char extra = '\0';
        if ((stream >> choice) && !(stream >> extra) && choice >= minimum && choice <= maximum)
            return choice;
        std::cout << COLOR_ERROR << "Please enter a number from " << minimum << " to "
                  << maximum << ".\n" << COLOR_RESET;
    }
}

bool readYesNo(const std::string& prompt) {
    while (true) {
        std::string answer = readText(prompt + " (y/n): ");
        if (answer == "y" || answer == "Y") return true;
        if (answer == "n" || answer == "N") return false;
        std::cout << COLOR_ERROR << "Please answer y or n.\n" << COLOR_RESET;
    }
}

std::unique_ptr<CrewMember> makeCrewMember(const std::string& name,
                                           bool onSite,
                                           const std::string& role,
                                           bool vipMember) {
    std::unique_ptr<CrewMember> member(onSite
        ? static_cast<CrewMember*>(new OnSiteCrew(name))
        : static_cast<CrewMember*>(new OffSiteCrew(name)));
    if (!role.empty()) member.reset(new RoleDecorator(std::move(member), role, vipMember));
    return member;
}

std::unique_ptr<Film> createSampleFilm() {
    std::unique_ptr<Film> film(new Film("The Last Frame"));
    std::unique_ptr<Area> production(new Area("Production Department"));
    std::unique_ptr<Area> studio(new Area("Studio Complex"));
    studio->add(std::unique_ptr<FilmElement>(new Location("Stage 1", true)));
    studio->add(std::unique_ptr<FilmElement>(new Location("Stage 2", false)));
    production->add(std::move(studio));
    production->add(std::unique_ptr<FilmElement>(new Location("Backlot", true)));
    film->add(std::move(production));
    film->add(std::unique_ptr<FilmElement>(new Location("Sound Booth", true)));
    film->crew().add(makeCrewMember("Maya", true, "Director", true));
    film->crew().add(makeCrewMember("Jon", true, "Actor", false));
    film->crew().add(makeCrewMember("Ravi", false, "Producer", true));
    return film;
}

FilmElement* findElement(Film& film, const std::string& name) {
    FilmIterator iterator = film.createIterator(FilmTraversalMode::AllElements);
    while (iterator.hasNext()) {
        FilmElement* element = iterator.next();
        if (element->name() == name) return element;
    }
    return nullptr;
}

void showFilm(Film& film) {
    title("Film status");
    std::cout << "Title: " << film.name() << "\nState: " << film.stateName() << "\n\n";
    film.print(0);
    std::cout << "\nCrew:\n";
    CrewIterator iterator = film.crew().iterator();
    while (iterator.hasNext()) {
        CrewMember* member = iterator.next();
        std::cout << "  " << member->name() << " - " << member->description()
                  << " [" << (member->isPresent() ? "present" : "unavailable") << "]\n";
    }
}

void clearLocation(Film& film) {
    std::string name = readText("Location to clear: ");
    FilmIterator iterator = film.createIterator(FilmTraversalMode::LocationsNeedingClearance);
    while (iterator.hasNext()) {
        FilmElement* location = iterator.next();
        if (location->name() == name) {
            location->clearForShooting();
            std::cout << COLOR_SUCCESS << "Location cleared.\n" << COLOR_RESET;
            return;
        }
    }
    std::cout << COLOR_ERROR << "That location either does not exist or is already clear.\n"
              << COLOR_RESET;
}

void addElement(Film& film) {
    int kind = readChoice("1. Location  2. Area: ", 1, 2);
    std::string name = readText("Name: ");
    if (name.empty()) {
        std::cout << COLOR_ERROR << "A name is required.\n" << COLOR_RESET;
        return;
    }
    if (kind == 1) {
        film.add(std::unique_ptr<FilmElement>(new Location(name, readYesNo("Already cleared"))));
    } else {
        film.add(std::unique_ptr<FilmElement>(new Area(name)));
    }
    std::cout << COLOR_SUCCESS << "Element added.\n" << COLOR_RESET;
}

void createCrew(Film& film) {
    std::string name = readText("Crew member name: ");
    bool onSite = readYesNo("Required on site");
    std::string role = readText("Role (leave blank for base decorator-free member): ");
    bool vipMember = !role.empty() && readYesNo("VIP responsibility");
    film.crew().add(makeCrewMember(name, onSite, role, vipMember));
    std::cout << COLOR_SUCCESS << "Crew member added.\n" << COLOR_RESET;
}

void togglePresence(Film& film) {
    std::string name = readText("Crew member: ");
    CrewMember* member = film.crew().find(name);
    if (!member) {
        std::cout << COLOR_ERROR << "Crew member not found.\n" << COLOR_RESET;
        return;
    }
    member->setPresent(!member->isPresent());
    film.refreshOperationalState();
    std::cout << COLOR_SUCCESS << name << " is now "
              << (member->isPresent() ? "present" : "unavailable") << ".\n" << COLOR_RESET;
}

void reportInjury(Film& film) {
    std::string name = readText("Injured crew member: ");
    film.reportInjury(name);
}

void replaceCrew(Film& film) {
    std::string injured = readText("Member to replace: ");
    std::string name = readText("Replacement name: ");
    bool onSite = readYesNo("Replacement is required on site");
    std::string role = readText("Replacement role: ");
    bool vipMember = !role.empty() && readYesNo("Replacement is VIP");
    film.replaceCrewMember(injured, makeCrewMember(name, onSite, role, vipMember));
}

void markFilmed(Film& film) {
    std::string name = readText("Location filmed: ");
    FilmElement* element = findElement(film, name);
    if (!element || !element->isLocation()) {
        std::cout << COLOR_ERROR << "Location not found.\n" << COLOR_RESET;
        return;
    }
    element->markFilmed();
    std::cout << COLOR_SUCCESS << "Location marked filmed when clearance permits.\n" << COLOR_RESET;
}

void iteratorDemo(Film& film) {
    title("Iterator demonstration");
    FilmIterator first = film.createIterator(FilmTraversalMode::AllElements);
    FilmIterator second = film.createIterator(FilmTraversalMode::LocationsNeedingClearance);
    if (first.hasNext()) std::cout << "First iterator: " << first.next()->name() << "\n";
    std::cout << "Second iterator has pending location: " << (second.hasNext() ? "yes" : "no") << "\n";
    film.add(std::unique_ptr<FilmElement>(new Location("Iterator Demo Set", false)));
    std::cout << "After a structural change, first invalidated: "
              << (first.isInvalidated() ? "yes" : "no") << ", second invalidated: "
              << (second.isInvalidated() ? "yes" : "no") << "\n";
}

void printMenu() {
    std::cout << "\n" << COLOR_OPTION
              << "1" << COLOR_RESET << "  Show film and crew\n"
              << COLOR_OPTION << "2" << COLOR_RESET << "  Add a location or area\n"
              << COLOR_OPTION << "3" << COLOR_RESET << "  Remove a top-level element\n"
              << COLOR_OPTION << "4" << COLOR_RESET << "  Clear one location\n"
              << COLOR_OPTION << "5" << COLOR_RESET << "  Contact authorities for clearance\n"
              << COLOR_OPTION << "6" << COLOR_RESET << "  Confirm areas\n"
              << COLOR_OPTION << "7" << COLOR_RESET << "  Start or resume shooting\n"
              << COLOR_OPTION << "8" << COLOR_RESET << "  Add a crew member/decorator\n"
              << COLOR_OPTION << "9" << COLOR_RESET << "  Toggle crew presence\n"
              << COLOR_OPTION << "10" << COLOR_RESET << "  Report an injury\n"
              << COLOR_OPTION << "11" << COLOR_RESET << "  Find a replacement\n"
              << COLOR_OPTION << "12" << COLOR_RESET << "  Mark a location filmed\n"
              << COLOR_OPTION << "13" << COLOR_RESET << "  Finish shooting\n"
              << COLOR_OPTION << "14" << COLOR_RESET << "  Demonstrate independent iterators\n"
              << COLOR_OPTION << "15" << COLOR_RESET << "  Run automated tests\n"
              << COLOR_OPTION << "16" << COLOR_RESET << "  Reset sample film\n"
              << COLOR_OPTION << "0" << COLOR_RESET << "  Exit\n";
}
}

int main() {
    std::unique_ptr<Film> film = createSampleFilm();
    title("TaskForge: The Last Frame");
    std::cout << COLOR_MUTED << "Interactive COS 214 practical demonstration\n" << COLOR_RESET;

    bool running = true;
    while (running && std::cin.good()) {
        printMenu();
        int choice = readChoice("\nSelect an action: ", 0, 16);
        switch (choice) {
        case 1: showFilm(*film); break;
        case 2: addElement(*film); break;
        case 3: {
            std::string name = readText("Top-level element to remove: ");
            std::cout << (film->remove(name) ? COLOR_SUCCESS + std::string("Element removed.\n")
                                               : COLOR_ERROR + std::string("Element not found.\n"))
                      << COLOR_RESET;
            break;
        }
        case 4: clearLocation(*film); break;
        case 5: film->contactAuthorities(); break;
        case 6: film->confirmAreas(); break;
        case 7: film->stateName() == "OtherMissing" ? film->resumeShooting() : film->startShooting(); break;
        case 8: createCrew(*film); break;
        case 9: togglePresence(*film); break;
        case 10: reportInjury(*film); break;
        case 11: replaceCrew(*film); break;
        case 12: markFilmed(*film); break;
        case 13: film->finishShooting(); break;
        case 14: iteratorDemo(*film); break;
        case 15: testfw::run_all_tests(); break;
        case 16: film = createSampleFilm(); std::cout << COLOR_SUCCESS << "Sample film reset.\n" << COLOR_RESET; break;
        case 0: running = false; break;
        default: break;
        }
    }
    std::cout << COLOR_TITLE << "Goodbye.\n" << COLOR_RESET;
    return 0;
}