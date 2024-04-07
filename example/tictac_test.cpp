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

void gaem_draw([[maybe_unused]] void *pEverything, [[maybe_unused]] const unsigned *updatesVct, [[maybe_unused]] int size)
{
}

void gaem_debugPrint([[maybe_unused]] void *pEverything, [[maybe_unused]] const char *debugStr)
{
}




class TicTacBoard : public Board
{
public:
    unsigned char pos[9+1] = {0};
    int currentPlayer = 1, turn = 0;
private:
    int availMovesIdx = -1;
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

    Move *getAnotherMove() override
    {
        // Done, all moves returned:
        if (availMovesIdx==0)
        {
            availMovesIdx = -1;
            return nullptr;
        }
        // New turn, create moves:
        else if (availMovesIdx == -1)
        {
            for (int i=0; i<9; ++i) 
                // find valid moves:
                if (pos[i]==0)
                {
                    availMovesIdx += 1;
                    availMoves[availMovesIdx] = Move{i, nullptr};
                }
            availMovesIdx += 1;
        }
        // Return another move:
        availMovesIdx -= 1;
        return &availMoves[ availMovesIdx ];
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

/*
struct TicTacPlayerBase
{
    virtual Move selectMove(const TicTacBoard&, const int) = 0;
    virtual ~TicTacPlayerBase() = default;
};

struct TicTacPlayer : TicTacPlayerBase
{*/
    Move selectMove([[maybe_unused]] const TicTacBoard& original, [[maybe_unused]] const int input) //override
    {
        return Move{input, nullptr};
    }
/*};

struct TicTacAiRand : TicTacPlayerBase 
{
    Ai_ctx<500, UDWORD> ai_ctx;
   */ 
    Move selectMove(Ai_ctx<500, UDWORD>& ai_ctx, [[maybe_unused]] const TicTacBoard& original, [[maybe_unused]] const int input) //override
    {
        TicTacBoard empty;
        struct RandRoll : Simulator
        {
            const int originalPlayer;

            explicit RandRoll(const int player) : originalPlayer(player) {}

            float simulate(const Board *original) const override
    	    {
    	        int simWins = 0;
    	        constexpr int MaxRandSims = 7;
    	        // Run simulations:
    	        for (int i=0; i<MaxRandSims; ++i)
    	        {
    	        	TicTacBoard ticTacSim;
    	        	original->cloneInto(&ticTacSim);
    	        	int nAvailMovesForThisTurn;
    	        	Move *firstMove, *move;

    	        	// Start single sim, run until end:
    	        	auto outcome = Board::Outcome::running;
    	        	do
    	        	{
    	        	    nAvailMovesForThisTurn = 0;
    	        	    firstMove = ticTacSim.getAnotherMove();
    	        	    move = firstMove;
    	        	    while (move)
    	        	    {
    	        	        move->next = ticTacSim.getAnotherMove();
    	        	        move = move->next;
    	        	        nAvailMovesForThisTurn += 1;
    	        	    }
    	        	    // Pick random move:
    	        	    nAvailMovesForThisTurn += nAvailMovesForThisTurn==0; // Remove div by 0
    	        	    nAvailMovesForThisTurn = pcgRand<UDWORD>()%nAvailMovesForThisTurn;
    	        	    while (nAvailMovesForThisTurn--)
    	        	        firstMove = firstMove->next;
    	        	    outcome = ticTacSim.doMove(*firstMove);
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
        const MCTS_result res = mcts_500_2000_1p20__32(&empty, &original, ai_ctx, &randRoll);
        return res.move;
    }
//};




class World
{
private:
   // TicTacPlayer h1, h2;
   // TicTacAiRand ai1;
    Ai_ctx<500, UDWORD> ai_ctx;
    typedef int (*fnReadInput)();
    fnReadInput readInput[3] = { nullptr,
                                 []() { return gaem_scan(nullptr); },
                                 []() { return 0; /* this will be ignored by aid */ },
                               };
   // TicTacPlayerBase* players[3] = { nullptr, &h1, &ai1 };
public:
    World()
    {
        //device.scan = gaem_scan;
    }

    void step()
    {
        TicTacBoard ticTacBoard;

        constexpr int MaxRounds = 69;
        for (int round=0; round<MaxRounds; ++round)
        {
            Board::Outcome running = Board::Outcome::running;
            while (running == Board::Outcome::running)
            {
                const int currentPlayer = ticTacBoard.getCurrentPlayer();
                std::printf("player %d\n", currentPlayer);
                //TicTacPlayerBase *pPlayer = players[currentPlayer];
                const int sel = readInput[currentPlayer](device); // Ignored by ai
                //const Move mv = players[currentPlayer]->selectMove(ticTacBoard, sel);
                Move mv;
                if (currentPlayer==1)
                    mv = /*h1.*/selectMove(ticTacBoard, sel);
                else
                    mv = /*ai1.*/selectMove(ai_ctx, ticTacBoard, sel);
                
                std::printf("  playing: %d \n", mv.moveIdx);
                running = ticTacBoard.doMove(mv);
                if (running == Board::Outcome::invalid)
                {
                    std::printf("invalid move \n");
                    continue;
                }
                else if (running == Board::Outcome::fin)
                {
                    std::printf("\033[1;3%dmwinner: %d \033[0m \n", currentPlayer, currentPlayer);
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
    }
};








int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    std::vector<World> world(1);
    world[0].step();
}
