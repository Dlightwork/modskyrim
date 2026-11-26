#include "games.h"
#include "assets.h"

#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tg
{
    namespace
    {
        constexpr float kPi = 3.1415926535f;

        inline ImTextureID TexId(const std::string& name)
        {
            const Texture* t = AssetManager::Get().GetTexture(name);
            return t ? reinterpret_cast<ImTextureID>(t->srv) : static_cast<ImTextureID>(0);
        }

        void DrawTexOrRect(ImDrawList* dl, const std::string& name, ImVec2 p0, ImVec2 p1,
                           ImU32 fallbackCol, ImVec2 uv0 = ImVec2(0, 0), ImVec2 uv1 = ImVec2(1, 1),
                           ImU32 tint = IM_COL32(255, 255, 255, 255))
        {
            ImTextureID id = TexId(name);
            if (id) {
                dl->AddImage(id, p0, p1, uv0, uv1, tint);
            } else {
                dl->AddRectFilled(p0, p1, fallbackCol, 8.0f);
            }
        }

        void DrawDiceFace(ImDrawList* dl, int face, ImVec2 p0, ImVec2 p1, bool held)
        {
            int idx = std::clamp(face, 1, 6) - 1;
            int gx = idx % 3;
            int gy = idx / 3;
            ImVec2 uv0(gx / 3.0f, gy / 2.0f);
            ImVec2 uv1((gx + 1) / 3.0f, (gy + 1) / 2.0f);
            DrawTexOrRect(dl, "dice_atlas", p0, p1, IM_COL32(235, 225, 200, 255), uv0, uv1);
            if (held) {
                dl->AddRect(ImVec2(p0.x - 4, p0.y - 4), ImVec2(p1.x + 4, p1.y + 4),
                            IM_COL32(245, 205, 78, 255), 10.0f, 0, 4.0f);
            }
        }

        // Vector suit drawing (works on every Windows machine without font glyph issues)
        void DrawSuitIcon(ImDrawList* dl, int suit, ImVec2 c, float r, ImU32 col)
        {
            if (suit == 2) { // Diamond
                ImVec2 pts[4] = {
                    ImVec2(c.x, c.y - r),
                    ImVec2(c.x + r * 0.78f, c.y),
                    ImVec2(c.x, c.y + r),
                    ImVec2(c.x - r * 0.78f, c.y)
                };
                dl->AddConvexPolyFilled(pts, 4, col);
            } else if (suit == 1) { // Heart
                float cr = r * 0.48f;
                dl->AddCircleFilled(ImVec2(c.x - cr * 0.85f, c.y - cr * 0.35f), cr, col, 16);
                dl->AddCircleFilled(ImVec2(c.x + cr * 0.85f, c.y - cr * 0.35f), cr, col, 16);
                dl->AddTriangleFilled(
                    ImVec2(c.x - r * 0.88f, c.y - r * 0.05f),
                    ImVec2(c.x + r * 0.88f, c.y - r * 0.05f),
                    ImVec2(c.x, c.y + r * 0.98f), col);
            } else if (suit == 0) { // Spade
                float cr = r * 0.44f;
                dl->AddTriangleFilled(
                    ImVec2(c.x, c.y - r * 0.98f),
                    ImVec2(c.x - r * 0.85f, c.y + r * 0.15f),
                    ImVec2(c.x + r * 0.85f, c.y + r * 0.15f), col);
                dl->AddCircleFilled(ImVec2(c.x - cr * 0.85f, c.y + cr * 0.40f), cr, col, 16);
                dl->AddCircleFilled(ImVec2(c.x + cr * 0.85f, c.y + cr * 0.40f), cr, col, 16);
                dl->AddTriangleFilled(
                    ImVec2(c.x, c.y + r * 0.18f),
                    ImVec2(c.x - r * 0.38f, c.y + r * 0.98f),
                    ImVec2(c.x + r * 0.38f, c.y + r * 0.98f), col);
            } else { // Club
                float cr = r * 0.42f;
                dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.42f), cr, col, 16);
                dl->AddCircleFilled(ImVec2(c.x - r * 0.46f, c.y + r * 0.12f), cr, col, 16);
                dl->AddCircleFilled(ImVec2(c.x + r * 0.46f, c.y + r * 0.12f), cr, col, 16);
                dl->AddCircleFilled(ImVec2(c.x, c.y), cr * 0.6f, col, 12);
                dl->AddTriangleFilled(
                    ImVec2(c.x, c.y + r * 0.15f),
                    ImVec2(c.x - r * 0.36f, c.y + r * 0.98f),
                    ImVec2(c.x + r * 0.36f, c.y + r * 0.98f), col);
            }
        }

        void DrawPlayingCard(ImDrawList* dl, int rank, int suit, bool faceDown, ImVec2 pos, ImVec2 size)
        {
            ImVec2 p1(pos.x + size.x, pos.y + size.y);
            dl->AddRectFilled(ImVec2(pos.x + 6, pos.y + 7), ImVec2(p1.x + 6, p1.y + 7),
                              IM_COL32(0, 0, 0, 110), 10.0f);
            if (faceDown) {
                DrawTexOrRect(dl, "card_back", pos, p1, IM_COL32(110, 24, 28, 255));
                return;
            }
            DrawTexOrRect(dl, "card_front", pos, p1, IM_COL32(240, 228, 198, 255));

            static const char* kRanks[] = { "", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };
            bool isRed = (suit == 1 || suit == 2);
            ImU32 col = isRed ? IM_COL32(178, 26, 26, 255) : IM_COL32(28, 24, 22, 255);
            const char* rStr = kRanks[std::clamp(rank, 1, 13)];

            ImFont* font = ImGui::GetFont();
            float smallSz = size.y * 0.16f;

            // Top-left rank + mini suit
            dl->AddText(font, smallSz, ImVec2(pos.x + 12, pos.y + 8), col, rStr);
            DrawSuitIcon(dl, suit, ImVec2(pos.x + 21, pos.y + 12 + smallSz * 1.25f), smallSz * 0.38f, col);

            // Center large vector suit
            DrawSuitIcon(dl, suit, ImVec2(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f), size.y * 0.18f, col);

            // Bottom-right rank
            ImVec2 rSz = font->CalcTextSizeA(smallSz, FLT_MAX, 0.0f, rStr);
            dl->AddText(font, smallSz, ImVec2(p1.x - rSz.x - 12, p1.y - smallSz - 10), col, rStr);
        }

        const char* DifficultyName(Difficulty d)
        {
            switch (d) {
                case Difficulty::Novice:  return "Новичок (Трактирщик)";
                case Difficulty::Veteran: return "Бывалый (Ветеран)";
                case Difficulty::Master:  return "Мастер (Шулер)";
            }
            return "Бывалый";
        }
    }

    TavernGamesUI& TavernGamesUI::Get()
    {
        static TavernGamesUI inst;
        return inst;
    }

    TavernGamesUI::TavernGamesUI()
        : rng_(std::random_device{}())
    {
        InitDarts();
        InitCheckers();
        InitDicePoker();
        InitBlackjack();
        InitThimbles();
    }

    const char* TavernGamesUI::GetPlayerRank() const
    {
        int w = AssetManager::Get().profile.wins;
        if (w >= 40) return "Легенда таверн Скайрима";
        if (w >= 25) return "Мастер игрального стола";
        if (w >= 12) return "Хитрый шулер";
        if (w >= 5)  return "Удачливый завсегдатай";
        return "Залётный путник";
    }

    bool TavernGamesUI::TryPayStake(int amount)
    {
        auto& prof = AssetManager::Get().profile;
        if (amount <= 0) return true;
        if (prof.septims < amount) {
            bannerMsg_ = "Недостаточно септимов! Пополните кошелёк в Зале славы или снизьте ставку.";
            bannerTimer_ = 3.0f;
            AssetManager::Get().PlaySfx("lose_thud");
            return false;
        }
        prof.septims -= amount;
        AssetManager::Get().SaveProfile();
        return true;
    }

    void TavernGamesUI::RecordResult(int outcome, int payout, const std::string& bannerText)
    {
        auto& prof = AssetManager::Get().profile;
        prof.totalGames++;
        if (outcome > 0) {
            prof.wins++;
            prof.septims += payout;
            prof.netProfit += (payout - currentStake_);
            AssetManager::Get().PlaySfx("coin_win");
        } else if (outcome < 0) {
            prof.losses++;
            prof.netProfit -= currentStake_;
            AssetManager::Get().PlaySfx("lose_thud");
        } else {
            prof.draws++;
            prof.septims += payout;
            AssetManager::Get().PlaySfx("wood_clack");
        }
        AssetManager::Get().SaveProfile();
        bannerMsg_ = bannerText;
        bannerTimer_ = 3.5f;
    }

    void TavernGamesUI::Render(float dt)
    {
        if (bannerTimer_ > 0.0f) bannerTimer_ = std::max(0.0f, bannerTimer_ - dt);

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 disp = io.DisplaySize;
        float winW = std::min(1160.0f, disp.x * 0.90f);
        float winH = std::min(760.0f, disp.y * 0.90f);
        ImVec2 winPos((disp.x - winW) * 0.5f, (disp.y - winH) * 0.5f);

        ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(0, 0), disp, IM_COL32(8, 5, 3, 165));

        ImGui::SetNextWindowPos(winPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 14));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.07f, 0.04f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.76f, 0.59f, 0.25f, 0.95f));

        if (ImGui::Begin("##TavernGamesVisualMain", nullptr, flags)) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 p0 = ImGui::GetWindowPos();
            ImVec2 p1(p0.x + winW, p0.y + winH);

            DrawTexOrRect(dl, "wood_table", p0, p1, IM_COL32(48, 30, 18, 255));
            dl->AddRect(ImVec2(p0.x + 4, p0.y + 4), ImVec2(p1.x - 4, p1.y - 4),
                        IM_COL32(196, 154, 68, 200), 10.0f, 0, 2.5f);

            RenderTopBar();

            if (bannerTimer_ > 0.0f && !bannerMsg_.empty()) {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.18f, 0.12f, 0.05f, 0.92f));
                ImGui::BeginChild("##Banner", ImVec2(-1, 34), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
                ImGui::SetCursorPosX(std::max(10.0f, (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(bannerMsg_.c_str()).x) * 0.5f));
                ImGui::TextColored(ImVec4(1.0f, 0.87f, 0.42f, 1.0f), "%s", bannerMsg_.c_str());
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }

            switch (screen_) {
                case Screen::Lobby:      RenderLobby(); break;
                case Screen::Darts:      RenderDarts(dt); break;
                case Screen::Checkers:   RenderCheckers(dt); break;
                case Screen::DicePoker:  RenderDicePoker(dt); break;
                case Screen::Blackjack:  RenderBlackjack(dt); break;
                case Screen::Thimbles:   RenderThimbles(dt); break;
                case Screen::CoinFlip:   RenderCoinFlip(dt); break;
                case Screen::HallOfFame: RenderHallOfFame(); break;
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

    void TavernGamesUI::RenderTopBar()
    {
        auto& am = AssetManager::Get();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.05f, 0.03f, 0.86f));
        ImGui::BeginChild("##TopBar", ImVec2(-1, 56), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);

        if (screen_ != Screen::Lobby) {
            if (ImGui::Button("<< К столу игр", ImVec2(145, 36))) {
                screen_ = Screen::Lobby;
                am.PlaySfx("wood_clack");
            }
            ImGui::SameLine(0, 14);
        }

        ImGui::SetCursorPosY(14);
        ImGui::TextColored(ImVec4(0.95f, 0.80f, 0.42f, 1.0f), "ТАВЕРННЫЕ ИГРЫ СКАЙРИМА");
        ImGui::SameLine(0, 22);

        ImTextureID coinId = TexId("coin_septim");
        if (coinId) {
            ImGui::SetCursorPosY(11);
            ImGui::Image(coinId, ImVec2(30, 30));
            ImGui::SameLine(0, 6);
        }
        ImGui::SetCursorPosY(14);
        ImGui::TextColored(ImVec4(1.0f, 0.88f, 0.35f, 1.0f), "%d септимов", am.profile.septims);

        float rightX = ImGui::GetWindowWidth() - 340.0f;
        ImGui::SameLine(std::max(ImGui::GetCursorPosX() + 10.0f, rightX));
        ImGui::SetCursorPosY(9);

        if (ImGui::Button(am.config.soundEnabled ? "Звук: ВКЛ" : "Звук: ВЫКЛ", ImVec2(105, 36))) {
            am.config.soundEnabled = !am.config.soundEnabled;
            am.SaveConfig();
        }
        ImGui::SameLine(0, 8);
        if (ImGui::Button("Зал славы", ImVec2(105, 36))) {
            screen_ = Screen::HallOfFame;
            am.PlaySfx("wood_clack");
        }
        ImGui::SameLine(0, 8);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.52f, 0.14f, 0.12f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.68f, 0.20f, 0.16f, 1.0f));
        if (ImGui::Button("Выйти [Esc]", ImVec2(105, 36))) {
            wantsClose = true;
        }
        ImGui::PopStyleColor(2);

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();
    }

    void TavernGamesUI::RenderLobby()
    {
        auto& am = AssetManager::Get();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.06f, 0.04f, 0.82f));
        ImGui::BeginChild("##BetStrip", ImVec2(-1, 56), ImGuiChildFlags_Borders, ImGuiWindowFlags_NoScrollbar);
        ImGui::SetCursorPosY(15);
        ImGui::TextColored(ImVec4(0.92f, 0.84f, 0.68f, 1.0f), "Ставка:");
        ImGui::SameLine(0, 10);
        ImGui::SetCursorPosY(10);
        static const int kStakes[] = { 10, 25, 50, 100, 250 };
        for (int s : kStakes) {
            bool active = (currentStake_ == s);
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.52f, 0.14f, 1.0f));
            char label[32];
            std::snprintf(label, sizeof(label), "%d зол.", s);
            if (ImGui::Button(label, ImVec2(76, 34))) {
                currentStake_ = s;
                am.PlaySfx("wood_clack");
            }
            if (active) ImGui::PopStyleColor();
            ImGui::SameLine(0, 6);
        }

        ImGui::SameLine(0, 24);
        ImGui::SetCursorPosY(15);
        ImGui::TextColored(ImVec4(0.92f, 0.84f, 0.68f, 1.0f), "Соперник:");
        ImGui::SameLine(0, 10);
        ImGui::SetCursorPosY(10);
        for (int d = 0; d < 3; ++d) {
            bool active = (static_cast<int>(difficulty_) == d);
            if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.58f, 0.20f, 0.16f, 1.0f));
            const char* names[3] = { "Новичок", "Бывалый", "Мастер" };
            if (ImGui::Button(names[d], ImVec2(92, 34))) {
                difficulty_ = static_cast<Difficulty>(d);
                am.PlaySfx("wood_clack");
            }
            if (active) ImGui::PopStyleColor();
            ImGui::SameLine(0, 6);
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::Spacing();

        struct GameCardInfo {
            const char* title;
            const char* subtitle;
            const char* tex;
            Screen target;
            bool isDiceAtlas;
        };
        static const GameCardInfo kCards[6] = {
            { "ДАРТС 301",       "Прицел, сила броска, x2/x3 и Буллзай",    "dartboard",      Screen::Darts,     false },
            { "ШАШКИ 8x8",       "Резная доска, дамки и умный соперник",    "checkers_board", Screen::Checkers,  false },
            { "ПОКЕР НА КОСТЯХ", "5 костей, перебросы и комбинации",        "dice_atlas",     Screen::DicePoker, true  },
            { "БЛЭКДЖЕК 21",     "Карты на красном сукне, удвоение ставки", "card_back",      Screen::Blackjack, false },
            { "НАПЁРСТКИ",       "3 кубка, следи за золотым септимом",      "cup_thimble",    Screen::Thimbles,  false },
            { "СЕПТИМ (МОНЕТА)", "Бросок имперской монеты на удачу",        "coin_septim",    Screen::CoinFlip,  false },
        };

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float gap = 16.0f;
        float cardW = (avail.x - gap * 2.0f) / 3.0f;
        float cardH = (avail.y - gap - 26.0f) / 2.0f;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 basePos = ImGui::GetCursorScreenPos();

        for (int i = 0; i < 6; ++i) {
            int col = i % 3;
            int row = i / 3;
            ImVec2 p0(basePos.x + col * (cardW + gap), basePos.y + row * (cardH + gap));
            ImVec2 p1(p0.x + cardW, p0.y + cardH);

            ImGui::SetCursorScreenPos(p0);
            char idBuf[32];
            std::snprintf(idBuf, sizeof(idBuf), "##GameTile%d", i);
            bool clicked = ImGui::InvisibleButton(idBuf, ImVec2(cardW, cardH));
            bool hovered = ImGui::IsItemHovered();

            DrawTexOrRect(dl, "red_velvet", p0, p1, IM_COL32(82, 18, 22, 255),
                          ImVec2(0, 0), ImVec2(1, 1),
                          hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(215, 210, 205, 240));

            dl->AddRect(p0, p1,
                        hovered ? IM_COL32(255, 216, 98, 255) : IM_COL32(168, 132, 62, 210),
                        10.0f, 0, hovered ? 3.5f : 2.0f);

            float imgSize = std::min(cardW * 0.46f, cardH * 0.56f);
            float imgW = (kCards[i].target == Screen::Blackjack) ? imgSize * 0.72f : imgSize;
            float imgH = imgSize;
            ImVec2 ip0(p0.x + (cardW - imgW) * 0.5f, p0.y + 18.0f);
            ImVec2 ip1(ip0.x + imgW, ip0.y + imgH);
            if (kCards[i].isDiceAtlas) {
                DrawDiceFace(dl, 6, ip0, ip1, false);
            } else {
                DrawTexOrRect(dl, kCards[i].tex, ip0, ip1, IM_COL32(180, 140, 60, 255));
            }

            ImVec2 band0(p0.x + 2, p1.y - 62);
            ImVec2 band1(p1.x - 2, p1.y - 2);
            dl->AddRectFilled(band0, band1, IM_COL32(16, 10, 6, 215), 8.0f, ImDrawFlags_RoundCornersBottom);

            ImFont* f = ImGui::GetFont();
            ImVec2 tSz = f->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, kCards[i].title);
            dl->AddText(f, 20.0f, ImVec2(p0.x + (cardW - tSz.x) * 0.5f, band0.y + 8),
                        hovered ? IM_COL32(255, 224, 118, 255) : IM_COL32(236, 204, 128, 255),
                        kCards[i].title);

            ImVec2 sSz = f->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, kCards[i].subtitle);
            dl->AddText(f, 14.0f, ImVec2(p0.x + (cardW - sSz.x) * 0.5f, band0.y + 34),
                        IM_COL32(210, 198, 178, 235), kCards[i].subtitle);

            if (clicked) {
                am.PlaySfx("wood_clack");
                screen_ = kCards[i].target;
                if (screen_ == Screen::Darts)     InitDarts();
                if (screen_ == Screen::Checkers)  InitCheckers();
                if (screen_ == Screen::DicePoker) InitDicePoker();
                if (screen_ == Screen::Blackjack) InitBlackjack();
                if (screen_ == Screen::Thimbles)  InitThimbles();
            }
        }

        ImGui::SetCursorScreenPos(ImVec2(basePos.x, basePos.y + 2 * cardH + gap + 4));
        ImGui::TextColored(ImVec4(0.84f, 0.78f, 0.65f, 0.9f),
            "Звание: %s   |   Побед: %d   Поражений: %d   |   Клавиша F8 — открыть/закрыть стол в любой момент",
            GetPlayerRank(), am.profile.wins, am.profile.losses);
    }

    // ==================== 1. DARTS 301 ====================
    void TavernGamesUI::InitDarts()
    {
        dartsPlayerScore_ = 301;
        dartsAiScore_ = 301;
        dartsLeft_ = 3;
        dartsRoundTotal_ = 0;
        dartsPlayerTurn_ = true;
        dartsMatchOver_ = false;
        dartsCharging_ = false;
        dartsCharge_ = 0.0f;
        dartsHits_.clear();
        dartsStatus_ = "Наведите на мишень, зажмите ЛКМ для броска и отпустите! (Ставка: " + std::to_string(currentStake_) + " зол.)";
    }

    void TavernGamesUI::EvaluateDart(float nx, float ny, int& outScore, std::string& outLabel) const
    {
        float dist = std::sqrt(nx * nx + ny * ny);
        const float rBull   = 24.0f / 512.0f;
        const float rOBull  = 56.0f / 512.0f;
        const float rTrpIn  = 212.0f / 512.0f;
        const float rTrpOut = 240.0f / 512.0f;
        const float rDblIn  = 355.0f / 512.0f;
        const float rDblOut = 385.0f / 512.0f;

        if (dist <= rBull)  { outScore = 50; outLabel = "ЯБЛОЧКО (50)"; return; }
        if (dist <= rOBull) { outScore = 25; outLabel = "БУЛЛ (25)"; return; }
        if (dist > rDblOut) { outScore = 0;  outLabel = "МИМО (0)"; return; }

        static const int kSectors[20] = { 20,1,18,4,13,6,10,15,2,17,3,19,7,16,8,11,14,9,12,5 };
        float ang = std::atan2(nx, -ny) * 180.0f / kPi;
        if (ang < 0.0f) ang += 360.0f;
        int idx = static_cast<int>(std::fmod(ang + 9.0f, 360.0f) / 18.0f) % 20;
        int base = kSectors[idx];

        if (dist >= rDblIn && dist <= rDblOut) {
            outScore = base * 2;
            outLabel = "x2 " + std::to_string(base) + " (" + std::to_string(outScore) + ")";
        } else if (dist >= rTrpIn && dist <= rTrpOut) {
            outScore = base * 3;
            outLabel = "x3 " + std::to_string(base) + " (" + std::to_string(outScore) + ")";
        } else {
            outScore = base;
            outLabel = std::to_string(base);
        }
    }

    void TavernGamesUI::RenderDarts(float dt)
    {
        dartsSwayTime_ += dt;
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float boardSz = std::min(avail.y - 16.0f, avail.x * 0.56f);

        ImGui::BeginGroup();
        ImVec2 bPos = ImGui::GetCursorScreenPos();
        ImVec2 bEnd(bPos.x + boardSz, bPos.y + boardSz);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        ImGui::InvisibleButton("##DartboardCanvas", ImVec2(boardSz, boardSz));
        bool hovered = ImGui::IsItemHovered();

        DrawTexOrRect(dl, "dartboard", bPos, bEnd, IM_COL32(30, 26, 24, 255));

        ImVec2 center(bPos.x + boardSz * 0.5f, bPos.y + boardSz * 0.5f);
        float half = boardSz * 0.5f;

        for (const auto& h : dartsHits_) {
            ImVec2 dp(center.x + h.nx * half, center.y + h.ny * half);
            ImU32 col = h.isPlayer ? IM_COL32(240, 72, 62, 255) : IM_COL32(72, 165, 245, 255);
            dl->AddCircleFilled(ImVec2(dp.x + 3, dp.y + 4), 7.0f, IM_COL32(0, 0, 0, 140));
            dl->AddLine(dp, ImVec2(dp.x + 14, dp.y - 18), IM_COL32(215, 200, 175, 255), 3.0f);
            dl->AddTriangleFilled(
                ImVec2(dp.x + 10, dp.y - 14),
                ImVec2(dp.x + 22, dp.y - 16),
                ImVec2(dp.x + 16, dp.y - 26), col);
            dl->AddCircleFilled(dp, 4.5f, IM_COL32(255, 245, 200, 255));
            dl->AddCircle(dp, 5.5f, col, 0, 2.0f);
        }

        if (dartsPlayerTurn_ && !dartsMatchOver_ && hovered) {
            ImVec2 m = ImGui::GetIO().MousePos;
            float swayAmp = dartsCharging_ ? (0.045f + 0.055f * std::abs(dartsCharge_ - 0.5f) * 2.0f) : 0.055f;
            float sx = std::sin(dartsSwayTime_ * 2.7f) * swayAmp * half;
            float sy = std::cos(dartsSwayTime_ * 2.1f) * swayAmp * half;
            ImVec2 aim(m.x + sx, m.y + sy);

            float ringR = 16.0f + (dartsCharging_ ? (1.0f - std::sin(dartsCharge_ * kPi)) * 18.0f : 8.0f);
            ImU32 aimCol = dartsCharging_ ? IM_COL32(255, 215, 65, 240) : IM_COL32(255, 255, 255, 200);
            dl->AddCircle(aim, ringR, aimCol, 24, 2.2f);
            dl->AddLine(ImVec2(aim.x - 10, aim.y), ImVec2(aim.x + 10, aim.y), aimCol, 1.5f);
            dl->AddLine(ImVec2(aim.x, aim.y - 10), ImVec2(aim.x, aim.y + 10), aimCol, 1.5f);

            if (ImGui::IsMouseClicked(0)) {
                if (dartsPlayerScore_ == 301 && dartsAiScore_ == 301 && dartsHits_.empty()) {
                    if (!TryPayStake(currentStake_)) { ImGui::EndGroup(); return; }
                }
                dartsCharging_ = true;
                dartsCharge_ = 0.0f;
            }
            if (dartsCharging_) {
                dartsCharge_ = std::fmod(dartsCharge_ + dt * 1.45f, 1.0f);
                if (ImGui::IsMouseReleased(0)) {
                    dartsCharging_ = false;
                    float err = std::abs(dartsCharge_ - 0.5f) * 0.22f;
                    std::uniform_real_distribution<float> jitter(-err, err);
                    float nx = (aim.x - center.x) / half + jitter(rng_);
                    float ny = (aim.y - center.y) / half + jitter(rng_);
                    int pts = 0;
                    std::string lbl;
                    EvaluateDart(nx, ny, pts, lbl);
                    AssetManager::Get().PlaySfx("dart_hit");
                    dartsHits_.push_back({ nx, ny, true, pts, lbl });
                    dartsRoundTotal_ += pts;

                    if (dartsPlayerScore_ - pts == 0) {
                        dartsPlayerScore_ = 0;
                        dartsMatchOver_ = true;
                        auto& prof = AssetManager::Get().profile;
                        if (dartsRoundTotal_ > prof.bestDartRound) prof.bestDartRound = dartsRoundTotal_;
                        RecordResult(1, currentStake_ * 2, "Победа в дартс! Выигрыш: +" + std::to_string(currentStake_) + " септимов!");
                        dartsStatus_ = "Вы закрыли 301 точным броском (" + lbl + ")!";
                    } else if (dartsPlayerScore_ - pts < 0) {
                        dartsStatus_ = "Перебор (" + lbl + ")! Очки не засчитаны, ход переходит трактирщику.";
                        dartsLeft_ = 0;
                    } else {
                        dartsPlayerScore_ -= pts;
                        dartsLeft_--;
                        dartsStatus_ = "Попадание: " + lbl + "! Осталось дротиков: " + std::to_string(dartsLeft_);
                    }

                    if (!dartsMatchOver_ && dartsLeft_ <= 0) {
                        auto& prof = AssetManager::Get().profile;
                        if (dartsRoundTotal_ > prof.bestDartRound) {
                            prof.bestDartRound = dartsRoundTotal_;
                            AssetManager::Get().SaveProfile();
                        }
                        dartsPlayerTurn_ = false;
                        dartsLeft_ = 3;
                        dartsRoundTotal_ = 0;
                        dartsAiTimer_ = 0.75f;
                    }
                }
            }
        }
        ImGui::EndGroup();

        if (!dartsPlayerTurn_ && !dartsMatchOver_) {
            dartsAiTimer_ -= dt;
            if (dartsAiTimer_ <= 0.0f) {
                float spread = (difficulty_ == Difficulty::Novice) ? 0.24f :
                               (difficulty_ == Difficulty::Veteran) ? 0.14f : 0.075f;
                float tx = 0.0f;
                float ty = (dartsAiScore_ <= 50) ? 0.0f : -0.44f;
                std::normal_distribution<float> distN(0.0f, spread);
                float nx = tx + distN(rng_);
                float ny = ty + distN(rng_);
                int pts = 0;
                std::string lbl;
                EvaluateDart(nx, ny, pts, lbl);
                AssetManager::Get().PlaySfx("dart_hit");
                dartsHits_.push_back({ nx, ny, false, pts, lbl });
                if (dartsHits_.size() > 9) dartsHits_.erase(dartsHits_.begin());

                if (dartsAiScore_ - pts == 0) {
                    dartsAiScore_ = 0;
                    dartsMatchOver_ = true;
                    RecordResult(-1, 0, "Трактирщик закрыл 301 первым! Вы потеряли " + std::to_string(currentStake_) + " септимов.");
                    dartsStatus_ = "Соперник попал в " + lbl + " и выиграл партию.";
                } else if (dartsAiScore_ - pts > 0) {
                    dartsAiScore_ -= pts;
                    dartsStatus_ = "Трактирщик попал: " + lbl;
                } else {
                    dartsStatus_ = "У трактирщика перебор!";
                }
                dartsLeft_--;
                if (!dartsMatchOver_ && dartsLeft_ <= 0) {
                    dartsPlayerTurn_ = true;
                    dartsLeft_ = 3;
                    dartsRoundTotal_ = 0;
                    dartsHits_.clear();
                    dartsStatus_ = "Ваш ход! 3 дротика. Цельтесь в сектор и зажимайте ЛКМ.";
                } else {
                    dartsAiTimer_ = 0.75f;
                }
            }
        }

        ImGui::SameLine(0, 18);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.06f, 0.04f, 0.88f));
        ImGui::BeginChild("##DartsPanel", ImVec2(-1, boardSz), ImGuiChildFlags_Borders);
        ImGui::TextColored(ImVec4(0.98f, 0.82f, 0.40f, 1.0f), "ДАРТС 301 (%s)", DifficultyName(difficulty_));
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.40f, 1.0f), "Вы (красные дротики): %d", dartsPlayerScore_);
        ImGui::TextColored(ImVec4(0.45f, 0.75f, 1.0f, 1.0f), "Соперник (синие): %d", dartsAiScore_);
        ImGui::Spacing();
        ImGui::Text("Ход: %s   |   Дротиков: %d", dartsPlayerTurn_ ? "ВАШ" : "СОПЕРНИК", dartsLeft_);
        ImGui::Spacing();

        ImGui::Text("Шкала точности (отпустите в центре!):");
        ImVec2 g0 = ImGui::GetCursorScreenPos();
        float gw = ImGui::GetContentRegionAvail().x;
        ImVec2 g1(g0.x + gw, g0.y + 26.0f);
        ImDrawList* pdl = ImGui::GetWindowDrawList();
        pdl->AddRectFilled(g0, g1, IM_COL32(42, 24, 18, 255), 6.0f);
        pdl->AddRectFilled(ImVec2(g0.x + gw * 0.38f, g0.y + 2), ImVec2(g0.x + gw * 0.62f, g1.y - 2),
                           IM_COL32(46, 155, 68, 220), 4.0f);
        float markerX = g0.x + gw * (dartsCharging_ ? dartsCharge_ : 0.5f);
        pdl->AddRectFilled(ImVec2(markerX - 4, g0.y - 2), ImVec2(markerX + 4, g1.y + 2),
                           IM_COL32(255, 225, 85, 255), 3.0f);
        ImGui::Dummy(ImVec2(gw, 34));

        ImGui::TextWrapped("%s", dartsStatus_.c_str());
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.85f, 0.78f, 0.62f, 1.0f), "Последние броски:");
        for (auto it = dartsHits_.rbegin(); it != dartsHits_.rend(); ++it) {
            ImGui::TextColored(it->isPlayer ? ImVec4(1.0f, 0.65f, 0.60f, 1.0f) : ImVec4(0.65f, 0.85f, 1.0f, 1.0f),
                "%s: %s", it->isPlayer ? "Вы" : "Соперник", it->label.c_str());
        }

        ImGui::SetCursorPosY(boardSz - 52);
        if (ImGui::Button("Новая партия", ImVec2(-1, 38))) {
            InitDarts();
            AssetManager::Get().PlaySfx("wood_clack");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    // ==================== 2. CHECKERS 8x8 ====================
    void TavernGamesUI::InitCheckers()
    {
        std::memset(board_, 0, sizeof(board_));
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 8; ++c)
                if ((r + c) % 2 == 1) board_[r][c] = -1;
        for (int r = 5; r < 8; ++r)
            for (int c = 0; c < 8; ++c)
                if ((r + c) % 2 == 1) board_[r][c] = 1;
        selR_ = selC_ = -1;
        chainR_ = chainC_ = -1;
        checkersPlayerTurn_ = true;
        checkersOver_ = false;
        checkersAiDelay_ = 0.0f;
        checkersStatus_ = "Ваш ход (белые). Кликните по шашке, затем по подсвеченной клетке.";
    }

    std::vector<TavernGamesUI::Move> TavernGamesUI::GetValidMoves(int side, int forcedR, int forcedC) const
    {
        std::vector<Move> captures, quiet;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (forcedR >= 0 && (r != forcedR || c != forcedC)) continue;
                int p = board_[r][c];
                if (p == 0 || (p > 0 ? 1 : -1) != side) continue;
                bool isKing = (std::abs(p) == 2);
                int drs[4] = { -1, -1, 1, 1 };
                int dcs[4] = { -1,  1,-1, 1 };
                for (int d = 0; d < 4; ++d) {
                    int dr = drs[d], dc = dcs[d];
                    int mr = r + dr, mc = c + dc;
                    int jr = r + 2 * dr, jc = c + 2 * dc;
                    if (jr >= 0 && jr < 8 && jc >= 0 && jc < 8) {
                        int mid = board_[mr][mc];
                        if (mid != 0 && (mid > 0 ? 1 : -1) == -side && board_[jr][jc] == 0) {
                            captures.push_back({ r, c, jr, jc, true, mr, mc });
                        }
                    }
                    if (forcedR < 0) {
                        if (!isKing && ((side == 1 && dr > 0) || (side == -1 && dr < 0))) continue;
                        if (mr >= 0 && mr < 8 && mc >= 0 && mc < 8 && board_[mr][mc] == 0) {
                            quiet.push_back({ r, c, mr, mc, false, -1, -1 });
                        }
                    }
                }
            }
        }
        return !captures.empty() ? captures : quiet;
    }

    void TavernGamesUI::ApplyCheckerMove(const Move& m)
    {
        int p = board_[m.r0][m.c0];
        board_[m.r0][m.c0] = 0;
        if (m.isCapture) board_[m.capR][m.capC] = 0;
        if (p == 1 && m.r1 == 0) p = 2;
        if (p == -1 && m.r1 == 7) p = -2;
        board_[m.r1][m.c1] = p;
        AssetManager::Get().PlaySfx("wood_clack");
    }

    void TavernGamesUI::DoAiCheckerMove()
    {
        auto moves = GetValidMoves(-1, chainR_, chainC_);
        if (moves.empty()) {
            checkersOver_ = true;
            RecordResult(1, currentStake_ * 2, "Победа в шашки! Соперник заблокирован! +" + std::to_string(currentStake_) + " зол.");
            checkersStatus_ = "Вы победили!";
            return;
        }
        Move best = moves[0];
        int bestScore = -9999;
        for (const auto& m : moves) {
            int sc = 0;
            if (m.isCapture) sc += 50;
            if (m.r1 == 7) sc += 30;
            sc += m.r1 * 2;
            if (m.c1 == 0 || m.c1 == 7) sc += 4;
            int noiseRange = (difficulty_ == Difficulty::Novice) ? 45 :
                             (difficulty_ == Difficulty::Veteran) ? 16 : 3;
            std::uniform_int_distribution<int> rnd(-noiseRange, noiseRange);
            sc += rnd(rng_);
            if (sc > bestScore) { bestScore = sc; best = m; }
        }
        ApplyCheckerMove(best);
        if (best.isCapture) {
            auto nextCaps = GetValidMoves(-1, best.r1, best.c1);
            if (!nextCaps.empty() && nextCaps[0].isCapture) {
                chainR_ = best.r1;
                chainC_ = best.c1;
                checkersAiDelay_ = 0.45f;
                return;
            }
        }
        chainR_ = chainC_ = -1;
        checkersPlayerTurn_ = true;

        auto pMoves = GetValidMoves(1);
        if (pMoves.empty()) {
            checkersOver_ = true;
            RecordResult(-1, 0, "Поражение в шашки! У вас не осталось ходов.");
            checkersStatus_ = "Трактирщик выиграл партию.";
        } else {
            checkersStatus_ = pMoves[0].isCapture ? "Ваш ход! Есть обязательное взятие!" : "Ваш ход (белые).";
        }
    }

    void TavernGamesUI::RenderCheckers(float dt)
    {
        if (!checkersPlayerTurn_ && !checkersOver_) {
            checkersAiDelay_ -= dt;
            if (checkersAiDelay_ <= 0.0f) DoAiCheckerMove();
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float boardSz = std::min(avail.y - 16.0f, avail.x * 0.58f);
        float borderPx = boardSz * (64.0f / 1024.0f);
        float sq = (boardSz - 2.0f * borderPx) / 8.0f;

        ImGui::BeginGroup();
        ImVec2 b0 = ImGui::GetCursorScreenPos();
        ImVec2 b1(b0.x + boardSz, b0.y + boardSz);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        ImGui::InvisibleButton("##CheckersBoard", ImVec2(boardSz, boardSz));
        bool clicked = ImGui::IsItemClicked(0);

        DrawTexOrRect(dl, "checkers_board", b0, b1, IM_COL32(72, 44, 24, 255));

        auto validMoves = (checkersPlayerTurn_ && !checkersOver_) ? GetValidMoves(1, chainR_, chainC_) : std::vector<Move>{};

        if (selR_ >= 0 && selC_ >= 0) {
            ImVec2 sp0(b0.x + borderPx + selC_ * sq, b0.y + borderPx + selR_ * sq);
            dl->AddRect(sp0, ImVec2(sp0.x + sq, sp0.y + sq), IM_COL32(255, 220, 72, 255), 0, 0, 4.0f);
            for (const auto& m : validMoves) {
                if (m.r0 == selR_ && m.c0 == selC_) {
                    ImVec2 dp0(b0.x + borderPx + m.c1 * sq, b0.y + borderPx + m.r1 * sq);
                    ImU32 hl = m.isCapture ? IM_COL32(235, 72, 52, 130) : IM_COL32(72, 215, 96, 120);
                    dl->AddRectFilled(dp0, ImVec2(dp0.x + sq, dp0.y + sq), hl);
                    dl->AddCircleFilled(ImVec2(dp0.x + sq * 0.5f, dp0.y + sq * 0.5f), sq * 0.16f,
                                        m.isCapture ? IM_COL32(255, 195, 65, 230) : IM_COL32(165, 255, 175, 220));
                }
            }
        }

        int whiteCnt = 0, blackCnt = 0;
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                int p = board_[r][c];
                if (p == 0) continue;
                if (p > 0) whiteCnt++; else blackCnt++;
                ImVec2 cp0(b0.x + borderPx + c * sq + sq * 0.06f, b0.y + borderPx + r * sq + sq * 0.06f);
                ImVec2 cp1(cp0.x + sq * 0.88f, cp0.y + sq * 0.88f);
                const char* tex = (p == 1) ? "piece_white" : (p == 2) ? "piece_white_king" :
                                  (p == -1) ? "piece_black" : "piece_black_king";
                DrawTexOrRect(dl, tex, cp0, cp1, (p > 0) ? IM_COL32(235, 215, 175, 255) : IM_COL32(45, 40, 38, 255));
            }
        }

        if (clicked && checkersPlayerTurn_ && !checkersOver_) {
            ImVec2 mp = ImGui::GetIO().MousePos;
            int cc = static_cast<int>((mp.x - (b0.x + borderPx)) / sq);
            int rr = static_cast<int>((mp.y - (b0.y + borderPx)) / sq);
            if (rr >= 0 && rr < 8 && cc >= 0 && cc < 8) {
                bool moved = false;
                for (const auto& m : validMoves) {
                    if (m.r0 == selR_ && m.c0 == selC_ && m.r1 == rr && m.c1 == cc) {
                        if (whiteCnt == 12 && blackCnt == 12) {
                            if (!TryPayStake(currentStake_)) { ImGui::EndGroup(); return; }
                        }
                        ApplyCheckerMove(m);
                        moved = true;
                        if (m.isCapture) {
                            auto more = GetValidMoves(1, m.r1, m.c1);
                            if (!more.empty() && more[0].isCapture) {
                                selR_ = chainR_ = m.r1;
                                selC_ = chainC_ = m.c1;
                                checkersStatus_ = "Серия взятий! Продолжайте бить этой же шашкой!";
                                break;
                            }
                        }
                        selR_ = selC_ = chainR_ = chainC_ = -1;
                        checkersPlayerTurn_ = false;
                        checkersAiDelay_ = 0.55f;
                        checkersStatus_ = "Трактирщик обдумывает ход...";
                        break;
                    }
                }
                if (!moved && board_[rr][cc] > 0) {
                    for (const auto& m : validMoves) {
                        if (m.r0 == rr && m.c0 == cc) {
                            selR_ = rr; selC_ = cc;
                            break;
                        }
                    }
                }
            }
        }
        ImGui::EndGroup();

        ImGui::SameLine(0, 18);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.06f, 0.04f, 0.88f));
        ImGui::BeginChild("##CheckersPanel", ImVec2(-1, boardSz), ImGuiChildFlags_Borders);
        ImGui::TextColored(ImVec4(0.98f, 0.82f, 0.40f, 1.0f), "ШАШКИ 8x8 (%s)", DifficultyName(difficulty_));
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Ставка: %d септимов", currentStake_);
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.96f, 0.88f, 0.72f, 1.0f), "Ваши шашки (белые): %d", whiteCnt);
        ImGui::TextColored(ImVec4(0.76f, 0.72f, 0.68f, 1.0f), "Шашки соперника (чёрные): %d", blackCnt);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextWrapped("%s", checkersStatus_.c_str());
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.75f, 0.70f, 0.60f, 0.9f),
            "Подсказка:\n- Взятие обязательно (подсвечено красным)\n- Дойдя до края, шашка становится дамкой\n- Можно бить цепочкой за один ход");

        ImGui::SetCursorPosY(boardSz - 52);
        if (ImGui::Button("Начать новую партию", ImVec2(-1, 38))) {
            InitCheckers();
            AssetManager::Get().PlaySfx("wood_clack");
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    // ==================== 3. DICE POKER ====================
    void TavernGamesUI::InitDicePoker()
    {
        diceRoundActive_ = false;
        diceShowAi_ = false;
        diceRollsLeft_ = 2;
        diceAnimTimer_ = 0.0f;
        pHold_.fill(false);
        diceStatus_ = "Нажмите «Бросить кости», чтобы сделать ставку и начать раунд.";
    }

    TavernGamesUI::HandRank TavernGamesUI::EvaluateDice(const std::array<int, 5>& d) const
    {
        int cnt[7]{};
        for (int v : d) cnt[std::clamp(v, 1, 6)]++;
        std::vector<std::pair<int, int>> groups;
        for (int f = 6; f >= 1; --f) if (cnt[f] > 0) groups.push_back({ cnt[f], f });
        std::sort(groups.begin(), groups.end(), [](auto a, auto b) {
            return a.first != b.first ? a.first > b.first : a.second > b.second;
        });

        int tb = 0;
        for (auto& g : groups) tb = tb * 10 + g.second;

        if (groups[0].first == 5) return { 7, tb, "ПОКЕР (5 одинаковых)!" };
        if (groups[0].first == 4) return { 6, tb, "Каре (4 одинаковых)" };
        if (groups[0].first == 3 && groups[1].first == 2) return { 5, tb, "Фулл-хаус" };
        if (cnt[2] && cnt[3] && cnt[4] && cnt[5] && cnt[6]) return { 4, 65432, "Большой стрит (2-6)" };
        if (cnt[1] && cnt[2] && cnt[3] && cnt[4] && cnt[5]) return { 3, 54321, "Малый стрит (1-5)" };
        if (groups[0].first == 3) return { 2, tb, "Сет (Тройка)" };
        if (groups[0].first == 2 && groups[1].first == 2) return { 1, tb, "Две пары" };
        if (groups[0].first == 2) return { 0, tb, "Пара" };
        return { -1, tb, "Старшая кость" };
    }

    void TavernGamesUI::RenderDicePoker(float dt)
    {
        if (diceAnimTimer_ > 0.0f) {
            diceAnimTimer_ = std::max(0.0f, diceAnimTimer_ - dt);
            std::uniform_int_distribution<int> d6(1, 6);
            for (int i = 0; i < 5; ++i) {
                if (!pHold_[i]) pDice_[i] = d6(rng_);
            }
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 t0 = ImGui::GetCursorScreenPos();
        ImVec2 t1(t0.x + avail.x, t0.y + avail.y - 8);

        DrawTexOrRect(dl, "red_velvet", t0, t1, IM_COL32(84, 18, 22, 255));
        dl->AddRect(t0, t1, IM_COL32(198, 158, 72, 220), 10.0f, 0, 3.0f);

        ImGui::SetCursorScreenPos(ImVec2(t0.x + 24, t0.y + 18));
        ImGui::BeginGroup();
        ImGui::TextColored(ImVec4(1.0f, 0.86f, 0.45f, 1.0f),
            "ПОКЕР НА КОСТЯХ   |   Ставка: %d септимов   |   Перебросов осталось: %d",
            currentStake_, diceRollsLeft_);
        ImGui::Spacing();

        float dieSz = std::min(118.0f, (avail.x - 160.0f) / 5.0f);
        float gap = 22.0f;
        float rowW = dieSz * 5.0f + gap * 4.0f;
        float startX = t0.x + (avail.x - rowW) * 0.5f;

        ImGui::SetCursorScreenPos(ImVec2(startX, t0.y + 66));
        if (diceShowAi_) {
            auto aiH = EvaluateDice(aiDice_);
            ImGui::TextColored(ImVec4(0.96f, 0.78f, 0.65f, 1.0f), "Кости трактирщика: %s", aiH.name.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.85f, 0.78f, 0.68f, 0.85f), "Кости трактирщика (вскроются в финале раунда):");
        }
        float aiY = t0.y + 96;
        for (int i = 0; i < 5; ++i) {
            ImVec2 dp0(startX + i * (dieSz + gap), aiY);
            ImVec2 dp1(dp0.x + dieSz, dp0.y + dieSz);
            if (diceShowAi_) {
                DrawDiceFace(dl, aiDice_[i], dp0, dp1, false);
            } else {
                dl->AddRectFilled(dp0, dp1, IM_COL32(38, 16, 16, 200), 16.0f);
                dl->AddRect(dp0, dp1, IM_COL32(180, 140, 62, 180), 16.0f, 0, 2.5f);
                ImVec2 qSz = ImGui::GetFont()->CalcTextSizeA(44.0f, FLT_MAX, 0, "?");
                dl->AddText(ImGui::GetFont(), 44.0f,
                            ImVec2(dp0.x + (dieSz - qSz.x) * 0.5f, dp0.y + (dieSz - qSz.y) * 0.5f),
                            IM_COL32(210, 172, 85, 220), "?");
            }
        }

        float plY = aiY + dieSz + 68;
        ImGui::SetCursorScreenPos(ImVec2(startX, plY - 30));
        if (diceRoundActive_ || diceShowAi_) {
            auto pH = EvaluateDice(pDice_);
            ImGui::TextColored(ImVec4(1.0f, 0.92f, 0.58f, 1.0f),
                "Ваши кости (кликайте по костям, чтобы УДЕРЖАТЬ): %s", pH.name.c_str());
        } else {
            ImGui::TextColored(ImVec4(0.95f, 0.88f, 0.75f, 1.0f), "Ваши кости:");
        }

        for (int i = 0; i < 5; ++i) {
            float wobble = (diceAnimTimer_ > 0.0f && !pHold_[i]) ? std::sin(diceAnimTimer_ * 45.0f + i * 2.0f) * 8.0f : 0.0f;
            ImVec2 dp0(startX + i * (dieSz + gap), plY + wobble);
            ImVec2 dp1(dp0.x + dieSz, dp0.y + dieSz);
            ImGui::SetCursorScreenPos(dp0);
            char id[24];
            std::snprintf(id, sizeof(id), "##PDie%d", i);
            if (ImGui::InvisibleButton(id, ImVec2(dieSz, dieSz)) && diceRoundActive_ && diceAnimTimer_ <= 0.0f) {
                pHold_[i] = !pHold_[i];
                AssetManager::Get().PlaySfx("wood_clack");
            }
            DrawDiceFace(dl, pDice_[i], dp0, dp1, pHold_[i]);
            if (pHold_[i]) {
                ImVec2 lSz = ImGui::CalcTextSize("УДЕРЖАТЬ");
                dl->AddText(ImVec2(dp0.x + (dieSz - lSz.x) * 0.5f, dp1.y + 8),
                            IM_COL32(255, 220, 88, 255), "УДЕРЖАТЬ");
            }
        }

        float btnY = plY + dieSz + 46;
        ImGui::SetCursorScreenPos(ImVec2(startX, btnY));
        ImGui::TextColored(ImVec4(0.96f, 0.90f, 0.78f, 1.0f), "%s", diceStatus_.c_str());

        ImGui::SetCursorScreenPos(ImVec2(startX, btnY + 34));
        if (!diceRoundActive_) {
            if (ImGui::Button("Бросить кости (Новая ставка)", ImVec2(270, 44))) {
                if (TryPayStake(currentStake_)) {
                    diceRoundActive_ = true;
                    diceShowAi_ = false;
                    diceRollsLeft_ = 2;
                    pHold_.fill(false);
                    diceAnimTimer_ = 0.45f;
                    std::uniform_int_distribution<int> d6(1, 6);
                    for (int i = 0; i < 5; ++i) { pDice_[i] = d6(rng_); aiDice_[i] = d6(rng_); }
                    int aiTries = (difficulty_ == Difficulty::Novice) ? 0 : (difficulty_ == Difficulty::Veteran) ? 1 : 2;
                    for (int t = 0; t < aiTries; ++t) {
                        int cnt[7]{};
                        for (int v : aiDice_) cnt[v]++;
                        int bestFace = 1;
                        for (int f = 1; f <= 6; ++f) if (cnt[f] >= cnt[bestFace]) bestFace = f;
                        for (int i = 0; i < 5; ++i) if (aiDice_[i] != bestFace) aiDice_[i] = d6(rng_);
                    }
                    AssetManager::Get().PlaySfx("dice_roll");
                    diceStatus_ = "Кликните по нужным костям для удержания, затем перебросьте или вскрывайтесь!";
                }
            }
        } else {
            ImGui::BeginDisabled(diceRollsLeft_ <= 0 || diceAnimTimer_ > 0.0f);
            if (ImGui::Button("Перебросить невыбранные", ImVec2(230, 44))) {
                diceRollsLeft_--;
                diceAnimTimer_ = 0.40f;
                AssetManager::Get().PlaySfx("dice_roll");
            }
            ImGui::EndDisabled();

            ImGui::SameLine(0, 16);
            ImGui::BeginDisabled(diceAnimTimer_ > 0.0f);
            if (ImGui::Button("Вскрыться и сравнить!", ImVec2(230, 44))) {
                diceRoundActive_ = false;
                diceShowAi_ = true;
                auto pH = EvaluateDice(pDice_);
                auto aH = EvaluateDice(aiDice_);
                bool pWin = (pH.category > aH.category) || (pH.category == aH.category && pH.tiebreak > aH.tiebreak);
                bool draw = (pH.category == aH.category && pH.tiebreak == aH.tiebreak);
                if (pWin) {
                    RecordResult(1, currentStake_ * 2, "Победа! " + pH.name + " бьёт " + aH.name + "! +" + std::to_string(currentStake_) + " зол.");
                    diceStatus_ = "Вы выиграли раунд! (" + pH.name + " > " + aH.name + ")";
                } else if (draw) {
                    RecordResult(0, currentStake_, "Ничья! Ставка возвращена.");
                    diceStatus_ = "Ничья! Обе стороны собрали " + pH.name;
                } else {
                    RecordResult(-1, 0, "Трактирщик выиграл (" + aH.name + " против " + pH.name + ").");
                    diceStatus_ = "Проигрыш. У трактирщика: " + aH.name;
                }
            }
            ImGui::EndDisabled();
        }
        ImGui::EndGroup();
    }

    // ==================== 4. BLACKJACK (21) ====================
    void TavernGamesUI::InitBlackjack()
    {
        bjPlayer_.clear();
        bjDealer_.clear();
        bjState_ = 0;
        bjCurrentBet_ = currentStake_;
        bjStatus_ = "Нажмите «Раздать карты», чтобы сыграть в Блэкджек (21).";
    }

    TavernGamesUI::Card TavernGamesUI::DrawCard()
    {
        if (bjDeck_.size() < 12) {
            bjDeck_.clear();
            for (int s = 0; s < 4; ++s)
                for (int r = 1; r <= 13; ++r)
                    bjDeck_.push_back({ r, s });
            std::shuffle(bjDeck_.begin(), bjDeck_.end(), rng_);
        }
        Card c = bjDeck_.back();
        bjDeck_.pop_back();
        return c;
    }

    int TavernGamesUI::CalcHandValue(const std::vector<Card>& h) const
    {
        int sum = 0, aces = 0;
        for (const auto& c : h) {
            if (c.rank == 1) { sum += 11; aces++; }
            else if (c.rank >= 10) sum += 10;
            else sum += c.rank;
        }
        while (sum > 21 && aces > 0) { sum -= 10; aces--; }
        return sum;
    }

    void TavernGamesUI::StartBlackjackHand()
    {
        bjCurrentBet_ = currentStake_;
        if (!TryPayStake(bjCurrentBet_)) return;
        bjPlayer_.clear();
        bjDealer_.clear();
        bjPlayer_.push_back(DrawCard());
        bjDealer_.push_back(DrawCard());
        bjPlayer_.push_back(DrawCard());
        bjDealer_.push_back(DrawCard());
        AssetManager::Get().PlaySfx("card_flip");

        if (CalcHandValue(bjPlayer_) == 21) {
            bjState_ = 3;
            int winPay = static_cast<int>(bjCurrentBet_ * 2.5f);
            RecordResult(1, winPay, "БЛЭКДЖЕК! Выплата 3:2 (+" + std::to_string(winPay - bjCurrentBet_) + " септимов)!");
            bjStatus_ = "Натуральные 21! Великолепная раздача!";
        } else {
            bjState_ = 1;
            bjStatus_ = "Ваш ход: возьмите карту, остановитесь или удвойте ставку.";
        }
    }

    void TavernGamesUI::RenderBlackjack(float dt)
    {
        if (bjState_ == 2) {
            bjDealerTimer_ -= dt;
            if (bjDealerTimer_ <= 0.0f) {
                int dv = CalcHandValue(bjDealer_);
                int stopAt = (difficulty_ == Difficulty::Novice) ? 16 : 17;
                if (dv < stopAt) {
                    bjDealer_.push_back(DrawCard());
                    AssetManager::Get().PlaySfx("card_flip");
                    bjDealerTimer_ = 0.55f;
                } else {
                    bjState_ = 3;
                    int pv = CalcHandValue(bjPlayer_);
                    if (dv > 21 || pv > dv) {
                        RecordResult(1, bjCurrentBet_ * 2, "Победа в 21! +" + std::to_string(bjCurrentBet_) + " септимов!");
                        bjStatus_ = "Вы победили! (" + std::to_string(pv) + " против " + std::to_string(dv) + ")";
                    } else if (pv == dv) {
                        RecordResult(0, bjCurrentBet_, "Ничья! Ставка возвращена.");
                        bjStatus_ = "Ровно " + std::to_string(pv) + " у обоих — ничья.";
                    } else {
                        RecordResult(-1, 0, "Дилер выиграл (" + std::to_string(dv) + " против " + std::to_string(pv) + ").");
                        bjStatus_ = "Дилер набрал " + std::to_string(dv) + " и забрал банк.";
                    }
                }
            }
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 t0 = ImGui::GetCursorScreenPos();
        ImVec2 t1(t0.x + avail.x, t0.y + avail.y - 8);
        DrawTexOrRect(dl, "red_velvet", t0, t1, IM_COL32(84, 18, 22, 255));
        dl->AddRect(t0, t1, IM_COL32(198, 158, 72, 220), 10.0f, 0, 3.0f);

        float cardH = std::min(185.0f, avail.y * 0.30f);
        float cardW = cardH * 0.705f;

        ImVec2 deckPos(t1.x - cardW - 36, t0.y + 42);
        DrawPlayingCard(dl, 1, 0, true, ImVec2(deckPos.x + 6, deckPos.y + 6), ImVec2(cardW, cardH));
        DrawPlayingCard(dl, 1, 0, true, deckPos, ImVec2(cardW, cardH));

        ImTextureID coinId = TexId("coin_septim");
        if (coinId && bjState_ != 0) {
            for (int k = 0; k < 3; ++k) {
                ImVec2 cp(t1.x - 120 + k * 14, t0.y + cardH + 78 - k * 6);
                dl->AddImage(coinId, cp, ImVec2(cp.x + 42, cp.y + 42));
            }
        }

        ImGui::SetCursorScreenPos(ImVec2(t0.x + 36, t0.y + 22));
        if (bjState_ == 1 && !bjDealer_.empty()) {
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.70f, 1.0f), "Карты трактирщика: ?");
        } else if (!bjDealer_.empty()) {
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.70f, 1.0f), "Карты трактирщика: %d", CalcHandValue(bjDealer_));
        } else {
            ImGui::TextColored(ImVec4(0.95f, 0.85f, 0.70f, 1.0f), "БЛЭКДЖЕК — «ДВАДЦАТЬ ОДНО»");
        }

        for (size_t i = 0; i < bjDealer_.size(); ++i) {
            bool hide = (bjState_ == 1 && i == 1);
            DrawPlayingCard(dl, bjDealer_[i].rank, bjDealer_[i].suit, hide,
                            ImVec2(t0.x + 36 + i * (cardW * 0.72f), t0.y + 52), ImVec2(cardW, cardH));
        }

        float py = t0.y + cardH + 92;
        ImGui::SetCursorScreenPos(ImVec2(t0.x + 36, py - 28));
        if (!bjPlayer_.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.92f, 0.52f, 1.0f),
                "Ваши карты: %d   |   Банк ставки: %d септимов", CalcHandValue(bjPlayer_), bjCurrentBet_);
        }
        for (size_t i = 0; i < bjPlayer_.size(); ++i) {
            DrawPlayingCard(dl, bjPlayer_[i].rank, bjPlayer_[i].suit, false,
                            ImVec2(t0.x + 36 + i * (cardW * 0.72f), py), ImVec2(cardW, cardH));
        }

        float cy = py + cardH + 20;
        ImGui::SetCursorScreenPos(ImVec2(t0.x + 36, cy));
        ImGui::TextColored(ImVec4(0.96f, 0.90f, 0.78f, 1.0f), "%s", bjStatus_.c_str());

        ImGui::SetCursorScreenPos(ImVec2(t0.x + 36, cy + 30));
        if (bjState_ == 0 || bjState_ == 3) {
            if (ImGui::Button("Раздать карты", ImVec2(210, 44))) {
                StartBlackjackHand();
            }
        } else if (bjState_ == 1) {
            if (ImGui::Button("Взять карту", ImVec2(170, 44))) {
                bjPlayer_.push_back(DrawCard());
                AssetManager::Get().PlaySfx("card_flip");
                int pv = CalcHandValue(bjPlayer_);
                if (pv > 21) {
                    bjState_ = 3;
                    RecordResult(-1, 0, "Перебор (" + std::to_string(pv) + ")! Вы проиграли " + std::to_string(bjCurrentBet_) + " септимов.");
                    bjStatus_ = "Перебор! Сумма: " + std::to_string(pv);
                } else if (pv == 21) {
                    bjState_ = 2;
                    bjDealerTimer_ = 0.55f;
                }
            }
            ImGui::SameLine(0, 12);
            if (ImGui::Button("Хватит (Пас)", ImVec2(170, 44))) {
                bjState_ = 2;
                bjDealerTimer_ = 0.55f;
                bjStatus_ = "Трактирщик открывает карты...";
            }
            ImGui::SameLine(0, 12);
            if (bjPlayer_.size() == 2 && ImGui::Button("Удвоить ставку", ImVec2(180, 44))) {
                if (TryPayStake(bjCurrentBet_)) {
                    bjCurrentBet_ *= 2;
                    bjPlayer_.push_back(DrawCard());
                    AssetManager::Get().PlaySfx("card_flip");
                    int pv = CalcHandValue(bjPlayer_);
                    if (pv > 21) {
                        bjState_ = 3;
                        RecordResult(-1, 0, "Перебор после удвоения! -" + std::to_string(bjCurrentBet_) + " зол.");
                        bjStatus_ = "Перебор (" + std::to_string(pv) + ").";
                    } else {
                        bjState_ = 2;
                        bjDealerTimer_ = 0.55f;
                    }
                }
            }
        }
    }

    // ==================== 5. THIMBLES (3 CUPS) ====================
    void TavernGamesUI::InitThimbles()
    {
        thimState_ = 0;
        thimBallCup_ = 1;
        thimPickedCup_ = -1;
        thimSlot_ = { 0, 1, 2 };
        thimX_ = { 0.0f, 1.0f, 2.0f };
        thimLift_ = { 0.0f, 1.0f, 0.0f };
        thimStatus_ = "Следите за золотым септимом под кубком! Нажмите «Начать раунд».";
    }

    void TavernGamesUI::StartThimblesRound()
    {
        if (!TryPayStake(currentStake_)) return;
        std::uniform_int_distribution<int> d3(0, 2);
        thimBallCup_ = d3(rng_);
        thimPickedCup_ = -1;
        thimSlot_ = { 0, 1, 2 };
        thimX_ = { 0.0f, 1.0f, 2.0f };
        thimLift_ = { 0.0f, 0.0f, 0.0f };
        thimLift_[thimBallCup_] = 1.0f;
        thimState_ = 1;
        thimTimer_ = 1.1f;
        thimSwapsLeft_ = (difficulty_ == Difficulty::Novice) ? 6 : (difficulty_ == Difficulty::Veteran) ? 10 : 14;
        thimSwapSpeed_ = (difficulty_ == Difficulty::Novice) ? 2.1f : (difficulty_ == Difficulty::Veteran) ? 3.2f : 4.4f;
        thimStatus_ = "Запоминайте, под каким кубком септим!";
    }

    void TavernGamesUI::RenderThimbles(float dt)
    {
        if (thimState_ == 1) {
            thimTimer_ -= dt;
            if (thimTimer_ < 0.35f) thimLift_[thimBallCup_] = std::max(0.0f, thimTimer_ / 0.35f);
            if (thimTimer_ <= 0.0f) {
                thimLift_.fill(0.0f);
                thimState_ = 2;
                thimSwapT_ = 1.0f;
                thimStatus_ = "Перемешиваем...";
            }
        } else if (thimState_ == 2) {
            thimSwapT_ += dt * thimSwapSpeed_;
            if (thimSwapT_ >= 1.0f) {
                for (int i = 0; i < 3; ++i) thimX_[i] = static_cast<float>(thimSlot_[i]);
                if (thimSwapsLeft_ <= 0) {
                    thimState_ = 3;
                    thimStatus_ = "Кликните по кубку, под которым спрятан золотой септим!";
                } else {
                    thimSwapsLeft_--;
                    std::uniform_int_distribution<int> d3(0, 2);
                    thimSwapA_ = d3(rng_);
                    do { thimSwapB_ = d3(rng_); } while (thimSwapB_ == thimSwapA_);
                    std::swap(thimSlot_[thimSwapA_], thimSlot_[thimSwapB_]);
                    thimSwapT_ = 0.0f;
                    AssetManager::Get().PlaySfx("cup_slide");
                }
            } else {
                float u = 0.5f - 0.5f * std::cos(thimSwapT_ * kPi);
                float fromA = static_cast<float>(thimSlot_[thimSwapB_]);
                float toA   = static_cast<float>(thimSlot_[thimSwapA_]);
                thimX_[thimSwapA_] = fromA + (toA - fromA) * u;
                thimX_[thimSwapB_] = toA + (fromA - toA) * u;
            }
        } else if (thimState_ == 4) {
            for (int i = 0; i < 3; ++i) thimLift_[i] = std::min(1.0f, thimLift_[i] + dt * 3.5f);
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 t0 = ImGui::GetCursorScreenPos();
        ImVec2 t1(t0.x + avail.x, t0.y + avail.y - 8);
        DrawTexOrRect(dl, "wood_table", t0, t1, IM_COL32(55, 34, 20, 255));
        dl->AddRect(t0, t1, IM_COL32(198, 158, 72, 220), 10.0f, 0, 3.0f);

        ImGui::SetCursorScreenPos(ImVec2(t0.x + 28, t0.y + 22));
        ImGui::TextColored(ImVec4(1.0f, 0.86f, 0.45f, 1.0f),
            "НАПЁРСТКИ (ТРИ КУБКА)   |   %s   |   Ставка: %d септимов (выплата x2)",
            DifficultyName(difficulty_), currentStake_);

        float cupSz = std::min(220.0f, avail.x * 0.23f);
        float spacing = cupSz * 1.32f;
        float centerX = t0.x + avail.x * 0.5f;
        float baseY = t0.y + avail.y * 0.52f;

        if (thimLift_[thimBallCup_] > 0.08f) {
            float cx = centerX + (thimX_[thimBallCup_] - 1.0f) * spacing;
            float coinSz = cupSz * 0.36f;
            DrawTexOrRect(dl, "coin_septim",
                          ImVec2(cx - coinSz * 0.5f, baseY + cupSz * 0.14f),
                          ImVec2(cx + coinSz * 0.5f, baseY + cupSz * 0.14f + coinSz),
                          IM_COL32(235, 190, 50, 255));
        }

        for (int i = 0; i < 3; ++i) {
            float arcY = 0.0f;
            if (thimState_ == 2) {
                if (i == thimSwapA_) arcY = -std::sin(thimSwapT_ * kPi) * 48.0f;
                if (i == thimSwapB_) arcY =  std::sin(thimSwapT_ * kPi) * 48.0f;
            }
            float cx = centerX + (thimX_[i] - 1.0f) * spacing;
            float cy = baseY - thimLift_[i] * 92.0f + arcY;
            ImVec2 cp0(cx - cupSz * 0.5f, cy - cupSz * 0.5f);
            ImVec2 cp1(cx + cupSz * 0.5f, cy + cupSz * 0.5f);

            ImGui::SetCursorScreenPos(cp0);
            char id[24];
            std::snprintf(id, sizeof(id), "##Cup%d", i);
            bool clicked = ImGui::InvisibleButton(id, ImVec2(cupSz, cupSz));
            bool hovered = ImGui::IsItemHovered() && (thimState_ == 3);

            DrawTexOrRect(dl, "cup_thimble", cp0, cp1, IM_COL32(130, 82, 42, 255),
                          ImVec2(0, 0), ImVec2(1, 1),
                          hovered ? IM_COL32(255, 240, 195, 255) : IM_COL32(255, 255, 255, 255));

            if (hovered) {
                dl->AddCircle(ImVec2(cx, cy + cupSz * 0.36f), cupSz * 0.34f, IM_COL32(255, 220, 88, 220), 32, 3.0f);
            }

            if (clicked && thimState_ == 3) {
                thimPickedCup_ = i;
                thimState_ = 4;
                if (i == thimBallCup_) {
                    RecordResult(1, currentStake_ * 2, "Зоркий глаз! Септим найден! +" + std::to_string(currentStake_) + " септимов!");
                    thimStatus_ = "Точно в цель! Вы угадали кубок!";
                } else {
                    RecordResult(-1, 0, "Пусто! Шулер перехитрил вас (-" + std::to_string(currentStake_) + " септимов).");
                    thimStatus_ = "Под этим кубком пусто!";
                }
            }
        }

        ImGui::SetCursorScreenPos(ImVec2(t0.x + 28, t1.y - 96));
        ImGui::TextColored(ImVec4(0.98f, 0.90f, 0.74f, 1.0f), "%s", thimStatus_.c_str());

        ImGui::SetCursorScreenPos(ImVec2(centerX - 125, t1.y - 60));
        ImGui::BeginDisabled(thimState_ == 1 || thimState_ == 2 || thimState_ == 3);
        if (ImGui::Button("Начать раунд (Перемешать)", ImVec2(250, 44))) {
            StartThimblesRound();
        }
        ImGui::EndDisabled();
    }

    // ==================== 6. COIN FLIP ====================
    void TavernGamesUI::RenderCoinFlip(float dt)
    {
        if (coinFlipping_) {
            coinAnim_ += dt * 1.4f;
            if (coinAnim_ >= 1.0f) {
                coinFlipping_ = false;
                coinAnim_ = 1.0f;
                if (coinResult_ == coinChoice_) {
                    RecordResult(1, currentStake_ * 2, "Выпала ваша сторона! +" + std::to_string(currentStake_) + " септимов!");
                    coinStatus_ = (coinResult_ == 0) ? "Выпал ДРАКОН! Победа!" : "Выпала КОРОНА! Победа!";
                } else {
                    RecordResult(-1, 0, "Не повезло! -" + std::to_string(currentStake_) + " септимов.");
                    coinStatus_ = (coinResult_ == 0) ? "Выпал ДРАКОН. Удача отвернулась." : "Выпала КОРОНА. Удача отвернулась.";
                }
            }
        }

        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 t0 = ImGui::GetCursorScreenPos();
        ImVec2 t1(t0.x + avail.x, t0.y + avail.y - 8);
        DrawTexOrRect(dl, "red_velvet", t0, t1, IM_COL32(84, 18, 22, 255));
        dl->AddRect(t0, t1, IM_COL32(198, 158, 72, 220), 10.0f, 0, 3.0f);

        float cx = t0.x + avail.x * 0.5f;
        float cy = t0.y + avail.y * 0.42f;
        float r = 92.0f;
        float lift = coinFlipping_ ? std::sin(coinAnim_ * kPi) * 115.0f : 0.0f;
        float scaleY = coinFlipping_ ? std::max(0.08f, std::abs(std::cos(coinAnim_ * kPi * 7.0f))) : 1.0f;

        dl->AddEllipseFilled(ImVec2(cx, cy + 78), ImVec2(r * (0.9f - lift * 0.003f), 18.0f), IM_COL32(0, 0, 0, 120));

        ImVec2 p0(cx - r, cy - lift - r * scaleY);
        ImVec2 p1(cx + r, cy - lift + r * scaleY);
        DrawTexOrRect(dl, "coin_septim", p0, p1, IM_COL32(240, 195, 58, 255));

        const char* faceTxt = (coinFlipping_ ? "..." : (coinResult_ == 0 ? "ДРАКОН" : "КОРОНА"));
        ImVec2 fSz = ImGui::GetFont()->CalcTextSizeA(22.0f, FLT_MAX, 0, faceTxt);
        dl->AddText(ImGui::GetFont(), 22.0f, ImVec2(cx - fSz.x * 0.5f, cy + 105),
                    IM_COL32(255, 225, 110, 255), faceTxt);

        ImGui::SetCursorScreenPos(ImVec2(cx - 210, cy + 148));
        ImGui::BeginDisabled(coinFlipping_);
        if (ImGui::Button("Ставить на ДРАКОНА", ImVec2(200, 46))) {
            if (TryPayStake(currentStake_)) {
                coinChoice_ = 0;
                coinResult_ = std::uniform_int_distribution<int>(0, 1)(rng_);
                coinFlipping_ = true;
                coinAnim_ = 0.0f;
                AssetManager::Get().PlaySfx("coin_win");
                coinStatus_ = "Септим в воздухе...";
            }
        }
        ImGui::SameLine(0, 20);
        if (ImGui::Button("Ставить на КОРОНУ", ImVec2(200, 46))) {
            if (TryPayStake(currentStake_)) {
                coinChoice_ = 1;
                coinResult_ = std::uniform_int_distribution<int>(0, 1)(rng_);
                coinFlipping_ = true;
                coinAnim_ = 0.0f;
                AssetManager::Get().PlaySfx("coin_win");
                coinStatus_ = "Септим в воздухе...";
            }
        }
        ImGui::EndDisabled();

        ImGui::SetCursorScreenPos(ImVec2(cx - ImGui::CalcTextSize(coinStatus_.c_str()).x * 0.5f, cy + 210));
        ImGui::TextColored(ImVec4(0.98f, 0.90f, 0.74f, 1.0f), "%s", coinStatus_.c_str());
    }

    // ==================== HALL OF FAME ====================
    void TavernGamesUI::RenderHallOfFame()
    {
        auto& am = AssetManager::Get();
        auto& p = am.profile;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.06f, 0.04f, 0.90f));
        ImGui::BeginChild("##Hall", ImVec2(-1, -1), ImGuiChildFlags_Borders);
        ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.38f, 1.0f), "ЗАЛ СЛАВЫ И КОШЕЛЁК ТАВЕРНЫ");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Текущее звание:  %s", GetPlayerRank());
        ImGui::Text("Кошелёк таверны: %d септимов", p.septims);
        ImGui::Text("Всего партий:    %d", p.totalGames);
        ImGui::Text("Побед: %d   |   Поражений: %d   |   Ничьих: %d", p.wins, p.losses, p.draws);
        float wr = (p.totalGames > 0) ? (100.0f * p.wins / p.totalGames) : 0.0f;
        ImGui::Text("Процент побед:   %.1f%%", wr);
        ImGui::Text("Чистый доход:    %+d септимов", p.netProfit);
        ImGui::Text("Рекорд в дартс (за 3 дротика): %d очков", p.bestDartRound);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.92f, 0.84f, 0.65f, 1.0f), "Управление кошельком:");
        if (ImGui::Button("Взять из сундука +500 септимов", ImVec2(280, 40))) {
            p.septims += 500;
            am.SaveProfile();
            am.PlaySfx("coin_win");
        }
        ImGui::SameLine(0, 14);
        if (ImGui::Button("Сбросить статистику", ImVec2(200, 40))) {
            p = PlayerProfile{};
            am.SaveProfile();
            am.PlaySfx("wood_clack");
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextWrapped("Управление: нажмите F8 (можно изменить в TavernGamesVisual.ini), чтобы открыть или скрыть стол тавернных игр в любой момент прямо поверх Skyrim SE.");
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
}
