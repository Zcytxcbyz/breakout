# Breakout - Brick Breaker Game

A brick breaker game based on the **Box2D** physics engine and **SFML** graphics library, using **ImGui** for the UI interface. Features random brick health, collision acceleration, and standalone executable operation.

---

## ✨ Game Features

- Classic brick breaker gameplay: move the paddle to bounce the ball, break all bricks to win
- **Random brick system**: Bricks spawn with a certain probability, have 1-3 health points, different colors represent different health levels
- **Physics collision**: Fully elastic collisions, natural interaction between ball, paddle, bricks, and boundaries
- **Dynamic difficulty**: Ball speed slightly increases after each collision, with a maximum speed limit, game pace gradually accelerates
- **Complete UI**:
  - Main menu (Start Game / Help / Exit)
  - Help window (control instructions and brick color reference)
  - Real-time score display
  - Game over window (win/loss, with options to restart or return to main menu)
- **Single-file executable**: All dependencies statically compiled, no additional DLLs required, only depends on Windows system libraries

---

## 🎮 Controls

| Key              | Function          |
|------------------|-------------------|
| `A` / `←`        | Move paddle left  |
| `D` / `→`        | Move paddle right |
| `P` / `ESC`      | Pause game        |

Objective: Break all bricks. If the ball falls off the bottom, the game ends in defeat.

---

## 📦 Dependencies

The project uses the following libraries:

- **SFML 3.0.2** (graphics, window, system)
- **Box2D 2.4.2** (physics engine)
- **Dear ImGui 1.91.9** (UI library)
- **imgui-sfml 3.0** (ImGui binding for SFML)

All dependencies are stored as source code in the `libs/` directory. A helper script is provided to automatically clone them.

---

## 🔧 Getting Dependencies

Run the provided Python script to clone all required dependencies into the `libs/` folder:

```bash
python clone_deps.py
```

This script requires **Python 3** and **Git** to be installed and available in your PATH. It will clone each library at the exact version used by the project.

---

## 🔨 Compilation and Building

### Requirements

- Windows 10/11
- Visual Studio 2022 or higher (or MinGW-w64)
- CMake 3.15 or higher
- Python 3 (for fetching dependencies, optional if you manually place them)

### Steps

1. **Clone this repository** (or download the source code):
   ```bash
   git clone https://github.com/Zcytxcbyz/breakout.git
   cd breakout
   ```

2. **Fetch dependencies** (if not already present):
   ```bash
   python clone_deps.py
   ```

3. **Generate project files**  
   Open a terminal and execute the following commands (using Visual Studio 2026 as an example):
   ```bash
   mkdir build
   cd build
   cmake .. -G "Visual Studio 18 2026" -A x64 -DCMAKE_BUILD_TYPE=Release
   ```

4. **Compile**  
   Open the generated `breakout.sln` in Visual Studio, select the `Release` configuration, and build the solution; or directly from the command line:
   ```bash
   cmake --build . --config Release
   ```

5. **Run**  
   After compilation, the executable file is located at `build/Release/breakout.exe`, run it directly.

> **Note**: If using MinGW-w64, change the generator to `"MinGW Makefiles"` and ensure the corresponding toolchain is installed.

---

## 🚀 Running and Distribution

- The **Release version** is statically linked with all dependencies and has the console window disabled, allowing direct double-click execution.
- For distribution, simply copy `breakout.exe` to the target machine; no additional DLLs are required.

---

## 🛠️ Custom Configuration

Game parameters and UI texts are embedded as resources within the executable. You can easily customize them using **Resource Hacker** without recompiling the game.

### Available Parameters

All game settings are stored in `config.ini` (embedded as `CONFIG` resource), including:

- **Window**: size, frame rate
- **Physics**: pixels per meter, gravity
- **Ball**: radius, density, restitution, initial speed, max speed, speed factor
- **Paddle**: width, height, speed, density, restitution
- **Walls**: thickness
- **Bricks**: rows, columns, spacing, spawn probability, health range, score per brick
- **Offsets**: ball and paddle vertical positions

All UI texts (menu labels, help messages, score display, etc.) are stored in `texts.ini` (embedded as `TEXTS` resource).

### How to Modify

1. Download and install [Resource Hacker](https://www.angusj.com/resourcehacker/).
2. Open `breakout.exe` with Resource Hacker.
3. In the left tree view, expand **RCDATA** → **CONFIG** (for game parameters) or **TEXTS** (for UI texts).
4. Edit the content directly in the right panel.
5. Click **Compile Script** and then **Save** the file.

The changes will take effect the next time you run the game. If a resource is missing or corrupted, the game falls back to the default values, ensuring it still runs.

> **Note**: Ensure that the INI syntax is preserved; comments (lines starting with `;`) are ignored and can be used for documentation.

---

## 📄 License

This project is for learning and communication purposes only. You are free to modify and distribute, but please retain the original author information.

---

## 🙏 Acknowledgments

- [SFML](https://www.sfml-dev.org/) – Simple and easy-to-use multimedia library
- [Box2D](https://box2d.org/) – Powerful 2D physics engine
- [Dear ImGui](https://github.com/ocornut/imgui) – Lightweight immediate mode GUI
- [imgui-sfml](https://github.com/SFML/imgui-sfml) – Bridge layer between ImGui and SFML

---

## 📧 Contact

If you have any questions or suggestions, feel free to submit an Issue or contact the author.

**Enjoy the game!**
