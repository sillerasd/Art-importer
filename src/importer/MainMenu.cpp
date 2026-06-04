#include "MainMenu.h"
#include "CreateArt.h"

bool MainMenu::init(CCArray* startObj, CreateArt* artImposter) {
    if (!Popup::init(240.0f, 160.0f)) return false;

    artPointer = artImposter;

    this->setTitle(" Choose a file to import ");

    // imports the art
    auto btn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Choose a file"),
        this,
        menu_selector(MainMenu::importArt)
    );
    btn->setPosition(MainMenu::m_size.width / 2, MainMenu::m_size.height / 2 + 20);
    this->m_buttonMenu->addChild(btn);

    // opens the settings menu
    btn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Settings"),
        this,
        menu_selector(MainMenu::openSettings)
    );
    btn->setPosition(MainMenu::m_size.width / 2, MainMenu::m_size.height / 2 - 20);
    this->m_buttonMenu->addChild(btn);

    // --- ТЕКСТОВОЕ ПОЛЕ ДЛЯ РАЗМЕРА ---
    sizeInput = CCTextInputNode::create("64x64", this, "bigFont.fnt", 100, 30);
    sizeInput->setPosition({ MainMenu::m_size.width / 2, MainMenu::m_size.height / 2 - 60 });
    sizeInput->setAllowedChars("0123456789xX");
    this->addChild(sizeInput);

    // info menu
    btn = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"),
        this,
        menu_selector(MainMenu::openInfo)
    );
    btn->setPosition(MainMenu::m_size.width, MainMenu::m_size.height);
    this->m_buttonMenu->addChild(btn);

    return true;
}

MainMenu* MainMenu::create(CCArray* startObj, CreateArt* artImposter) {
    auto ret = new MainMenu();
    if (ret && ret->init(startObj, artImposter)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

void MainMenu::openInfo(CCObject* sender) {
    geode::createQuickPopup(
        "Info",
        "Enter size like <cg>128x72</c> to scale the art.",
        "OK", nullptr
    );
}

void MainMenu::importArt(CCObject* sender) {
    artPointer->updateSettings();

    // читаем строку из поля
    std::string s = sizeInput->getString();

    // парсим WIDTHxHEIGHT
    int w = 0, h = 0;
    sscanf(s.c_str(), "%dx%d", &w, &h);

    // передаём в CreateArt
    artPointer->setTargetSize(w, h);

    // импорт
    artPointer->importArt();
}
