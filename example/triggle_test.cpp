#include <iostream>
#define INCLUDEAI_IMPLEMENTATION
#include "../includeai.hpp"
#include <cstdint>
#include <bitset>
#include <unistd.h> // sleeep()
#include <numeric> // iota

using namespace include_ai;


consteval std::array<uint16_t, 3*16> makeEdgeIds()
{
    constexpr const char str[3*16][3] = {"ad", "eh","fi", "jm","kn","lo", "ps","qt","ru","sv", "wz","xA","yB", "CF","DG", "HK",
                                         "ap", "bq","fw", "cr","gx","lC", "ds","hy","mD","sH", "iz","nE","tI", "oF","uJ", "vK",
                                         "pH", "jD","qI", "ey","kE","rJ", "as","fz","lF","sK", "bt","gA","mG", "cu","hB", "dv"
                                        };
    std::array<uint16_t, 3*16> edgeIds{};
    for (int i=0; i<edgeIds.size(); ++i)
    {
        edgeIds[i] = (uint16_t((str[i][0]-'a') & 0x3F) << 8)
                   | (uint16_t((str[i][1]-'a') & 0x3F));
    }
    return edgeIds;
}

class TriggleBoard
{
public:
    using Move = uint16_t;
    using StorageForMoves = Move[3*16];
    int currentPlayer=1, winner, playerCount=2;
    std::array<int, 5> captured = {0,0,0,0,0};
    static constexpr auto edgeIds = makeEdgeIds();
private:
    std::bitset<3*16> remainingFreeEdges;
    std::array<Move, 3*16> remainingMoves;
    std::array<uint8_t, 54> triangleHasEdge = {};

    // Display only:
    static constexpr char      label[38] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK";
    std::array<uint8_t, 54>    triangleColor;
    std::bitset<38>            hasRubber[5];
public:
    constexpr TriggleBoard() { reset(); }
    TriggleBoard(const TriggleBoard&) = delete;
    TriggleBoard& operator=(const TriggleBoard&) = delete;
    TriggleBoard(TriggleBoard&&) = default;

    constexpr TriggleBoard clone() const
    {
        TriggleBoard dst;
        dst.currentPlayer = currentPlayer;
        dst.playerCount = playerCount;
        for (int i=0;i<5;++i) dst.captured[i] = captured[i];
        dst.remainingFreeEdges = remainingFreeEdges;
        dst.remainingMoves = remainingMoves;
        dst.triangleHasEdge = triangleHasEdge;
        return dst;
    }

    int generateMovesAndGetCnt(Move *availMoves)
    {
        int pos=0;
        for (int i=0; i<remainingMoves.size(); ++i)
            if (remainingFreeEdges[i])
                availMoves[pos++] = remainingMoves[i];
        return pos;
    }

    Outcome doMove(const Move mv)
    {
        constexpr uint8_t edgeLookup[3*16][4] = {
            { 0, 1, 2, 3}, // "ad"
            { 4, 5, 6, 7}, // "eh"
            { 5, 6, 7, 8}, // "fi"
            { 9,10,11,12}, // "jm"
            {10,11,12,13}, // "kn"
            {11,12,13,14}, // "lo"
            {15,16,17,18}, // "ps"
            {16,17,18,19}, // "qt"
            {17,18,19,20}, // "ru"
            {18,19,20,21}, // "sv"
            {22,23,24,25}, // "wz"
            {23,24,25,26}, // "xA"
            {24,25,26,27}, // "yB"
            {28,29,30,31}, // "CF"
            {29,30,31,32}, // "DG"
            {33,34,35,36}, // "HK"

            { 0, 4, 9,15}, // "ap"
            { 1, 5,10,16}, // "bq"
            { 5,10,16,22}, // "fw"
            { 2, 6,11,17}, // "cr"
            { 6,11,17,23}, // "gx"
            {11,17,23,28}, // "lC"
            { 3, 7,12,18}, // "ds"
            { 7,12,18,24}, // "hy"
            {12,18,24,29}, // "mD"
            {18,24,29,33}, // "sH"
            { 8,13,19,25}, // "iz"
            {13,19,25,30}, // "nE"
            {19,25,30,34}, // "tI"
            {14,20,26,31}, // "oF"
            {20,26,31,35}, // "uJ"
            {21,27,32,36}, // "vK"

            {15,22,28,33}, // "pH"
            { 9,16,23,29}, // "jD"
            {16,23,29,34}, // "qI"
            { 4,10,17,24}, // "ey"
            {10,17,24,30}, // "kE"
            {17,24,30,35}, // "rJ"
            { 0, 5,11,18}, // "as"
            { 5,11,18,25}, // "fz"
            {11,18,25,31}, // "lF"
            {18,25,31,36}, // "sK"
            { 1, 6,12,19}, // "bt"
            { 6,12,19,26}, // "gA"
            {12,19,26,32}, // "mG"
            { 2, 7,13,20}, // "cu"
            { 7,13,20,27}, // "hB"
            { 3, 8,14,21}, // "dv"
        };
        int offset = 1;
        offset = offset << (mv>=16);
        offset = offset << (mv>=32);
        hasRubber[offset][edgeLookup[mv][0]] = true;
        hasRubber[offset][edgeLookup[mv][1]] = true;
        hasRubber[offset][edgeLookup[mv][2]] = true;
        hasRubber[offset][edgeLookup[mv][3]] = true;

        constexpr uint8_t triangleLookup[3*16][6] = {
            { 1, 3, 5, 5, 5, 5}, // "ad"
            { 0, 2, 4, 8,10,12}, // "eh"
            { 2, 4, 6,10,12,14}, // "fi"
            { 7, 9,11,17,19,21}, // "jm"
            { 9,11,13,19,21,23}, // "kn"
            {11,13,15,21,23,25}, // "lo"
            {16,18,20,27,29,31}, // "ps"
            {18,20,22,29,31,33}, // "qt"
            {20,22,24,31,33,35}, // "ru"
            {22,24,26,33,35,37}, // "sv"
            {28,30,32,38,40,42}, // "wz"
            {30,32,34,40,42,44}, // "xA"
            {32,34,36,42,44,46}, // "yB"
            {39,41,43,47,49,51}, // "CF"
            {41,53,45,49,51,53}, // "DG"
            {48,50,52,52,52,52}, // "HK"

            { 0, 7,16,16,16,16}, // "ap",
            { 1, 2, 8, 9,17,18}, // "bq"
            { 8, 9,17,18,27,28}, // "fw"
            { 3, 4,10,11,19,20}, // "cr"
            {10,11,19,20,29,30}, // "gx"
            {19,20,29,30,38,39}, // "lC"
            { 5, 6,12,13,21,22}, // "ds"
            {12,13,21,22,31,32}, // "hy"
            {21,22,31,32,40,41}, // "mD"
            {31,32,40,41,47,48}, // "sH"
            {14,15,23,24,33,34}, // "iz"
            {23,24,33,34,42,43}, // "nE"
            {33,34,42,43,49,50}, // "tI"
            {25,26,35,36,44,45}, // "oF"
            {35,36,44,45,51,52}, // "uJ"
            {37,46,53,37,37,37}, // "vK"

            {27,38,47,27,27,27}, // "pH"
            {16,17,28,29,39,40}, // "jD"
            {28,29,39,40,48,49}, // "qI"
            { 7, 8,18,19,30,31}, // "ey"
            {18,19,30,31,41,42}, // "kE"
            {30,31,41,42,50,51}, // "rJ"
            { 0, 1, 9,10,20,21}, // "as"
            { 9,10,20,21,32,33}, // "fz"
            {20,21,32,33,43,44}, // "lF"
            {32,33,43,44,52,53}, // "sK"
            { 2, 3,11,12,22,23}, // "bt"
            {11,12,22,23,34,35}, // "gA"
            {22,23,34,35,41,42}, // "mG"
            { 4, 5,13,14,24,25}, // "cu"
            {13,14,24,25,36,37}, // "hB"
            { 6,15,26,26,26,26}  // "dv"
        };

        for (int i=0; i<6; ++i)
        {
            const uint8_t currentTriangle = triangleHasEdge[triangleLookup[mv][i]];
            triangleHasEdge[triangleLookup[mv][i]] = currentTriangle | offset;
        }

        for (int i=0; i<triangleHasEdge.size(); ++i)
        {
            uint8_t& currentTriangle = triangleHasEdge[i];
            if (currentTriangle == 0b0111)
            {
                currentTriangle = 0b1111;
                triangleColor[i] = currentPlayer;
                captured[currentPlayer] += 1;
            }
        }

        remainingFreeEdges[mv] = false;

        std::array<int, 5> captureCopy = captured;
        std::sort(captureCopy.begin(), captureCopy.end(), std::greater<int>());
        if ((captureCopy[0]-captureCopy[1]) > remainingFreeEdges.count())
        {
            winner = currentPlayer;
            return Outcome::fin;
        }
        else
            return Outcome::running;
    }

    void switchPlayer()
    {
        currentPlayer += 1;
        if (currentPlayer > playerCount) currentPlayer = 1;
    }

    int getCurrentPlayer() const { return currentPlayer; }

    int getWinner() const { return winner; }

    void reset()
    {
        currentPlayer = 1;
        winner = 0;
        remainingFreeEdges.set();
        std::iota(remainingMoves.begin(), remainingMoves.end(), 0);
        std::fill(triangleColor.begin(), triangleColor.end(), 7);
        std::fill(triangleHasEdge.begin(), triangleHasEdge.end(), 0);
    }

    void printBoard() const
    {
        int posIndex = 0, triIndex = 0;
        std::printf("\033[37m");
        for (int r=0; r<13; ++r)
        {
            for (int i=0; i<("33221100112233"[r]-'0'); ++i)
                putchar(' ');
            if ((r&1) == 1)
            {
                if (r>6) putchar(' ');
                for (int c=0; c<("44556665544"[r-1]-'0'); ++c)
                {
                    if (r<6)
                    {
                        std::printf("\033[3%dm", triangleColor[triIndex++]);
                        printf("⅄");
                        if (c<(":3:4:5:6:5:4:3"[r]-'0'))
                        {
                            std::printf("\033[3%dm", triangleColor[triIndex++]);
                            putchar('Y');
                        }
                    }
                    else
                    {
                        std::printf("\033[3%dm", triangleColor[triIndex++]);
                        putchar('Y');
                        if (c<(":3:4:5:5:4:3"[r]-'0'))
                        {
                            std::printf("\033[3%dm", triangleColor[triIndex++]);
                            printf("⅄");
                        }
                    }
                    std::printf("\033[37m");
                }
            }
            else
            {
                for (int c=0; c<("4455667665544"[r]-'0'); ++c)
                {
                    if (hasRubber[1][posIndex])      std::printf("\033[0m%c\033[37m", label[posIndex]);
                    else if (hasRubber[2][posIndex]) std::printf("\033[0m%c\033[37m", label[posIndex]);
                    else if (hasRubber[4][posIndex]) std::printf("\033[0m%c\033[37m", label[posIndex]);
                    else                             putchar(label[posIndex]);
                    putchar(' ');
                    posIndex += 1;
                }
            }
            putchar('\n');
        }
        constexpr const char str[3*16][3] = {"ad", "eh","fi", "jm","kn","lo", "ps","qt","ru","sv", "wz","xA","yB", "CF","DG", "HK",
                                             "ap", "bq","fw", "cr","gx","lC", "ds","hy","mD","sH", "iz","nE","tI", "oF","uJ", "vK",
                                             "pH", "jD","qI", "ey","kE","rJ", "as","fz","lF","sK", "bt","gA","mG", "cu","hB", "dv"
                                            };
        std::printf("captured: %d %d %d %d\n Remaining moves: ", captured[1], captured[2], captured[3], captured[4]);
        for (int i=0; i<remainingMoves.size(); ++i)
            if (remainingFreeEdges[i])
                std::printf("\033[0;32m%s \033[0m", str[remainingMoves[i]]);
            else
                std::printf("\033[0;31m%s \033[0m", str[remainingMoves[i]]);
    }
};


struct TrigglePlayerBase
{
    virtual TriggleBoard::Move selectMove(const TriggleBoard&) = 0;
    virtual ~TrigglePlayerBase() = default;
};

struct TrigglePlayer : TrigglePlayerBase
{
    TriggleBoard::Move selectMove([[maybe_unused]] const TriggleBoard& original) override
    {
        std::string moveStr;
        std::cout << "Enter move (e.g., \"qt\"): ";
        std::cin >> moveStr;
        uint16_t from = (moveStr[1]-'a') & 0x3F;
        uint16_t to   = (moveStr[0]-'a') & 0x3F;
        if (from>to) std::swap(from, to);
        const uint16_t edge = (from << 8) | to;
        const auto it = std::find(TriggleBoard::edgeIds.begin(), TriggleBoard::edgeIds.end(), edge);
        if (it != TriggleBoard::edgeIds.end())
        {
            return it - TriggleBoard::edgeIds.begin();
        }
        return 65535;
    }
};


struct TriggleAiMCTS : TrigglePlayerBase
{
    using PatternType = int; // unused for now
    static constexpr int MaxPatterns = 0; // unused for now
    Ai_ctx<15000, TriggleBoard::Move, UQWORD, PatternType, MaxPatterns> ai_ctx;

    TriggleBoard::Move selectMove([[maybe_unused]] const TriggleBoard& original) override
    {
        constexpr int simDepth = 13;
        constexpr int minimaxDepth = 2;
        const auto res = mcts<1500, simDepth, minimaxDepth, TriggleBoard::Move, UQWORD>(
            original, ai_ctx, []{ return pcgRand<UDWORD>(); });
        return res.best;
    }
};






int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    pcg32rand(0x696969ull);

    std::printf("Starting\n");
    TrigglePlayer h1, h2, h3, h4;
    TriggleAiMCTS ai1, ai2;
    TrigglePlayerBase *players[3] = { nullptr, &h1, &ai2 };
    int scores[5] = {0,0,0,0,0};
    TriggleBoard triggleBoard;
    triggleBoard.reset();
    triggleBoard.printBoard();
    Outcome running = Outcome::running;
    while (running == Outcome::running)
    {
        const int currentPlayer = triggleBoard.getCurrentPlayer();
        std::printf("player %d - score: %d %d \n", currentPlayer, 0, 0);
        const TriggleBoard::Move mv = players[currentPlayer]->selectMove(triggleBoard);
        if (players[currentPlayer]==&h1 || players[currentPlayer]==&h2 || players[currentPlayer]==&h3 || players[currentPlayer]==&h4)
        {
            TriggleBoard::StorageForMoves availMoves;
            const int numMoves = triggleBoard.generateMovesAndGetCnt(availMoves);
            bool isValid = false;
            for (int i = 0; i < numMoves; ++i) {
                if (availMoves[i] == mv) {
                    isValid = true;
                    break;
                }
            }
            if (!isValid)
            {
                std::printf("invalid move: %d \n", mv);
                exit(1);
            }
        }

        running = triggleBoard.doMove(mv);
        if (running == Outcome::fin)
        {
            scores[currentPlayer] += 1;
            std::printf("\033[1;3%dmwinner: %d \033[0m \n", currentPlayer+1, currentPlayer);
        }
        else if (running == Outcome::draw)
        {
            std::printf("\033[0;34mdraw \033[0m \n");
        }
        triggleBoard.switchPlayer();
        triggleBoard.printBoard();
    }
    triggleBoard.reset();
    sleep(1);
}
