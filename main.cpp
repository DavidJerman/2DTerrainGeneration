#define OLC_PGE_APPLICATION

#include "olcPixelGameEngine.h"
#include <vector>

namespace res {

    struct Tree {
        int x;
        int y;
        int radius;
        int height;
        int width;
        olc::Pixel barkColor;
        olc::Pixel leafColor;

        Tree(int x, int y, int r, int h, int w, olc::Pixel barkColor, olc::Pixel leafColor)
                : x(x), y(y), radius(r), height(h), width(w), barkColor(barkColor), leafColor(leafColor) {}
    };

    typedef std::vector<Tree *> TreeList;

    typedef std::vector<double> NoiseArray;
}

class ResourceContainer {
private:
    std::vector<uint32_t> earthColorPalette = {
            // BGR color format
            0xff213c51, 0xff28455a, 0xff285a33, 0xff399e28
    };
    std::vector<uint32_t> treeColorPalette = {
            // BGR color format - 4. is bark color, 1-3 leaves color
            0xff2aa220, 0xff2aa23a, 0xff2bc311, 0xff143a69
    };
public:
    ResourceContainer() = default;

    ~ResourceContainer() = default;

    uint32_t getEarthColor(int index) {
        return earthColorPalette[index];
    }

    uint32_t getTreeColor(int index) {
        return treeColorPalette[index];
    }
};

class Lehmer32 {
private:
    uint32_t lehmerState = 0;

public:
    explicit Lehmer32(uint32_t lehmerState = 0) {
        this->lehmerState = lehmerState;
    }

    uint32_t get() {
        lehmerState += 0xe120fc15;
        uint64_t tmp;
        tmp = (uint64_t) lehmerState * 0x4a39b70d;
        uint32_t m1 = (tmp >> 32) ^ tmp;
        tmp = (uint64_t) m1 * 0x12fad5c9;
        uint32_t m2 = (tmp >> 32) ^ tmp;
        return m2;
    }

    int rndInt(int min, int max) {
        return (int) (get() % (max - min)) + min;
    }

    // Returns a random double between 0.0 and 1.0 - a little fix was made to the code, since id did not return correct values
    double rndDouble(double min, double max) {
        auto res = ((double) get() / (double) (0x7FFFFFFF)) * (max - min) + min;
        if (res < min) res += (max - min);
        else if (res > max) res -= (max - min);
        return res;
    }

    bool rndBool() {
        return get() % 2;
    }
};

class World : public olc::PixelGameEngine {

    uint32_t seed = 0;
    res::NoiseArray noiseArray;
    res::TreeList treeList;

    static const int UPPER_BOUND = 200;
    static const int LOWER_BOUND = 800;
    static const int TREE_FREQ = 120;
    static const int TREE_BARK_HIDE_OFFSET = 15;

public:
    World() {
        sAppName = "2D World Generation";
    }

    bool OnUserCreate() override {
        // On create, create the noise array
        Lehmer32 rnd(seed);
        noiseArray = getNoiseArray(ScreenWidth(), rnd, 100, ScreenHeight() - 100, 2, 8);
        for (auto &tree: treeList)
            delete tree;
        ResourceContainer resources;
        treeList = getTreeList(ScreenWidth(), TREE_FREQ, noiseArray, resources, rnd);
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        Clear(olc::BLACK);

        ResourceContainer resources;

        // If space is pressed, generate a new seed and regenerate the noise array
        if (GetKey(olc::SPACE).bPressed) {
            seed += 1;
            Lehmer32 rnd(seed);
            noiseArray = getNoiseArray(ScreenWidth(), rnd, 100, ScreenHeight() - 100, 2, 8);
            for (auto &tree: treeList)
                delete tree;
            treeList = getTreeList(ScreenWidth(), TREE_FREQ, noiseArray, resources, rnd);
        }

        // If c is held, generate a new seed and regenerate the noise array repeatedly
        if (GetKey(olc::C).bHeld) {
            seed += 1;
            Lehmer32 rnd(seed);
            noiseArray = getNoiseArray(ScreenWidth(), rnd, 100, ScreenHeight() - 100, 2, 8);
            for (auto &tree: treeList)
                delete tree;
            treeList = getTreeList(ScreenWidth(), TREE_FREQ, noiseArray, resources, rnd);
        }

        // Draw the sky
        for (int i = 0; i < noiseArray.size(); i++) {
            for (int j = 0; j < noiseArray[i]; j++) {
                olc::Pixel pixel{0xffc5b576};
                Draw(i, j, pixel);
            }
        }

        // Draw the trees
        for (const auto &tree: treeList)
            drawTree(tree);

        // Draw the noise array - this is the ground
        for (int i = 0; i < noiseArray.size(); i++) {
            auto sub1 = ScreenHeight() - noiseArray[i];
            for (int j = 0; j <= ScreenHeight() - noiseArray[i]; j++) {
                int index;
                double percentage = (double) j / (double) sub1;
                if (percentage > 0.4) index = 0;
                else index = 1;
                if (j > 12 && j < 45) index = 2;
                else if (j <= 12) index = 3;
                olc::Pixel pixel{resources.getEarthColor(index)};
                Draw(i, (int) noiseArray[i] + j, pixel);
            }
        }

        return true;
    }

    static res::NoiseArray
    getNoiseArray(size_t size, Lehmer32 &lehmer, const int startRangeFrom, const int startRangeTo, const int range = 2,
                  const double SMOOTH_FACTOR = 8, const double VEL_RATIO = -1.0) {

        res::NoiseArray noiseArray(size);
        noiseArray[0] = lehmer.rndInt(startRangeFrom, startRangeTo);

        double vel = 0;

        // Create the noise array
        for (size_t i = 1; i < size; i++) {
            auto from = noiseArray[i - 1] - range;
            auto to = noiseArray[i - 1] + range;
            noiseArray[i] = lehmer.rndDouble(from - vel, to + vel);

            // Bounds check
            if (noiseArray[i] < UPPER_BOUND) noiseArray[i] = lehmer.rndDouble(UPPER_BOUND, UPPER_BOUND + range);
            else if (noiseArray[i] > LOWER_BOUND)
                noiseArray[i] = lehmer.rndDouble(LOWER_BOUND - range, LOWER_BOUND);

            // Velocity change
            auto acc = lehmer.rndDouble(-vel * 0.1, vel * 0.1) + vel;
            vel += acc;
            if (vel > range / (VEL_RATIO / 2)) vel = range / (VEL_RATIO / 3);
            if (vel < -range / (VEL_RATIO / 2)) vel = -range / (VEL_RATIO / 3);
        }

        // Smooth out the terrain - this is a bit slow, but it works
        for (size_t j = 0; j < size; j++) {
            double avg = 0;
            int c = 0;
            for (int k = 0; k < SMOOTH_FACTOR && (j - k) > 0; k++, c++)
                avg += noiseArray[j - k];
            if (c)
                avg /= c;
            else
                continue;
            noiseArray[j] = avg;
        }

        return noiseArray;
    }

    // drawTree(new res::Tree(100, 100, 16, 30, 10, resources.getTreeColor(3), resources.getTreeColor(0)));
    void drawTree(const res::Tree *tree) {
        FillRect(tree->x, tree->y - tree->height + TREE_BARK_HIDE_OFFSET, tree->width, tree->height, tree->barkColor);
        FillCircle(tree->x + tree->width / 2, tree->y - tree->radius / 2 - tree->height + TREE_BARK_HIDE_OFFSET,
                   tree->radius, tree->leafColor);
    }

    res::TreeList
    getTreeList(int size, int frequency, res::NoiseArray &noiseArray, ResourceContainer &resources, Lehmer32 &rnd) {
        res::TreeList tList;
        int x = 40;
        x += rnd.rndInt(frequency / 2, frequency / 2 * 3);
        while (x < ScreenWidth() - 40) {
            auto y = noiseArray[x];
            auto w = rnd.rndInt(6, 14);
            auto h = rnd.rndInt(36, 56) + TREE_BARK_HIDE_OFFSET;
            auto r = rnd.rndDouble(2.1, 2.6) * w;
            auto leaf = rnd.rndInt(0, 3);
            auto bark = 3;
            tList.push_back(
                    new res::Tree(x, (int) y, (int) r, h, w, resources.getTreeColor(bark),
                                  resources.getTreeColor(leaf)));
            x += rnd.rndInt(frequency / 2, frequency / 2 * 3);
        }
        return tList;
    }
};

int main() {

    if (World demo; demo.Construct(2048, 1024, 1, 1))
        demo.Start();

    return 0;
}
