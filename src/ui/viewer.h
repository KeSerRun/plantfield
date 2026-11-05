#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "game/match.h"
#include "raylib.h"

class Viewer {
public:
    Viewer(const std::vector<PlantTemplate>& pool,
           const std::vector<Gardener>& gardeners,
           std::uint32_t seed,
           int humanGardener = 0,
           int autoRounds = 0);
    ~Viewer();

    void run();
    void setScreenshot(const std::string& path) { shotPath_ = path; }

private:
    enum class Zone { None, Shop, Hand, Board, Discover };

    struct CombatFx {
        enum class Kind { ShieldPop, Poison };
        Kind kind = Kind::ShieldPop;
        Rectangle rect{};
        float timer = 0.0f;
    };

    void restart();
    void loadPlantArt();
    void loadGardenerArt();
    void startCombat();
    void startCombatBeat(int firstFrame);
    bool combatCardPos(const CombatFrame& f, int instanceId, Vector2* out) const;
    void fireCombatFx(const CombatFrame& before, const CombatFrame& after);
    void drawCombatFx() const;
    int combatHealthDrop(const CombatFrame& before, const CombatFrame& after,
                         int instanceId) const;
    void updateCombatPlayback();
    void skipCombat();
    void cycleSpeed();
    void handleInput();
    void handleGardenerSelect();
    void draw();
    void drawGardenerSelect();
    void drawRecruitScene();
    void drawSelfCircle();
    void drawUpgradeButton();
    void drawDiscoverOverlay();
    void drawCombatScene();
    void drawImGui();
    void drawShopButtons();
    void drawGoButton();
    void drawLogWindow();
    void drawCardTooltip();
    const Plant* hoveredPlant() const;
    Zone zoneAt(Vector2 m) const;
    Rectangle dropBand(Zone zone) const;
    int boardInsertIndex(float mouseX) const;
    void drawDragGhost();
    std::string describeEffect(const Effect& e) const;
    std::vector<std::string> abilityLines(const PlantTemplate& t) const;
    void drawText(const std::string& text, float x, float y, float size, Color color) const;
    void drawGradientRect(Rectangle r, Color top, Color bottom) const;
    void drawZonePanel(Rectangle r, Color top, Color bottom, Color border,
                       const std::string& label, Color labelColor) const;
    void drawLawnTexture(Rectangle r) const;
    void drawTextCentered(const std::string& text, float y, float size, Color color) const;
    void drawTextCenteredX(const std::string& text, float centerX, float y, float size,
                           Color color) const;
    void drawOpponents();
    void drawGardenerTooltip(int side);
    void drawGardenerBadge(int gardenerIndex, Rectangle r, Color ring, bool hot,
                           const std::string& label);
    int gardenerIndexOf(const Gardener& gd) const;
    void drawTextureCover(const Texture2D& tex, Rectangle r) const;
    void drawTextWrapped(const std::string& text, float x, float y, float size, Color color,
                         float maxWidth, int maxLines) const;
    Rectangle opponentCircleRect(int index) const;
    Rectangle selfCircleRect() const;
    Rectangle gardenerCardRect(int index) const;
    Rectangle rowCard(Zone zone, int index) const;
    Rectangle handCardRect(int index) const;
    Rectangle discoverCardRect(int index) const;
    Rectangle combatCard(int side, int index, int count) const;
    void drawCard(const Plant& p, Rectangle r, int outline) const;
    bool drawCardArt(const Plant& p, Rectangle r) const;
    void drawThornFrame(Rectangle r) const;
    void drawShieldBubble(Rectangle r) const;
    void drawPoisonMark(Rectangle r) const;
    static std::string keywordText(const Plant& p, const PlantTemplate& t);
    Color raceColor(const std::string& race) const;

    const std::vector<PlantTemplate>* pool_;
    const std::vector<Gardener>* gardeners_;
    std::uint32_t seed_;
    int humanGardener_ = 0;
    int autoRounds_ = 0;
    std::unique_ptr<Match> match_;
    std::vector<std::string> corpus_;
    std::vector<Texture2D> plantArt_;
    std::vector<Texture2D> gardenerArt_;
    std::vector<Texture2D> gardenerFace_;
    Font font_{};
    bool fontLoaded_ = false;
    bool showLog_ = false;
    bool selecting_ = true;
    int hoverGardener_ = -1;
    bool logScrollInit_ = false;
    Zone hoverZone_ = Zone::None;
    int hoverIndex_ = -1;
    int combatHoverSide_ = -1;
    int combatHoverIndex_ = -1;
    int hoveredOpponent_ = -1;
    bool hoveredSelf_ = false;
    bool dragging_ = false;
    Zone dragZone_ = Zone::None;
    int dragIndex_ = -1;
    Vector2 dragPos_{0.0f, 0.0f};
    int frameIndex_ = 0;
    int beatFirst_ = 0;
    int beatLast_ = 0;
    float beatTimer_ = 0.0f;
    float beatDuration_ = 0.0f;
    bool popFired_ = false;
    std::vector<CombatFx> combatFx_;
    float endTimer_ = 0.0f;
    float combatSpeed_ = 1.0f;
    std::string shotPath_;
    int frameCounter_ = 0;
};
