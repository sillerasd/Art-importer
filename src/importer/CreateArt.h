#pragma once
#include <Geode/Geode.hpp>
#include <Geode/Bindings.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/utils/async.hpp>

#include "stb_image.h"

using namespace geode::prelude;

static auto ALLOWED_TYPES2 = file::FilePickOptions{
    std::nullopt,
    {
        {
            "Image Files",
            { "*.png", "*.jpg" }
        }
    }
};

class CreateArt final{
private:
    // object IDs for various object sizes
    static constexpr int pixelObjID = 3097;
    static constexpr int medPixelObjID = 3094;
    static constexpr int bigPixelObjID = 3093;
    static constexpr int largePixelObjID = 3092;

    // old pixel objects
    static constexpr int oldPixelObjID = 917;
    static constexpr int oldLargePixelObjID = 211;

    // z order layering
    static constexpr int zOrder = 1;

    // size of the objects
    static constexpr float objSize = 5.0f;

    // base scale between pixels
    static constexpr float scale = 5.0f;

    // number of channels
    int channels = 4;

    // image size
    int width = 0;
    int height = 0;

    // NEW — scale multipliers from text input
    float scaleMulX = 1.0f;
    float scaleMulY = 1.0f;

    // background optimisation color
    uint8_t Mr = 0;
    uint8_t Mg = 0;
    uint8_t Mb = 0;

    // image data
    unsigned char* data = nullptr;

    // object string
    std::ostringstream objInLevel;

    // selected object
    GameObject* obj = nullptr;

    // close menu callback
    std::function<void()> closeMenu = nullptr;

    // settings
    bool limitSize = Mod::get()->getSettingValue<bool>("Disable-limit");
    int sizeLimit = Mod::get()->getSettingValue<int>("Size-limit");
    int colourChannel = Mod::get()->getSettingValue<int>("Colour-channel");
    bool useOldPixel = Mod::get()->getSettingValue<bool>("Use-OlderObjects");
    int tolerance = Mod::get()->getSettingValue<int>("Tolerance");
    std::string basic = Mod::get()->getSettingValue<std::string>("Optimise-Type");
    bool backgroundOp = Mod::get()->getSettingValue<bool>("Background-Optimisation");

    // main import logic
    void placeArt(const std::string& p);

    // import methods
    void simpleImport(const std::string& p);
    void basicOptimiseImport(const std::string& p);
    void scaleOptimiseImport(const std::string& p);
    void optimiseJPG();

    // helpers
    void formatHSV(float red, float green, float blue, std::string& objColour) const;
    void RGBtoHSV(float& r, float& g, float& b) const;
    int bestFit(std::vector<std::vector<bool>>& p, unsigned char const* data, int x, int y, int ch, int wid, int hi);
    bool comparePixels(unsigned char const*& data, int p1, int p2) const;
    bool comparePixels(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) const;
    int scalePixX(std::vector<std::vector<bool>>& p, unsigned char const* data, int x, int y, int ch, int wid, int hi);
    int scalePixY(std::vector<std::vector<bool>>& p, unsigned char const* data, int x, int y, int ch, int wid, int hi, int xScale);

public:
    CreateArt() {}
    ~CreateArt() {}

    // importing the art
    void importArt();

    // update settings
    void updateSettings();

    // setters
    void setSelectedObject(GameObject* o) { obj = o; }
    void setCloseMenu(std::function<void()> c) { closeMenu = c; }

    // NEW — set target size from text input
    void setTargetSize(int w, int h);
};
