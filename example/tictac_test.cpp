#include "../src/ai.hpp"
#include "../src/micro_math.hpp" // pcg
#include <cstdio>
#include <vector>
#include <assert.h>

using namespace include_ai;

int gaem_scan([[maybe_unused]] void *pEverything)
{
    int i;
    [[maybe_unused]] const auto x = std::scanf("%d", &i);
    return i;
}




class TicTacBoard
{
public:
    using Move = int;
    using AvailMoves = Move[9];
    unsigned char pos[9+1] = {0,0,0,
                              0,0,0,
                              0,0,0
                             };
    int currentPlayer=1, winner=0, turn=0;
public:
    constexpr TicTacBoard() {}
    TicTacBoard(const TicTacBoard&) = delete;
    TicTacBoard& operator=(const TicTacBoard&) = delete;
    TicTacBoard(TicTacBoard&&) = default;

    constexpr TicTacBoard clone() const
    {
        TicTacBoard dst;
       	for (int i=0; i<9; ++i)
   	        dst.pos[i] = pos[i];
       	dst.currentPlayer = currentPlayer;
        dst.turn = turn;
        return dst;
    }

    int generateMovesAndGetCnt(Move *availMoves)
    {
        int availMovesCtr = 0;
        for (int i=0; i<9; ++i)
        {
            // find valid moves:
            if (pos[i]==0)
                availMoves[availMovesCtr++] = i;
        }
        // Don't do this: "availMovesCtr -= 1"
        return availMovesCtr;
    }

    Outcome doMove(const Move mv)
    {
        if (pos[mv]) return Outcome::invalid;
        turn += 1;
        pos[mv] = currentPlayer;

        int win = pos[0] && (pos[0]==pos[1]) && (pos[0]==pos[2]);
        win += pos[3] && (pos[3]==pos[4]) && (pos[3]==pos[5]);
        win += pos[6] && (pos[6]==pos[7]) && (pos[6]==pos[8]);

        win += pos[0] && (pos[0]==pos[3]) && (pos[0]==pos[6]);
        win += pos[1] && (pos[1]==pos[4]) && (pos[1]==pos[7]);
        win += pos[2] && (pos[2]==pos[5]) && (pos[2]==pos[8]);

        win += pos[0] && (pos[0]==pos[4]) && (pos[0]==pos[8]);
        win += pos[2] && (pos[2]==pos[4]) && (pos[2]==pos[6]);

        if (win >= 1)
        {
            winner = currentPlayer;
            return Outcome::fin;
        }
        if (turn >= 9) return Outcome::draw;
        return Outcome::running;
    }

    void switchPlayer() { currentPlayer=3-currentPlayer; }

    int getCurrentPlayer() const { return currentPlayer; }

    int getWinner() const { return winner; }

    void reset()
    {
        pos[0]=0; pos[1]=0; pos[2]=0;
        pos[3]=0; pos[4]=0; pos[5]=0;
        pos[6]=0; pos[7]=0; pos[8]=0;
        currentPlayer = 1;
        turn = 0;
        winner = 0;
    }
};


struct TicTacPlayerBase
{
    virtual TicTacBoard::Move selectMove(const TicTacBoard&, const int) = 0;
    virtual ~TicTacPlayerBase() = default;
};

struct TicTacPlayer : TicTacPlayerBase
{
    TicTacBoard::Move selectMove([[maybe_unused]] const TicTacBoard& original, [[maybe_unused]] const int input) override
    {
        return TicTacBoard::Move{input};
    }
};


struct TicTacAiMCTS : TicTacPlayerBase
{
    Ai_ctx<13000, TicTacBoard::Move, UQWORD> ai_ctx;

    TicTacBoard::Move selectMove([[maybe_unused]] const TicTacBoard& original, [[maybe_unused]] const int input) override
    {
        /*
            float simulate(const Board *original) const override
    	    {
                TicTacBoard clone;
                original->cloneInto(&clone);
                //clone.switchPlayer();
                int nMoves=clone.generateMovesAndGetCnt();
                nMoves -= 1;
                int yyy = nMoves;
                int minimaxScore = -2;
                int ggg1[12] = {0};
                int ggg2[12] = {0};
                while (nMoves>=0)
                {
                    const Move moveHere = clone.getMove(nMoves);
                    ggg1[nMoves] = moveHere;
                    const int mnx = -minimax(clone, moveHere);
                    ggg2[nMoves] = mnx;
                    if (mnx>minimaxScore)
                        minimaxScore = mnx;
                    nMoves -= 1;
                }

                for (; yyy>=0; yyy -= 1)
                    std::printf("mv: %d mx: %d \n", ggg1[yyy], ggg2[yyy]);

                return minimaxScore;
            }


        } randRoll(original.getCurrentPlayer());*/
        const auto res =
            mcts<13000, 1633, 9,9, int, UQWORD>(original, ai_ctx, []{return pcgRand<UDWORD>();});
        return res.move;
    }
};

/*
struct TicTacAiMinimax : TicTacPlayerBase
{
    SWORD minimax(const TicTacBoard& prev, const TicTacBoard::Move mv)
    {
        TicTacBoard clone;
        prev.cloneInto(&clone);
        clone.switchPlayer();
        const auto outcome = clone.doMove(mv);
        if (outcome!=include_ai::Outcome::running)
      	{
            if (clone.getCurrentPlayer() == clone.getWinner())
                return -1;
            else
                return 1;
        }
        int nMoves = clone.generateMovesAndGetCnt();
        nMoves -= 1;
        int minimaxScore = -2;
        while (nMoves > -1)
        {
            const TicTacBoard::Move moveHere = clone.getMove(nMoves);
            const int mnx = -minimax(clone, moveHere);
            if (mnx>minimaxScore)
                minimaxScore = mnx;
            nMoves -= 1;
        }
        return minimaxScore==-2 ? 0 : minimaxScore;
    }

    TicTacBoard::Move selectMove([[maybe_unused]] const TicTacBoard& original, [[maybe_unused]] const int input) override
    {
        TicTacBoard clone = original.clone();
        clone.switchPlayer();
        TicTacBoard::AvailMoves availMoves;
        int nMoves=clone.generateMovesAndGetCnt(availMoves);
        nMoves -= 1;
        int minimaxScore = -2;
        SWORD selMove=availMoves[0];
        while (nMoves >= 0)
        {
            const TicTacBoard::Move moveHere = availMoves[nMoves];
            const int mnx = -minimax(clone, moveHere);
            if (mnx>minimaxScore)
            {
                minimaxScore = mnx;
                selMove = moveHere;
            }
            nMoves -= 1;
            std::printf("%d %d \n", moveHere , mnx);
        }
        std::putchar(clone.pos[0]+'0');std::putchar(clone.pos[1]+'0');std::putchar(clone.pos[2]+'0');std::putchar('\n');
        std::putchar(clone.pos[3]+'0');std::putchar(clone.pos[4]+'0');std::putchar(clone.pos[5]+'0');std::putchar('\n');
        std::putchar(clone.pos[6]+'0');std::putchar(clone.pos[7]+'0');std::putchar(clone.pos[8]+'0');std::putchar('\n');
        std::putchar(clone.getCurrentPlayer()+'0');std::putchar('\n');
        return selMove;
    }
};*/




class World
{
private:
    TicTacPlayer h1, h2;
    TicTacAiMCTS ai1, ai2;
  //  TicTacAiMinimax ai3;
    typedef int (*fnReadInput)();
    fnReadInput readInput[3] = { nullptr,
                                 []() { return gaem_scan(nullptr); },
                                 []() { return 0; /* this will be ignored by aid */ },
                               };
    TicTacPlayerBase *players[3] = { nullptr, &h1, &ai2 };
public:
    World() { pcg32rand(0x696969ull); }

    void step()
    {
        TicTacBoard ticTacBoard;

        for (int mutations=0; mutations<1; ++mutations)
        {
            for (int swapped=0; swapped<2; ++swapped)
            {
                constexpr int MaxRounds = 296;
                for (int round=0; round<MaxRounds; ++round)
                {
                    Outcome running = Outcome::running;
                    while (running == Outcome::running)
                    {
                        const int currentPlayer = ticTacBoard.getCurrentPlayer();
                        std::printf("player %d - score: %d %d - rnd: %d\n", currentPlayer, 0, 0, round);
                        const int sel = readInput[currentPlayer](); // Player only, ignored by ai

                        if (false) //sel<1 || sel>=9)
                        {
                            std::printf("exiting...\n");
                            return;
                        }
                        const TicTacBoard::Move mv = players[currentPlayer]->selectMove(ticTacBoard, sel);
                        std::printf("  playing: %d \n", mv);
                        running = ticTacBoard.doMove(mv);
                        if (running == Outcome::invalid)
                        {
                            std::printf("invalid move: %d \n", mv);
                            continue;
                        }
                        else if (running == Outcome::fin)
                        {
                            //players[currentPlayer]->score += 1;
                            std::printf("\033[1;3%dmwinner: %d \033[0m \n", currentPlayer+1, currentPlayer);
                        }
                        else if (running == Outcome::draw)
                        {
                            std::printf("\033[0;34mdraw \033[0m \n");
                        }
                        ticTacBoard.switchPlayer();
                        #define PUT(x) std::putchar(ticTacBoard.pos[(x)]+'0')
                        PUT(0); PUT(1); PUT(2); std::putchar('\n');
                        PUT(3); PUT(4); PUT(5); std::putchar('\n');
                        PUT(6); PUT(7); PUT(8); std::putchar('\n');
                        #undef PUT
                    }
                    ticTacBoard.reset();
                }
                players[1]=&ai2; players[2]=&ai1; // swap to ensure both AIs play each side
                std::printf("swapping\n");
            }
            //mutate!
        }
    }
};








int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    std::vector<World> world(1);
    world[0].step();
}



// todo: sign tdoc
