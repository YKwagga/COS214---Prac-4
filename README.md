# TaskForge: The Last Frame

COS 214 Practical 4 implementation in C++11. The chosen domain is a film studio
preparing and executing a shoot. The program models nested production areas and
locations, crew availability, decorators, iterators, and the film lifecycle.

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

The State context is `Film`, with these states: `NotShooting`, `ReadyForShoot`,
`Shooting`, `OtherMissing`, `Cancelled`, and `Finished`. State objects own
transition decisions. Area confirmation uses the location iterator; shooting
recovery uses the crew iterator. A missing VIP or an injured VIP cancels the
shoot permanently. A missing or injured non-VIP enters `OtherMissing` and can
recover through an internal replacement or a talent-agency replacement.
Invalid actions print a reason and do not mutate the lifecycle.

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

On Windows with MinGW, use `mingw32-make` and run `taskforge.exe`. The clean
target handles both Windows and Linux shells.

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

## Interactive demonstration

The executable starts with a sample film and a numbered menu. The menu exposes:

- Composite traversal and hierarchy printing.
- Adding and removing areas and locations.
- Individual location clearance and authority clearance for all remaining locations.
- Area confirmation, starting, resuming, and finishing a shoot.
- Creating on-site and off-site crew members with role decorators and VIP flags.
- Presence changes, injury reporting, missing-crew recovery, and replacement selection.
- Marking locations filmed and demonstrating independent iterator invalidation.
- Running the automated tests and resetting the sample film.

The original scripted `main` remains commented in `src/main.cpp` as a reference.
The live `main` delegates behavior to the production classes rather than
duplicating lifecycle logic.

## Submission checklist

- Build and run the source with the required C++11 warning-as-error flags.
- Include the editable Draw.io diagrams and source files with the submission.
- Capture genuine GDB commands/screenshots for one investigated bug.
- Capture final Valgrind leak-check output where Valgrind is available.
- Include the final executable transcript in `output.txt`.
- Verify that the transcript and README use the implemented state names.

The editable Draw.io diagrams are the workflow reference for clearance,
authority contact, injury, and replacement. The PlantUML files are retained as
supporting documentation and are not modified by this implementation pass.