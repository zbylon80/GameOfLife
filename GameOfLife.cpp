#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

class World {
public:
    World(int w, int h)
        : width(w), height(h), cells(w* h, 0), next(w* h, 0) {
    }

    void setAlive(int x, int y, bool alive = true) {
        if (!inBounds(x, y)) return;
        cells[index(x, y)] = alive ? 1 : 0;
    }

    bool isAlive(int x, int y) const {
        if (!inBounds(x, y)) return false;
        return cells[index(x, y)] != 0;
    }

    void render() const {
        // Czyścimy ekran w prosty sposób (ANSI). Na Windows 11 zwykle działa w Windows Terminal.
        std::cout << "\x1B[2J\x1B[H";

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                std::cout << (isAlive(x, y) ? '#' : '.');
            }
            std::cout << '\n';
        }
    }

    void step() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int n = countNeighborsTorus(x, y);
                bool alive = isAlive(x, y);

                bool nextAlive = false;
                if (alive) {
                    // żywa: przetrwa przy 2 lub 3 sąsiadach
                    nextAlive = (n == 2 || n == 3);
                }
                else {
                    // martwa: ożyje przy dokładnie 3 sąsiadach
                    nextAlive = (n == 3);
                }

                next[index(x, y)] = nextAlive ? 1 : 0;
            }
        }
        cells.swap(next);
    }

private:
    int width;
    int height;
    std::vector<uint8_t> cells;
    std::vector<uint8_t> next;

    bool inBounds(int x, int y) const {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    int index(int x, int y) const {
        return y * width + x;
    }

    // Liczenie sąsiadów z zawijaniem (torus)
    int countNeighborsTorus(int x, int y) const {
        int count = 0;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;

                int nx = (x + dx + width) % width;
                int ny = (y + dy + height) % height;

                count += isAlive(nx, ny) ? 1 : 0;
            }
        }
        return count;
    }
};

// Prosty “pattern”: glider
void placeGlider(World& w, int x, int y) {
    w.setAlive(x + 1, y + 0);
    w.setAlive(x + 2, y + 1);
    w.setAlive(x + 0, y + 2);
    w.setAlive(x + 1, y + 2);
    w.setAlive(x + 2, y + 2);
}

int main() {
    World world(40, 20);

    // Warunek początkowy (na start gotowy pattern)
    placeGlider(world, 1, 1);

    const int fps = 10;
    const auto frameTime = std::chrono::milliseconds(1000 / fps);

    while (true) {
        world.render();
        world.step();
        std::this_thread::sleep_for(frameTime);
    }

    return 0;
}
