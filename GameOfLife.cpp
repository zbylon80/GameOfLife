#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdint>
#include <conio.h> // _kbhit, _getch (Windows)

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
        for (uint8_t c : cells)
            sum += (c != 0);
        return sum;
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

    void render(bool paused) const {
        std::cout << "\x1B[2J\x1B[H";

        std::cout
            << "Generation: " << generation
            << " | Alive: " << getAliveCount()
            << " | Edges: " << (torus ? "Torus" : "Hard")
            << " | " << (paused ? "PAUSED" : "RUNNING")
            << "\n";
        std::cout
            << "[Space]=Pause  [N]=Step  [R]=Reset  [G]=Glider  [Esc]=Quit\n\n";

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << (isAlive(x, y) ? '#' : '.');
            }
            std::cout << '\n';
        }
    }

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
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = (x + dx + width) % width;
                int ny = (y + dy + height) % height;
                count += isAlive(nx, ny);
            }
        return count;
    }

    int countNeighborsHardEdges(int x, int y) const {
        int count = 0;
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx = x + dx;
                int ny = y + dy;
                if (!inBounds(nx, ny)) continue;
                count += isAlive(nx, ny);
            }
        return count;
    }
};

void placeGlider(World& w, int x, int y) {
    w.setAlive(x + 1, y);
    w.setAlive(x + 2, y + 1);
    w.setAlive(x, y + 2);
    w.setAlive(x + 1, y + 2);
    w.setAlive(x + 2, y + 2);
}

int main() {
    World world(40, 20);
    world.setTorus(false);

    bool running = true;
    bool paused = true;

    const int fps = 10;
    const auto frameTime = std::chrono::milliseconds(1000 / fps);

    while (running) {
        // INPUT (non-blocking)
        if (_kbhit()) {
            char key = _getch();

            switch (key) {
            case ' ':
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
                break;
            case 'g':
            case 'G':
                world.clear();
                placeGlider(world, 1, 1);
                paused = true;
                break;
            case 27: // ESC
                running = false;
                break;
            }
        }

        // UPDATE
        if (!paused) {
            world.step();
        }

        // RENDER
        world.render(paused);

        std::this_thread::sleep_for(frameTime);
    }

    return 0;
}
