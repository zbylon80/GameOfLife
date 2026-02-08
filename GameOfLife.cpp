#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdint>
#include <conio.h>
#include <algorithm>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef _WIN32
class WinConsole {
public:
    static void init() {
        hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    static void hideCursor() {
        if (!hOut) return;
        CONSOLE_CURSOR_INFO info{};
        info.dwSize = 25;
        info.bVisible = FALSE;
        SetConsoleCursorInfo(hOut, &info);
    }

    static void showCursor() {
        if (!hOut) return;
        CONSOLE_CURSOR_INFO info{};
        info.dwSize = 25;
        info.bVisible = TRUE;
        SetConsoleCursorInfo(hOut, &info);
    }

    static void home() {
        if (!hOut) return;
        COORD c{ 0, 0 };
        SetConsoleCursorPosition(hOut, c);
    }

    static void clearScreen() {
        if (!hOut) return;

        CONSOLE_SCREEN_BUFFER_INFO csbi{};
        if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;

        DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
        DWORD written = 0;
        COORD homeCoord{ 0, 0 };

        FillConsoleOutputCharacterA(hOut, ' ', cellCount, homeCoord, &written);
        FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, homeCoord, &written);
        SetConsoleCursorPosition(hOut, homeCoord);
    }

private:
    static inline HANDLE hOut = nullptr;
};
#endif

class World {
public:
    World(int w, int h)
        : width(w), height(h),
        cells(w* h, 0),
        next(w* h, 0) {
    }

    void setTorus(bool enabled) { torus = enabled; }

    void clear() {
        std::fill(cells.begin(), cells.end(), 0);
        generation = 0;
    }

    uint64_t getGeneration() const { return generation; }

    int getAliveCount() const {
        int sum = 0;
        for (uint8_t c : cells) sum += (c != 0);
        return sum;
    }

    void toggleCell(int x, int y) {
        if (!inBounds(x, y)) return;
        auto& c = cells[index(x, y)];
        c = (c == 0) ? 1 : 0;
    }

    void setAlive(int x, int y, bool alive = true) {
        if (!inBounds(x, y)) return;
        cells[index(x, y)] = alive ? 1 : 0;
    }

    bool isAlive(int x, int y) const {
        if (!inBounds(x, y)) return false;
        return cells[index(x, y)] != 0;
    }

    void step() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int n = torus ? countNeighborsTorus(x, y)
                    : countNeighborsHardEdges(x, y);

                bool alive = isAlive(x, y);
                bool nextAlive = alive ? (n == 2 || n == 3) : (n == 3);
                next[index(x, y)] = nextAlive ? 1 : 0;
            }
        }
        cells.swap(next);
        generation++;
    }

    // Render: budujemy ramkę w stringu i wypisujemy raz
    void render(bool paused,
        int cursorX,
        int cursorY,
        const std::string& sizeLabel,
        bool fullClearOnce) const
    {
#ifdef _WIN32
        if (fullClearOnce) WinConsole::clearScreen();
        else WinConsole::home();
#else
        // Fallback (ANSI) dla innych systemów
        if (fullClearOnce) std::cout << "\x1B[2J\x1B[H";
        else std::cout << "\x1B[H";
#endif

        std::string frame;
        frame.reserve(static_cast<size_t>((width + 1) * height + 512));

        frame += "Generation: " + std::to_string(generation)
            + " | Alive: " + std::to_string(getAliveCount())
            + " | Edges: " + std::string(torus ? "Torus" : "Hard")
            + " | " + (paused ? std::string("PAUSED") : std::string("RUNNING"))
            + "\n";

        frame += "[P]=Pause/Run  [WASD/Arrows]=Move (paused)  [Space]=Toggle (paused)  [N]=Step (paused)\n";
        frame += "[E]=Toggle Edges  [R]=Reset  [Esc]=Quit\n";
        frame += "[0]=Clear  [1]=Glider  [2]=Block  [3]=Blinker  [4]=Toad  [5]=Beacon (place at cursor, paused)\n\n";

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bool alive = isAlive(x, y);

                if (paused && x == cursorX && y == cursorY) {
                    frame += (alive ? 'X' : '@');
                }
                else {
                    frame += (alive ? '#' : '.');
                }
            }
            frame += '\n';
        }

        std::cout << frame;
        std::cout.flush();
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    int width;
    int height;
    bool torus = false;
    uint64_t generation = 0;

    std::vector<uint8_t> cells;
    std::vector<uint8_t> next;

    bool inBounds(int x, int y) const {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    int index(int x, int y) const {
        return y * width + x;
    }

    int countNeighborsTorus(int x, int y) const {
        int count = 0;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = (x + dx + width) % width;
                int ny = (y + dy + height) % height;
                count += isAlive(nx, ny);
            }
        }
        return count;
    }

    int countNeighborsHardEdges(int x, int y) const {
        int count = 0;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx;
                int ny = y + dy;
                if (!inBounds(nx, ny)) continue;
                count += isAlive(nx, ny);
            }
        }
        return count;
    }
};

// ---------- Patterns ----------
void placeGlider(World& w, int x, int y) {
    w.setAlive(x + 1, y + 0);
    w.setAlive(x + 2, y + 1);
    w.setAlive(x + 0, y + 2);
    w.setAlive(x + 1, y + 2);
    w.setAlive(x + 2, y + 2);
}

void placeBlock(World& w, int x, int y) {
    w.setAlive(x + 0, y + 0);
    w.setAlive(x + 1, y + 0);
    w.setAlive(x + 0, y + 1);
    w.setAlive(x + 1, y + 1);
}

void placeBlinker(World& w, int x, int y) {
    w.setAlive(x + 0, y + 0);
    w.setAlive(x + 1, y + 0);
    w.setAlive(x + 2, y + 0);
}

void placeToad(World& w, int x, int y) {
    w.setAlive(x + 1, y + 0);
    w.setAlive(x + 2, y + 0);
    w.setAlive(x + 3, y + 0);

    w.setAlive(x + 0, y + 1);
    w.setAlive(x + 1, y + 1);
    w.setAlive(x + 2, y + 1);
}

void placeBeacon(World& w, int x, int y) {
    w.setAlive(x + 0, y + 0);
    w.setAlive(x + 1, y + 0);
    w.setAlive(x + 0, y + 1);
    w.setAlive(x + 1, y + 1);

    w.setAlive(x + 2, y + 2);
    w.setAlive(x + 3, y + 2);
    w.setAlive(x + 2, y + 3);
    w.setAlive(x + 3, y + 3);
}

int clamp(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

struct SizePreset {
    int w;
    int h;
    const char* label;
};

int main() {
#ifdef _WIN32
    WinConsole::init();
    WinConsole::hideCursor();
#else
    std::cout << "\x1B[?25l";
#endif

    const SizePreset presets[] = {
        { 40, 20, "Small"  },
        { 60, 25, "Medium" },
        { 80, 30, "Large"  }
    };
    const int presetCount = sizeof(presets) / sizeof(presets[0]);

    int presetIndex = 0;
    bool torusEdges = false;

    World world(presets[presetIndex].w, presets[presetIndex].h);
    world.setTorus(torusEdges);

    bool fullClearNextRender = true;

    auto recreateWorld = [&]() {
        world = World(presets[presetIndex].w, presets[presetIndex].h);
        world.setTorus(torusEdges);
        fullClearNextRender = true;
    };

    bool running = true;
    bool paused = true;

    int cursorX = 0;
    int cursorY = 0;

    const int fps = 30;
    const auto frameTime = std::chrono::milliseconds(1000 / fps);

    while (running) {
        if (_kbhit()) {
            int key = _getch();

            // Arrow keys: 0 or 224 + second code
            if (key == 0 || key == 224) {
                int special = _getch();
                if (paused) {
                    switch (special) {
                    case 72: cursorY--; break; // Up
                    case 80: cursorY++; break; // Down
                    case 75: cursorX--; break; // Left
                    case 77: cursorX++; break; // Right
                    }
                }
            }
            else {
                switch (key) {
                case 'p':
                case 'P':
                    paused = !paused;
                    break;

                case 'n':
                case 'N':
                    if (paused) world.step();
                    break;

                case 'r':
                case 'R':
                    world.clear();
                    paused = true;
                    cursorX = 0;
                    cursorY = 0;
                    fullClearNextRender = true;
                    break;

                case 27: // ESC
                    running = false;
                    break;

                case 'e':
                case 'E':
                    torusEdges = !torusEdges;
                    world.setTorus(torusEdges);
                    break;

                case ' ':
                    if (paused) world.toggleCell(cursorX, cursorY);
                    break;

                // WASD movement (paused)
                case 'w':
                case 'W':
                    if (paused) cursorY--;
                    break;
                case 's':
                case 'S':
                    if (paused) cursorY++;
                    break;
                case 'a':
                case 'A':
                    if (paused) cursorX--;
                    break;
                case 'd':
                case 'D':
                    if (paused) cursorX++;
                    break;

                // Pattern hotkeys (paused)
                case '0':
                    if (paused) { world.clear(); fullClearNextRender = true; }
                    break;
                case '1':
                    if (paused) placeGlider(world, cursorX, cursorY);
                    break;
                case '2':
                    if (paused) placeBlock(world, cursorX, cursorY);
                    break;
                case '3':
                    if (paused) placeBlinker(world, cursorX, cursorY);
                    break;
                case '4':
                    if (paused) placeToad(world, cursorX, cursorY);
                    break;
                case '5':
                    if (paused) placeBeacon(world, cursorX, cursorY);
                    break;
                }
            }
        }

        cursorX = clamp(cursorX, 0, world.getWidth() - 1);
        cursorY = clamp(cursorY, 0, world.getHeight() - 1);

        if (!paused) {
            world.step();
        }

        world.render(paused, cursorX, cursorY, presets[presetIndex].label, fullClearNextRender);
        fullClearNextRender = false;

        std::this_thread::sleep_for(frameTime);
    }

#ifdef _WIN32
    WinConsole::showCursor();
#else
    std::cout << "\x1B[?25h";
#endif

    return 0;
}
