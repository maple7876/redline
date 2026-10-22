# REDLINE — Ashwick Prototype

A custom-coded, top-down retro street-racing career game in **C + raylib**. This is an early playable slice, not the finished game. The existing Python terminal game in `~/redline/main.py` should remain untouched; develop this in a separate folder/branch.

## Current prototype
- Retro low-resolution title screen.
- Opening story cutscene with dialogue, letterboxing, and typewriter text.
- First explorable top-down night scene.
- WASD / arrow-key driving.
- Simple speed, steering, and screen-boundary collision.
- Pause screen.
- Original setting details inspired by McHenry County, Illinois: rail lines, depot, water tower, prairie/tree-line edge, county road, small industrial buildings, and a roadside station.

## Build on macOS (Apple Silicon)
Install Apple's Command Line Tools if needed:
```bash
xcode-select --install
```

Install raylib with Homebrew:
```bash
brew install raylib
```

Compile from the project root:
```bash
clang -std=c11 -Wall -Wextra -O2 src/main.c -o redline \
  -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
```

Run:
```bash
./redline
```

If Homebrew installs raylib somewhere nonstandard, inspect:
```bash
brew --prefix raylib
```
and add the corresponding include/library paths to the compile command.

Controls:
- Enter/Space: advance title and cutscene.
- W/A/S/D or arrow keys: drive.
- Enter: dismiss driving help.
- Escape: pause/resume.
- Backspace from pause: return to title.

## Full game vision (keep in scope)
REDLINE is a polished top-down, story-driven retro street-racing career game set in **fictional Ashwick, Illinois**, inspired heavily by McHenry County and nearby Great Lakes communities. The setting should feel like northern Illinois rather than a generic racing city: old downtown storefronts, Metra-era commuter rail/depot infrastructure, freight tracks, Fox River and lake-country atmosphere, forest preserves, glacially shaped gentle hills, prairie and cornfields, water towers, strip malls, independent garages, diners, and late-night gas stations. Use fictional names/layouts and avoid implying real locations host illegal races.

Tone: atmospheric, grounded, nostalgic, character-driven. Pixel-art presentation with a low-resolution render target, night/rain atmosphere, readable silhouettes, amber streetlights, wet-road highlights, and original/licensed audio.

Core career:
1. **First Lap** — easy circuit event; learn handling and race against a small field.
2. **Sideways** — medium drift competition with scoring, combos, and a specialist rival.
3. **The Long Way Home** — hard touge-inspired head-to-head on a fictional closed course with winding wooded roads. Northern Illinois isn't mountainous; convey the challenge through bends, river bluffs, wooded corridors, and terrain inspired by the wider Great Lakes region.

Full game systems to build after the prototype:
- Start menu: New Career, Continue, Garage, Records, Settings.
- Story cutscenes and dialogue between events; character portraits and scene transitions.
- Town map and districts: old downtown, industrial/rail district, suburban commercial strip, county-line countryside, lake/woodland district.
- Garage, car viewing/customization, upgrades, repairs, money, reputation, save system.
- Rival, mechanic, drift specialist, event organizer, veteran racer.
- AI opponents, track checkpoints, race positions, lap timing, results screens.
- Distinct race rules, story consequences, soundtrack and sound effects.
- Race outcomes influence dialogue/reputation but do not dead-end the story.

Development principle: ship a small polished playable slice first, then add race logic, AI, drift scoring, touge, career progression, and more story content. Do not claim a feature is implemented until it has been coded and tested.

## McHenry County research and inspiration
This is a fictionalized setting, not a literal map recreation. Real-world inspiration grounded in the following sources:

- McHenry County's official scenic roads program emphasizes rural roads, vistas, community identity, and preservation of natural/historic road corridors:
  https://www.mchenrycountyil.gov/departments/planning-development/commissions-committees-boards/historic-preservation-commission/scenic-drives
- The county's rural historic survey documents the Fox River corridor, including the river's path through McHenry, Nunda/Crystal Lake-area townships, Fox River Grove, and Algonquin:
  https://www.mchenrycountyil.gov/home/showpublisheddocument/56865/635847434261500000
- County historical material describes Woodstock's role as county seat and the historical impact of railroads and settlement:
  https://www.mchenrycountyil.gov/county-government/county-yearbook/facts-to-remember
- Illinois DNR describes the Chain O'Lakes region across McHenry and Lake counties, with interconnected lakes and the Fox River:
  https://dnr.illinois.gov/parks/about/park.chainolakes.html
- McHenry County history of railroads describes the importance of the rail network and communities including Crystal Lake, Cary, Huntley, Harvard, and Woodstock:
  https://www.mchenrycountyliving.com/history/railroadingthroughthecounty

Design translations:
- Use a railway depot, tracks, industrial sheds, and commuter-town rhythms as visual motifs.
- Contrast denser downtown blocks with low-rise commercial corridors and rural edges.
- Use Fox River/lake-country scenery as a regional identity cue.
- Use prairie, farm fields, tree lines, and forest-preserve edges; avoid exaggerated mountains in northern Illinois.
- Use fictional street layouts, businesses, people, and race locations. The racing events occur on closed fictional courses in-game, not real public roads.

## Important
raylib is a multimedia programming library, not a full game engine. A Slack challenge thread reportedly answered "Works" to a question about raylib, SFML, and raw OpenGL, but the reply appears to be from a participant rather than a definitive organizer ruling. Verify eligibility with organizers if needed.
