# TaskForge: The Last Frame

COS 214 Practical 4 implementation in C++11. The chosen domain is a film studio
preparing and executing a shoot. The program models nested production areas and
locations, crew availability, and the film's operational lifecycle.

## Design decisions

The primary Composite hierarchy is `Film -> Area -> Location`. `Film` and `Area`
are composites and `Location` is a leaf. Areas can contain other areas, so the
runtime demonstration has more than three levels of nesting. The common
`FilmElement` interface lets the client treat a complete area and one location
uniformly without exposing STL containers.

The Iterator pattern has two independent implementations of traversal behavior:
`FilmIterator` performs depth-first traversal of all elements or filters to
locations needing clearance. `CrewIterator` traverses the owned crew roster to
classify missing personnel. Film iterators snapshot raw element pointers at
creation, while the film owns the objects through `std::unique_ptr`. Every
structural mutation increments a shared version counter; an existing film
iterator then becomes invalid and a fresh iterator must be created. This avoids
using pointers into a changed hierarchy.

The State context is `Film`, with these states: `Initial`,
`PendingAreaConfirmation`, `ReadyForShoot`, `Shooting`, `VIPMissing`,
`OtherMissing`, and `Finished`. State objects own transition decisions. Area
confirmation uses the location iterator; shooting recovery uses the crew
iterator. Invalid actions print a reason and do not mutate the lifecycle.

The Decorator component is `CrewMember`. `OnSiteCrew` and `OffSiteCrew` are
concrete components. `RoleDecorator` adds role and VIP responsibilities at
runtime and can be stacked, as shown by the actor with two role decorators.
Decorators own their wrapped component. `Film` owns its areas, and `CrewRoster`
owns crew members; all polymorphic bases have virtual destructors.

## Build and run

The executable is named `taskforge`.

```text
make clean
make
./taskforge
```

On Windows with MinGW, use `mingw32-make` and run `taskforge.exe`.

## Docker, GDB, and Valgrind

```text
docker build -t taskforge .
docker run --rm taskforge
docker run --rm -it taskforge sh
make
gdb ./taskforge
valgrind --leak-check=full --show-leak-kinds=all ./taskforge
```

The final PDF should include actual GDB screenshots/commands, one genuine bug
investigation from development, and the final Valgrind output. Those evidence
items must be captured during development and are intentionally not fabricated
in this source README.

## Demonstration story

The executable first prints the nested production hierarchy. It then attempts
area confirmation while Stage 2 is uncleared, clears it through a filtered
iterator, and reaches `ReadyForShoot`. A second pair of independent iterators
is created; adding Reshoot Stage invalidates the first traversal. The program
starts shooting, removes the Director, enters `VIPMissing`, recovers when the
Director returns, prints decorated crew responsibilities, films every cleared
location, and reaches `Finished`.

## UML sources

PlantUML source files are in `docs/`:

- `class-diagram.puml` maps the four pattern participants and ownership.
- `object-diagram.puml` shows the runtime production hierarchy and decorated crew.
- `state-diagram.puml` shows the film lifecycle.
- `activity-clearance.puml` shows visible location traversal.
- `activity-shooting.puml` shows lifecycle decisions and parallel crew/location work.
- `activity-recovery.puml` shows crew absence and recovery.

## Assumptions

The repository started with only a README and no prescribed input format, so the
application uses deterministic console scenarios instead of external files or a
menu. It does not model video files, persistence, networking, or a GUI; those
would distract from the required patterns. Team members should add their real
names, contribution statement, commit references, and debugging evidence to the
submission PDF.

