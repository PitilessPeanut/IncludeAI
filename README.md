# IncludeAI

**IncludeAI** is a custom, hand-written implementation of a **Zero-Knowledge Reinforcement Learning** algorithm that, unlike AlphaZero, does not suffer from the [shallow trap](https://nullprogram.com/blog/2017/04/27) problem and should (given enough resources) be able to play games with *complete information*[^1] perfectly.

## TAKE YOUR GAME-AI TO THE NEXT LEVEL!!!

Start a program with `#include <includeai.hpp>` to literally include an [stb](https://github.com/nothings/stb/)-style single-header library providing you with a General Game Playing, aka. *Effective Decision Making with Uncertain or Incomplete Information* (EDMUII), AI.
The goal of this library is to implement [Artificial General Intelligence](https://en.wikipedia.org/wiki/Artificial_general_intelligence) as a single-header drop-in library. Quick deployment and ease-of-use are priotitized.
*General Game Playing*, while sounding fun, is the second most difficult problem in the universe, far surpassing quantum physics, elliptic curve arithmetic and axionic singularity modulation, but slightly "easier" than mapping homomorphic endofunctors to manifolds of Hilbert space. As such, it remains an ongoing research project! Thread carefully 🔬


### Building
Not necessary, just copying 'includeai.hpp' into your project should be enough. If you want to "build" it anyway here is how:
1. 'python3 generate.py'
2. profit

### Install
1. '#define INCLUDEAI_IMPLEMENTATION' in ONE source file before `#include`ing includeai.hpp
2. Inspect the examples (in the `example` folder) on how to use
3. Build your project using a compiler from the 21st century. I don't know which one, maybe `gcc` or `clang`? If you get some errors try updating your compiler to the latest version.
4. Profit...

### Features
- no dynamic memory
- simple API
- drop-in library. No CMAKE, Autotools, etc needed!
- no trap states ([See below](#about-trap-states))
- no dependencies (other than stdlib)
- cross platform (tries to be...)
- fast?

### How to use
Create an `Ai_ctx` object. This object stores the entire ai memory and can be rather large. For example, during testing the game Yavalath more than 90000 nodes are required. With less, due to early exhausted memory, the stopping condition may be triggered before all iterations are completed, potentially resulting in lower-quality outcomes. Finding the right number of nodes for your use-case requires careful testing and calibration.
The template parameters for Ai_ctx are `<int NumNodes, GameMove MoveType, BitfieldMemoryType BitfieldType, class Pattern, int MaxPatterns>`. If you decide to store your moves/actions as a uint64_t and your bitfield type is also uint64_t, the size of the Ai_ctx object could be something like `(NumNodes * sizeof(Node<uint64_t>)) + ((NumNodes/64) * sizeof(uint64_t)) + (MaxPatterns * sizeof(Pattern))`. Hope thats clear... Other than putting it somewhere into memory there is nothing you need to do with Ai_ctx. Theoretically a single Ai_ctx object can be resued for multiple AI players since it does not store game state. However, if AI players play concurrently, as opposed to taking turns, then each one needs their own Ai_ctx to avoid cuncurrency issues. It really doesn't matter when and where you create and place the Ai_ctx object as it contains only the memory used during a call to `mcts`. However, since it is pretty large I recommend you reuse it as much as possible. During training, unlike during normal play, the Ai_ctx must persist until training is complete. This may strech accross many games. In games with hidden information (Poker/Starcraft/etc.) you must pay attention to pass the correct `Gameview` for each player when calling `mcts`, since those might differ from one player to another. 
Calling `mcts<Iterations, Max simulation depth, Minimax depth, Move type, Bitfield Int type>(Gameworld/board/view, Ai_ctx, random number functor)` will return a `MCTS_result`. Accessing `MCTS_result.best` will give you the ai's favorite move for the given board position, the type of which will be your `Move` type. For example if you `mcts<500, 10, 5, unsigned int, ...` your `MCTS_result.best` will be an 'unsigned int'.

### WASM support
It should work. See how to include above^, compile with SIMD enabled: `em++ mygame.cpp -o mygame.js -s WASM=1 -msimd128`

### About Trap States
Trap states can happen even after all branches of a node were fully explored. In fact, it can even happen after the entire tree has been fully explored! Therefore, trying to force the search to explore all possible child branches of a node is not only not going to work, but may lead to the tree becoming excessively shallow, or "near-sighted" in addition of failing to detect traps, also missing out on more complex strategies at a deeper level! The real reason traps happen is because the "random rollout" simulation terminates (on avarage!) too early when it discovers a winning node even if that winning position can easily be negated by the opposing player, resulting in the simulation providing (on avarage!) the wrong result. Even running a high number of simulations is not going to work if, on avarage, the simulations terminate on the wrong outcome. Upon closer investigation we can observe that _trap states are not a problem but a symptom. The real problem is incorrect branch selection!_ The root cause is a fundamentally false assumtion about the result of random rollouts. For example, if there are three loss states and one win state attached to a root node, the rollout selection will score .25, leading to the branch being discarded, even when this would be the only branch with a possible winning move. The reason for this is that a random rollout does not take *depth* into account when calculating the final score! This can easily be demonstrated by relpacing the random rollout function with a lookup table. We can conclude that random rollout functions, as shown in the AlphaZero research paper, and regardless of how many times they are being run, can not be used as a reliable heuristic for branch score estimation.
Unfortunately this issue seems unsolvable unless you have an iq of 150+ (which I don't claim to have...). If the *simulation* part could yield a clear result (win/loss) then the ai could play perfectly. However, in such a case we might as well replace the entire search tree with a simple minimax since we would no longer be dealing with a probabilistic search! Instead we approach this problem by splitting the simulation part into three pieces:  
1st: minimax up to a predefined depth  
2nd: if that fails: value network estimation  
3rd: if estimations vague or uncertain: random rollout (yolo)  
With this approach we have effectively eliminated the *shallow trap* problem for the vast majority of cases, assuming enough resources have been made available to complete the search:  
 Shown here is an example for the game Yavalath:  
```
// a = ai
// p = player
// X = last move made

// called with 350 iterations, and 50000 nodes pre-allocated:
Ai_ctx<50000, YavalathBoard::YavMove, unsigned int, PatternType, MaxPatterns> ai_ctx;
MCTS_result mcts_res = mcts<350, 21, 3, YavalathBoard::YavMove, UQWORD>(view, ai_ctx, randFn);

    . . . . .                        . . . . .                         . . . . .
   . . . a . .                      . . . a . .                       . . . a . .
  . . . p p . a                    . X . p p . a   <- move the ai    . . . p p . a 
 . . . . . . . .                  . . . . . . . .     should have   . . . . . . . . 
. . . p . a p . .                . . . p . a p . .    made!        . . . p . a p . . 
 . . . X . p . .  <- player       . . . p . p . .                   . . . p . p . .
  . . . . . . .      moved here    . . . . . . .                     . . . . . . . 
   a a p a . .                      a a p a . .                        a a p a . .
    . . . . .                        . . . . .                          X . . . .    <- the move the
                                                                                        ai actually made
```
Improved version:
```
// called with 1000 iterations, and 100000 nodes pre-allocated:
Ai_ctx<100000, YavalathBoard::YavMove, unsigned int, PatternType, MaxPatterns> ai_ctx;
MCTS_result mcts_res = mcts<1000, 21, 3, YavalathBoard::YavMove, UQWORD>(view, ai_ctx, randFn);

    . . . . .                          . . . . .                     
   . . . . . .                        . . . . . .                      
  . . . p p . a                      . X . p p . a    <- Ai successfully prevented
 . . . a . . . .                    . . . a . . . .      possible losing position!
. . . p . a p . .                  . . . p . a p . .      
 . . . X . p . .  <- last move by   . . . p . p . .                 
  . . . . . . .      player          . . . . . . .                    
   a a p a . .                        a a p a . .                     
    . . . . .                          . . . . .                        
                                                                                       
```
Some initial tests suggest that past a number of iterations of about 8-9000 (and ~600000+ nodes), the AI becomes effectively undefeatable (in Yavalath!). These numbers may vary depending on your game/usecase/etc.  
**The key idea is to push any trap state to a depth beyond the search "horizon", thereby reducing the likelihood of its selection to 0%! **[^2]  


### Neural Network implementation
Unlike traditional feed-forward networks, the one included in this library does not feature 'layers' in the classical sense. Instead it is implemented as a sparse graph with the possibility for nodes to be connected randomly or even cyclically! This style of implementation allows for more flexibility to be molded for any given problem domain as opposed to the rigid structure of a layered network.

### Limitations / Assumtions
While there is no limitation on the number of players (2,3,4...), and it is possible for a player to take two consecutive turns, it is assumed that each player plays one move/action before ending their turn (see note below!). That means that compound moves, such as moving 4 steps forward and 1 step left should be consolidated into a single move instead of taking 5 turns!
In coop games, where multiple players work together, the `getWinner()` function should always return the current player if an allied player wins. For example, if player 1 and 2 are on the same team, and player 2 scores a victory during player 1's turn, the `getWinner()` should return `1` even if player 2 is making the winning move! A victory by any allied player should be considered equivalent to a victory by the current (maximizing) player.
NOTE: 'turn' does not strictly imply a restriction to the use of this library in turn based games alone! It should be possible to run this in a background thread continuously polling best moves at a certain frequency without having to wait for another player to make a 'move'.

### License
BSD 4-Clause! But for a small donation 💰💰💰 I'm willing to adjust...

### Disclaimer / legalese
Three things you should note if you decide to use this:
1) No guarantees! Use at own risk!
2) All Trademarks are owned by their respective owners. Lawyers love tautologies.
3) I have basically no idea what I'm doing...


[^1]: Like Chess, not Poker: No secrets, dice or cards
[^2]: Solving the Trap State problem in mcts is analogous to solving the Double-Spend problem in peer-to-peer payment networks: Seems impossible until someone finds a solution. But who??
