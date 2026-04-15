#include <cstdio>
#define INCLUDEAI_IMPLEMENTATION
#define AI_DEBUG
#include "../src/micro_math.hpp"
#include "../src/ai.hpp"
#include <vector>


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
    using StorageForMoves = Move[9];
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
    TicTacBoard& operator=(TicTacBoard&&) = default;

    constexpr TicTacBoard clone() const
    {
        TicTacBoard dst;
        for (int i=0; i<9; ++i) { dst.pos[i] = pos[i]; }
        dst.currentPlayer = currentPlayer;
        dst.turn = turn;
        return dst;
    }

    int generateMovesAndGetCnt(Move *availMoves) const
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

    float getBoardScore() const { return 0.f; }

    float *getNetworkInputs() { return nullptr; }

    void randomize() {}

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
    static constexpr int simDepth=0, minimaxDepth=9;
    struct NeuralDummy
    {
        FLOAT x;
        FLOAT *evaluate(const FLOAT *inputs, const FLOAT boardScore)
        {
            x = boardScore;
            return &x;
        }
    } dummy_nn;
    TicTacBoard::Move selectMove([[maybe_unused]] const TicTacBoard& original, [[maybe_unused]] const int input) override
    {
        const auto res =
            mcts<150, simDepth, minimaxDepth, int, UQWORD>(original, ai_ctx, dummy_nn, []{return pcgRand<UDWORD>();});
        return res.best;
    }
};




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
                        //if (running == Outcome::invalid)
                        //{
                        //    std::printf("invalid move: %d \n", mv);
                        //    continue;
                        //}
                        if (running == Outcome::fin)
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
