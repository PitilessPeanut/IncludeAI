#define INCLUDEAI_IMPLEMENTATION
#include <vector>
#include <algorithm>
#include <bitset>
#include <cstdio>
#include <unistd.h>
#include "../includeai.hpp"

using namespace include_ai;


char input_scan2([[maybe_unused]] void *pEverything)
{
    char c[10];
    [[maybe_unused]] const auto x = std::scanf("%s", c);
    return c[0];
}




class YavalathBoard
{
public:
    static constexpr int CELLS = 61;
    using YavMove = int;
    using StorageForMoves = YavMove[CELLS];
    unsigned char board[11*11] =
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
    Pattern<CELLS+CELLS> patternInputs;
    int getNextPatternCtr = 0;
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
            dst.board[i] = board[i];
        }
        dst.currentPlayer = currentPlayer;
        dst.openpos = openpos;
        return dst;
    }

    int generateMovesAndGetCnt(YavMove *availMoves)
    {
        int availMovesCtr = 0;
        for (int i=15; i<(11*11); ++i)
        {
            // find valid moves:
            const bool open = openpos[i];
            if ((board[i]>'0') && (board[i]<248) && open)
                availMoves[availMovesCtr++] = i;
        }
        // Don't do this: "availMovesCtr -= 1"
        return availMovesCtr;
    }

    Outcome doMove(const YavMove mv)
    {
        const unsigned char currentId = currentPlayer+248;
        board[mv] = currentId;
        constexpr decltype(mv) z=0, edge=11*11;
        openpos[std::clamp(mv-33, z, edge)] = true;
        openpos[std::clamp(mv-30, z, edge)] = true;
        openpos[std::clamp(mv-22, z, edge)] = true;
        openpos[std::clamp(mv-21, z, edge)] = true;
        openpos[std::clamp(mv-20, z, edge)] = true;
        openpos[std::clamp(mv-12, z, edge)] = true;
        openpos[std::clamp(mv-11, z, edge)] = true;
        openpos[std::clamp(mv-10, z, edge)] = true;
        openpos[std::clamp(mv- 9, z, edge)] = true;
        openpos[std::clamp(mv- 3, z, edge)] = true;
        openpos[std::clamp(mv- 2, z, edge)] = true;
        openpos[std::clamp(mv- 1, z, edge)] = true;
        openpos[std::clamp(mv+ 1, z, edge)] = true;
        openpos[std::clamp(mv+ 2, z, edge)] = true;
        openpos[std::clamp(mv+ 3, z, edge)] = true;
        openpos[std::clamp(mv+ 9, z, edge)] = true;
        openpos[std::clamp(mv+10, z, edge)] = true;
        openpos[std::clamp(mv+11, z, edge)] = true;
        openpos[std::clamp(mv+12, z, edge)] = true;
        openpos[std::clamp(mv+20, z, edge)] = true;
        openpos[std::clamp(mv+21, z, edge)] = true;
        openpos[std::clamp(mv+22, z, edge)] = true;
        openpos[std::clamp(mv+30, z, edge)] = true;
        openpos[std::clamp(mv+33, z, edge)] = true;
        int haveThreeHoriz=0, haveFourHoriz=0, haveThreeVert=0, haveFourVert=0, haveThreeDiag=0, haveFourDiag=0;
        int loser=0;
        int drawCntr = 0;
        for (int y=1; y<11; ++y) {
        for (int x=1; x<11; ++x)
        {
            unsigned char unit = board[(y*11)+x];
            if (unit < 248)
                continue;
            else
                drawCntr += 1;
            haveThreeHoriz = (unit&currentId) == currentId;
            haveThreeHoriz += (board[(y*11)+x+1]&currentId) == currentId;
            haveThreeHoriz += (board[(y*11)+x+2]&currentId) == currentId;
            haveFourHoriz = ((board[(y*11)+x+3]&currentId) == currentId) + haveThreeHoriz;

            haveThreeVert = (unit&currentId) == currentId;
            haveThreeVert += (board[((y+1)*11)+x]&currentId) == currentId;
            haveThreeVert += (board[((y+2)*11)+x]&currentId) == currentId;
            haveFourVert = ((board[((y+3)*11)+x]&currentId) == currentId) + haveThreeVert;

            haveThreeDiag = (unit&currentId) == currentId;
            haveThreeDiag += (board[((y+1)*11)+x-1]&currentId) == currentId;
            haveThreeDiag += (board[((y+2)*11)+x-2]&currentId) == currentId;
            haveFourDiag = ((board[((y+3)*11)+x-3]&currentId) == currentId) + haveThreeDiag;

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
        }}
        if (loser)
        {
            winner = 3-loser;
            return Outcome::fin;
        }
        else if (drawCntr == CELLS)
        {
            return Outcome::draw;
        }

        return Outcome::running;
    }

    void switchPlayer() { currentPlayer=3-currentPlayer; }

    int getCurrentPlayer() const { return currentPlayer; }

    int getWinner() const { return winner; }

    Pattern<CELLS+CELLS> *getNextPattern()
    {
        if (getNextPatternCtr == 1)
        {
            getNextPatternCtr = 0;
            return nullptr;
        }
        getNextPatternCtr = 1;

        constexpr int indeces[CELLS] = {
                                16, 17, 18, 19, 20,
                            26, 27, 28, 29, 30, 31,
                        36, 37, 38, 39, 40, 41, 42,
                    46, 47, 48, 49, 50, 51, 52, 53,
                56, 57, 58, 59, 60, 61, 62, 63, 64,
                67, 68, 69, 70, 71, 72, 73, 74,
                78, 79, 80, 81, 82, 83, 84,
                89, 90, 91, 92, 93, 94,
                100,101,102,103,104
            };
        for (int i=0; i<CELLS; ++i)
        {
            patternInputs.pattern[i      ] = board[indeces[i]] == (currentPlayer+248);
            patternInputs.pattern[i+CELLS] = board[indeces[i]] != (currentPlayer+248);
        }
        return &patternInputs;
    }

    void reset() {}
};


struct YavalathPlayerBase
{
    virtual char readInput() const = 0;
    virtual YavalathBoard::YavMove selectMove(const YavalathBoard&, const int) = 0;
    virtual ~YavalathPlayerBase() = default;
};

struct YavalathPlayer : YavalathPlayerBase
{
    char readInput() const override { return input_scan2(nullptr); }

    YavalathBoard::YavMove selectMove([[maybe_unused]] const YavalathBoard& original, [[maybe_unused]] const int input) override
    {
        for (int i=15; i<(11*11); ++i)
            if (original.board[i] == input)
                return YavalathBoard::YavMove{i};
    }
};

struct YavalathAiMCTS : YavalathPlayerBase
{
    bool fresh = true;
    using PatternType = Pattern<2*YavalathBoard::CELLS>;
    static constexpr int MaxPatterns = 60;
    Ai_ctx<210000, YavalathBoard::YavMove, UQWORD, PatternType, MaxPatterns> ai_ctx;

    char readInput() const override { return '\0'; /* this does nothing */ }

    YavalathBoard::YavMove selectMove([[maybe_unused]] const YavalathBoard& original, [[maybe_unused]] const int input) override
    {
        if (fresh)
        {
            fresh = false;
            return pcgRand<UDWORD>() % 121;
        }
        constexpr int simDepth=21, minimaxDepth=3;
        const MCTS_result<YavalathBoard::YavMove> res =
            mcts<1800, simDepth, minimaxDepth, YavalathBoard::YavMove, UQWORD>(original, ai_ctx, []{return pcgRand<UDWORD>();});
        std::printf("simulations: %d, minimaxes: %d \n", res.statistics[MCTS_result<YavalathBoard::YavMove>::simulations], res.statistics[MCTS_result<YavalathBoard::YavMove>::minimaxes]);
        return res.best;
    }
};








int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    YavalathPlayer h1, h2;
    std::vector<YavalathAiMCTS> ai(2);
    int scores[3] = {0, 0, 0};
    int draws = 0;

    for (int tests=0; tests<900; ++tests)
    {
        YavalathPlayerBase *players[3] = { nullptr, &h1, &ai[1] };

        YavalathBoard yavalathBoard;
        yavalathBoard.switchPlayer(); // switch back to player 1 after start

        Outcome running = Outcome::running;
        goto start;
        while (running == Outcome::running)
        {
            {
                // prompt:
                const int currentPlayer = yavalathBoard.getCurrentPlayer();
                std::printf("player %d - score: %d %d - rnd: %d\n", currentPlayer, scores[1], scores[2], tests);
                [[maybe_unused]] const char sel = players[currentPlayer]->readInput();
                const YavalathBoard::YavMove mv = players[currentPlayer]->selectMove(yavalathBoard, sel);
                running = yavalathBoard.doMove(mv);
                if (running == Outcome::invalid)
                {
                    std::printf("invalid move: %d \n", mv);
                    continue;
                }
                else if (running == Outcome::fin)
                {
                    const auto w = yavalathBoard.getWinner();
                    scores[w] += 1;
                    std::printf("\033[1;3%dmwinner: %d \033[0m scre: %d %d, %d\n", w+1, w, scores[1], scores[2], draws);
                }
                else if (running == Outcome::draw)
                {
                    draws += 1;
                    std::printf("\033[0;34mdraw \033[0m \n");
                }

                if (tests < 300)
                {
                    ai[0].ai_ctx.collectPattern(yavalathBoard);
                    ai[1].ai_ctx.collectPattern(yavalathBoard);
                }
                else
                {
                    ai[0].ai_ctx.train(yavalathBoard);
                    ai[1].ai_ctx.train(yavalathBoard);
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
                    auto c = yavalathBoard.board[idx];
                    if (c>=(248+1))
                    {
                        c = c==(248+1) ? 'X' : 'O';
                        std::printf("\033[1;3%dm", c=='X' ? 1 : 4);
                    }
                    else if (yavalathBoard.openpos[idx])
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
                    auto c = yavalathBoard.board[idx];
                    if (c>=(248+1))
                    {
                        c = c==(248+1) ? 'X' : 'O';
                        std::printf("\033[1;3%dm", c=='X' ? 1 : 4);
                    }
                    else if (yavalathBoard.openpos[idx])
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
        ai[0].fresh=true; ai[1].fresh=true;
        sleep(7);
    }

}
