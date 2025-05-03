#include "ai.hpp"


/****************************************/
/*                                Tests */
/****************************************/
    struct TicTacTest
    {
        using Move = int;
        using StorageForMoves = Move[9];
        unsigned char pos[9+1] = {0,0,0,
                                  0,0,0,
                                  0,0,0
                                 };
        int currentPlayer=1, winner=0;
        float neuralInputs[9+9] = {0};

        constexpr TicTacTest() {}
        TicTacTest(const TicTacTest&) = delete;
        TicTacTest& operator=(const TicTacTest&) = delete;
        TicTacTest(TicTacTest&&) = default;

        constexpr TicTacTest clone() const
        {
            TicTacTest dst;
            for (int i=0; i<9; ++i) { dst.pos[i] = pos[i]; }
            dst.currentPlayer = currentPlayer;
            return dst;
        }

        constexpr int generateMovesAndGetCnt(TicTacTest::Move *availMoves)
        {
            int availMovesCtr = 0;
            for (int i=0; i<9; ++i)
            {
                // find valid moves:
                if (pos[i]==0) availMoves[availMovesCtr++] = i;
            }
            // Don't do this: "availMovesCtr -= 1"
            return availMovesCtr;
        }

        constexpr include_ai::Outcome doMove(const TicTacTest::Move mv)
        {
            // checking if valid input only needed when running on a server:
            if (pos[mv]) [[unlikely]]
                return include_ai::Outcome::invalid; // <- Therefore this is for demo only!

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
                return include_ai::Outcome::fin;
            }
            // Counting turns can NOT be used here for testing:
            if (pos[0]&&pos[1]&&pos[2]&&pos[3]&&pos[4]&&pos[5]&&pos[6]&&pos[7]&&pos[8])
                return include_ai::Outcome::draw;
            return include_ai::Outcome::running;
        }

        constexpr void switchPlayer() { currentPlayer=3-currentPlayer; }

        constexpr int getCurrentPlayer() const { return currentPlayer; }

        constexpr int getWinner() const { return winner; }

        float *getNetworkInputs()
        {
            for (int i=0; i<9; ++i)
            {
                neuralInputs[i  ] = pos[i] == currentPlayer;
                neuralInputs[i+9] = pos[i] != currentPlayer;
            }
            return neuralInputs;
        }
    };

    static_assert([]
                  {
                      UQWORD xoroshiro128plus_state[2] = {0x9E3779B97f4A7C15ull,0xBF58476D1CE4E5B9ull};

                      //   Written in 2016-2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)
                      // To the extent possible under law, the author has dedicated all copyright
                      // and related and neighboring rights to this software to the public domain
                      // worldwide. This software is distributed without any warranty.
                      // See <http://creativecommons.org/publicdomain/zero/1.0/>.
                      auto xoroshiro128plus = [](UQWORD (&state)[2]) -> UQWORD
                      {
                          // The generators ending with + have weak low bits, so they
                          // are recommended for floating point number generation
                          const UQWORD s0 = state[0];
                          UQWORD s1 = state[1];
                          const UQWORD result = s0 + s1;

                          s1 ^= s0;
                          state[0] = ((s0 << 55) | (s0 >> (64 - 55))) ^ s1 ^ (s1 << 14); // a, b
                          state[1] = (s1 << 36) | (s1 >> (64 - 36)); // c
                          return result;
                      };

                      using namespace include_ai;
                      TicTacTest t1;
                      t1.pos[0]=2; t1.pos[1]=1; t1.pos[2]=2;
                      t1.pos[3]=1; t1.pos[4]=1; t1.pos[5]=2;
                      t1.pos[6]=0; t1.pos[7]=2; t1.pos[8]=1;
                      t1.currentPlayer = 1;
                      float res = simulate<1>(t1, [&xoroshiro128plus_state, xoroshiro128plus]{ return xoroshiro128plus(xoroshiro128plus_state); });
                      bool ok = (res < .1f) && (res > -.1f); // draw
                      TicTacTest t2;
                      t2.pos[0]=1; t2.pos[1]=1; t2.pos[2]=0;
                      t2.pos[3]=2; t2.pos[4]=0; t2.pos[5]=0;
                      t2.pos[6]=0; t2.pos[7]=2; t2.pos[8]=0;
                      t2.currentPlayer = 1;
                      res = simulate<1>(t2, [&xoroshiro128plus_state, xoroshiro128plus]{ return xoroshiro128plus(xoroshiro128plus_state); });
                      return ok && (res>.5f); // Player 1 wins
                  }()
                 );

    static_assert([]
                  {
                      using namespace include_ai;
                      TicTacTest t;
                      t.pos[0]=1; t.pos[1]=0; t.pos[2]=2;
                      t.pos[3]=0; t.pos[4]=1; t.pos[5]=0;
                      t.pos[6]=0; t.pos[7]=0; t.pos[8]=0;
                      t.currentPlayer = 2;
                      SWORD res = minimax<TicTacTest, TicTacTest::Move>(t, 10);
                      bool ok = res == MinimaxDraw; // Player 2 can still play position 8!
                      t.pos[5] = 2; // ... but plays  5 instead
                      t.currentPlayer = 1;
                      res = minimax<TicTacTest, TicTacTest::Move>(t, 9);
                      return ok && (res == MinimaxWin); // Player 2 lost!
                  }()
                 );

    static_assert([]
                  {
                      using namespace include_ai;
                      TicTacTest t;
                      t.pos[0]=2; t.pos[1]=0; t.pos[2]=0;
                      t.pos[3]=0; t.pos[4]=1; t.pos[5]=0;
                      t.pos[6]=0; t.pos[7]=2; t.pos[8]=1;
                      t.currentPlayer = 1;
                      const SWORD res = minimax<TicTacTest, TicTacTest::Move>(t, 9);
                      // This position is a guaranteed win for '1':
                      return res == MinimaxWin;
                  }()
                 );

    static_assert([]
                  {
                      using namespace include_ai;
                      TicTacTest t;
                      t.pos[0]=2; t.pos[1]=0; t.pos[2]=0;
                      t.pos[3]=0; t.pos[4]=1; t.pos[5]=0;
                      t.pos[6]=0; t.pos[7]=0; t.pos[8]=0;
                      t.currentPlayer = 1;
                      const SWORD res = minimax<TicTacTest, TicTacTest::Move>(t, 9);
                      // winner not yet determined:
                      return res == MinimaxDraw; // 0
                  }()
                 );

    static_assert([]
                  {
                      using namespace include_ai;
                      TicTacTest t;
                      t.pos[0]=2; t.pos[1]=1; t.pos[2]=2;
                      t.pos[3]=1; t.pos[4]=1; t.pos[5]=2;
                      t.pos[6]=0; t.pos[7]=2; t.pos[8]=1;
                      t.currentPlayer = 1;
                      const SWORD res = minimax<TicTacTest, TicTacTest::Move>(t, 9);
                      // draw:
                      return res == MinimaxDraw; // 0
                  }()
                 );

    static_assert([]
                  {
                      using namespace include_ai;
                      TicTacTest t;
                      t.pos[0]=0; t.pos[1]=0; t.pos[2]=1;
                      t.pos[3]=2; t.pos[4]=0; t.pos[5]=0;
                      t.pos[6]=2; t.pos[7]=0; t.pos[8]=1;
                      t.currentPlayer = 1;
                      const SWORD res = minimax<TicTacTest, TicTacTest::Move>(t, 9);
                      return res == MinimaxWin; // 1
                  }()
                 );

   /* static_assert([]
                  {
                      using namespace include_ai;
                      TicTacTest t;
                      t.pos[0]=1; t.pos[1]=2; t.pos[2]=0;
                      t.pos[3]=0; t.pos[4]=0; t.pos[5]=0;
                      t.pos[6]=0; t.pos[7]=0; t.pos[8]=0;
                      t.currentPlayer = 1;
                      HALF *networkInputs = t.getNetworkInputs();
                      bool ok = networkInputs[0] < 1.f;
                      return ok;
                  }()
                 );*/






/****************************************/
/*                             Research */
/****************************************/
/*
    https://u.cs.biu.ac.il/~sarit/advai2018/MCTS.pdf
    https://drum.lib.umd.edu/items/07cb7ac2-115a-443c-9c9e-fe6dad2a6b41 "MONTE CARLO TREE SEARCH AND MINIMAX COMBINATION – APPLICATION OF SOLVING PROBLEMS IN THE GAME OF GO" (minimax+mcts+code)
    https://www.ijcai.org/proceedings/2018/0782.pdf "MCTS-Minimax Hybrids with State Evaluations"
    http://www.talkchess.com/forum3/viewtopic.php?t=67235 MCTS beginner questions
    http://www.talkchess.com/forum3/viewtopic.php?f=7&t=70694
    http://www.talkchess.com/forum3/viewtopic.php?f=7&t=69993 <- Tic-Tac-Toe






    todo: http://www.incompleteideas.net/609%20dropbox/other%20readings%20and%20resources/MCTS-survey.pdf
*/

/* Whatever we conceive well we express clearly, and words flow with ease. (Nicolas Boileau-Despreaux) */
