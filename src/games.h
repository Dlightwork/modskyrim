#pragma once
#include <array>
#include <random>
#include <string>
#include <vector>

namespace tg
{
    enum class Screen
    {
        Lobby,
        Darts,
        Checkers,
        DicePoker,
        Blackjack,
        Thimbles,
        CoinFlip,
        HallOfFame
    };

    enum class Difficulty
    {
        Novice = 0,
        Veteran = 1,
        Master = 2
    };

    class TavernGamesUI
    {
    public:
        static TavernGamesUI& Get();

        TavernGamesUI();
        void Render(float dt);

        bool wantsClose = false;

    private:
        void RenderTopBar();
        void RenderLobby();
        void RenderHallOfFame();

        bool TryPayStake(int amount);
        void RecordResult(int outcome, int payout, const std::string& bannerText);
        const char* GetPlayerRank() const;

        // 1. DARTS (301)
        struct DartHit { float nx, ny; bool isPlayer; int score; std::string label; };
        void InitDarts();
        void RenderDarts(float dt);
        void EvaluateDart(float nx, float ny, int& outScore, std::string& outLabel) const;

        int dartsPlayerScore_ = 301;
        int dartsAiScore_ = 301;
        int dartsLeft_ = 3;
        int dartsRoundTotal_ = 0;
        bool dartsPlayerTurn_ = true;
        bool dartsMatchOver_ = false;
        bool dartsCharging_ = false;
        float dartsCharge_ = 0.0f;
        float dartsSwayTime_ = 0.0f;
        float dartsAiTimer_ = 0.0f;
        std::vector<DartHit> dartsHits_;
        std::string dartsStatus_;

        // 2. CHECKERS (8x8)
        struct Move { int r0, c0, r1, c1; bool isCapture; int capR, capC; };
        void InitCheckers();
        void RenderCheckers(float dt);
        std::vector<Move> GetValidMoves(int side, int forcedR = -1, int forcedC = -1) const;
        void ApplyCheckerMove(const Move& m);
        void DoAiCheckerMove();

        int board_[8][8]{};
        int selR_ = -1, selC_ = -1;
        int chainR_ = -1, chainC_ = -1;
        bool checkersPlayerTurn_ = true;
        bool checkersOver_ = false;
        float checkersAiDelay_ = 0.0f;
        std::string checkersStatus_;

        // 3. DICE POKER
        struct HandRank { int category; int tiebreak; std::string name; };
        void InitDicePoker();
        void RenderDicePoker(float dt);
        HandRank EvaluateDice(const std::array<int, 5>& d) const;

        std::array<int, 5> pDice_{ 1, 2, 3, 4, 5 };
        std::array<int, 5> aiDice_{ 1, 2, 3, 4, 5 };
        std::array<bool, 5> pHold_{ false, false, false, false, false };
        int diceRollsLeft_ = 2;
        bool diceRoundActive_ = false;
        bool diceShowAi_ = false;
        float diceAnimTimer_ = 0.0f;
        std::string diceStatus_;

        // 4. BLACKJACK (21)
        struct Card { int rank; int suit; };
        void InitBlackjack();
        void StartBlackjackHand();
        void RenderBlackjack(float dt);
        int CalcHandValue(const std::vector<Card>& h) const;
        Card DrawCard();

        std::vector<Card> bjDeck_;
        std::vector<Card> bjPlayer_;
        std::vector<Card> bjDealer_;
        int bjState_ = 0;
        int bjCurrentBet_ = 25;
        float bjDealerTimer_ = 0.0f;
        std::string bjStatus_;

        // 5. THIMBLES (3 cups)
        void InitThimbles();
        void StartThimblesRound();
        void RenderThimbles(float dt);

        int thimState_ = 0;
        int thimBallCup_ = 1;
        int thimPickedCup_ = -1;
        std::array<int, 3> thimSlot_{ 0, 1, 2 };
        std::array<float, 3> thimX_{ 0.0f, 1.0f, 2.0f };
        std::array<float, 3> thimLift_{ 1.0f, 1.0f, 1.0f };
        int thimSwapsLeft_ = 0;
        int thimSwapA_ = 0, thimSwapB_ = 1;
        float thimSwapT_ = 0.0f;
        float thimSwapSpeed_ = 2.6f;
        float thimTimer_ = 0.0f;
        std::string thimStatus_;

        // 6. COIN FLIP
        void RenderCoinFlip(float dt);
        int coinChoice_ = 0;
        int coinResult_ = 0;
        bool coinFlipping_ = false;
        float coinAnim_ = 0.0f;
        std::string coinStatus_;

        Screen screen_ = Screen::Lobby;
        Difficulty difficulty_ = Difficulty::Veteran;
        int currentStake_ = 25;
        std::string bannerMsg_;
        float bannerTimer_ = 0.0f;
        std::mt19937 rng_;
    };
}
