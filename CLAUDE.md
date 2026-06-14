# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

- **Solution file:** `easyX-test.slnx` (VS2022+ format)
- **Toolchain:** MSVC v145, C++20, Unicode charset, x64 Debug/Release
- **Required extension:** EasyX graphics library (installed as VS .vsix extension)
- Build in Visual Studio: open `.slnx` → `Ctrl+Shift+B`, run with `F5`
- The `FileName.cpp` in the project is a vestigial reference file (entirely commented out)

## Architecture

Menu-driven application: `main.cpp` runs a loop that always shows `menu()` first, then dispatches to pages or the game.

```
main.cpp  →  menu()  →  GameStart()         [blocks until ESC in pause/game-over]
                    →  HelpPage()           [blocks until ESC]
                    →  AboutPage()          [blocks until ESC]
                    →  LeaderboardPage()    [blocks until ESC]
                    →  return 0 (exit app)
```

### Game loop (`GameStart`)
Runs in a single `while (g_inGame)` loop with no batch-draw mode:

1. `HandleGameInput()` — drains `peekmessage(&msg, EX_KEY)` queue each frame
2. If `paused || over` → only render, skip physics
3. Otherwise → `UpdateGame()` (physics, obstacles, collision) → `DrawGame()` → `Sleep(16)`

### Pause & exit state machine
- **ESC during gameplay:** `gs.paused = true` (game renders pause overlay)
- **Space/Up/Enter during pause:** unpause
- **ESC during pause or game-over:** `g_inGame = false` → `GameStart` returns → back to menu
- **R during game-over:** `InitGameState()` + immediate `return` from `HandleGameInput` (avoids stale input)
- The game **never** calls `menu()` internally — all menu interaction is handled in `main.cpp`

## Key files

| File | Role |
|------|------|
| `main.h` | Shared header: EasyX includes, `WINDOW_WID=650`, `WINDOW_HEI=500`, all function declarations, `extern IMAGE bg2` |
| `main.cpp` | Entry point, `while(result) { result = menu(); }` loop |
| `menu.cpp` | Menu rendering, keyboard navigation, `EnterMenu()` dispatches to pages/game |
| `game.cpp` | Entire parkour game: structs, state, physics, rendering, input, collision |
| `help.cpp` / `about.cpp` / `rang.cpp` | Static info pages with ESC-to-return |

## Cross-file linkages

- `int g_highScore` — defined in `game.cpp`, referenced via `extern` in `rang.cpp`
- `bool g_inGame` — defined in `game.cpp`, set true on game start, false on exit; declared `extern` in `main.h`
- `IMAGE bg2` — defined in `menu.cpp`, loaded by `InitMenu()`; declared `extern` in `main.h`, used by help/about/rang pages for background

## Sprite resources (`public/`)

| File | Used by | Role |
|------|---------|------|
| `b1.png`, `b2.png` | game.cpp | Player sprites (b1 = stand/run, b2 = crouch/alt-frame) |
| `c1.png`, `c2.png` | game.cpp | Obstacle sprites (two types, randomly chosen) |
| `bg1.png`, `bg2.png` | menu.cpp, help/about/rang | Background images (650×500) |
| `button.png`, `arrow.png` | menu.cpp | Menu UI elements |

## Conventions & pitfalls

- **String macros:** Use `_T("...")` and `TCHAR` throughout (not `L"..."` or `wchar_t`). The project is Unicode but `_T` keeps it consistent with EasyX's `loadimage(_T("path"))` pattern. Use `_stprintf_s` for formatted strings — it has a template overload that deduces buffer size from fixed arrays.
- **No batch drawing:** The game renders directly to screen. `BeginBatchDraw`/`FlushBatchDraw`/`EndBatchDraw` are deliberately NOT used — mixing batch mode with menu/page drawing causes the menu to render to the wrong buffer.
- **No Unicode escapes:** Avoid `\xNNNN` escape sequences in string literals. Use plain ASCII text instead (e.g., `">"` not `"\x25B6"`).
- **Struct layout:** `Player` and `Obstacle` are file-scope structs in `game.cpp`. The game state `gs` is a file-scope anonymous struct with `static` linkage.
- **Collision:** AABB with inset margins (8px X, 5px Y) for fairness. Defined in `CheckCollision()`.
- **Speed curve:** `BASE_SPEED=5`, +1 per 100 points, `MAX_SPEED=14`. Spawn delay decreases with speed.
