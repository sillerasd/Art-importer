#define STB_IMAGE_IMPLEMENTATION
#include "CreateArt.h"

void CreateArt::importArt() {
    async::spawn(file::pick(file::PickMode::OpenFile, ALLOWED_TYPES2),
        [this](Result<std::optional<std::filesystem::path>> result) {
            if (result.isOk()) {
                auto opt = result.unwrap();
                if (opt) {
                    std::string const pathStr = utils::string::pathToString(opt.value());
                    this->placeArt(pathStr);
                }
            }
        }
    );
}

void CreateArt::placeArt(std::string const& p) {

    objInLevel.str("");
    objInLevel.clear();

    data = stbi_load(p.c_str(), &width, &height, &channels, 0);

    if (!data) {
        geode::createQuickPopup(
            "Error",
            "Failed to load image!\nFile path: " + p,
            "OK", nullptr
        );
        return;
    }

    if ((width * height > sizeLimit) && !limitSize) {
        FLAlertLayer::create("Error", "Image too large.", "OK")->show();
        stbi_image_free(data);
        return;
    }

    // если пользователь не ввёл размер — оставляем 1.0
    if (scaleMulX <= 0 || scaleMulY <= 0) {
        scaleMulX = 1.0f;
        scaleMulY = 1.0f;
    }

    // выбор режима
    if (basic == "Basic Optimisation" && !useOldPixel) {
        basicOptimiseImport(p);
    }
    else if (basic == "Scale Optimisation") {
        if (backgroundOp) optimiseJPG();
        scaleOptimiseImport(p);
    }
    else {
        simpleImport(p); // ← ТОЛЬКО ЭТО МЫ МАСШТАБИРУЕМ
    }
}

void CreateArt::simpleImport(std::string const& p) {
    float const startX = obj->getPositionX();
    float const startY = obj->getPositionY();
    std::string objString;

    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {

            int const pixelIndex = (y * width + x) * channels;
            uint8_t const alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
            if (alpha == 0) continue;

            std::string objColour;
            formatHSV(data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2], objColour);

            // МАСШТАБИРОВАНИЕ ТОЛЬКО ЗДЕСЬ
            float px = startX + x * scale * scaleMulX;
            float py = startY - y * scale * scaleMulY;

            if (useOldPixel) {
                objInLevel
                    << "1," << oldPixelObjID
                    << ",2," << px
                    << ",3," << py
                    << ",21," << colourChannel
                    << ",41,1,43," << objColour
                    << ",25," << zOrder
                    << ",32," << objSize / 5
                    << ";";
            }
            else {
                objInLevel
                    << "1," << pixelObjID
                    << ",2," << px
                    << ",3," << py
                    << ",21," << colourChannel
                    << ",41,1,43," << objColour
                    << ",25," << zOrder
                    << ",32," << objSize
                    << ";";
            }
        }
    }

    objString = objInLevel.str();
    objString.pop_back();

    LevelEditorLayer::get()->createObjectsFromString(objString.c_str(), true, true);
    FLAlertLayer::create("Success!", "Art imported", "OK")->show();

    stbi_image_free(data);
    data = nullptr;

    closeMenu();
}

// -------------------------
// Остальные режимы — ОРИГИНАЛ
// -------------------------

void CreateArt::basicOptimiseImport(const std::string& p) {
    float const startX = obj->getPositionX();
    float const startY = obj->getPositionY();
    std::string objString;

    std::vector<std::vector<bool>> placed(height, std::vector<bool>(width, false));

    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            if (placed[y][x]) continue;

            int const pixelIndex = (y * width + x) * channels;
            uint8_t const alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
            if (alpha == 0) continue;

            int const pixelType = bestFit(placed, data, x, y, channels, width, height);

            std::string objColour;
            formatHSV(data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2], objColour);

            switch (pixelType) {
            case 3097:
                objInLevel << "1," << pixelType << ",2," << startX + x * scale
                    << ",3," << startY - y * scale
                    << ",21," << colourChannel << ",41,1,43," << objColour
                    << ",25," << zOrder << ",32," << objSize << ";";
                break;
            case 3094:
                objInLevel << "1," << pixelType << ",2," << (startX + x * scale) + 2.5f
                    << ",3," << (startY - y * scale) + 2.5f
                    << ",21," << colourChannel << ",41,1,43," << objColour
                    << ",25," << zOrder << ",32," << objSize * 2.0f << ";";
                break;
            case 3093:
                objInLevel << "1," << pixelType << ",2," << (startX + x * scale) + 5.0f
                    << ",3," << (startY - y * scale) + 5.0f
                    << ",21," << colourChannel << ",41,1,43," << objColour
                    << ",25," << zOrder << ",32," << objSize * 3.0f << ";";
                break;
            case 3092:
                objInLevel << "1," << pixelType << ",2," << (startX + x * scale) + 12.5f
                    << ",3," << (startY - y * scale) + 12.5f
                    << ",21," << colourChannel << ",41,1,43," << objColour
                    << ",25," << zOrder << ",32," << objSize * 6.0f << ";";
                break;
            }
        }
    }

    std::string objString = objInLevel.str();
    objString.pop_back();

    LevelEditorLayer::get()->createObjectsFromString(objString.c_str(), true, true);
    FLAlertLayer::create("Success!", "Art imported", "OK")->show();

    stbi_image_free(data);
    data = nullptr;

    closeMenu();
}

void CreateArt::optimiseJPG() {
    std::unordered_map<uint32_t, size_t> freq;
    for (int i = 0; i < width * height; i++) {
        freq[(data[i * channels] << 16) | (data[i * channels + 1] << 8) | data[i * channels + 2]]++;
    }

    size_t bestCount = 0;
    uint32_t bestCol = 0;

    for (auto& [col, count] : freq) {
        if (count > bestCount) {
            bestCol = col;
            bestCount = count;
        }
    }

    Mr = (bestCol >> 16) & 0xFF;
    Mg = (bestCol >> 8) & 0xFF;
    Mb = bestCol & 0xFF;
}

void CreateArt::scaleOptimiseImport(std::string const& p) {
    float const startX = obj->getPositionX();
    float const startY = obj->getPositionY();
    std::string objString;

    std::vector<std::vector<bool>> placed(height, std::vector<bool>(width, false));

    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            if (placed[y][x]) continue;

            int const pixelIndex = (y * width + x) * channels;
            uint8_t const alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
            if (alpha == 0) continue;

            if (backgroundOp) {
                if (comparePixels(Mr, Mg, Mb, data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2])) {
                    placed[y][x] = true;
                    continue;
                }
            }

            int scaleX = scalePixX(placed, data, x, y, channels, width, height);
            int scaleY = scalePixY(placed, data, x, y, channels, width, height, scaleX);

            std::string objColour;
            formatHSV(data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2], objColour);

            float xSize = scale * scaleX;
            float ySize = scale * scaleY;

            objInLevel
                << "1," << largePixelObjID
                << ",2," << startX + x * scale + xSize / 2
                << ",3," << startY - y * scale + ySize / 2
                << ",21," << colourChannel
                << ",41,1,43," << objColour
                << ",25," << zOrder
                << ",128," << xSize
                << ",129," << ySize
                << ";";
        }
    }

    std::string objString = objInLevel.str();
    objString.pop_back();

    LevelEditorLayer::get()->createObjectsFromString(objString.c_str(), true, true);
    FLAlertLayer::create("Success!", "Art imported", "OK")->show();

    stbi_image_free(data);
    data = nullptr;

    closeMenu();
}

bool CreateArt::comparePixels(const unsigned char*& data, int p1, int p2) const {
    uint8_t const a1 = (channels == 4) ? data[p1 + 3] : 255;
    uint8_t const a2 = (channels == 4) ? data[p2 + 3] : 255;
    return (std::abs(data[p1] - data[p2]) <= tolerance)
        && (std::abs(data[p1 + 1] - data[p2 + 1]) <= tolerance)
        && (std::abs(data[p1 + 2] - data[p2 + 2]) <= tolerance)
        && (std::abs(a1 - a2) <= tolerance);
}

bool CreateArt::comparePixels(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2) const {
    return (std::abs(r1 - r2) <= tolerance)
        && (std::abs(g1 - g2) <= tolerance)
        && (std::abs(b1 - b2) <= tolerance);
}

int CreateArt::bestFit(std::vector<std::vector<bool>>& p, const unsigned char* data, int px, int py, int ch, int wid, int hi) {
    int const referencePixel = (py * wid + px) * ch;
    std::vector<int> const pixelSizes = { 1, 2, 3, 6 };
    std::vector<int> const pixelObjIDs = { pixelObjID, medPixelObjID, bigPixelObjID, largePixelObjID };

    for (int i = 0; i < pixelSizes.size(); ++i) {
        bool fits = true;

        if (px + pixelSizes[i] > wid || py - pixelSizes[i] + 1 < 0) {
            fits = false;
        }

        if (fits) {
            for (int y = py; y >= py - pixelSizes[i] + 1 && fits; --y) {
                for (int x = px; x < px + pixelSizes[i] && fits; ++x) {
                    if (!comparePixels(data, referencePixel, (y * wid + x) * ch)) {
                        fits = false;
                    }
                }
            }
        }

        if (!fits) {
            int const sizeP = (i == 0) ? 1 : pixelSizes[i - 1];
            int const returnID = (i == 0) ? pixelObjID : pixelObjIDs[i - 1];

            for (int y = 0; y < sizeP; ++y) {
                for (int x = 0; x < sizeP; ++x) {
                    p[py - y][px + x] = true;
                }
            }
            return returnID;
        }
    }

    for (int y = 0; y < pixelSizes.back(); ++y) {
        for (int x = 0; x < pixelSizes.back(); ++x) {
            p[py - y][px + x] = true;
        }
    }
    return pixelObjIDs.back();
}

int CreateArt::scalePixX(std::vector<std::vector<bool>>& p, unsigned char const* data, int px, int py, int ch, int wid, int hi) {
    int const refPix = (py * wid + px) * ch;
    bool fits = true;
    int off = 1;

    do {
        if (px + off >= wid) {
            fits = false;
        }
        else {
            if (comparePixels(data, refPix, (py * wid + px + off) * ch)) {
                p[py][px + off] = true;
                ++off;
            }
            else {
                fits = false;
            }
        }
    } while (fits);

    p[py][px] = true;
    return off;
}

int CreateArt::scalePixY(std::vector<std::vector<bool>>& p, unsigned char const* data, int px, int py, int ch, int wid, int hi, int xScale) {
    int const refPix = (py * wid + px) * ch;
    bool fits = true;
    int off = 1;

    do {
        if (py - off <= 0) {
            fits = false;
        }
        else {
            for (int i = 0; i < xScale && fits; ++i) {
                if (!comparePixels(data, refPix, ((py - off) * wid + px + i) * ch)) {
                    fits = false;
                }
            }

            if (fits) {
                for (int i = 0; i < xScale; ++i) {
                    p[py - off][px + i] = true;
                }
                ++off;
            }
        }
    } while (fits);

    return off;
}

void CreateArt::formatHSV(float r, float g, float b, std::string& objColour) const {
    RGBtoHSV(r, g, b);
    objColour = std::to_string(r) + "a" + std::to_string(g)
        + "a" + std::to_string(b) + "a1a1";
}

void CreateArt::RGBtoHSV(float& r, float& g, float& b) const {
    r /= 255.0f;
    g /= 255.0f;
    b /= 255.0f;

    float max = std::max(r, std::max(g, b));
    float min = std::min(r, std::min(g, b));
    float del = max - min;

    float hue = 0.0f, sat = 0.0f, val = max;

    if (del > 0.0f) {
        sat = del / max;

        if (max == r) hue = 60.0f * ((g - b) / del);
        else if (max == g) hue = 60.0f * (((b - r) / del) + 2.0f);
        else hue = 60.0f * (((r - g) / del) + 4.0f);
    }

    if (hue < 0.0f) hue += 360.0f;

    hue = ((static_cast<int>(hue) + 180) % 360) - 180;

    if (hue == 0.0f) hue++;

    r = hue;
    g = sat;
    b = val;
}

void CreateArt::updateSettings() {
    limitSize = Mod::get()->getSettingValue<bool>("Disable-limit");
    sizeLimit = Mod::get()->getSettingValue<int>("Size-limit");
    colourChannel = Mod::get()->getSettingValue<int>("Colour-channel");
    useOldPixel = Mod::get()->getSettingValue<bool>("Use-OlderObjects");
    tolerance = Mod::get()->getSettingValue<int>("Tolerance");
    basic = Mod::get()->getSettingValue<std::string>("Optimise-Type");
}

void CreateArt::setTargetSize(int w, int h) {
    if (w > 0 && h > 0 && width > 0 && height > 0) {
        scaleMulX = (float)w / (float)width;
        scaleMulY = (float)h / (float)height;
    }
}
