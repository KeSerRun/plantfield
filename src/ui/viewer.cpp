#include "ui/viewer.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "core/aura.h"
#include "core/format.h"
#include "imgui.h"
#include "rlImGui.h"

namespace {

constexpr int kWindowWidth = 1440;
constexpr int kWindowHeight = 900;
constexpr int kFaceSize = 128;

constexpr float kBarY = 46.0f;
constexpr float kBarH = 42.0f;

constexpr float kBoardY = 118.0f;
constexpr float kBoardCardW = 176.0f;
constexpr float kBoardCardH = 250.0f;
constexpr float kBoardGap = 11.0f;

constexpr float kHandY = 410.0f;
constexpr float kHandCardW = 120.0f;
constexpr float kHandCardH = 132.0f;
constexpr float kHandGap = 7.0f;

constexpr float kShopY = 580.0f;
constexpr float kShopCardW = 202.0f;
constexpr float kShopCardH = 262.0f;
constexpr float kShopGap = 12.0f;

constexpr float kPlayLeft = 8.0f;
constexpr float kPlayRight = 1320.0f;
constexpr float kOppX = 1330.0f;
constexpr float kOppW = 102.0f;
constexpr float kOppD = 86.0f;

constexpr float kHandPanelTop = 384.0f;
constexpr float kHandPanelH = 162.0f;
constexpr float kHandRowLeft = 118.0f;
constexpr float kHandFanSpan = 720.0f;
constexpr float kHandFanStep = 78.0f;
constexpr float kHandHoverLift = 18.0f;
constexpr float kSelfCircleX = 18.0f;
constexpr float kSelfCircleY = kHandPanelTop + (kHandPanelH - kOppD) * 0.5f;

constexpr float kCombatFoeY = 76.0f;
constexpr float kCombatMineY = 476.0f;
constexpr float kCombatCardW = 189.0f;
constexpr float kCombatCardH = 300.0f;
constexpr float kCombatGap = 12.0f;
constexpr float kCombatFrameSeconds = 0.22f;
constexpr float kCombatContact = 0.5f;
constexpr float kCombatLungeRatio = 0.52f;
constexpr float kCombatLungeMax = 200.0f;
constexpr float kCombatRecoil = 11.0f;
constexpr float kCombatFxSeconds = 0.5f;

constexpr float kDropBoardTop = 92.0f;
constexpr float kDropBoardBottom = 384.0f;
constexpr float kDropHandTop = 384.0f;
constexpr float kDropHandBottom = 546.0f;
constexpr float kDropShopTop = 546.0f;
constexpr float kDropShopBottom = 846.0f;

const char* kUiText =
    "你的花园对手第回合等级阳光血商店行动上场战斗获胜存活株造成点伤害平局双方各受到自己同归于尽"
    "达到上限以优势空手牌已满种植铲除浇水重抽升级到级最不足"
    "攻击护盾免疫了这次受剩余毒刺被毒杀死亡召唤增益获得再生复活结果全灭胜利阵容"
    "空格下一重开退出结束进行中生命保鲜锁定解锁操作面板购买已满级回合左右键日志"
    "阶段事件速度跳过合计招募等待即将算结算护甲园丁修剪催芽护根日照堆肥被动已使技能"
    "选择始主你的对手将使用点击卡片开始对局草坪→未知揭晓后方可"
    "荆棘护盾速生毒刺再生播种凋零光合切换奖励株拖拽调站位内整"
    "：（） ，！｜★·—…";

Color tierColor(int tier) {
    switch (tier) {
    case 1: return Color{175, 175, 175, 255};
    case 2: return Color{120, 210, 120, 255};
    case 3: return Color{110, 170, 240, 255};
    case 4: return Color{190, 140, 240, 255};
    case 5: return Color{240, 170, 90, 255};
    case 6: return Color{240, 110, 110, 255};
    default: return Color{180, 180, 180, 255};
    }
}

std::vector<int> utf8ToCodepoints(const std::string& text) {
    std::vector<int> out;
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        int cp = 0;
        int len = 0;
        if (c < 0x80) {
            cp = c;
            len = 1;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F;
            len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F;
            len = 3;
        } else {
            cp = c & 0x07;
            len = 4;
        }
        for (int k = 1; k < len && i + static_cast<std::size_t>(k) < text.size(); ++k) {
            cp = (cp << 6) |
                 (static_cast<unsigned char>(text[i + static_cast<std::size_t>(k)]) & 0x3F);
        }
        out.push_back(cp);
        i += static_cast<std::size_t>(len);
    }
    return out;
}

std::vector<int> collectCodepoints(const std::vector<PlantTemplate>& pool,
                                   const std::vector<Gardener>& gardeners,
                                   const std::vector<std::string>& corpus) {
    std::vector<int> cps;
    for (int cp = 32; cp <= 126; ++cp) {
        cps.push_back(cp);
    }
    const auto append = [&cps](const std::string& s) {
        for (int cp : utf8ToCodepoints(s)) {
            bool seen = false;
            for (int existing : cps) {
                if (existing == cp) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                cps.push_back(cp);
            }
        }
    };
    append(kUiText);
    for (const PlantTemplate& t : pool) {
        append(t.name);
        append(t.race);
    }
    for (const Gardener& g : gardeners) {
        append(g.name);
        append(g.skill.name);
        append(g.skill.description);
    }
    for (const std::string& line : corpus) {
        append(line);
    }
    return cps;
}

const char* cjkFontPath() {
    static const char* candidates[] = {"C:/Windows/Fonts/simhei.ttf", "C:/Windows/Fonts/Deng.ttf",
                                       "C:/Windows/Fonts/simkai.ttf", "C:/Windows/Fonts/msyh.ttc"};
    for (const char* path : candidates) {
        if (FileExists(path)) {
            return path;
        }
    }
    return nullptr;
}

const char* keywordDescription(Keyword k) {
    switch (k) {
    case Keyword::Thorns:   return "荆棘：必须被优先攻击（嘲讽）";
    case Keyword::Shield:   return "护盾：免疫一次伤害";
    case Keyword::FastGrow: return "速生：每次出手攻击两次";
    case Keyword::Poison:   return "毒刺：被其伤害的植物直接死亡";
    case Keyword::Reborn:   return "再生：死亡后以 1 血复活一次";
    }
    return "";
}

const char* selectorName(TargetSelector t) {
    switch (t) {
    case TargetSelector::Self:                     return "自身";
    case TargetSelector::Adjacent:                 return "相邻植物";
    case TargetSelector::RandomEnemy:              return "随机敌人";
    case TargetSelector::AllEnemies:               return "所有敌人";
    case TargetSelector::RandomFriendly:           return "随机友方植物";
    case TargetSelector::RandomFriendlyOther:      return "随机其他友方植物";
    case TargetSelector::AllFriendlies:            return "所有友方植物";
    case TargetSelector::AllFriendliesOther:       return "所有其他友方植物";
    case TargetSelector::AllFriendliesSameRace:    return "所有同族植物";
    case TargetSelector::AllFriendliesSameRaceOther: return "所有同族其他植物";
    }
    return "?";
}

}  // namespace

Viewer::Viewer(const std::vector<PlantTemplate>& pool,
               const std::vector<Gardener>& gardeners,
               std::uint32_t seed,
               int humanGardener,
               int autoRounds)
    : pool_(&pool), gardeners_(&gardeners), seed_(seed), humanGardener_(humanGardener),
      autoRounds_(autoRounds) {
    InitWindow(kWindowWidth, kWindowHeight, "Plant Battlegrounds");
    SetTargetFPS(60);
    loadPlantArt();
    loadGardenerArt();

    {
        Match probe(pool, gardeners, seed, humanGardener);
        probe.setSilent(true);
        while (!probe.finished()) {
            probe.autoRound();
        }
        corpus_ = probe.logLines();
    }

    const std::vector<int> codepoints = collectCodepoints(pool, gardeners, corpus_);
    const char* path = cjkFontPath();
    if (path != nullptr) {
        const Font loaded = LoadFontEx(path, 24, const_cast<int*>(codepoints.data()),
                                       static_cast<int>(codepoints.size()));
        if (loaded.glyphs != nullptr && loaded.texture.id > 0) {
            font_ = loaded;
            fontLoaded_ = true;
            SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
            TraceLog(LOG_INFO, "FONT: loaded %s (%i glyphs)", path, font_.glyphCount);
        }
    }
    if (!fontLoaded_) {
        TraceLog(LOG_WARNING, "FONT: falling back to default (ASCII only)");
        font_ = GetFontDefault();
    }

    rlImGuiBeginInitImGui();
    ImGui::StyleColorsDark();
    if (path != nullptr) {
        ImGuiIO& io = ImGui::GetIO();
        ImFontGlyphRangesBuilder builder;
        builder.AddRanges(io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
        builder.AddText("\u2605\uFF1A\uFF08\uFF09\uFF0C\uFF01\uFF5C\u00B7\u2014\u2026\u2192\u25B6");
        ImVector<ImWchar> ranges;
        builder.BuildRanges(&ranges);
        ImFont* imFont = io.Fonts->AddFontFromFileTTF(path, 18.0f, nullptr, ranges.Data);
        if (imFont != nullptr) {
            io.FontDefault = imFont;
        }
    }
    rlImGuiEndInitImGui();

    selecting_ = autoRounds_ <= 0;
    restart();
}

Viewer::~Viewer() {
    for (const Texture2D& tex : plantArt_) {
        if (tex.id != 0) {
            UnloadTexture(tex);
        }
    }
    for (const Texture2D& tex : gardenerArt_) {
        if (tex.id != 0) {
            UnloadTexture(tex);
        }
    }
    for (const Texture2D& tex : gardenerFace_) {
        if (tex.id != 0) {
            UnloadTexture(tex);
        }
    }
    rlImGuiShutdown();
    if (fontLoaded_) {
        UnloadFont(font_);
    }
    CloseWindow();
}

void Viewer::loadPlantArt() {
    plantArt_.assign(pool_->size(), Texture2D{});
    static const char* prefixes[] = {"assets/plants/", "../assets/plants/",
                                     "../../assets/plants/"};
    int found = 0;
    for (std::size_t i = 0; i < pool_->size(); ++i) {
        const std::string file = (*pool_)[i].id + ".png";
        for (const char* prefix : prefixes) {
            const std::string path = std::string(prefix) + file;
            if (!FileExists(path.c_str())) {
                continue;
            }
            const Texture2D tex = LoadTexture(path.c_str());
            if (tex.id != 0) {
                plantArt_[i] = tex;
                SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
                ++found;
            }
            break;
        }
    }
    TraceLog(LOG_INFO, "ART: loaded %i/%i plant textures", found, static_cast<int>(pool_->size()));
}

void Viewer::loadGardenerArt() {
    gardenerArt_.assign(gardeners_->size(), Texture2D{});
    gardenerFace_.assign(gardeners_->size(), Texture2D{});
    static const char* prefixes[] = {"assets/gardeners/", "../assets/gardeners/",
                                     "../../assets/gardeners/"};
    int found = 0;
    for (std::size_t i = 0; i < gardeners_->size(); ++i) {
        const std::string file = (*gardeners_)[i].id + ".png";
        for (const char* prefix : prefixes) {
            const std::string path = std::string(prefix) + file;
            if (!FileExists(path.c_str())) {
                continue;
            }
            Image img = LoadImage(path.c_str());
            if (img.data == nullptr) {
                break;
            }

            const Texture2D card = LoadTextureFromImage(img);
            if (card.id != 0) {
                SetTextureFilter(card, TEXTURE_FILTER_BILINEAR);
                gardenerArt_[i] = card;
                ++found;
            }

            // 圆框头像：裁到头部区域再压成圆形 alpha
            ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            const float side = static_cast<float>(img.height) * 0.78f;
            Rectangle crop{(static_cast<float>(img.width) - side) * 0.5f,
                           static_cast<float>(img.height) * 0.42f - side * 0.5f, side, side};
            if (crop.x < 0.0f) {
                crop.x = 0.0f;
            }
            if (crop.y < 0.0f) {
                crop.y = 0.0f;
            }
            if (crop.x + crop.width > static_cast<float>(img.width)) {
                crop.x = static_cast<float>(img.width) - crop.width;
            }
            if (crop.y + crop.height > static_cast<float>(img.height)) {
                crop.y = static_cast<float>(img.height) - crop.height;
            }
            ImageCrop(&img, crop);
            ImageResize(&img, kFaceSize, kFaceSize);

            const float half = static_cast<float>(kFaceSize) * 0.5f;
            for (int y = 0; y < img.height; ++y) {
                for (int x = 0; x < img.width; ++x) {
                    const float dx = static_cast<float>(x) + 0.5f - half;
                    const float dy = static_cast<float>(y) + 0.5f - half;
                    float mul = half - sqrtf(dx * dx + dy * dy);
                    mul = mul > 1.0f ? 1.0f : (mul < 0.0f ? 0.0f : mul);
                    Color* px = reinterpret_cast<Color*>(img.data) + y * img.width + x;
                    px->a = static_cast<unsigned char>(static_cast<float>(px->a) * mul);
                }
            }

            const Texture2D face = LoadTextureFromImage(img);
            if (face.id != 0) {
                SetTextureFilter(face, TEXTURE_FILTER_BILINEAR);
                gardenerFace_[i] = face;
            }
            UnloadImage(img);
            break;
        }
    }
    TraceLog(LOG_INFO, "ART: loaded %i/%i gardener textures", found,
             static_cast<int>(gardeners_->size()));
}

void Viewer::restart() {
    match_ = std::make_unique<Match>(*pool_, *gardeners_, seed_, humanGardener_);
    match_->setSilent(true);
    logScrollInit_ = false;
    frameIndex_ = 0;
    beatFirst_ = 0;
    beatLast_ = 0;
    beatTimer_ = 0.0f;
    beatDuration_ = 0.0f;
    popFired_ = false;
    combatFx_.clear();
    endTimer_ = 0.0f;
    for (int i = 0; i < autoRounds_ && !match_->finished(); ++i) {
        match_->autoRound();
    }
    match_->startRound();
}

void Viewer::startCombat() {
    if (!match_->awaitingPlayer()) {
        return;
    }
    match_->beginCombat();
    frameIndex_ = 0;
    beatTimer_ = 0.0f;
    combatFx_.clear();
    endTimer_ = 0.0f;
    logScrollInit_ = true;
    startCombatBeat(0);
}

void Viewer::startCombatBeat(int firstFrame) {
    const std::vector<CombatFrame>& frames = match_->combatFrames();
    const int total = static_cast<int>(frames.size());
    beatFirst_ = firstFrame;
    beatLast_ = firstFrame;
    beatTimer_ = 0.0f;
    popFired_ = false;

    // 一次攻击产生「撞击 -> A 打 B -> B 反击 A」三帧，并成同一拍，让双方同时掉血
    if (firstFrame >= 0 && firstFrame < total &&
        frames[static_cast<std::size_t>(firstFrame)].type == EventType::Attack) {
        const int a = frames[static_cast<std::size_t>(firstFrame)].sourceId;
        const int t = frames[static_cast<std::size_t>(firstFrame)].targetId;
        int j = firstFrame + 1;
        while (j < total && frames[static_cast<std::size_t>(j)].type == EventType::Damage &&
               ((frames[j].sourceId == a && frames[j].targetId == t) ||
                (frames[j].sourceId == t && frames[j].targetId == a))) {
            beatLast_ = j;
            ++j;
        }
    }
    beatDuration_ = kCombatFrameSeconds * static_cast<float>(beatLast_ - beatFirst_ + 1);
}

bool Viewer::combatCardPos(const CombatFrame& f, int instanceId, Vector2* out) const {
    for (int side = 0; side < 2; ++side) {
        const std::vector<Plant>& board = side == 0 ? f.left : f.right;
        for (int i = 0; i < static_cast<int>(board.size()); ++i) {
            if (board[static_cast<std::size_t>(i)].instanceId != instanceId) {
                continue;
            }
            const Rectangle r = combatCard(side, i, static_cast<int>(board.size()));
            out->x = r.x + r.width * 0.5f;
            out->y = r.y + r.height * 0.5f;
            return true;
        }
    }
    return false;
}

void Viewer::fireCombatFx(const CombatFrame& before, const CombatFrame& after) {
    for (int side = 0; side < 2; ++side) {
        const std::vector<Plant>& b0 = side == 0 ? before.left : before.right;
        const std::vector<Plant>& b1 = side == 0 ? after.left : after.right;
        for (int i = 0; i < static_cast<int>(b0.size()); ++i) {
            const Plant& p = b0[static_cast<std::size_t>(i)];
            for (const Plant& q : b1) {
                if (q.instanceId != p.instanceId) {
                    continue;
                }
                // 位置取变化前的布局：毒杀的植物随后会被移出场，位置就查不到了
                const Rectangle r = combatCard(side, i, static_cast<int>(b0.size()));
                if (p.hasShield && !q.hasShield) {
                    combatFx_.push_back(CombatFx{CombatFx::Kind::ShieldPop, r, 0.0f});
                }
                if (!p.destroyed && q.destroyed) {
                    combatFx_.push_back(CombatFx{CombatFx::Kind::Poison, r, 0.0f});
                }
                break;
            }
        }
    }
}

void Viewer::drawCombatFx() const {
    for (const CombatFx& fx : combatFx_) {
        const float k = std::min(fx.timer / kCombatFxSeconds, 1.0f);
        const unsigned char fade = static_cast<unsigned char>(255.0f * (1.0f - k));
        const Vector2 c{fx.rect.x + fx.rect.width * 0.5f, fx.rect.y + fx.rect.height * 0.5f};

        if (fx.kind == CombatFx::Kind::ShieldPop) {
            // 护盾碎裂：向外扩散的圆角环 + 六片碎片
            const float grow = 8.0f + 26.0f * k;
            DrawRectangleRoundedLinesEx(
                Rectangle{fx.rect.x - grow, fx.rect.y - grow, fx.rect.width + grow * 2.0f,
                          fx.rect.height + grow * 2.0f},
                0.16f, 8, 2.5f, Color{188, 228, 255, fade});
            const float ex = fx.rect.width * 0.5f + 6.0f + 16.0f * k;
            const float ey = fx.rect.height * 0.5f + 6.0f + 16.0f * k;
            for (int i = 0; i < 6; ++i) {
                const float ang = static_cast<float>(i) * (PI / 3.0f) + 0.45f;
                const float ca = cosf(ang);
                const float sa = sinf(ang);
                DrawLineEx(Vector2{c.x + ca * ex, c.y + sa * ey},
                           Vector2{c.x + ca * (ex + 14.0f), c.y + sa * (ey + 14.0f)}, 2.5f,
                           Color{200, 234, 255, fade});
            }
        } else {
            // 毒杀：紫色毒雾上飘 + 卡面染紫
            DrawRectangleRec(fx.rect, Color{124, 72, 192, static_cast<unsigned char>(78.0f * (1.0f - k))});
            for (int i = 0; i < 7; ++i) {
                const float t = static_cast<float>(i) / 6.0f;
                const float bx = fx.rect.x + fx.rect.width * (0.16f + 0.68f * t) +
                                 sinf(k * 6.0f + t * 5.0f) * 8.0f;
                const float by = c.y + fx.rect.height * 0.18f - k * (58.0f + 26.0f * t);
                const float rad = 5.0f + 9.0f * t;
                DrawCircleV(Vector2{bx, by}, rad, Color{150, 96, 214, fade});
                DrawCircleLinesV(Vector2{bx, by}, rad, Color{204, 164, 248, fade});
            }
        }
    }
}

int Viewer::combatHealthDrop(const CombatFrame& before, const CombatFrame& after,
                             int instanceId) const {
    for (int side = 0; side < 2; ++side) {
        const std::vector<Plant>& b0 = side == 0 ? before.left : before.right;
        const std::vector<Plant>& b1 = side == 0 ? after.left : after.right;
        for (const Plant& p : b0) {
            if (p.instanceId != instanceId) {
                continue;
            }
            for (const Plant& q : b1) {
                if (q.instanceId == instanceId) {
                    return std::max(0, p.health - q.health);
                }
            }
            return std::max(0, p.health);
        }
    }
    return 0;
}

void Viewer::skipCombat() {
    const int total = static_cast<int>(match_->combatFrames().size());
    if (frameIndex_ < total) {
        frameIndex_ = total;
        beatFirst_ = total;
        beatLast_ = total;
    } else {
        match_->finishCombat();
        frameIndex_ = 0;
        beatFirst_ = 0;
        beatLast_ = 0;
        beatTimer_ = 0.0f;
        beatDuration_ = 0.0f;
        popFired_ = false;
        combatFx_.clear();
        endTimer_ = 0.0f;
    }
}

void Viewer::cycleSpeed() {
    combatSpeed_ = combatSpeed_ >= 4.0f ? 1.0f : combatSpeed_ * 2.0f;
}

void Viewer::updateCombatPlayback() {
    const float dt = GetFrameTime() * combatSpeed_;
    for (std::size_t i = 0; i < combatFx_.size();) {
        combatFx_[i].timer += dt;
        if (combatFx_[i].timer >= kCombatFxSeconds) {
            combatFx_.erase(combatFx_.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }

    const std::vector<CombatFrame>& frames = match_->combatFrames();
    const int total = static_cast<int>(frames.size());
    if (frameIndex_ < total) {
        beatTimer_ += dt;

        // 破盾 / 毒杀：撞击拍在接触瞬间结算，其它拍在拍首结算
        if (!popFired_ && beatFirst_ < total && beatLast_ < total) {
            const bool clash =
                beatLast_ > beatFirst_ && frames[static_cast<std::size_t>(beatFirst_)].type ==
                                              EventType::Attack;
            if (!clash) {
                if (beatFirst_ > 0) {
                    fireCombatFx(frames[static_cast<std::size_t>(beatFirst_ - 1)],
                                 frames[static_cast<std::size_t>(beatFirst_)]);
                }
                popFired_ = true;
            } else if (beatTimer_ >= beatDuration_ * kCombatContact) {
                fireCombatFx(frames[static_cast<std::size_t>(beatFirst_)],
                             frames[static_cast<std::size_t>(beatLast_)]);
                popFired_ = true;
            }
        }

        if (beatTimer_ >= beatDuration_) {
            frameIndex_ = beatLast_ + 1;
            beatTimer_ = 0.0f;
            if (frameIndex_ < total) {
                startCombatBeat(frameIndex_);
            }
        }
        return;
    }
    endTimer_ += GetFrameTime();
    if (endTimer_ >= 0.9f) {
        skipCombat();
    }
}

Rectangle Viewer::rowCard(Zone zone, int index) const {
    const GardenState& me = match_->player(0);
    int count = 0;
    float w = 0.0f;
    float h = 0.0f;
    float gap = 0.0f;
    float y = 0.0f;
    switch (zone) {
    case Zone::Shop:
        count = static_cast<int>(me.shop.size());
        w = kShopCardW;
        h = kShopCardH;
        gap = kShopGap;
        y = kShopY;
        break;
    case Zone::Hand:
        count = static_cast<int>(me.hand.size());
        w = kHandCardW;
        h = kHandCardH;
        gap = kHandGap;
        y = kHandY;
        break;
    default:
        count = static_cast<int>(me.board.size());
        w = kBoardCardW;
        h = kBoardCardH;
        gap = kBoardGap;
        y = kBoardY;
        break;
    }
    if (count <= 0 || index < 0 || index >= count) {
        return Rectangle{0.0f, 0.0f, 0.0f, 0.0f};
    }
    if (zone == Zone::Hand) {
        float step = kHandCardW + kHandGap;
        if (count > 1) {
            const float spread =
                (kHandFanSpan - kHandCardW) / static_cast<float>(count - 1);
            step = std::min(step, std::max(kHandFanStep, spread));
        }
        const float total = kHandCardW + static_cast<float>(count - 1) * step;
        const float x0 = kHandRowLeft + ((kPlayRight - kHandRowLeft) - total) * 0.5f;
        return Rectangle{x0 + static_cast<float>(index) * step, y, kHandCardW, kHandCardH};
    }
    const float total = static_cast<float>(count) * w + static_cast<float>(count - 1) * gap;
    const float x0 = kPlayLeft + ((kPlayRight - kPlayLeft) - total) * 0.5f;
    return Rectangle{x0 + static_cast<float>(index) * (w + gap), y, w, h};
}

Rectangle Viewer::handCardRect(int index) const {
    Rectangle r = rowCard(Zone::Hand, index);
    r.y -= kHandHoverLift;
    r.height += kHandHoverLift;
    return r;
}

Rectangle Viewer::combatCard(int side, int index, int count) const {
    if (count <= 0 || index < 0 || index >= count) {
        return Rectangle{0.0f, 0.0f, 0.0f, 0.0f};
    }
    const float total =
        static_cast<float>(count) * kCombatCardW + static_cast<float>(count - 1) * kCombatGap;
    const float x0 = (static_cast<float>(kWindowWidth) - total) * 0.5f;
    return Rectangle{x0 + static_cast<float>(index) * (kCombatCardW + kCombatGap),
                     side == 0 ? kCombatMineY : kCombatFoeY, kCombatCardW, kCombatCardH};
}

Rectangle Viewer::discoverCardRect(int index) const {
    constexpr float w = 230.0f;
    constexpr float h = 307.0f;
    constexpr float gap = 30.0f;
    const float total = 3.0f * w + 2.0f * gap;
    const float x0 = (static_cast<float>(kWindowWidth) - total) * 0.5f;
    return Rectangle{x0 + static_cast<float>(index) * (w + gap), 300.0f, w, h};
}

Rectangle Viewer::gardenerCardRect(int index) const {
    constexpr float w = 380.0f;
    constexpr float h = 236.0f;
    constexpr float gap = 30.0f;
    const int col = index % 3;
    const int row = index / 3;
    const float x0 = (static_cast<float>(kWindowWidth) - (3.0f * w + 2.0f * gap)) * 0.5f;
    return Rectangle{x0 + static_cast<float>(col) * (w + gap),
                     200.0f + static_cast<float>(row) * (h + gap), w, h};
}

void Viewer::handleGardenerSelect() {
    hoverGardener_ = -1;
    const int count = static_cast<int>(gardeners_->size());
    if (count == 0) {
        return;
    }

    const Vector2 m = GetMousePosition();
    for (int i = 0; i < count; ++i) {
        if (CheckCollisionPointRec(m, gardenerCardRect(i))) {
            hoverGardener_ = i;
            break;
        }
    }

    if (hoverGardener_ >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        humanGardener_ = hoverGardener_;
        selecting_ = false;
        restart();
    }
}

Viewer::Zone Viewer::zoneAt(Vector2 m) const {
    if (m.x > kPlayRight) {
        return Zone::None;
    }
    if (m.y >= kDropBoardTop && m.y < kDropBoardBottom) {
        return Zone::Board;
    }
    if (m.y >= kDropHandTop && m.y < kDropHandBottom) {
        return Zone::Hand;
    }
    if (m.y >= kDropShopTop && m.y < kDropShopBottom) {
        return Zone::Shop;
    }
    return Zone::None;
}

Rectangle Viewer::dropBand(Zone zone) const {
    switch (zone) {
    case Zone::Board:
        return Rectangle{kPlayLeft, kDropBoardTop, kPlayRight - kPlayLeft,
                         kDropBoardBottom - kDropBoardTop};
    case Zone::Hand:
        return Rectangle{kPlayLeft, kDropHandTop, kPlayRight - kPlayLeft,
                         kDropHandBottom - kDropHandTop};
    case Zone::Shop:
        return Rectangle{kPlayLeft, kDropShopTop, kPlayRight - kPlayLeft,
                         kDropShopBottom - kDropShopTop};
    default:
        return Rectangle{0.0f, 0.0f, 0.0f, 0.0f};
    }
}

int Viewer::boardInsertIndex(float mouseX) const {
    const GardenState& me = match_->player(0);
    const int count = static_cast<int>(me.board.size());
    for (int i = 0; i < count; ++i) {
        const Rectangle r = rowCard(Zone::Board, i);
        if (mouseX < r.x + r.width * 0.5f) {
            return i;
        }
    }
    return count;
}

void Viewer::drawDragGhost() {
    if (!dragging_) {
        return;
    }
    const GardenState& me = match_->player(0);
    const Plant* p = nullptr;
    if (dragZone_ == Zone::Shop && dragIndex_ < static_cast<int>(me.shop.size())) {
        p = &me.shop[static_cast<std::size_t>(dragIndex_)];
    } else if (dragZone_ == Zone::Hand && dragIndex_ < static_cast<int>(me.hand.size())) {
        p = &me.hand[static_cast<std::size_t>(dragIndex_)];
    } else if (dragZone_ == Zone::Board && dragIndex_ < static_cast<int>(me.board.size())) {
        p = &me.board[static_cast<std::size_t>(dragIndex_)];
    }
    if (p == nullptr) {
        return;
    }

    const Zone target = zoneAt(dragPos_);
    bool valid = false;
    if (dragZone_ == Zone::Shop && target == Zone::Hand) {
        valid = true;
    } else if (dragZone_ == Zone::Hand && target == Zone::Board) {
        valid = true;
    } else if (dragZone_ == Zone::Board && (target == Zone::Shop || target == Zone::Board)) {
        valid = true;
    }

    if (valid) {
        const Rectangle band = dropBand(target);
        DrawRectangleRec(band, Color{70, 150, 80, 55});
        DrawRectangleLinesEx(band, 2.0f, Color{140, 220, 150, 190});
    }

    const float w = kShopCardW;
    const float h = kShopCardH;
    drawCard(*p, Rectangle{dragPos_.x - w * 0.5f, dragPos_.y - h * 0.5f, w, h}, valid ? 1 : 0);
}

void Viewer::handleInput() {
    hoverZone_ = Zone::None;
    hoverIndex_ = -1;

    const GardenState& me = match_->player(0);
    const bool uiMouse = ImGui::GetIO().WantCaptureMouse;
    const Vector2 m = GetMousePosition();

    hoveredOpponent_ = -1;
    if (!uiMouse) {
        const int count = match_->playerCount();
        const int human = match_->humanIndex();
        int slot = 0;
        for (int p = 0; p < count; ++p) {
            if (p == human) {
                continue;
            }
            if (CheckCollisionPointRec(m, opponentCircleRect(slot))) {
                hoveredOpponent_ = slot;
                break;
            }
            ++slot;
        }
    }

    hoveredSelf_ = false;
    if (!uiMouse && CheckCollisionPointRec(m, selfCircleRect())) {
        hoveredSelf_ = true;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            match_->playerAct(PlayerAction::Skill, 0);
        }
    }

    if (!me.discover.empty()) {
        dragging_ = false;
        if (!uiMouse) {
            for (int i = 0; i < static_cast<int>(me.discover.size()); ++i) {
                if (CheckCollisionPointRec(m, discoverCardRect(i))) {
                    hoverZone_ = Zone::Discover;
                    hoverIndex_ = i;
                    break;
                }
            }
        }
        if (hoverZone_ == Zone::Discover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            match_->playerAct(PlayerAction::PickDiscover, hoverIndex_);
        }
        return;
    }

    const bool canPlay = match_->awaitingPlayer() && !uiMouse;

    if (!dragging_ && canPlay) {
        const struct {
            Zone zone;
            int count;
        } zones[] = {{Zone::Shop, static_cast<int>(me.shop.size())},
                     {Zone::Hand, static_cast<int>(me.hand.size())},
                     {Zone::Board, static_cast<int>(me.board.size())}};
        for (const auto& z : zones) {
            const bool topmostLast = z.zone == Zone::Hand;
            for (int k = 0; k < z.count; ++k) {
                const int i = topmostLast ? z.count - 1 - k : k;
                const Rectangle r = topmostLast ? handCardRect(i) : rowCard(z.zone, i);
                if (CheckCollisionPointRec(m, r)) {
                    hoverZone_ = z.zone;
                    hoverIndex_ = i;
                    break;
                }
            }
            if (hoverZone_ != Zone::None) {
                break;
            }
        }
    }

    if (dragging_) {
        hoverZone_ = Zone::None;
        hoverIndex_ = -1;
        dragPos_ = m;

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            const Zone target = zoneAt(m);
            if (dragZone_ == Zone::Shop && target == Zone::Hand) {
                match_->playerAct(PlayerAction::Buy, dragIndex_);
            } else if (dragZone_ == Zone::Hand && target == Zone::Board) {
                match_->playerAct(PlayerAction::Play, dragIndex_, boardInsertIndex(m.x));
            } else if (dragZone_ == Zone::Board && target == Zone::Shop) {
                match_->playerAct(PlayerAction::Sell, dragIndex_);
            } else if (dragZone_ == Zone::Board && target == Zone::Board) {
                int dest = boardInsertIndex(m.x);
                if (dest > dragIndex_) {
                    --dest;
                }
                match_->playerAct(PlayerAction::Move, dragIndex_, dest);
            }
            dragging_ = false;
            dragZone_ = Zone::None;
            dragIndex_ = -1;
        }
        return;
    }

    if (!canPlay) {
        return;
    }

    if (hoverZone_ != Zone::None && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        dragging_ = true;
        dragZone_ = hoverZone_;
        dragIndex_ = hoverIndex_;
        dragPos_ = m;
    }

    if (!ImGui::GetIO().WantCaptureKeyboard && IsKeyPressed(KEY_SPACE)) {
        startCombat();
    }
}

void Viewer::drawText(const std::string& text, float x, float y, float size, Color color) const {
    DrawTextEx(font_, text.c_str(), Vector2{x, y}, size, 1.0f, color);
}

void Viewer::drawTextCenteredX(const std::string& text, float centerX, float y, float size,
                               Color color) const {
    const Vector2 m = MeasureTextEx(font_, text.c_str(), size, 1.0f);
    drawText(text, centerX - m.x * 0.5f, y, size, color);
}

void Viewer::drawTextWrapped(const std::string& text, float x, float y, float size,
                             Color color, float maxWidth, int maxLines) const {
    std::string line;
    float lineY = y;
    int lines = 1;
    const char* p = text.c_str();
    while (*p != '\0') {
        int charBytes = 0;
        GetCodepointNext(p, &charBytes);
        std::string ch(p, static_cast<std::size_t>(charBytes));
        p += charBytes;
        // 连续 ASCII 视为一个整体，避免 "+3/+3" 这类被从中间拆开
        if (static_cast<unsigned char>(ch[0]) < 0x80) {
            while (*p != '\0' && static_cast<unsigned char>(*p) < 0x80) {
                ch += *p;
                ++p;
            }
        }
        if (line.empty() && !ch.empty() && ch[0] == ' ') {
            ch.erase(0, 1);
        }
        if (!line.empty() &&
            MeasureTextEx(font_, (line + ch).c_str(), size, 1.0f).x > maxWidth) {
            drawText(line, x, lineY, size, color);
            if (++lines > maxLines) {
                return;
            }
            lineY += size + 4.0f;
            line.clear();
        }
        line += ch;
    }
    if (!line.empty()) {
        drawText(line, x, lineY, size, color);
    }
}

void Viewer::drawGardenerTooltip(int side) {
    const GardenState& g = match_->player(side);
    const Gardener& gd = match_->gardenerOf(side);
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(370.0f);
    ImGui::Text("%s · %s", gd.name.c_str(), gd.skill.name.c_str());
    ImGui::Text("血 %d    护甲 %d    草坪 %d 级", std::max(g.health, 0), g.shield, g.gardenTier);
    ImGui::Separator();
    ImGui::TextWrapped("%s", gd.skill.description.c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

Rectangle Viewer::selfCircleRect() const {
    return Rectangle{kSelfCircleX, kSelfCircleY, kOppD, kOppD};
}

Rectangle Viewer::opponentCircleRect(int index) const {
    return Rectangle{kOppX + (kOppW - kOppD) * 0.5f,
                     104.0f + static_cast<float>(index) * (kOppD + 12.0f), kOppD, kOppD};
}

int Viewer::gardenerIndexOf(const Gardener& gd) const {
    return static_cast<int>(&gd - gardeners_->data());
}

void Viewer::drawGardenerBadge(int gardenerIndex, Rectangle r, Color ring, bool hot,
                               const std::string& label) {
    const Vector2 c{r.x + r.width * 0.5f, r.y + r.height * 0.5f};
    const float radius = r.width * 0.5f;

    DrawCircleV(c, radius, Color{22, 30, 24, 255});
    if (gardenerIndex >= 0 && gardenerIndex < static_cast<int>(gardenerFace_.size())) {
        const Texture2D& face = gardenerFace_[static_cast<std::size_t>(gardenerIndex)];
        if (face.id != 0) {
            DrawTexturePro(face,
                           Rectangle{0.0f, 0.0f, static_cast<float>(face.width),
                                     static_cast<float>(face.height)},
                           r, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        }
    }

    DrawCircleLinesV(c, radius, hot ? Color{255, 255, 255, 255} : ring);
    DrawCircleLinesV(c, radius - 4.0f, Color{255, 255, 255, 40});

    constexpr float kLabelSize = 13.0f;
    const float textW = MeasureTextEx(font_, label.c_str(), kLabelSize, 1.0f).x;
    const Rectangle plate{c.x - (textW + 12.0f) * 0.5f, c.y + radius * 0.42f - 10.0f,
                          textW + 12.0f, 20.0f};
    DrawRectangleRounded(plate, 0.5f, 8, Color{10, 20, 13, 205});
    drawTextCenteredX(label, c.x, plate.y + 3.0f, kLabelSize, Color{252, 242, 208, 255});
}

void Viewer::drawOpponents() {
    const int count = match_->playerCount();
    const int human = match_->humanIndex();
    if (count <= 1) {
        return;
    }

    drawZonePanel(Rectangle{kOppX, 92.0f, kOppW, 756.0f}, Color{42, 28, 30, 255},
                  Color{26, 17, 19, 255}, Color{104, 60, 66, 255}, "", Color{0, 0, 0, 0});

    int slot = 0;
    for (int p = 0; p < count; ++p) {
        if (p == human) {
            continue;
        }
        const Gardener& gd = match_->gardenerOf(p);
        const bool hot = hoveredOpponent_ == slot;
        drawGardenerBadge(gardenerIndexOf(gd), opponentCircleRect(slot), Color{146, 88, 96, 255},
                          hot, gd.name);
        ++slot;
    }
}


void Viewer::drawGradientRect(Rectangle r, Color top, Color bottom) const {
    constexpr int steps = 28;
    const float h = r.height / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(steps - 1);
        const Color c{static_cast<unsigned char>(top.r + (bottom.r - top.r) * t),
                      static_cast<unsigned char>(top.g + (bottom.g - top.g) * t),
                      static_cast<unsigned char>(top.b + (bottom.b - top.b) * t), 255};
        DrawRectangleRec(Rectangle{r.x, r.y + static_cast<float>(i) * h, r.width, h + 1.0f}, c);
    }
}

void Viewer::drawZonePanel(Rectangle r, Color top, Color bottom, Color border,
                           const std::string& label, Color labelColor) const {
    drawGradientRect(r, top, bottom);
    DrawRectangleRec(Rectangle{r.x, r.y, r.width, 2.0f},
                     Color{static_cast<unsigned char>(border.r + 30),
                           static_cast<unsigned char>(border.g + 30),
                           static_cast<unsigned char>(border.b + 30), 255});
    DrawRectangleLinesEx(r, 1.0f, border);
    if (!label.empty()) {
        drawText(label, r.x + 12.0f, r.y + 7.0f, 17.0f, labelColor);
    }
}

void Viewer::drawLawnTexture(Rectangle r) const {
    const Color grass{54, 88, 56, 255};
    const float baseY = r.y + r.height - 3.0f;
    for (int i = 0; i < 120; ++i) {
        const float x = r.x + 12.0f + static_cast<float>(i) * 11.7f;
        if (x > r.x + r.width - 12.0f) {
            break;
        }
        const float h = 3.0f + static_cast<float>(i % 4);
        DrawLine(static_cast<int>(x), static_cast<int>(baseY),
                 static_cast<int>(x), static_cast<int>(baseY - h), grass);
    }
}

void Viewer::drawTextCentered(const std::string& text, float y, float size, Color color) const {
    const Vector2 m = MeasureTextEx(font_, text.c_str(), size, 1.0f);
    drawText(text, (static_cast<float>(kWindowWidth) - m.x) * 0.5f, y, size, color);
}

Color Viewer::raceColor(const std::string& race) const {
    if (race == "flower") return Color{240, 190, 110, 255};
    if (race == "vine") return Color{130, 215, 130, 255};
    if (race == "fungus") return Color{185, 155, 235, 255};
    if (race == "tree") return Color{150, 195, 115, 255};
    if (race == "thorn") return Color{225, 160, 105, 255};
    if (race == "aquatic") return Color{115, 190, 240, 255};
    if (race == "carnivorous") return Color{235, 130, 155, 255};
    return Color{200, 200, 200, 255};
}

void Viewer::drawTextureCover(const Texture2D& tex, Rectangle r) const {
    const float texAspect = static_cast<float>(tex.width) / static_cast<float>(tex.height);
    const float cardAspect = r.width / r.height;
    Rectangle src{0.0f, 0.0f, static_cast<float>(tex.width), static_cast<float>(tex.height)};
    if (texAspect > cardAspect) {
        src.width = static_cast<float>(tex.height) * cardAspect;
        src.x = (static_cast<float>(tex.width) - src.width) * 0.5f;
    } else {
        src.height = static_cast<float>(tex.width) / cardAspect;
        src.y = (static_cast<float>(tex.height) - src.height) * 0.5f;
    }
    DrawTexturePro(tex, src, r, Vector2{0.0f, 0.0f}, 0.0f, WHITE);
}

bool Viewer::drawCardArt(const Plant& p, Rectangle r) const {
    if (p.templateId < 0 || p.templateId >= static_cast<int>(plantArt_.size())) {
        return false;
    }
    const Texture2D& tex = plantArt_[static_cast<std::size_t>(p.templateId)];
    if (tex.id == 0) {
        return false;
    }
    drawTextureCover(tex, r);
    return true;
}

void Viewer::drawThornFrame(Rectangle r) const {
    const float step = std::max(14.0f, std::min(26.0f, r.width * 0.13f));
    const float half = step * 0.30f;
    const float len = std::max(5.0f, std::min(9.0f, r.width * 0.048f));
    const Color bone{226, 216, 190, 255};

    // rlImGui 结束后背面剔除是开着的，同一个三角正反各画一次保证可见
    const auto spike = [](Vector2 a, Vector2 b, Vector2 tip, Color col) {
        DrawTriangle(a, b, tip, col);
        DrawTriangle(tip, b, a, col);
    };

    for (float x = r.x + step * 0.5f; x < r.x + r.width - half; x += step) {
        spike(Vector2{x - half, r.y + 1.0f}, Vector2{x + half, r.y + 1.0f},
              Vector2{x, r.y - len}, bone);
        spike(Vector2{x - half, r.y + r.height - 1.0f},
              Vector2{x + half, r.y + r.height - 1.0f}, Vector2{x, r.y + r.height + len}, bone);
    }
    for (float y = r.y + step * 0.5f; y < r.y + r.height - half; y += step) {
        spike(Vector2{r.x + 1.0f, y - half}, Vector2{r.x + 1.0f, y + half},
              Vector2{r.x - len, y}, bone);
        spike(Vector2{r.x + r.width - 1.0f, y - half},
              Vector2{r.x + r.width - 1.0f, y + half}, Vector2{r.x + r.width + len, y}, bone);
    }
}

void Viewer::drawShieldBubble(Rectangle r) const {
    const Rectangle inner{r.x + 4.0f, r.y + 4.0f, r.width - 8.0f, r.height - 8.0f};
    DrawRectangleRounded(inner, 0.16f, 8, Color{132, 196, 255, 30});
    DrawRectangleRoundedLinesEx(inner, 0.16f, 8, 2.5f, Color{158, 218, 255, 210});
    DrawLineEx(Vector2{inner.x + inner.width * 0.13f, inner.y + inner.height * 0.30f},
               Vector2{inner.x + inner.width * 0.30f, inner.y + inner.height * 0.10f}, 3.0f,
               Color{230, 248, 255, 170});
    DrawLineEx(Vector2{inner.x + inner.width * 0.13f, inner.y + inner.height * 0.45f},
               Vector2{inner.x + inner.width * 0.23f, inner.y + inner.height * 0.30f}, 2.0f,
               Color{230, 248, 255, 120});
}

void Viewer::drawPoisonMark(Rectangle r) const {
    // 毒刺：底边毒液条 + 交替下垂的毒滴，和荆棘尖刺/护盾泡区分开
    const float stripH = std::max(4.0f, r.height * 0.02f);
    DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y + r.height - stripH),
                  static_cast<int>(r.width), static_cast<int>(stripH + 1.0f),
                  Color{124, 196, 72, 215});
    DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y + r.height * 0.78f),
                  static_cast<int>(r.width), static_cast<int>(r.height * 0.22f),
                  Color{124, 196, 72, 26});

    const float step = std::max(16.0f, std::min(30.0f, r.width * 0.22f));
    const float radius = std::max(3.0f, std::min(5.5f, r.width * 0.028f));
    int k = 0;
    for (float x = r.x + step * 0.5f; x < r.x + r.width - radius; x += step, ++k) {
        const float cy = r.y + r.height + 1.0f + 7.0f * static_cast<float>(k % 2);
        DrawCircleV(Vector2{x, cy}, radius, Color{150, 226, 96, 255});
        DrawCircleLinesV(Vector2{x, cy}, radius, Color{92, 54, 176, 255});
    }
}

void Viewer::drawCard(const Plant& p, Rectangle r, int outline) const {
    const PlantTemplate& tmpl = (*pool_)[static_cast<std::size_t>(p.templateId)];
    const Color accent = raceColor(tmpl.race);

    Color border = p.goldenLevel > 0 ? Color{255, 210, 60, 255} : accent;
    float thickness = p.goldenLevel > 0 ? 3.0f : 2.0f;
    if (outline == 1) {
        border = Color{255, 255, 255, 255};
        thickness = 3.0f;
    } else if (outline == 2) {
        border = Color{255, 225, 90, 255};
        thickness = 4.0f;
    } else if (outline == 3) {
        border = Color{255, 80, 80, 255};
        thickness = 4.0f;
    }

    if (drawCardArt(p, r)) {
        DrawRectangleRec(r, Color{9, 13, 9, 120});
        DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width),
                      static_cast<int>(r.height * 0.34f), Color{8, 11, 8, 130});
        DrawRectangle(static_cast<int>(r.x),
                      static_cast<int>(r.y + r.height * 0.66f),
                      static_cast<int>(r.width), static_cast<int>(r.height * 0.34f),
                      Color{8, 11, 8, 165});
    } else {
        DrawRectangleRec(r, Color{34, 40, 34, 255});
    }
    DrawRectangleLinesEx(r, thickness, border);

    if (p.hasThorns) {
        drawThornFrame(r);
    }
    if (p.hasShield) {
        drawShieldBubble(r);
    }
    if (p.hasPoison) {
        drawPoisonMark(r);
    }

    const float nameSize = std::max(15.0f, r.width * 0.155f);
    const float raceSize = std::max(11.0f, r.width * 0.095f);
    const float statSize = std::max(15.0f, r.width * 0.165f);
    const float kwSize = std::max(12.0f, r.width * 0.088f);
    const float tierSize = std::max(12.0f, r.width * 0.10f);

    drawText(tmpl.name + (p.goldenLevel > 0 ? "★" : ""), r.x + 12.0f, r.y + 10.0f, nameSize,
             Color{240, 240, 230, 255});

    float nextY = r.y + 10.0f + nameSize + 7.0f;
    if (r.width >= 150.0f) {
        drawText(tmpl.race, r.x + 12.0f, nextY, raceSize, accent);
        nextY += raceSize + 9.0f;
    }
    if (r.width >= 110.0f) {
        const std::string kw = keywordText(p, tmpl);
        if (!kw.empty()) {
            drawText(kw, r.x + 12.0f, nextY, kwSize, Color{178, 198, 178, 255});
        }
    }

    const float statY = r.y + r.height - statSize - 14.0f;
    drawText(formatStats(p.attack, p.health), r.x + 12.0f, statY, statSize,
             Color{255, 230, 160, 255});
    drawText("T" + std::to_string(tmpl.tier), r.x + r.width - tierSize * 1.7f - 12.0f, statY,
             tierSize, tierColor(tmpl.tier));
}

std::string Viewer::keywordText(const Plant& p, const PlantTemplate& t) {
    std::string s;
    const auto add = [&s](const char* word) {
        if (!s.empty()) {
            s += " ";
        }
        s += word;
    };
    if (p.hasThorns) {
        add("荆棘");
    }
    if (p.hasShield) {
        add("护盾");
    }
    if (p.hasFastGrow) {
        add("速生");
    }
    if (p.hasPoison) {
        add("毒刺");
    }
    if (p.hasReborn) {
        add("再生");
    }
    if (!t.battlecry.empty()) {
        add("播种");
    }
    if (!t.deathrattle.empty()) {
        add("凋零");
    }
    if (!t.aura.empty()) {
        add("光合");
    }
    return s;
}

void Viewer::drawRecruitScene() {
    const GardenState& me = match_->player(0);

    drawText("招募阶段", 20.0f, 9.0f, 26.0f, Color{150, 220, 160, 255});

    drawTextCentered("阳光 " + std::to_string(me.sunlight), 0.0f, 38.0f,
                     Color{255, 226, 140, 255});

    const std::string hpText = "血 " + std::to_string(std::max(me.health, 0));
    const std::string armorText = "护甲 " + std::to_string(me.shield);
    constexpr float kStatSize = 21.0f;
    constexpr float kStatGap = 24.0f;
    const float hpW = MeasureTextEx(font_, hpText.c_str(), kStatSize, 1.0f).x;
    const float armorW = MeasureTextEx(font_, armorText.c_str(), kStatSize, 1.0f).x;
    const float statX =
        (static_cast<float>(kWindowWidth) - (hpW + kStatGap + armorW)) * 0.5f;
    drawText(hpText, statX, 46.0f, kStatSize, Color{238, 180, 180, 255});
    drawText(armorText, statX + hpW + kStatGap, 46.0f, kStatSize, Color{178, 205, 232, 255});

    drawZonePanel(Rectangle{kPlayLeft, 92.0f, kPlayRight - kPlayLeft, 292.0f},
                  Color{26, 42, 28, 255}, Color{17, 27, 19, 255}, Color{60, 94, 64, 255},
                  "草坪 " + std::to_string(me.gardenTier) + " 级", tierColor(me.gardenTier));
    drawLawnTexture(Rectangle{kPlayLeft, 92.0f, kPlayRight - kPlayLeft, 292.0f});
    drawZonePanel(Rectangle{kPlayLeft, 384.0f, kPlayRight - kPlayLeft, 162.0f},
                  Color{32, 36, 44, 255}, Color{21, 25, 31, 255}, Color{72, 82, 98, 255}, "手牌",
                  Color{165, 180, 205, 255});
    drawZonePanel(Rectangle{kPlayLeft, 546.0f, kPlayRight - kPlayLeft, 300.0f},
                  Color{44, 36, 25, 255}, Color{30, 24, 17, 255}, Color{106, 88, 60, 255},
                  "商店 · 草坪" + std::to_string(me.gardenTier) + " 级（可出 T1~T" +
                      std::to_string(me.gardenTier) + "）",
                  Color{218, 188, 134, 255});

    drawSelfCircle();
    drawOpponents();

    for (int i = 0; i < static_cast<int>(me.board.size()); ++i) {
        const bool hot = hoverZone_ == Zone::Board && hoverIndex_ == i;
        drawCard(me.board[static_cast<std::size_t>(i)], rowCard(Zone::Board, i), hot ? 1 : 0);
    }
    const int handCount = static_cast<int>(me.hand.size());
    const int hotHand = hoverZone_ == Zone::Hand ? hoverIndex_ : -1;
    for (int i = 0; i < handCount; ++i) {
        if (i == hotHand) {
            continue;
        }
        drawCard(me.hand[static_cast<std::size_t>(i)], rowCard(Zone::Hand, i), 0);
    }
    if (hotHand >= 0 && hotHand < handCount) {
        Rectangle r = rowCard(Zone::Hand, hotHand);
        r.y -= kHandHoverLift;
        drawCard(me.hand[static_cast<std::size_t>(hotHand)], r, 1);
    }
    for (int i = 0; i < static_cast<int>(me.shop.size()); ++i) {
        const bool hot = hoverZone_ == Zone::Shop && hoverIndex_ == i;
        drawCard(me.shop[static_cast<std::size_t>(i)], rowCard(Zone::Shop, i), hot ? 1 : 0);
    }

    if (match_->finished()) {
        drawTextCentered(match_->verdictText(), 860.0f, 28.0f, Color{255, 220, 120, 255});
    }
    drawText("拖拽：商店→手牌 购买    手牌→草坪 上场    草坪→商店 铲除    草坪内 调整站位    "
             "空格 出战    F1 日志    R 重开    ESC 退出",
             20.0f, 870.0f, 17.0f, Color{140, 150, 140, 255});
}

void Viewer::drawSelfCircle() {
    const GardenState& me = match_->player(0);
    const Gardener& gd = match_->gardenerOf(0);
    const GardenerSkill& sk = gd.skill;
    const bool ready = match_->awaitingPlayer() && me.discover.empty() && sk.active &&
                       !me.skillUsed && me.sunlight >= sk.cost;

    Color ring{96, 120, 150, 255};
    if (sk.active) {
        ring = ready ? Color{124, 204, 134, 255} : Color{84, 98, 86, 255};
    }
    std::string label = sk.name;
    if (sk.active) {
        label += me.skillUsed ? " 已用" : (" " + std::to_string(sk.cost));
    }
    drawGardenerBadge(gardenerIndexOf(gd), selfCircleRect(), ring, hoveredSelf_, label);
}

void Viewer::drawUpgradeButton() {
    const GardenState& me = match_->player(0);
    const bool canAct = match_->awaitingPlayer() && me.discover.empty();
    const int cost = upgradeCostForTier(me.gardenTier);

    ImGui::SetNextWindowPos(ImVec2(1214.0f, 94.0f));
    ImGui::SetNextWindowSize(ImVec2(106.0f, 24.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
    ImGui::Begin("##upgrade", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    ImGui::BeginDisabled(!canAct || cost < 0 || me.sunlight < cost);
    const std::string label =
        cost >= 0 ? ("升级 " + std::to_string(cost)) : std::string("已满级");
    if (ImGui::Button(label.c_str(), ImVec2(102.0f, 20.0f))) {
        match_->playerAct(PlayerAction::Upgrade, 0);
    }
    ImGui::EndDisabled();
    ImGui::End();
    ImGui::PopStyleVar();
}

void Viewer::drawDiscoverOverlay() {
    const GardenState& me = match_->player(0);
    const int count = static_cast<int>(me.discover.size());
    if (count == 0) {
        return;
    }

    DrawRectangle(0, 0, kWindowWidth, kWindowHeight, Color{0, 0, 0, 170});

    const int tier = std::min(me.gardenTier + 1, kMaxGardenTier);
    drawTextCentered("三连奖励 · 选择一株（草坪 " + std::to_string(tier) + " 级）", 196.0f, 36.0f,
                     Color{255, 225, 150, 255});
    for (int i = 0; i < count; ++i) {
        const bool hot = hoverZone_ == Zone::Discover && hoverIndex_ == i;
        drawCard(me.discover[static_cast<std::size_t>(i)], discoverCardRect(i), hot ? 1 : 0);
    }
    drawTextCentered("点击卡片选择，选完才能继续", 650.0f, 24.0f, Color{200, 205, 200, 255});
}

void Viewer::drawCombatScene() {
    const std::vector<CombatFrame>& frames = match_->combatFrames();
    const int total = static_cast<int>(frames.size());

    drawText("战斗阶段", 20.0f, 8.0f, 30.0f, Color{255, 200, 120, 255});
    drawText("第 " + std::to_string(match_->round()) + " 回合", 195.0f, 12.0f, 26.0f,
             Color{220, 220, 220, 255});

    drawZonePanel(Rectangle{0.0f, 48.0f, 1440.0f, 332.0f}, Color{46, 27, 27, 255},
                  Color{26, 15, 15, 255}, Color{112, 62, 62, 255}, "对手",
                  Color{232, 142, 142, 255});
    drawZonePanel(Rectangle{0.0f, 448.0f, 1440.0f, 352.0f}, Color{25, 42, 27, 255},
                  Color{15, 26, 17, 255}, Color{66, 106, 70, 255}, "你",
                  Color{152, 222, 158, 255});

    if (total == 0) {
        return;
    }

    const int first = std::max(0, std::min(beatFirst_, total - 1));
    const int last = std::max(first, std::min(beatLast_, total - 1));
    const CombatFrame& pre = frames[static_cast<std::size_t>(first)];
    const CombatFrame& post = frames[static_cast<std::size_t>(last)];
    const float progress =
        beatDuration_ > 0.0f ? std::min(beatTimer_ / beatDuration_, 1.0f) : 1.0f;

    // 撞击拍：攻击者冲向目标，接触瞬间切到结算后的棋盘 —— 双方同时掉血
    const bool clash = last > first && pre.type == EventType::Attack;
    const bool after = clash && progress >= kCombatContact;
    const CombatFrame& frame = after ? post : pre;

    Vector2 atkPos{0.0f, 0.0f};
    Vector2 tgtPos{0.0f, 0.0f};
    Vector2 lunge{0.0f, 0.0f};
    Vector2 away{0.0f, 0.0f};
    float recoil = 0.0f;
    float flash = 0.0f;
    bool paired = false;

    if (clash) {
        float reach = 0.0f;
        if (progress < kCombatContact) {
            const float k = progress / kCombatContact;
            reach = k * k * (3.0f - 2.0f * k);
        } else {
            const float k = (progress - kCombatContact) / (1.0f - kCombatContact);
            const float back = 1.0f - k;
            reach = back * back * (3.0f - 2.0f * back);
            recoil = sinf(k * 9.4248f) * kCombatRecoil * back;
            flash = back;
        }

        paired = combatCardPos(pre, pre.sourceId, &atkPos) &&
                 combatCardPos(pre, pre.targetId, &tgtPos);
        if (paired) {
            const float dx = tgtPos.x - atkPos.x;
            const float dy = tgtPos.y - atkPos.y;
            const float dist = sqrtf(dx * dx + dy * dy);
            if (dist > 1.0f) {
                const float travel =
                    std::min(dist * kCombatLungeRatio, kCombatLungeMax) * reach;
                lunge = Vector2{dx / dist * travel, dy / dist * travel};
                away = Vector2{-dx / dist, -dy / dist};
            }
        }
    }

    const auto animatedRect = [&](Rectangle r, const Plant& p) {
        if (!paired) {
            return r;
        }
        if (p.instanceId == pre.sourceId) {
            r.x += lunge.x;
            r.y += lunge.y;
        } else if (p.instanceId == pre.targetId) {
            r.x += away.x * recoil;
            r.y += away.y * recoil;
        }
        return r;
    };

    const auto drawSlot = [&](int side, const std::vector<Plant>& board, int i) {
        const Plant& p = board[static_cast<std::size_t>(i)];
        const int outline =
            p.instanceId == frame.sourceId ? 2 : (p.instanceId == frame.targetId ? 3 : 0);
        const Rectangle r = animatedRect(combatCard(side, i, static_cast<int>(board.size())), p);
        drawCard(p, r, outline);
        if (after && paired &&
            (p.instanceId == pre.sourceId || p.instanceId == pre.targetId)) {
            DrawRectangleRec(r, Color{255, 72, 72, static_cast<unsigned char>(95.0f * flash)});
        }
    };

    // 攻击者放到最后画，撞击时压在所有卡牌最上层
    int atkSide = -1;
    int atkIndex = -1;
    if (paired) {
        for (int side = 0; side < 2 && atkSide < 0; ++side) {
            const std::vector<Plant>& board = side == 0 ? frame.left : frame.right;
            for (int i = 0; i < static_cast<int>(board.size()); ++i) {
                if (board[static_cast<std::size_t>(i)].instanceId == pre.sourceId) {
                    atkSide = side;
                    atkIndex = i;
                    break;
                }
            }
        }
    }

    const auto drawRow = [&](int side, const std::vector<Plant>& board) {
        for (int i = 0; i < static_cast<int>(board.size()); ++i) {
            if (side == atkSide && i == atkIndex) {
                continue;
            }
            drawSlot(side, board, i);
        }
    };

    drawRow(1, frame.right);
    drawRow(0, frame.left);
    if (atkSide >= 0) {
        drawSlot(atkSide, atkSide == 0 ? frame.left : frame.right, atkIndex);
    }

    drawCombatFx();

    if (after && paired) {
        const float k = (progress - kCombatContact) / (1.0f - kCombatContact);
        const auto alpha = static_cast<unsigned char>(255.0f * (1.0f - k));
        const float rise = 30.0f * k;

        int toTarget = 0;
        int toAttacker = 0;
        for (int j = first + 1; j <= last; ++j) {
            const CombatFrame& f = frames[static_cast<std::size_t>(j)];
            if (f.type != EventType::Damage) {
                continue;
            }
            const int drop = combatHealthDrop(frames[static_cast<std::size_t>(j - 1)], f, f.targetId);
            if (f.targetId == pre.targetId) {
                toTarget += drop;
            } else if (f.targetId == pre.sourceId) {
                toAttacker += drop;
            }
        }

        if (toTarget > 0) {
            drawTextCenteredX("-" + std::to_string(toTarget), tgtPos.x + away.x * recoil,
                              tgtPos.y - kCombatCardH * 0.26f - rise, 36.0f,
                              Color{255, 88, 88, alpha});
        }
        if (toAttacker > 0) {
            drawTextCenteredX("-" + std::to_string(toAttacker), atkPos.x + lunge.x,
                              atkPos.y - kCombatCardH * 0.26f - rise, 36.0f,
                              Color{255, 88, 88, alpha});
        }
    }

    drawTextCentered(frame.line, 388.0f, 28.0f, Color{235, 235, 220, 255});
    drawTextCentered("事件 " + std::to_string(std::min(beatLast_ + 1, total)) + " / " +
                         std::to_string(total) + "    速度 x" +
                         std::to_string(static_cast<int>(combatSpeed_)),
                     420.0f, 22.0f, Color{170, 180, 170, 255});

    drawText("空格 跳到结果   S 切换速度   F1 日志   R 重开   ESC 退出", 20.0f, 868.0f, 18.0f,
             Color{140, 150, 140, 255});
}

void Viewer::drawGardenerSelect() {
    drawText("选择你的园丁", 120.0f, 88.0f, 40.0f, Color{240, 230, 180, 255});
    drawText("点击卡片开始对局", 120.0f, 146.0f, 22.0f, Color{170, 180, 170, 255});

    const int count = static_cast<int>(gardeners_->size());
    for (int i = 0; i < count; ++i) {
        const Gardener& gd = (*gardeners_)[static_cast<std::size_t>(i)];
        const Rectangle r = gardenerCardRect(i);
        const bool hot = hoverGardener_ == i;

        DrawRectangleRec(r, Color{30, 38, 30, 255});

        drawText(gd.name, r.x + 22.0f, r.y + 20.0f, 32.0f, Color{245, 240, 225, 255});
        const std::string kind =
            gd.skill.active ? ("主动 · " + std::to_string(gd.skill.cost) + " 阳光") : "被动";
        drawText(kind, r.x + 22.0f, r.y + 74.0f, 20.0f,
                 gd.skill.active ? Color{255, 215, 130, 255} : Color{150, 210, 240, 255});
        drawText(gd.skill.name, r.x + 22.0f, r.y + 112.0f, 26.0f, Color{200, 235, 200, 255});
        drawTextWrapped(gd.skill.description, r.x + 22.0f, r.y + 156.0f, 17.0f,
                        Color{190, 195, 190, 255}, 192.0f, 4);

        const int gi = gardenerIndexOf(gd);
        if (gi >= 0 && gi < static_cast<int>(gardenerArt_.size()) &&
            gardenerArt_[static_cast<std::size_t>(gi)].id != 0) {
            drawTextureCover(gardenerArt_[static_cast<std::size_t>(gi)],
                             Rectangle{r.x + 222.0f, r.y + 9.0f, 146.0f, 218.0f});
        }

        DrawRectangleLinesEx(r, hot ? 4.0f : 2.0f,
                             hot ? Color{255, 255, 255, 255} : Color{105, 145, 105, 255});
    }

    if (hoverGardener_ >= 0 && count > 0) {
        const int foe = (hoverGardener_ + 3) % count;
        drawText("你的对手将使用：" + (*gardeners_)[static_cast<std::size_t>(foe)].name, 120.0f,
                 806.0f, 24.0f, Color{230, 170, 170, 255});
    } else {
        drawText("ESC 退出", 120.0f, 806.0f, 20.0f, Color{150, 160, 150, 255});
    }
}

void Viewer::draw() {
    if (selecting_) {
        drawGardenerSelect();
        return;
    }
    if (match_->scene() == MatchScene::Combat) {
        drawCombatScene();
    } else {
        drawRecruitScene();
        drawDiscoverOverlay();
        drawDragGhost();
    }
}

void Viewer::drawShopButtons() {
    const GardenState& me = match_->player(0);
    const bool canAct = match_->awaitingPlayer() && me.discover.empty();

    bool pendingRefresh = false;
    bool pendingFreeze = false;

    ImGui::SetNextWindowPos(ImVec2(1152.0f, 549.0f));
    ImGui::SetNextWindowSize(ImVec2(164.0f, 28.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 3.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
    ImGui::Begin("##shopbtns", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

    ImGui::BeginDisabled(!canAct || me.sunlight < kRefreshCost);
    if (ImGui::Button("浇水 1", ImVec2(75.0f, 22.0f))) {
        pendingRefresh = true;
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(!canAct);
    if (ImGui::Button(me.shopFrozen ? "保鲜 ●" : "保鲜", ImVec2(75.0f, 22.0f))) {
        pendingFreeze = true;
    }
    ImGui::EndDisabled();

    ImGui::End();
    ImGui::PopStyleVar(2);

    if (pendingRefresh) {
        match_->playerAct(PlayerAction::Refresh, 0);
    }
    if (pendingFreeze) {
        match_->playerAct(PlayerAction::Freeze, 0);
    }
}

void Viewer::drawGoButton() {
    const bool canAct = match_->awaitingPlayer();
    ImGui::SetNextWindowPos(ImVec2(kOppX - 4.0f, 752.0f));
    ImGui::SetNextWindowSize(ImVec2(kOppW + 8.0f, 96.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
    ImGui::Begin("##go", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);
    ImGui::BeginDisabled(!canAct);
    if (ImGui::Button("出战  ▶", ImVec2(kOppW - 4.0f, 80.0f))) {
        startCombat();
    }
    ImGui::EndDisabled();
    ImGui::End();
    ImGui::PopStyleVar();
}

void Viewer::drawLogWindow() {
    if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) {
        showLog_ = !showLog_;
    }
    if (!showLog_) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(190.0f, 130.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1060.0f, 620.0f), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("战斗日志 (F1)", &showLog_)) {
        for (const std::string& line : match_->logLines()) {
            ImGui::TextUnformatted(line.c_str());
        }
        if (match_->scene() == MatchScene::Combat) {
            const std::vector<CombatFrame>& frames = match_->combatFrames();
            const int upto = std::min(frameIndex_, static_cast<int>(frames.size()));
            for (int i = 0; i < upto; ++i) {
                ImGui::TextUnformatted(frames[static_cast<std::size_t>(i)].line.c_str());
            }
        }
        if (!logScrollInit_ || ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 30.0f) {
            ImGui::SetScrollHereY(1.0f);
            logScrollInit_ = true;
        }
    }
    ImGui::End();
}

const Plant* Viewer::hoveredPlant() const {
    const GardenState& me = match_->player(0);
    const int i = hoverIndex_;
    if (i < 0) {
        return nullptr;
    }
    switch (hoverZone_) {
    case Zone::Board:
        return i < static_cast<int>(me.board.size()) ? &me.board[static_cast<std::size_t>(i)]
                                                     : nullptr;
    case Zone::Hand:
        return i < static_cast<int>(me.hand.size()) ? &me.hand[static_cast<std::size_t>(i)]
                                                    : nullptr;
    case Zone::Shop:
        return i < static_cast<int>(me.shop.size()) ? &me.shop[static_cast<std::size_t>(i)]
                                                    : nullptr;
    case Zone::Discover:
        return i < static_cast<int>(me.discover.size()) ? &me.discover[static_cast<std::size_t>(i)]
                                                        : nullptr;
    default:
        return nullptr;
    }
}

std::string Viewer::describeEffect(const Effect& e) const {
    if (e.action == EffectAction::Summon) {
        const std::string name = (*pool_)[static_cast<std::size_t>(e.summonTemplateId)].name;
        return "召唤 " + std::to_string(e.summonCount) + " 个" + name;
    }
    if (e.action == EffectAction::Damage) {
        return std::string("对") + selectorName(e.target) + "造成 " + std::to_string(e.damage) +
               " 点伤害";
    }
    return std::string(selectorName(e.target)) + " +" + std::to_string(e.attackBonus) + "/" +
           "+" + std::to_string(e.healthBonus);
}

std::vector<std::string> Viewer::abilityLines(const PlantTemplate& t) const {
    std::vector<std::string> out;
    const auto join = [&out, this](const char* label, const std::vector<Effect>& fx) {
        for (const Effect& e : fx) {
            out.push_back(std::string(label) + "：" + describeEffect(e));
        }
    };
    join("播种", t.battlecry);
    join("凋零", t.deathrattle);
    join("光合", t.aura);
    return out;
}

void Viewer::drawCardTooltip() {
    if (selecting_) {
        return;
    }

    if (match_->scene() != MatchScene::Combat) {
        if (hoveredSelf_) {
            drawGardenerTooltip(match_->humanIndex());
            return;
        }
        if (hoveredOpponent_ >= 0) {
            int slot = 0;
            int target = -1;
            for (int p = 0; p < match_->playerCount(); ++p) {
                if (p == match_->humanIndex()) {
                    continue;
                }
                if (slot == hoveredOpponent_) {
                    target = p;
                    break;
                }
                ++slot;
            }
            if (target >= 0) {
                drawGardenerTooltip(target);
                return;
            }
        }
    }

    const Plant* p = nullptr;
    int effA = 0;
    int effH = 0;

    if (match_->scene() == MatchScene::Combat) {
        if (combatHoverSide_ < 0) {
            return;
        }
        const std::vector<CombatFrame>& frames = match_->combatFrames();
        const int total = static_cast<int>(frames.size());
        if (total == 0) {
            return;
        }
        const int idx = std::max(0, std::min(frameIndex_, total) - 1);
        const CombatFrame& frame = frames[static_cast<std::size_t>(idx)];
        const std::vector<Plant>& board = combatHoverSide_ == 0 ? frame.left : frame.right;
        if (combatHoverIndex_ >= static_cast<int>(board.size())) {
            return;
        }
        p = &board[static_cast<std::size_t>(combatHoverIndex_)];
        effA = effectiveAttack(board, *pool_, combatHoverIndex_);
        effH = effectiveHealth(board, *pool_, combatHoverIndex_);
    } else {
        p = hoveredPlant();
        if (p == nullptr) {
            return;
        }
        effA = p->attack;
        effH = p->health;
        if (hoverZone_ == Zone::Board) {
            const GardenState& me = match_->player(0);
            effA = effectiveAttack(me.board, *pool_, hoverIndex_);
            effH = effectiveHealth(me.board, *pool_, hoverIndex_);
        }
    }

    const PlantTemplate& t = (*pool_)[static_cast<std::size_t>(p->templateId)];

    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(370.0f);

    ImGui::Text("%s%s   T%d   %s", t.name.c_str(), p->goldenLevel > 0 ? "★" : "", t.tier,
                t.race.c_str());
    if (effA != p->attack || effH != p->health) {
        ImGui::Text("%d/%d   （基础 %d/%d）", effA, effH, p->attack, p->health);
    } else {
        ImGui::Text("%d/%d", p->attack, p->health);
    }

    bool any = false;
    for (Keyword k : t.keywords) {
        ImGui::BulletText("%s", keywordDescription(k));
        any = true;
    }
    for (const std::string& line : abilityLines(t)) {
        ImGui::BulletText("%s", line.c_str());
        any = true;
    }
    if (!any) {
        ImGui::TextDisabled("无特殊能力");
    }

    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

void Viewer::drawImGui() {
    if (selecting_) {
        return;
    }
    drawLogWindow();
    drawCardTooltip();
    if (match_->scene() != MatchScene::Combat) {
        drawUpgradeButton();
        drawShopButtons();
        drawGoButton();
    }
}

void Viewer::run() {
    while (!WindowShouldClose()) {
        if (!selecting_ && !ImGui::GetIO().WantCaptureKeyboard && IsKeyPressed(KEY_R)) {
            restart();
        }

        if (selecting_) {
            handleGardenerSelect();
        } else if (match_->scene() == MatchScene::Combat) {
            const bool typing = ImGui::GetIO().WantCaptureKeyboard;
            if (!typing && IsKeyPressed(KEY_SPACE)) {
                skipCombat();
            }
            if (!typing && IsKeyPressed(KEY_S)) {
                cycleSpeed();
            }
            updateCombatPlayback();

            combatHoverSide_ = -1;
            combatHoverIndex_ = -1;
            const std::vector<CombatFrame>& frames = match_->combatFrames();
            const int total = static_cast<int>(frames.size());
            if (total > 0 && !ImGui::GetIO().WantCaptureMouse) {
                const int idx = std::max(0, std::min(frameIndex_, total) - 1);
                const CombatFrame& frame = frames[static_cast<std::size_t>(idx)];
                const Vector2 m = GetMousePosition();
                for (int side = 0; side < 2 && combatHoverSide_ < 0; ++side) {
                    const std::vector<Plant>& board = side == 0 ? frame.left : frame.right;
                    for (int i = 0; i < static_cast<int>(board.size()); ++i) {
                        if (CheckCollisionPointRec(
                                m, combatCard(side, i, static_cast<int>(board.size())))) {
                            combatHoverSide_ = side;
                            combatHoverIndex_ = i;
                            break;
                        }
                    }
                }
            }
        } else {
            handleInput();
        }

        BeginDrawing();
        ClearBackground(Color{12, 16, 12, 255});
        draw();
        rlImGuiBegin();
        drawImGui();
        rlImGuiEnd();

        ++frameCounter_;
        if (!shotPath_.empty() && frameCounter_ == 180) {
            TakeScreenshot(shotPath_.c_str());
            EndDrawing();
            break;
        }
        EndDrawing();
    }
}
