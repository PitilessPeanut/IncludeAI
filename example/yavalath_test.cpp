#define INCLUDEAI_IMPLEMENTATION
#include <algorithm>
#include <bitset>
#include <assert.h>
#include <cstdio>
#include "../includeai.hpp"

using namespace include_ai;


char gaem_scan2([[maybe_unused]] void *pEverything)
{
    char c[10];
    [[maybe_unused]] const auto x = std::scanf("%s", c);
    return c[0];
}




class YavalathBoard
{
public:
    static constexpr int CELLS = 61;
    using AvailMoves = include_ai::Move[CELLS];
    unsigned char pos[11*11] =
        {0, 0,  0,  0,  0,  0,  0,  0,  0,  0, 0,
         0, 0,  0,  0,  0, 'a','b','c','d','e',0,
         0, 0,  0,  0, 'f','g','h','i','j','k',0,
         0, 0,  0, 'l','m','n','o','p','q','r',0,
         0, 0, 's','t','u','v','w','x','y','z',0,
         0,'A','B','C','D','E','F','G','H','I',0,
         0,'J','K','L','M','N','O','P','Q', 0, 0,
         0,'R','S','T','U','V','W','X', 0,  0, 0,
         0,'Y','Z','1','2','3','4', 0,  0,  0, 0,
         0,'5','6','7','8','9', 0,  0,  0,  0, 0,
         0, 0,  0,  0,  0,  0,  0,  0,  0,  0, 0
        };
private:
    int currentPlayer = 1, winner;
public:
    std::bitset<128> openpos;
public:
    constexpr YavalathBoard() {}
    YavalathBoard(const YavalathBoard&) = delete;
    YavalathBoard& operator=(const YavalathBoard&) = delete;
    YavalathBoard(YavalathBoard&&) = default;

    constexpr YavalathBoard clone() const
    {
        YavalathBoard dst;
        for (int i=15; i<(11*11); ++i)
        {
            dst.pos[i] = pos[i];
        }
        dst.currentPlayer = currentPlayer;
        dst.openpos = openpos;
        return dst;
    }

    int generateMovesAndGetCnt(include_ai::Move *availMoves)
    {
        int availMovesCtr = 0;
        for (int i=15; i<(11*11); ++i)
        {
            // find valid moves:
            const bool open = openpos[i];// == 1;
            if ((pos[i]>'0') && (pos[i]<248) && open)
                availMoves[availMovesCtr++] = i;
        }
        // Don't do this: "availMovesCtr -= 1"
        return availMovesCtr;
    }

    Outcome doMove(const Move mv)
    {
        const unsigned char currentId = currentPlayer+248;
        pos[mv] = currentId;
        constexpr decltype(mv) z=0, edge=11*11;
        openpos[std::clamp(mv-22, z, edge)] = true;
        openpos[std::clamp(mv-21, z, edge)] = true;
        openpos[std::clamp(mv-20, z, edge)] = true;
        openpos[std::clamp(mv-12, z, edge)] = true;
        openpos[std::clamp(mv-11, z, edge)] = true;
        openpos[std::clamp(mv-10, z, edge)] = true;
        openpos[std::clamp(mv- 9, z, edge)] = true;
        openpos[std::clamp(mv- 2, z, edge)] = true;
        openpos[std::clamp(mv- 1, z, edge)] = true;
        openpos[std::clamp(mv+ 1, z, edge)] = true;
        openpos[std::clamp(mv+ 2, z, edge)] = true;
        openpos[std::clamp(mv+ 9, z, edge)] = true;
        openpos[std::clamp(mv+10, z, edge)] = true;
        openpos[std::clamp(mv+11, z, edge)] = true;
        openpos[std::clamp(mv+12, z, edge)] = true;
        openpos[std::clamp(mv+20, z, edge)] = true;
        openpos[std::clamp(mv+21, z, edge)] = true;
        openpos[std::clamp(mv+22, z, edge)] = true;
        int haveThreeHoriz=0, haveFourHoriz=0, haveThreeVert=0, haveFourVert=0, haveThreeDiag=0, haveFourDiag=0;
        int loser=0;
        for (int y=1; y<11; ++y)
        for (int x=1; x<11; ++x)
        {
            unsigned char unit = pos[(y*11)+x];
            if (unit < 248)
                continue;
            haveThreeHoriz = (unit&currentId) == currentId;
            haveThreeHoriz += (pos[(y*11)+x+1]&currentId) == currentId;
            haveThreeHoriz += (pos[(y*11)+x+2]&currentId) == currentId;
            haveFourHoriz = ((pos[(y*11)+x+3]&currentId) == currentId) + haveThreeHoriz;

            haveThreeVert = (unit&currentId) == currentId;
            haveThreeVert += (pos[((y+1)*11)+x]&currentId) == currentId;
            haveThreeVert += (pos[((y+2)*11)+x]&currentId) == currentId;
            haveFourVert = ((pos[((y+3)*11)+x]&currentId) == currentId) + haveThreeVert;

            haveThreeDiag = (unit&currentId) == currentId;
            haveThreeDiag += (pos[((y+1)*11)+x-1]&currentId) == currentId;
            haveThreeDiag += (pos[((y+2)*11)+x-2]&currentId) == currentId;
            haveFourDiag = ((pos[((y+3)*11)+x-3]&currentId) == currentId) + haveThreeDiag;

            if ((haveFourHoriz==4) + (haveFourVert==4) + (haveFourDiag==4))
            {
                winner = currentPlayer;
                // winning has priority over losing:
                return Outcome::fin;
            }
            else if ((haveThreeHoriz==3) + (haveThreeVert==3) + (haveThreeDiag==3))
            {
                loser = currentPlayer;
            }
        }
        if (loser)
        {
            winner = 3-loser;
            return Outcome::fin;
        }

        return Outcome::running;
    }

    void switchPlayer() { currentPlayer=3-currentPlayer; }

    int getCurrentPlayer() const { return currentPlayer; }

    int getWinner() const { return winner; }

    void reset()
    {}
};


struct YavalathPlayerBase
{
    int score = 0;
    virtual Move selectMove(const YavalathBoard&, const int) = 0;
    virtual ~YavalathPlayerBase() = default;
};

struct YavalathPlayer : YavalathPlayerBase
{
    Move selectMove([[maybe_unused]] const YavalathBoard& original, [[maybe_unused]] const int input) override
    {
        int i=15;
        for (; i<(11*11); ++i)
            if (original.pos[i] == input)
                break;
        return Move{i};
    }
};

struct YavalathAiMCTS : YavalathPlayerBase
{
    bool fresh = true;
    Ai_ctx<90000, UQWORD> ai_ctx;

    Move selectMove([[maybe_unused]] const YavalathBoard& original, [[maybe_unused]] const int input) override
    {
        if (fresh)
        {
            fresh = false;
            return 121/2;
        }
        const MCTS_result res =
            mcts<90000, 1400, 30, 3, UQWORD>(original, ai_ctx, []{return pcgRand<UDWORD>();});
        return res.move;
    }
};








int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    YavalathPlayer h1, h2;
    YavalathAiMCTS a1;
    typedef char (*fnReadInput)();
    fnReadInput readInput[3] = { nullptr,
                                 []() { return gaem_scan2(nullptr); },
                                 //  []() { return gaem_scan2(nullptr); },
                                 []() { return '\0'; /* this will be ignored by ai */ },
                               };
    YavalathPlayerBase *players[3] = { nullptr, &h1, &a1 };

    YavalathBoard yavalathBoard;
    yavalathBoard.switchPlayer(); // switch back to player 1 after start

    Outcome running = Outcome::running;
    goto start;
    while (running == Outcome::running)
    {
        {
            // prommpt:
            const int currentPlayer = yavalathBoard.getCurrentPlayer();
            std::printf("player %d - score: %d %d - rnd: %d\n", currentPlayer, players[1]->score, players[2]->score, 666);
            const char sel = readInput[currentPlayer](); // Player only, ignored by ai
            const Move mv = players[currentPlayer]->selectMove(yavalathBoard, sel);
            running = yavalathBoard.doMove(mv);
            if (running == Outcome::invalid)
            {
                std::printf("invalid move: %llu \n", mv);
                continue;
            }
            else if (running == Outcome::fin)
            {
                //players[currentPlayer]->score += 1;
                const auto w = yavalathBoard.getWinner();
                std::printf("\033[1;3%dmwinner: %d \033[0m \n", w+1, w);
            }
            else if (running == Outcome::draw)
            {
                std::printf("\033[0;34mdraw \033[0m \n");
            }
        } // close scope before 'goto' label

        start:
        // draw:
        for (int y=1; y<6; ++y)
        {
            for (int x=0; x<y; ++x)
                std::putchar(' ');
            for (int x=1; x<11; ++x)
            {
                const auto idx = (y*11)+x;
                auto c = yavalathBoard.pos[idx];
                if (c>=(248+1))
                {
                    c = c==(248+1) ? 'X' : 'O';
                    std::printf("\033[1;3%dm", c=='X' ? 1 : 4);
                }
                else if (yavalathBoard.openpos[idx]) //((yavalathBoard.openpos[idx>>6] >> (idx&63)) & 1)
                    ;
                else
                    c = '.' * !!c;
                std::putchar(c ? c : ' ');
                std::printf("\033[0m");
                std::putchar(' ');
            }
            std::putchar('\n');
        }
        for (int y=6; y<11; ++y)
        {
            for (int x=0; x<y; ++x)
                std::putchar(' ');
            for (int x=1; x<11; ++x)
            {
                const auto idx = (y*11)+x;
                auto c = yavalathBoard.pos[idx];
                if (c>=(248+1))
                {
                    c = c==(248+1) ? 'X' : 'O';
                    std::printf("\033[1;3%dm", c=='X' ? 1 : 4);
                }
                else if (yavalathBoard.openpos[idx]) // ((yavalathBoard.openpos[idx>>6] >> (idx&63)) & 1)
                    ;
                else
                    c = '.' * !!c;
                std::putchar(c ? c : ' ');
                std::printf("\033[0m");
                std::putchar(' ');
            }
            std::putchar('\n');
        }

        yavalathBoard.switchPlayer();
    }
    yavalathBoard.reset();
}
