#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdint>  // uint8_t
#include <algorithm> // std::count

class World {
public:
    World(int w, int h)
        : width(w), height(h), cells(w* h, 0), next(w* h, 0) {
    }

    void setTorus(bool enabled) { torus = enabled; }

    // Debug/info getters
    uint64_t getGeneration() const { return generation; }

    int getAliveCount() const {
        // cells trzyma 0/1, więc sumowanie daje liczbę żywych
        int sum = 0;
        for (uint8_t c : cells) sum += (c != 0) ? 1 : 0;
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

    void render() const {
        // Clear screen + home cursor (ANSI)
        std::cout << "\x1B[2J\x1B[H";

        std::cout << "Generation: " << generation
            << " | Alive: " << getAliveCount()
            << " | Edges: " << (torus ? "Torus (wrap)" : "Hard")
            << "\n\n";

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
                int n = torus ? countNeighborsTorus(x, y)
                    : countNeighborsHardEdges(x, y);

                bool alive = isAlive(x, y);

                bool nextAlive = false;
                if (alive) {
                    // Survive with 2 or 3 neighbors
                    nextAlive = (n == 2 || n == 3);
                }
                else {
                    // Born with exactly 3 neighbors
                    nextAlive = (n == 3);
                }

                next[index(x, y)] = nextAlive ? 1 : 0;
            }
        }

        cells.swap(next);
        generation++;
    }

private:
    int width;
    int height;

    bool torus = true;
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

                count += isAlive(nx, ny) ? 1 : 0;
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

                if (nx < 0 || nx >= width || ny < 0 || ny >= height)
                    continue;

                count += isAlive(nx, ny) ? 1 : 0;
            }
        }
        return count;
    }
};

// Pattern: glider
void placeGlider(World& w, int x, int y) {
    w.setAlive(x + 1, y + 0);
    w.setAlive(x + 2, y + 1);
    w.setAlive(x + 0, y + 2);
    w.setAlive(x + 1, y + 2);
    w.setAlive(x + 2, y + 2);
}

// Pattern: stable 2x2 block
void placeBlock(World& w, int x, int y) {
    w.setAlive(x + 0, y + 0);
    w.setAlive(x + 1, y + 0);
    w.setAlive(x + 0, y + 1);
    w.setAlive(x + 1, y + 1);
}

int main() {
    World world(40, 20);

    // Switch edges:
    // world.setTorus(true);   // wrap edges
    world.setTorus(false);     // hard edges

    // Start setup:
    placeGlider(world, 1, 1);
    // placeBlock(world, 10, 10);

    const int fps = 10;
    const auto frameTime = std::chrono::milliseconds(1000 / fps);

    while (true) {
        world.render();
        world.step();
        std::this_thread::sleep_for(frameTime);
    }

    return 0;
}
