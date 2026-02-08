#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdint>
#include <conio.h>
#include <algorithm>

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

    void render(bool paused, int cursorX, int cursorY) const {
        std::cout << "\x1B[H";

        std::cout
            << "Generation: " << generation
            << " | Alive: " << getAliveCount()
            << " | Edges: " << (torus ? "Torus" : "Hard")
            << " | " << (paused ? "PAUSED" : "RUNNING")
            << "\n";

        std::cout
            << "[P]=Pause/Run  [WASD/Arrows]=Move (paused)  [Space]=Toggle (paused)  [N]=Step (paused)\n"
            << "[0]=Clear  [1]=Glider  [2]=Block  [3]=Blinker  [4]=Toad  [5]=Beacon  [Esc]=Quit\n\n";

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                bool alive = isAlive(x, y);

                if (paused && x == cursorX && y == cursorY) {
                    std::cout << (alive ? 'X' : '@');
                }
                else {
                    std::cout << (alive ? '#' : '.');
                }
            }
            std::cout << '\n';
        }

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

int main() {
    std::cout << "\x1B[?25l"; // hide cursor

    World world(40, 20);
    world.setTorus(false);

    bool running = true;
    bool paused = true;

    int cursorX = 0;
    int cursorY = 0;

    const int fps = 30;
    const auto frameTime = std::chrono::milliseconds(1000 / fps);

    while (running) {
        if (_kbhit()) {
            int key = _getch();

            // Arrow keys and some special keys come as 0 or 224, then another code
            if (key == 0 || key == 224) {
                int special = _getch();
                if (paused) {
                    switch (special) {
                    case 72: // Up
                        cursorY--;
                        break;
                    case 80: // Down
                        cursorY++;
                        break;
                    case 75: // Left
                        cursorX--;
                        break;
                    case 77: // Right
                        cursorX++;
                        break;
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
                    break;

                case 27: // ESC
                    running = false;
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
                    if (paused) world.clear();
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

        world.render(paused, cursorX, cursorY);
        std::this_thread::sleep_for(frameTime);
    }

    std::cout << "\x1B[?25h"; // show cursor
    return 0;
}
