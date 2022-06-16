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

    struct CloudPart {
        int x;
        int y;
        int r;
        olc::Pixel color;

        CloudPart(int x, int y, int r, uint32_t color) : x(x), y(y), r(r), color(color) {}
    };

    struct Cloud {
        int x;
        int y;
        std::vector<CloudPart *> cloudParts;

        Cloud(int x, int y) : x(x), y(y) {}

        ~Cloud() {
            for (auto &cloudPart: cloudParts)
                delete cloudPart;
            cloudParts.clear();
        }
    };

    typedef std::vector<Tree *> TreeList;

    typedef std::vector<double> NoiseArray;

    typedef std::vector<Cloud *> CloudList;
}

class ResourceContainer {
private:
    std::vector<uint32_t> earthColorPalette = {
            // BGR color format
            0xff213c51, 0xff28455a, 0xff285a33, 0xff399e28,
            0xff4390c9, 0xff94cfef
    };
    std::vector<uint32_t> treeColorPalette = {
            // BGR color format - 4. is bark color, 1-3 leaves color
            0xff2aa220, 0xff2aa23a, 0xff2bc311, 0xff143a69
    };
    uint32_t waterColor = 0xffb0811e;
    uint32_t cloudColor = 0xffffffff;
public:
    ResourceContainer() = default;

    ~ResourceContainer() = default;

    [[nodiscard]] uint32_t getEarthColor(int index) const {
        return earthColorPalette[index];
    }

    [[nodiscard]] uint32_t getTreeColor(int index) const {
        return treeColorPalette[index];
    }

    [[nodiscard]] uint32_t getWaterColor() const {
        return waterColor;
    }

    [[nodiscard]] uint32_t getCloudColor() const {
        return cloudColor;
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
    res::CloudList cloudList;

    static const int UPPER_BOUND = 200;
    static const int LOWER_BOUND = 800;

    static const int TREE_FREQ = 120;
    static const int TREE_BARK_HIDE_OFFSET = 15;

    static const int CLOUD_FREQ = 200;

    static const int CLOUD_PART_RADIUS_MIN = 10;
    static const int CLOUD_PART_RADIUS_MAX = 28;

    static const int CLOUD_UPPER_BOUND = 50;
    static const int CLOUD_LOWER_BOUND = 145;

    static const int N_CLOUD_PARTICLES_MIN = 12;
    static const int N_CLOUD_PARTICLES_MAX = 20;

    static const int X_CLOUD_PARTICLE_RANGE = 45;
    static const int Y_CLOUD_PARTICLE_RANGE = 12;

    double minLandHeight{};
    double maxLandHeight{};
    double avgLandHeight{};

    int waterBoundHeight{};

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
        for (auto &cloud: cloudList)
        delete cloud;
        ResourceContainer resources;
        treeList = getTreeList(TREE_FREQ, noiseArray, resources, rnd);
        cloudList = getCloudList(CLOUD_FREQ, resources, rnd);
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        Clear(olc::BLACK);

        ResourceContainer resources;

        // If space is pressed, generate a new seed and regenerate the noise array
        if (GetKey(olc::SPACE).bPressed) {
            seed += 1;
            Lehmer32 rnd(seed);
            noiseArray = getNoiseArray(ScreenWidth(), rnd, 100, ScreenHeight() - 100, 2, 12);
            for (auto &tree: treeList)
                delete tree;
            for (auto &cloud: cloudList)
                delete cloud;
            treeList = getTreeList(TREE_FREQ, noiseArray, resources, rnd);
            cloudList = getCloudList(CLOUD_FREQ, resources, rnd);
        }

        // If c is held, generate a new seed and regenerate the noise array repeatedly
        if (GetKey(olc::C).bHeld) {
            seed += 1;
            Lehmer32 rnd(seed);
            noiseArray = getNoiseArray(ScreenWidth(), rnd, 100, ScreenHeight() - 100, 2, 12);
            for (auto &tree: treeList)
                delete tree;
            treeList = getTreeList(TREE_FREQ, noiseArray, resources, rnd);
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
            bool isWater = (noiseArray[i]) > waterBoundHeight;
            for (int j = 0; j <= ScreenHeight() - noiseArray[i]; j++) {
                int index;
                double percentage = (double) j / (double) sub1;
                if (percentage > 0.4) index = 0;
                else index = 1;
                if (j > 12 && j < 45) {
                    if (isWater) index = 4;
                    else index = 2;
                } else if (j <= 12) {
                    if (isWater) index = 5;
                    else index = 3;
                }
                olc::Pixel pixel{resources.getEarthColor(index)};
                Draw(i, (int) noiseArray[i] + j, pixel);
            }
        }

        // Draw the water
        for (int i = 0; i < ScreenWidth(); i++) {
            if (noiseArray[i] > waterBoundHeight) {
                for (int j = waterBoundHeight; j < noiseArray[i]; j++) {
                    olc::Pixel pixel{resources.getWaterColor()};
                    Draw(i, j, pixel);
                }
            }
        }

        // Draw the clouds
        for (const auto &cloud: cloudList)
            drawCloud(cloud);

        // Post-processing
        for (int i = 0; i < ScreenWidth(); i++) {
            int y = (int) noiseArray[i];
            if (y > waterBoundHeight - 5 && y < waterBoundHeight + 5) {
                // TODO: Smooth out the terrain transition
            }
        }

        return true;
    }

    res::NoiseArray
    getNoiseArray(size_t size, Lehmer32 &lehmer, const int startRangeFrom, const int startRangeTo, const int range = 2,
                  const double SMOOTH_FACTOR = 8, const double VEL_RATIO = -1.0) {

        res::NoiseArray noiseArr(size);
        noiseArr[0] = lehmer.rndInt(startRangeFrom, startRangeTo);

        double vel = 0;

        // Create the noise array
        for (size_t i = 1; i < size; i++) {
            auto from = noiseArr[i - 1] - range;
            auto to = noiseArr[i - 1] + range;
            noiseArr[i] = lehmer.rndDouble(from - vel, to + vel);

            // Bounds check
            if (noiseArr[i] < UPPER_BOUND) noiseArr[i] = lehmer.rndDouble(UPPER_BOUND, UPPER_BOUND + range);
            else if (noiseArr[i] > LOWER_BOUND)
                noiseArr[i] = lehmer.rndDouble(LOWER_BOUND - range, LOWER_BOUND);

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
                avg += noiseArr[j - k];
            if (c)
                avg /= c;
            else
                continue;
            noiseArr[j] = avg;
        }

        avgLandHeight = 0;
        minLandHeight = ScreenHeight();
        maxLandHeight = 0;

        for (const auto &noise: noiseArr) {
            avgLandHeight += noise;
            if (noise < minLandHeight) minLandHeight = noise;
            if (noise > maxLandHeight) maxLandHeight = noise;
        }

        avgLandHeight /= noiseArr.size();

        waterBoundHeight = (int) ((2 * avgLandHeight + maxLandHeight) / 3);

        return noiseArr;
    }

    // drawTree(new res::Tree(100, 100, 16, 30, 10, resources.getTreeColor(3), resources.getTreeColor(0)));
    void drawTree(const res::Tree *tree) {
        FillRect(tree->x, tree->y - tree->height + TREE_BARK_HIDE_OFFSET, tree->width, tree->height, tree->barkColor);
        FillCircle(tree->x + tree->width / 2, tree->y - tree->radius / 2 - tree->height + TREE_BARK_HIDE_OFFSET,
                   tree->radius, tree->leafColor);
    }

    res::TreeList
    getTreeList(int frequency, res::NoiseArray &noiseArr, ResourceContainer &resources, Lehmer32 &rnd) {
        res::TreeList tList;
        int x = 40;
        x += rnd.rndInt(frequency / 2, frequency / 2 * 3);
        while (x < ScreenWidth() - 40) {
            if (noiseArr[x] < avgLandHeight) {
                auto y = noiseArr[x];
                auto w = rnd.rndInt(6, 14);
                auto h = rnd.rndInt(36, 56) + TREE_BARK_HIDE_OFFSET;
                auto r = rnd.rndDouble(2.1, 2.6) * w;
                auto leaf = rnd.rndInt(0, 3);
                auto bark = 3;
                tList.push_back(
                        new res::Tree(x, (int) y, (int) r, h, w, resources.getTreeColor(bark),
                                      resources.getTreeColor(leaf)));
            }
            x += rnd.rndInt(frequency / 2, frequency / 2 * 3);
        }
        return tList;
    }

    void
    drawCloud(const res::Cloud *cloud) {
        for (const auto &cloudPart: cloud->cloudParts) {
            FillCircle(cloudPart->x, cloudPart->y, cloudPart->r, cloudPart->color);
        }
    }

    res::CloudList
    getCloudList(int frequency, ResourceContainer &resources, Lehmer32 &rnd) {
        res::CloudList cList;
        int x = 60;
        x += rnd.rndInt(frequency / 2, frequency / 2 * 5);
        while (x < ScreenWidth() - 60) {
            int y = rnd.rndInt(CLOUD_UPPER_BOUND, CLOUD_LOWER_BOUND);
            cList.push_back(getCloud(rnd.rndInt(N_CLOUD_PARTICLES_MIN, N_CLOUD_PARTICLES_MAX), x, y, rnd, resources));
            x += rnd.rndInt(frequency / 2, frequency / 2 * 5);
        }
        return cList;
    }

    static res::Cloud *
    getCloud(int nParticles, int x, int y, Lehmer32 &rnd, ResourceContainer &resources) {
        auto cloudPtr = new res::Cloud(x, y);
        while (nParticles-- > 0) {
            cloudPtr->cloudParts.push_back(
                    new res::CloudPart(x + rnd.rndInt(-X_CLOUD_PARTICLE_RANGE, +X_CLOUD_PARTICLE_RANGE),
                                       y + rnd.rndInt(-Y_CLOUD_PARTICLE_RANGE, +Y_CLOUD_PARTICLE_RANGE),
                                       rnd.rndInt(CLOUD_PART_RADIUS_MIN, CLOUD_PART_RADIUS_MAX),
                                       resources.getCloudColor()));
        }
        return cloudPtr;
    }
};

int main() {
    if (World demo; demo.Construct(1864, 920, 1, 1))
        demo.Start();

    return 0;
}
