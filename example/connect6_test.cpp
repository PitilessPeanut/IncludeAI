#include <iostream>
#include <memory>
#define INCLUDEAI_IMPLEMENTATION
#include <assert.h>
#include <unistd.h>
#include "../includeai.hpp"


using namespace include_ai;


class Connect6Board
{
public:
    static constexpr int WIDTH = 13;//19;
    static constexpr int HEIGHT = 13;//19;
    static constexpr int CELLS = WIDTH * HEIGHT;
    static constexpr int MaxNetworkInputs = CELLS + 1;
    using Move = int; // Represents a linear index (y * WIDTH + x)
    using StorageForMoves = Move[CELLS];
    int board[CELLS];
private:
    int currentPlayer = 1; // 1 (Black) starts
    int winner = 0;
    int stonesPlaced = 0;
    float networkInputs[MaxNetworkInputs];
    constexpr bool checkWin(const int idx) const
    {
        const int x = idx % WIDTH;
        const int y = idx / WIDTH;
        const int playerStone = board[idx];
        if (playerStone == 0) return false; // No stone placed here

        // Directions: horizontal, vertical, diagonal \, diagonal /
        const int directions[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
        for (const auto& [dx, dy] : directions)
        {
            int count = 1; // Count the current stone
            // Check in the positive direction
            for (int step = 1; step < 6; ++step)
            {
                const int nx = x + dx * step;
                const int ny = y + dy * step;
                if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) break;
                if (board[ny * WIDTH + nx] == playerStone) count++;
                else break;
            }
            // Check in the negative direction
            for (int step = 1; step < 6; ++step)
            {
                const int nx = x - dx * step;
                const int ny = y - dy * step;
                if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) break;
                if (board[ny * WIDTH + nx] == playerStone) count++;
                else break;
            }
            if (count >= 6) return true; // Win condition met
        }
        return false; // No win found
    }
public:
    constexpr Connect6Board() { reset(); }
    Connect6Board(const Connect6Board&) = delete;
    Connect6Board& operator=(const Connect6Board&) = delete;
    Connect6Board(Connect6Board&&) = default;

    constexpr Connect6Board clone() const
    {
        Connect6Board dst;
        for (int i = 0; i < CELLS; ++i) dst.board[i] = board[i];
        dst.currentPlayer = currentPlayer;
        dst.winner = winner;
        dst.stonesPlaced = stonesPlaced;
        return dst;
    }

    int generateMovesAndGetCnt(Move *availMoves) const
    {
        // 1. If board is empty (first move), only suggest the center:
        if (stonesPlaced == 0)
        {
            availMoves[0] = (HEIGHT / 2) * WIDTH + (WIDTH / 2);
            return 1;
        }

        int availMovesCtr = 0;
        constexpr int SEARCH_RADIUS = 2;
        for (int i = 0; i < CELLS; ++i)
        {
            if (board[i] != 0) continue; // Only examine this cell if it is empty
            const int cx = i % WIDTH;
            const int cy = i / WIDTH;
            bool relevant = false;
            // Check neighbors within Radius
            // Logic: If I place a stone at [cx, cy], is it near anyone else?
            const int minY = std::max(0, cy - SEARCH_RADIUS);
            const int maxY = std::min(HEIGHT - 1, cy + SEARCH_RADIUS);
            const int minX = std::max(0, cx - SEARCH_RADIUS);
            const int maxX = std::min(WIDTH - 1, cx + SEARCH_RADIUS);
            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    // Must be horizontal (dy==0), vertical (dx==0), or true diagonal (dx==dy)
                    // to eliminate irrelevant "knight-jumps":
                    int dx = std::abs(x - cx);
                    int dy = std::abs(y - cy);
                    if (dx != 0 && dy != 0 && dx != dy) continue;

                    // If we find ANY existing stone (black or white) nearby
                    if (board[y * WIDTH + x] != 0)
                    {
                        relevant = true;
                        goto found_neighbor;
                    }
                }
            }
            found_neighbor:
            if (relevant)
                availMoves[availMovesCtr++] = i;
        }
        return availMovesCtr;
    }

    constexpr Outcome doMove(const Move mv)
    {
        board[mv] = (unsigned char)currentPlayer;
        stonesPlaced++;
        if (checkWin(mv))
        {
            winner = currentPlayer;
            return Outcome::fin;
        }
        if (stonesPlaced >= CELLS) { return Outcome::draw; }
        return Outcome::running;
    }

    constexpr void switchPlayer()
    {
        if (stonesPlaced % 2 != 0)
            currentPlayer = 3 - currentPlayer;
    }

    int getCurrentPlayer() const { return currentPlayer; }

    int getWinner() const { return winner; }

    float getBoardScore() const { return 0.f; }

    float *getNetworkInputs()
    {
        for (int i = 0; i < CELLS; ++i)
        {
            if (board[i] == 0) // Empty
                networkInputs[i] = 0.0f;
            else if (board[i] == currentPlayer) // Our stones
                networkInputs[i] = 1.0f;
            else // Opponent stones
                networkInputs[i] = -1.0f;
        }
        // Track turn parity (Are we placing the 1st or 2nd stone of our turn?)
        networkInputs[CELLS] = (stonesPlaced % 2 == 0) ? -1.0f : 1.0f;
        return networkInputs;
    }

    constexpr void randomize() {}

    int getStone(int idx) const
    {
        if (idx < 0 || idx >= CELLS) return 0;
            return board[idx];
    }

    void drawBoard(int highlight1 = -1, int highlight2 = -1)
    {
        std::printf("   ");
        for (int x = 0; x < WIDTH; ++x) std::printf("%c ", 'A' + x);
        std::printf("\n");
        for (int y = 0; y < HEIGHT; ++y)
        {
            std::printf("%2d ", y + 1);
            for (int x = 0; x < WIDTH; ++x)
            {
                const int idx = y * WIDTH + x;
                const int cell = board[idx];
                const bool isHighlight = (idx == highlight1) || (idx == highlight2);
                if (isHighlight) std::printf("\033[1;33m"); // Yellow Bold
                if (cell == 1) std::printf("X ");
                else if (cell == 2) std::printf("O ");
                else std::printf(". ");
                if (isHighlight) std::printf("\033[0m");
            }
            std::printf("\n");
        }
    }

    constexpr void reset()
    {
        for (int i = 0; i < CELLS; ++i) board[i] = 0;
        currentPlayer = 1;
        winner = 0;
        stonesPlaced = 0;
    }
};


struct Connect6PlayerBase
{
    static constexpr auto halfHeight = Connect6Board::HEIGHT / 2;
    static constexpr auto halfWidth = Connect6Board::WIDTH / 2;
    virtual Connect6Board::Move selectMove(const Connect6Board&) = 0;
    virtual ~Connect6PlayerBase() = default;
};

struct Connect6PlayerLocal : Connect6PlayerBase
{
    Connect6Board::Move selectMove(const Connect6Board& original) override
    {
        int x, y;
        char colChar;
        int rowInt;
        [[maybe_unused]] int ret = std::scanf(" %c%d", &colChar, &rowInt);
        if (colChar >= 'a' && colChar <= 's') x = colChar - 'a';
        else if (colChar >= 'A' && colChar <= 'S') x = colChar - 'A';
        else x = -1; // Invalid
        y = rowInt - 1;
        while (true)
        {
            if (x >= 0 && x < Connect6Board::WIDTH && y >= 0 && y < Connect6Board::HEIGHT)
            {
                int idx = y * Connect6Board::WIDTH + x;
                if (original.board[idx] == 0) return idx;
            }
            std::printf("Invalid input or cell occupied. Try again (e.g., 'J 10'): ");
        } // while (true)
    }
};

struct Connect6AiMCTS : Connect6PlayerBase
{
    std::unique_ptr<Ai_ctx<280000, Connect6Board::Move, UQWORD>> ai_ctx =
        std::make_unique<Ai_ctx<280000, Connect6Board::Move, UQWORD>>();
    Connect6Board::Move selectMove(const Connect6Board& original) override
    {
        if (original.board[halfHeight*Connect6Board::WIDTH+halfWidth] == 0)
            return halfHeight*Connect6Board::WIDTH + halfWidth; // Center
        constexpr int simDepth = 5; // Shorter depth for Connect6 due to branching factor
        constexpr int minimaxDepth = 2;
        struct NeuralDummy
        {
            FLOAT x;
            FLOAT *evaluate(const FLOAT *inputs, const FLOAT boardScore)
            {
                x = boardScore;
                return &x;
            }
        } dummy_nn;
        const auto start = std::chrono::high_resolution_clock::now();
        const MCTS_result<Connect6Board::Move> res =
            mcts<150, simDepth, minimaxDepth, Connect6Board::Move, UQWORD>(
                original,
                *ai_ctx,
                dummy_nn,
                []{ return pcgRand<UDWORD>(); }
            );
        const auto end = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        using Res = MCTS_result<Connect6Board::Move>;
        std::printf("AI Stats - sims: %.0f | minmax: %.0f | thrLvl: %1.2f | netEvals: %.0f | terminals: %.0f | bestScore: %.2f | visits: %.0f \n",
            res.statistics[Res::simulations],
            res.statistics[Res::minimaxes],
            res.statistics[Res::thresholdLevel],
            res.statistics[Res::networkEvaluated],
            res.statistics[Res::terminalReached],
            res.statistics[Res::score],
            res.statistics[Res::visits]
        );
        std::printf("time taken: %lld ms\n", duration.count());
        if (res.errorOutOfMem)
            std::printf("\033[1;30mWarning: MCTS ran out of memory and may return suboptimal move.\n\033[0m");
        return res.best;
    }
};

constexpr int aiMinimax(const Connect6Board& current, const Connect6Board::Move move, int alpha, int beta, const int depth)
{
    Connect6Board clone = current.clone();
    const Outcome outcome = clone.doMove(move);
    clone.switchPlayer();
    if (outcome != Outcome::running)
    {
        if (outcome == Outcome::draw)
            return MinimaxDraw;
        else if (clone.getWinner() != current.getCurrentPlayer())
            return MinimaxLose;
        else
            return MinimaxWin;
    }
    if (depth<=0) { return MinimaxDraw; }
    typename Connect6Board::StorageForMoves storageForMoves;
    int nMoves = clone.generateMovesAndGetCnt(storageForMoves);
    nMoves -= 1;
    int minimaxScore = MinimaxInit;
    while (nMoves >= 0)
    {
        const Connect6Board::Move moveHere = storageForMoves[nMoves];
        int returnedScore;
        if (clone.getCurrentPlayer() == current.getCurrentPlayer())
            returnedScore = aiMinimax(clone, moveHere, alpha, beta, depth-1);
        else
            returnedScore = -aiMinimax(clone, moveHere, -beta, -alpha, depth-1);
        minimaxScore = aiMax(minimaxScore, returnedScore);
        alpha        = aiMax(alpha, minimaxScore);
        if (minimaxScore == MinimaxWin) break;
        if (beta <= alpha) {
            break; // Beta Cutoff
        }
        nMoves -= 1;
    }
    return minimaxScore==MinimaxInit ? MinimaxDraw : minimaxScore;
}

struct Connect6AiMinimax : Connect6PlayerBase
{
    Connect6Board::Move selectMove(const Connect6Board& original) override
    {
        if (original.board[halfHeight * Connect6Board::WIDTH + halfWidth] == 0)
            return halfHeight * Connect6Board::WIDTH + halfWidth;
        typename Connect6Board::StorageForMoves storageForMoves;
        int nMoves = original.generateMovesAndGetCnt(storageForMoves);
        nMoves -= 1;
        int bestScore = MinimaxInit;
        int bestPos = 0;
        int alpha = MinimaxLose;
        constexpr int MAX_DEPTH = 4;
        while (nMoves >= 0)
        {
            const Connect6Board::Move moveHere = storageForMoves[nMoves];
            const int mnx = aiMinimax(original, moveHere, alpha, MinimaxWin, MAX_DEPTH);
            if (mnx > bestScore)
            {
                bestScore = mnx;
                bestPos = moveHere;
                alpha = aiMax(alpha, bestScore);
            }
            if (bestScore == MinimaxWin) break;
            nMoves -= 1;
        }
        return bestPos;
    }
};

struct Connect6AiNeural : Connect6PlayerBase
{
    struct NetRng { UDWORD operator()() { return pcgRand<UDWORD>(); } } netRng;
    static constexpr int INPUTS = Connect6Board::MaxNetworkInputs;
    static constexpr int OUTPUTS = 1, LAYERS = 3, HIDDEN_WIDTH = 96;
    using MyNetwork = Neural<INPUTS, OUTPUTS, LAYERS, HIDDEN_WIDTH, NetRng>;
    std::unique_ptr<MyNetwork> nn_ptr;
    std::unique_ptr<Ai_ctx<280000, Connect6Board::Move, UQWORD>> ai_ctx;
    struct NNAdapter
    {
        MyNetwork *net;
        FLOAT *evaluate(const FLOAT* inputs, [[maybe_unused]] const FLOAT boardScore)
        {
            return net->evaluate(inputs);
        }
    } nn_adapter;

    Connect6AiNeural()
      : nn_ptr(std::make_unique<MyNetwork>(netRng)),
        ai_ctx(std::make_unique<Ai_ctx<280000, Connect6Board::Move, UQWORD>>()),
        nn_adapter{nn_ptr.get()}
    {}



    Connect6Board::Move selectMove(const Connect6Board& original) override
    {
        if (original.board[halfHeight*Connect6Board::WIDTH+halfWidth] == 0)
            return halfHeight*Connect6Board::WIDTH + halfWidth; // Center
        constexpr int simDepth = 5;
        constexpr int minimaxDepth = 2;
        const auto start = std::chrono::high_resolution_clock::now();
        const MCTS_result<Connect6Board::Move> res =
            mcts<150, simDepth, minimaxDepth, Connect6Board::Move, UQWORD>(
                original,
                *ai_ctx,
                nn_adapter,
                []{ return pcgRand<UDWORD>(); }
            );
        const auto end = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        using Res = MCTS_result<Connect6Board::Move>;
        std::printf("Neural Stats - sims: %.0f | minmax: %.0f | thrLvl: %1.2f | netEvals: %.0f | terminals: %.0f | bestScore: %.2f | visits: %.0f \n",
            res.statistics[Res::simulations],
            res.statistics[Res::minimaxes],
            res.statistics[Res::thresholdLevel],
            res.statistics[Res::networkEvaluated],
            res.statistics[Res::terminalReached],
            res.statistics[Res::score],
            res.statistics[Res::visits]
        );
        std::printf("time taken: %lld ms\n", duration.count());
        if (res.errorOutOfMem)
            std::printf("\033[1;30mWarning: Neural ran out of memory and may return suboptimal move.\n\033[0m");
        return res.best;
    }
};




void playRegular()
{
    Connect6PlayerLocal h1, h2;
    Connect6AiMCTS ai;
    Connect6AiMinimax ai2;
    Connect6PlayerBase *players[3] = { nullptr, &h1, &ai2 };
    int scores[3] = {0, 0, 0};
    int draws = 0;
    for (int game = 0; game < 7; ++game)
    {
        Connect6Board board;
        Outcome outcome = Outcome::running;
        int lastMove = -1, prevMove = -1;
        std::printf("\n=== New Game %d ===\n", game);
        std::printf("Input format: 'Column Row' (e.g., J 10)\n");
        while (outcome == Outcome::running)
        {
            int highlight2 = -1;
            if (lastMove != -1 && prevMove != -1)
            {
                if (board.getStone(prevMove) == board.getStone(lastMove))
                    highlight2 = prevMove;
            }
            board.drawBoard(lastMove, highlight2);
            const int currentPlayer = board.getCurrentPlayer();
            const Connect6Board::Move mv = players[currentPlayer]->selectMove(board);
            outcome = board.doMove(mv);
            prevMove = lastMove;
            lastMove = mv;
            if (outcome == Outcome::fin)
            {
                const int h2 = (board.getStone(prevMove) == board.getStone(lastMove)) ? prevMove : -1;
                board.drawBoard(lastMove, h2);
                scores[board.getWinner()]++;
                std::printf("\033[1;32mWinner: Player %d!\033[0m Scores: P1:%d P2:%d Draws:%d\n",
                            board.getWinner(), scores[1], scores[2], draws);
            }
            else if (outcome == Outcome::draw)
            {
                board.drawBoard(lastMove, -1);
                draws++;
                std::printf("\033[1;34mGame Draw!\033[0m Scores: P1:%d P2:%d Draws:%d\n", scores[1], scores[2], draws);
            }
            board.switchPlayer();
        }
    }
}




void playTraining()
{
    struct TrainingSample
    {
        float boardState[Connect6Board::MaxNetworkInputs];
        float gameOutcome; // Will be +1.0 (win), -1.0 (loss)
    };
    Connect6PlayerLocal h1;
    Connect6AiMCTS aiMCTS;
    Connect6AiNeural aiNeural;
    int scores[3] = {0, 0, 0};
    int lastMove = -1, prevMove = -1;
    std::deque<TrainingSample> win_samples;
    std::deque<TrainingSample> loss_samples;
    for (int round=0; round<8000; ++round)
    {
        Connect6Board board;
        std::vector<std::pair<std::vector<float>, int>> single_game_history;
        Outcome outcome = Outcome::running;
        Connect6PlayerBase *players[3] = { nullptr, &aiMCTS, &aiNeural };
        if (round == 0)
            players[1] = &h1;
        while (outcome == Outcome::running)
        {
            const int currentPlayer = board.getCurrentPlayer();

            const float *currentInputs = board.getNetworkInputs();

            single_game_history.emplace_back(
                std::vector<float>(currentInputs, currentInputs + board.MaxNetworkInputs),
                currentPlayer
            );

            const Connect6Board::Move mv = players[currentPlayer]->selectMove(board);
            outcome = board.doMove(mv);

            // "prevMove" and "lastMove" used for highlight last moves:
            prevMove = lastMove;
            lastMove = mv;
            //if (outcome == Outcome::fin)
                //{
                const int h2 = (board.getStone(prevMove) == board.getStone(lastMove)) ? prevMove : -1;
                board.drawBoard(lastMove, h2);
                //}

                if (outcome == Outcome::draw)
            {
                board.drawBoard(lastMove, -1);
            }
            board.switchPlayer();
        } // while (running == Outcome::running)

        if (outcome == Outcome::fin)
        {
            const auto w = board.getWinner();
            scores[w] += 1;
            std::printf("\033[1;3%dmwinner: %d \033[0m scre: %d %d\n", w+1, w, scores[1], scores[2]);
            float learning_rate = 0.001f;
            float total_mse = 0.f;
            for (auto& step : single_game_history)
            {
                const auto& [boardState, player] = step;
                const float target = (player == board.getWinner()) ? 1.f : -1.f;
                total_mse += aiNeural.nn_ptr->train(boardState.data(), &target, learning_rate);
            }
            std::printf("training MSE: %f\n", total_mse / single_game_history.size());
        }
        sleep(4+(pcgRand<UDWORD>()&1));

        // Prevent samples from getting too big:
        while (win_samples.size() > 2500)
            win_samples.pop_front();
        while (loss_samples.size() > 2500)
            loss_samples.pop_front();
    } // for round
}




int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    playTraining();
}
