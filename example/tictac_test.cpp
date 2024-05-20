#include "../src/ai.hpp"
#include "../src/micro_math.hpp"
#include <cstdio>
#include <vector>


int gaem_scan([[maybe_unused]] void *pEverything)
{
    int i;
    [[maybe_unused]] const auto x = std::scanf("%d", &i);
    return i;
}




class TicTacBoard : public Board
{
public:
    unsigned char pos[9+1] = {0};
    int currentPlayer = 1, turn = 0;
private:
    Move availMoves[9]; 
public:
    TicTacBoard() {}
    TicTacBoard(const TicTacBoard&) = delete;
    TicTacBoard& operator=(const TicTacBoard&) = delete;

    void cloneInto(Board *dst) const override
    {
    	TicTacBoard& clone = *(TicTacBoard *)dst;
    	for (int i=0; i<9; ++i)
    	    clone.pos[i] = pos[i];
    	clone.currentPlayer = currentPlayer;
    	clone.turn = turn;
    }

    int getMovesCnt() override
    {
        int availMovesCtr = 0;
        for (int i=0; i<9; ++i) 
        {
            // find valid moves:
            if (pos[i]==0)
                availMoves[availMovesCtr++] = Move{i, nullptr};
        }
        return availMovesCtr;
    }

    Move getMove(int idx) override
    {
        return availMoves[ idx ];
    }

    Board::Outcome doMove(const Move mv) override
    {
        if (pos[ mv.moveIdx ])
            return Outcome::invalid;
        turn += 1;
        pos[mv.moveIdx] = currentPlayer;
        int win = pos[0] && (pos[0]==pos[1]) && (pos[0]==pos[2]);
        win += pos[3] && (pos[3]==pos[4]) && (pos[3]==pos[5]);
        win += pos[6] && (pos[6]==pos[7]) && (pos[6]==pos[8]);

        win += pos[0] && (pos[0]==pos[3]) && (pos[0]==pos[6]);
        win += pos[1] && (pos[1]==pos[4]) && (pos[1]==pos[7]);
        win += pos[2] && (pos[2]==pos[5]) && (pos[2]==pos[8]);

        win += pos[0] && (pos[0]==pos[4]) && (pos[0]==pos[8]);
        win += pos[2] && (pos[2]==pos[4]) && (pos[2]==pos[6]);
        if (win == 1) return Outcome::fin;
        if (turn >= 9) return Outcome::draw;
        return Outcome::running;
    }

    void switchPlayer() override { currentPlayer=3-currentPlayer; }

    int getCurrentPlayer() const override { return currentPlayer; }

    void reset()
    {
        pos[0]=0; pos[1]=0; pos[2]=0;
        pos[3]=0; pos[4]=0; pos[5]=0;
        pos[6]=0; pos[7]=0; pos[8]=0;
        currentPlayer = 1;
        turn = 0;
    }
};


struct TicTacPlayerBase
{
    int score = 0;
    virtual Move selectMove(const TicTacBoard&, const int) = 0;
    virtual ~TicTacPlayerBase() = default;
};

struct TicTacPlayer : TicTacPlayerBase
{
    Move selectMove([[maybe_unused]] const TicTacBoard& original, [[maybe_unused]] const int input) override
    {
        return Move{input, nullptr};
    }
};

template <int ExplC, UQWORD hyperparams>
inline constexpr MCTS_result selectMCTS(Board *boardEmpty, const Board *boardOriginal, Ai_ctx<500, UQWORD>& ai_ctx, const Simulator *simulator)
{

    #define SEL_MCTS_CUTOFF(base,fract,cntvis) \
      if constexpr ((cutoff_scoring&hyperparams)==cutoff_scoring)\
          return mcts_500_1500_##base##p##fract##cntvis##c__u64(boardEmpty, boardOriginal, ai_ctx, simulator);\
      else\
          return mcts_500_1500_##base##p##fract##cntvis##__u64(boardEmpty, boardOriginal, ai_ctx, simulator);
                  

    #define SEL(base,fract)\
      if constexpr (ExplC==((base*100)+fract))\
      {\
          if constexpr ((count_visits_only&hyperparams)==count_visits_only)\
          {\
              SEL_MCTS_CUTOFF(base,fract, __v) \
          }\
          else\
          {\
              SEL_MCTS_CUTOFF(base,fract, __) \
          }\
      }

    /*#define SEL_MCTS_COUNT_VISITS(nodes,rots,base,fract, intstr,itype) \
      SEL_MCTS_CUTOFF() \
      SEL_MCTS_CUTOFF()

    #define SEL_MCTS(nodes,rots,base,fract) \
      SEL_MCTS_COUNT_VISITS(nodes,rots,base,fract, u32,UDWORD) \
      SEL_MCTS_COUNT_VISITS(nodes,rots,base,fract, u64,UQWORD)
*/
    SEL(0,80)
    SEL(1,20)
    SEL(2,20)
            
    #undef X
}

template <int ExplC, UQWORD hyperparams>
struct TicTacAiRand : TicTacPlayerBase 
{
    Ai_ctx<500, UQWORD> ai_ctx;
   
    Move selectMove([[maybe_unused]] const TicTacBoard& original, [[maybe_unused]] const int input) override
    {
        TicTacBoard empty;
        struct RandRoll : Simulator
        {
            const int originalPlayer;

            explicit RandRoll(const int player) : originalPlayer(player) {}

            float simulate(const Board *original) const override
    	    {
    	        int simWins = 0;
    	        constexpr int MaxRandSims = 20;
    	        // Run simulations:
    	        for (int i=0; i<MaxRandSims; ++i)
    	        {
    	        	TicTacBoard ticTacSim;
    	        	original->cloneInto(&ticTacSim);
    	        	//int nAvailMovesForThisTurn;
    	        	Move *firstMove, *move;

    	        	// Start single sim, run until end:
                    auto outcome = Board::Outcome::running;
                    do
                    {
                    //    nAvailMovesForThisTurn = 0;
                    //    firstMove = ticTacSim.getAnotherMove();
                    //    move = firstMove;
                    //    while (move)
                    //    {
                    //        move->next = ticTacSim.getAnotherMove();
                    //        move = move->next;
                    //        nAvailMovesForThisTurn += 1;
                    //    }
                        // Pick random move:
                    //    nAvailMovesForThisTurn += nAvailMovesForThisTurn==0; // Remove div by 0
                    //    nAvailMovesForThisTurn = pcgRand<UDWORD>()%nAvailMovesForThisTurn;
                    //    while (nAvailMovesForThisTurn--)
                    //        firstMove = firstMove->next;
                        const int nAvailMovesForThisTurn = ticTacSim.getMovesCnt();
                        if (nAvailMovesForThisTurn == 0)
                            break;
                        const int idx = pcgRand<UDWORD>()%nAvailMovesForThisTurn;
                        outcome = ticTacSim.doMove( ticTacSim.getMove(idx) );
                        ticTacSim.switchPlayer();
                    } while (outcome==Board::Outcome::running);

    	        	// Count winner/loser:
    	        	if (outcome!=Board::Outcome::draw)
    	        	{
    	        	    const bool weWon = ticTacSim.getCurrentPlayer() != originalPlayer;
    	        	    simWins += weWon;
    	        	    simWins -= !weWon;
    	        	}
    	        }
                const float winRatio = (simWins-0.f) / (MaxRandSims-0.f);
                return winRatio + 1.f; // +1 to convert range from [-1,1] to [0,2] with 1.0 being 50/50.
            }
        } randRoll(original.getCurrentPlayer());
        const MCTS_result res = selectMCTS<ExplC, hyperparams>(&empty, &original, ai_ctx, &randRoll);
        return res.move;
    }
};




class World
{
private:
    TicTacPlayer h1, h2;
    TicTacAiRand<120,              0> ai1;
    TicTacAiRand<120, cutoff_scoring> ai2;
    typedef int (*fnReadInput)();
    fnReadInput readInput[3] = { nullptr,
                                 []() { return 0; }, //gaem_scan(nullptr); },
                                 []() { return 0; /* this will be ignored by aid */ },
                               };
    TicTacPlayerBase *players[3] = { nullptr, &ai1, &ai2 };
public:
    World() { pcgRand<UQWORD>(0x696969ull); }

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
                    Board::Outcome running = Board::Outcome::running;
                    while (running == Board::Outcome::running)
                    {
                        const int currentPlayer = ticTacBoard.getCurrentPlayer();
                        std::printf("player %d - score: %d %d - rnd: %d\n", currentPlayer, ai1.score, ai2.score, round);
                        const int sel = readInput[currentPlayer](); // Player only, ignored by ai
                        if (sel == 9)
                        {
                            std::printf("exiting...\n");
                            return;
                        }
                        const Move mv = players[currentPlayer]->selectMove(ticTacBoard, sel);
                        std::printf("  playing: %d \n", mv.moveIdx);
                        running = ticTacBoard.doMove(mv);
                        if (running == Board::Outcome::invalid)
                        {
                            std::printf("invalid move \n");
                            continue;
                        }
                        else if (running == Board::Outcome::fin)
                        { 
                            players[currentPlayer]->score += 1;
                            std::printf("\033[1;3%dmwinner: %d \033[0m \n", currentPlayer+1, currentPlayer);
                        }
                        else if (running == Board::Outcome::draw)
                        {
                            std::printf("\033[0;34mdraw \033[0m \n");
                        }
                        ticTacBoard.switchPlayer();
                        #define PUT(x) std::putchar(x)
                        PUT(ticTacBoard.pos[0]+'0'); PUT(ticTacBoard.pos[1]+'0'); PUT(ticTacBoard.pos[2]+'0'); PUT('\n');
                        PUT(ticTacBoard.pos[3]+'0'); PUT(ticTacBoard.pos[4]+'0'); PUT(ticTacBoard.pos[5]+'0'); PUT('\n');
                        PUT(ticTacBoard.pos[6]+'0'); PUT(ticTacBoard.pos[7]+'0'); PUT(ticTacBoard.pos[8]+'0'); PUT('\n');
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
