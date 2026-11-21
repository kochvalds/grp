# GRAND RESCUE PROTOCOL
Open-world parody action game in an 80s neon-city style by the fictional studio **KZstar Kokshetay**.
Unofficial parody. Not affiliated with Rockstar Games / Take-Two. All textures, music and sounds are generated in code.

## Features
- 40x40-block city (~1.7 km): downtown, beach, docks, suburbs, rich hills, procedural textures, day/night, rain
- 11 story missions with cinematic cutscenes (camera cuts, portraits, 8 characters)
- Cars (sedan/sport/van), police, gangs, beer kiosks, drunk effects, phone, wanted system
- Built-in synthesizer radio (3 stations + theme) and generated sound effects

## Controls
WASD move, Mouse camera, Shift sprint, Space jump / handbrake, LMB shoot, 1/2/3 weapons,
E enter/exit car or buy beer, B drink beer, TAB phone, Q radio (in car), Esc pause.

## Build
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j

## GitHub
Push to GitHub: the workflow builds a Windows .exe (zip) and a Linux AppImage (Actions -> Artifacts).
Tag a release (`git tag v0.2.0 && git push --tags`) to publish both files in Releases.
