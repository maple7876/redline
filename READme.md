# REDLINE
<img width="1988" height="1272" alt="image" src="https://github.com/user-attachments/assets/78d005e8-240d-4d52-aeb5-197ce60199c4" />

REDLINE is a top-down arcade racing game built in C using raylib. I wanted to make something that feels fast, fun, and actually worth playing, with different cars, tracks, game modes, and stuff to unlock as you keep playing.

## Features

- 8 cars to unlock, each with different stats
- 6 unique tracks
- 3 game modes: Endless, Time Attack, and Overtake Challenge
- Nitro boosts
- Traffic and collision mechanics
- Near-miss bonuses and score combos
- Missions and objectives
- Garage with car selection and paint customization
- Unlockable cars and tracks
- High scores and saved progression
- Custom top-down graphics, road curves, roadside scenery, and speed effects

## How to Play

**Controls**

| Action | Controls |
|---|---|
| Accelerate | W / Up Arrow |
| Brake | S / Down Arrow |
| Steer | A / D or Left / Right Arrow |
| Nitro | Space / Shift |
| Pause / Back | P / Escape |
| Select / Start / Restart | Enter |
| Track Selection | T |
| Garage | G |
| Race Mode | M |
| Change Paint | C |

Use the arrow keys to navigate menus and Enter to select.

## Running on macOS

### Requirements
- macOS
- Xcode Command Line Tools
- Homebrew
- raylib

### 1. Install the requirements

Install the Command Line Tools:

```bash
xcode-select --install
```

Install raylib using Homebrew:

```bash
brew install raylib
```

### 2. Download the game

```bash
git clone https://github.com/maple7876/redline.git
cd redline
```

### 3. Compile

```bash
clang -std=c11 -Wall -Wextra -O2 \
  -I"$(brew --prefix raylib)/include" \
  src/main.c -o redline \
  -L"$(brew --prefix raylib)/lib" \
  -lraylib -framework OpenGL -framework Cocoa \
  -framework IOKit -framework CoreVideo
```

### 4. Run

```bash
./redline
```

## Running on Windows

### Requirements
- Windows
- GCC through MinGW-w64
- raylib for Windows, compatible with your compiler

### 1. Install the requirements

Install a MinGW-w64 toolchain and download a matching Windows version of raylib from the [official raylib GitHub repository](https://github.com/raysan5/raylib).

Make sure GCC is available in your terminal and note where raylib's `include` and `lib` folders are located.

### 2. Download the game

```bash
git clone https://github.com/maple7876/redline.git
cd redline
```

### 3. Compile

The following example assumes raylib is installed in `C:\raylib`. Change the paths if your installation is somewhere else.

Run in Windows Command Prompt:

```bat
gcc -std=c11 -Wall -Wextra -O2 src/main.c -o redline.exe ^
  -IC:/raylib/include -LC:/raylib/lib ^
  -lraylib -lopengl32 -lgdi32 -lwinmm
```

Your raylib package must match your compiler and system architecture.

### 4. Run

```bat
redline.exe
```

## Built With

- **C** for the game code
- **raylib** for graphics, input, and game functionality
- **Clang** on macOS
- **GCC / MinGW-w64** for the Windows build

REDLINE is my submission for **Hack Club's Wrong Tool**.
