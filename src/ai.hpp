#ifndef AI_HPP
#define AI_HPP

#include "asmtypes.hpp"
#include "bitalloc.hpp"
#include <concepts>
#include <cmath>
#include <cstdio>

namespace include_ai {


/****************************************/
/*       Util functions / configuration */
/****************************************/
// sqrt
    inline constexpr float aiSqrt(auto x) { return std::sqrt(x-0.f); }

// log
    inline constexpr float aiLog(auto x) { return std::log(x-0.f); }

// assert/debug
   // #if defined(AI_DEBUG)
      #include <assert.h>
      #define aiAssert(x) assert(x)
      #define aiDebug(x) x
  //  #else
  //    #define aiAssert(x)
  // #define aiDebug(x)
   // #endif


/****************************************/
/* A 'move' by a user/player within the */
/* game context                         */
/****************************************/
template <typename T>
concept GameMove = std::integral<T>; // todo!!!!!!
    //#if defined(AI_32BIT_ONLY)
//      using Move = SDWORD; // A generic 'move'. Can represent anything
    //#else
      using Move = SQWORD; // ^^
  //  #endif


/****************************************/
/*            outcome of a single round */
/****************************************/
    enum class Outcome { running, draw, fin, invalid };


/****************************************/
/*                        Board concept */
/****************************************/
    template <typename T>
    concept Gameboard =
        !std::copy_constructible<T>
        && !std::is_copy_assignable<T>::value
        && requires (T obj, const T cobj)
        {
            {cobj.clone()} -> std::same_as<T>;
            {obj.generateMovesAndGetCnt(nullptr)} -> std::convertible_to<int>;
            {obj.doMove(int())} -> std::same_as<Outcome>;
            {obj.switchPlayer()};
            {cobj.getCurrentPlayer()} -> std::equality_comparable;
            {cobj.getWinner()} -> std::equality_comparable;
        };


/****************************************/
/*      Node (should fit into 64 bytes) */
/****************************************/
    struct Node
    {
        static constexpr SWORD never_expanded = -1;
        static constexpr SWORD removed = -2; // Debug
        SWORD    activeBranches = never_expanded; // Must be signed!
        SWORD    createdBranches = 0;             // Must be signed!
        Node    *parent = nullptr;
        Node    *branches = nullptr;  // Must be initialized to NULL
        float    score = 0.f; //0.01f;    // Start with some positive value to prevent divide by zero error
        float    UCBscore;         // Placeholder used during UCB calc.
        Move     moveHere;
        SWORD    visits = 1; // Must be '1' to stop 'nan' // todo if changed also chage in ucbselect()!

SWORD branchScore = 0;
int dp = 9999;

        constexpr Node()
          : moveHere(0)
        {}

        constexpr Node(Node *newParent, Move move)
          : activeBranches(never_expanded),
            parent(newParent),
            // Ownership is implicit (the 'owner' is the player who makes this move!):
            moveHere(move)
        {}

        Node(const Node&)            = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&)                 = delete;
        Node& operator=(Node&& other)
        {
            if (this != &other)
            {
                activeBranches  = other.activeBranches;
                createdBranches = other.createdBranches;
                parent          = other.parent;
                branches        = other.branches;
                visits          = other.visits;
                score           = other.score;
                #if 0
                  UCBscore = other.UCBscore;
                #endif
                moveHere        = other.moveHere;
                branchScore = other.branchScore;
                dp = other.dp;
                for (int i=0; i<other.createdBranches; ++i)
                    branches[i].parent = this;
            }
            return *this;
        }
    };

    static_assert(sizeof(Node) <= 64);


/****************************************/
/*                           Ai context */
/****************************************/
    template <int NumNodes, class IntType>
    struct Ai_ctx
    {
        static constexpr int numNodes = NumNodes;
        BitAlloc<NumNodes, IntType> bitalloc;
        Node nodePool[NumNodes];

        Ai_ctx() {}
        Ai_ctx(const Ai_ctx&) = delete;
        Ai_ctx& operator=(const Ai_ctx&) = delete;
    };


    template <int NumNodes, typename IntType>
    inline Node *disconnectBranch(Ai_ctx<NumNodes, IntType>& ai_ctx, Node *parent, const Node *removeMe)
    {
        aiAssert(parent);
        aiAssert(parent->activeBranches > 0);
        aiAssert(parent->branches);
        aiAssert(parent == removeMe->parent);
        aiAssert((removeMe-ai_ctx.nodePool)>=0 && (removeMe-ai_ctx.nodePool)<ai_ctx.numNodes);
        aiAssert((parent->parent==nullptr) || (removeMe->activeBranches <= 0));

        std::printf("disc pos: %d len: %d parnode: %p \n", removeMe-ai_ctx.nodePool, 1, parent-ai_ctx.nodePool);


        const auto posOfChild = removeMe - parent->branches;
        Node& swapDst = parent->branches[posOfChild]; // Bypass the 'const'

        // Discard all child branches of each of the child nodes of the node
        // we want to remove (recursive):
        //for (int i=0; i<swapDst.createdBranches; ++i)
        //    discard(ai_ctx, swapDst.branches[i]);

        // 'parent->parent' is correct:
        const bool branchIsChildOfRoot = parent->parent == nullptr; // Keep full set of moves for root
        if (removeMe->createdBranches && !branchIsChildOfRoot)
        {
            std::printf("rem: %d \n", removeMe->createdBranches);
            ai_ctx.bitalloc.free(removeMe->branches - ai_ctx.nodePool, removeMe->createdBranches);
        }

        // But don't discard() the child node, that is the slot we will swap into:
        //discard(ai_ctx, swapDst); // incorrect!! // todo test!!


        const Move removedMoveHere = swapDst.moveHere;
        const auto removedVisits   = swapDst.visits;
        const auto removedScore    = swapDst.score;
        const auto removedBranchScore = swapDst.branchScore;

        Node& swapSrc = parent->branches[parent->activeBranches-1];

        // Don't swap w/ itself if the to-be-removed node is last:
        if (&swapDst == &parent->branches[parent->activeBranches-1])
        {
            parent->activeBranches -= 1;
            return &swapSrc;
        }

        swapDst.activeBranches  = swapSrc.activeBranches;
        swapDst.createdBranches = swapSrc.createdBranches;
        swapDst.moveHere        = swapSrc.moveHere;
        swapDst.visits          = swapSrc.visits;
        swapDst.score           = swapSrc.score;
        swapDst.branches        = swapSrc.branches;


        swapDst.branchScore = swapSrc.branchScore;
        swapDst.dp = swapSrc.dp;


        // Establish new "parent" for each branch node after swap
        // (the "parent" was prev. &swapSrc):
        //for (int i=0; i<swapDst.createdBranches; ++i)
        for (int i=0; i<swapDst.activeBranches; ++i)
            swapDst.branches[i].parent = &swapDst;

     //   #if 0
          // None of this is needed:
          // // ^^ WRONG!!! ALL of thiis needed!!!!
          swapSrc.activeBranches = Node::removed;
          swapSrc.moveHere       = removedMoveHere;
          swapSrc.visits         = removedVisits;
          swapSrc.score          = removedScore;
          swapSrc.branches       = nullptr;
          swapSrc.branchScore = removedBranchScore;

          //for (int i=0; i<swapSrc.createdBranches; ++i)
            //swapSrc.branches[i].parent = &swapSrc;

       // #endif

        parent->activeBranches -= 1;
        aiAssert(swapSrc.parent == parent);
        return &swapSrc;
    }


/****************************************/
/*                               Memory */
/****************************************/
    template <typename Ctx, GameMove Move>
    inline constexpr auto& insertNodeIntoPool(Ctx& ctx, const int pos, Node *node, Move move)
    {
        aiAssert(pos < ctx.numNodes);
        return ctx.nodePool[pos] = Node(node, move);
    }


/****************************************/
/*                            Simulator */
/****************************************/
    template <int MaxRandSims, Gameboard Board, typename Rndfunc>
    constexpr float simulate(const Board& original, Rndfunc rand)
    {
        int simWins = 0;
        // Run simulations:
        for (int i=0; i<MaxRandSims; ++i)
        {
            Board boardSim = original.clone();
            // Start single sim, run until end:
            auto outcome = Outcome::running;
            do
            {
                typename Board::AvailMoves availMoves;
                const int nAvailMovesForThisTurn = boardSim.generateMovesAndGetCnt(availMoves);
                if (nAvailMovesForThisTurn == 0)
                    break;
                const int idx = rand() % nAvailMovesForThisTurn;
                outcome = boardSim.doMove( availMoves[idx] );
                boardSim.switchPlayer();
            } while (outcome==Outcome::running);
            // Count winner/loser:
            if (outcome != Outcome::draw)
            {
                const bool weWon = boardSim.getWinner() == original.getCurrentPlayer();
                simWins += weWon;
                simWins -= !weWon;
            }
        }
        const float winRatio = (simWins-0.f) / (MaxRandSims-0.f);
        return winRatio; // range from [-1,1]
    }


/****************************************/
/*                              Minimax */
/* Both "undeterminable" and            */
/* "indeterminable" are correct and can */
/* be used interchangeably in many      */
/* contexts. However, "indeterminable"  */
/* is more commonly used in formal or   */
/* academic writing.                    */
/* (https://chatgptfree.ai/)            */
/****************************************/
    inline constexpr SWORD indeterminable = -9998;
    static_assert((indeterminable&1) == 0);

    template <Gameboard Board>
    constexpr SWORD minimax(const Board& current, const Move mv, const int depth)
    {
        if (depth==0)
            return indeterminable;

        Board clone = current.clone();
        clone.switchPlayer();
        const Outcome outcome = clone.doMove(mv);
        if (outcome != Outcome::running)
        {
            if (outcome == Outcome::draw)
                return 0;
            else if (clone.getWinner() != current.getCurrentPlayer())
                return -1;
            else
                return 1;
        }
        typename Board::AvailMoves availMoves;
        int nMoves = clone.generateMovesAndGetCnt(availMoves);
        nMoves -= 1;
        int minimaxScore = -2;
        while (nMoves >= 0)
        {
            const Move moveHere = availMoves[nMoves];
            const int mnx = -minimax(clone, moveHere, depth-1);
            if (mnx>minimaxScore)
                minimaxScore = mnx;
            nMoves -= 1;
        }
        return minimaxScore==-2 ? 0 : minimaxScore;
    }

    template <Gameboard Board>
    inline constexpr SWORD minimax(const Board& current, const int MaxDepth)
    {
        Board clone = current.clone();
        clone.switchPlayer();
        typename Board::AvailMoves availMoves;
        int nMoves = clone.generateMovesAndGetCnt(availMoves);
        nMoves -= 1;
        SWORD best = -2;
        while (nMoves >= 0)
        {
            const Move moveHere = availMoves[nMoves];
            const int mnx = -minimax(clone, moveHere, MaxDepth);
            if (mnx>best)
                best = mnx;
            nMoves -= 1;
        }
        return best==-2 ? 0 : best;
    }


/****************************************/
/*                     Save the planet!!*/
/* Reuse existing search-tree!          */
/****************************************/
   // void activateBranch(Node *root)
    //{
        // wasm prohibits tail-call opt.:
        // https://github.com/WebAssembly/tail-call/blob/master/proposals/tail-call/Overview.md
    //}


/****************************************/
/*                               result */
/****************************************/



/****************************************/
/*                                 mcts */
/****************************************/
    template <int NumNodes, int MaxIterations, int SimDepth, int MinimaxDepth, std::integral IntType, Gameboard Board, typename Rndfunc>
    constexpr auto mcts(const Board& boardOriginal, Ai_ctx<NumNodes, IntType>& ai_ctx, Rndfunc rand)
    {
        [[maybe_unused]] auto UCBselectBranch =
            [](const Node& node) -> Node *
            {
                for (int i=0; i<node.activeBranches; ++i)
                {
                    aiAssert(node.branches);
                    Node& arm = node.branches[i];
                    if (arm.visits == 1)
                    {
                        // UCB requires each slot-machine 'arm' to be tried at least once (https://u.cs.biu.ac.il/~sarit/advai2018/MCTS.pdf):
                        return &arm; // Prevent x/0
                    }

                    // todo: epl/expl shld be adjusted to utilize max node use: check rate of node use vs nodes released and adjust based on that!
                    const float exploit = arm.score / (arm.visits-0.f);
                    constexpr float exploration_C = 1.21f;
                    constexpr float some_multiplier = 2.f; // (http://www.incompleteideas.net/609%20dropbox/other%20readings%20and%20resources/MCTS-survey.pdf)
                    const float explore = exploration_C * aiSqrt((some_multiplier * aiLog(node.visits)) / arm.visits);
                    arm.UCBscore = exploit + explore; // <- 'Promising' meaning "good information" not "winning"!
                }
                int pos = node.activeBranches - 1;
                float best = node.branches[pos].UCBscore;
                for (int i=pos-1; i>=0; --i)
                {
                    if (node.branches[i].UCBscore > best)
                    {
                        best = node.branches[i].UCBscore;
                        pos = i;
                    }
                }
                return &node.branches[pos];
            };

        [[maybe_unused]] auto pickUnexplored =
            [](const Node& node) -> Node *
            {
                for (int i=0; i<node.activeBranches; ++i)
                {
                    Node& branch = node.branches[i];
                    if (branch.visits == 0)
                        return &branch;
                }
                return &node.branches[0];
            };

        [[maybe_unused]] auto pickRandom =
            [](const Node& node) -> Node *
            {
                return nullptr;
            };

        Node *root = &insertNodeIntoPool(ai_ctx, 0, nullptr, Move{0});
        ai_ctx.bitalloc.clearAll();
        [[maybe_unused]] const auto throwaway = ai_ctx.bitalloc.largestAvailChunk(1);
        //for (int i=0; i<NumNodes; ++i)
        {
            //   ai_ctx.nodePool[i].activeBranches = -1;
            //   ai_ctx.nodePool[i].createdBranches = 0;
        }

        int cutoffDepth = 9999;
        SWORD rootMovesRemaining;
        for (int iterations=0; root->activeBranches!=0 && iterations<MaxIterations; ++iterations)
        {
            Node *selectedNode = root;
            Board boardClone = boardOriginal.clone();
            Outcome outcome = Outcome::running;
            int depth = 1;




            // 1. Traverse tree and select leaf:
            Node *parentOfSelected;
            while ((selectedNode->activeBranches > 0) && (rootMovesRemaining == 0))
            {
                parentOfSelected = selectedNode;
                aiAssert(selectedNode->branches);
                selectedNode = UCBselectBranch(*selectedNode);
                //aiAssert(selectedNode->branchScore == 0);
                aiAssert(selectedNode->parent == parentOfSelected);
                const Move moveHere = selectedNode->moveHere;
                outcome = boardClone.doMove(moveHere);
                boardClone.switchPlayer();
                depth += 1;
                if (outcome != Outcome::running)
                {
                    //cutoffDepth = cutoffDepth < depth ? cutoffDepth : depth;
                    //  selectedNode = selectedNode->parent;
                }
                if (depth == cutoffDepth)
                {
                    //std::printf("\033[1;36m %d \033[0m \n", depth);
                    //assert(false);
                    //break;
                }
            }

            //if (selectedNode->activeBranches != Node::never_expanded) { std::printf("------never exp. ---%d \n", selectedNode==root); break; } // insuff nodes!
            //if (selectedNode->parent && selectedNode->parent->activeBranches>0) aiAssert(selectedNode->parent->branches);




            //aiAssert(selectedNode->activeBranches <= 0);
            //aiAssert(selectedNode->score < 1);




            // 2. Add (allocate/expand) branch/child nodes to leaf:
            if (selectedNode->activeBranches==Node::never_expanded && outcome==Outcome::running)
            {
                typename Board::AvailMoves availMoves;
                int nValidMoves = boardClone.generateMovesAndGetCnt(availMoves);
                const auto availNodes = ai_ctx.bitalloc.largestAvailChunk(nValidMoves);
                nValidMoves = availNodes.len; // This line is critical!
                int nodePos = availNodes.posOfAvailChunk;
                if (selectedNode == root)
                    rootMovesRemaining = nValidMoves;

                std::printf("np:%d, avl:%d \n", nodePos, nValidMoves);

                if ((nodePos+nValidMoves) >= ai_ctx.numNodes) [[unlikely]] // This can happen at the end
                {
                    std::printf("\033[1;35mexceeded! %d vs  %d \n\033[0m", ai_ctx.numNodes, nodePos+nValidMoves);
                    // Can't be salvaged. Once we are out of nodes we can't release already
                    // alloc'd branches (cuz we must reach a terminal node for that). We are stuck:
                    aiAssert(false);
                    break;
                    //nValidMoves = nValidMoves - ((nodePos+nValidMoves)-nValidMoves);
                    //nValidMoves -= 1;
                }
                if (nValidMoves == 0) [[unlikely]]
                {
                    std::printf("\033[1;35m................npos %d mves %d iii: %d \n\033[0m", nodePos, nValidMoves, iterations);
                    // todo: flag!
                    //iterations = MaxIterations;
                    // not enough nodes!!

                    aiAssert(false); // outcome is 'running' but should be 'draw'!!!
                    break; // Top loop!
                }

                //aiAssert((nodePos+nValidMoves) >= 0); &&
                //aiAssert((nodePos+nValidMoves) <= ai_ctx.numNodes);
                //if ((nodePos+nValidMoves) < 0 || (nodePos+nValidMoves) >= ai_ctx.numNodes) [[unlikely]]
                //    continue;
                selectedNode->activeBranches  = nValidMoves;
                selectedNode->createdBranches = nValidMoves;
                nValidMoves -= 1;

                if (ai_ctx.nodePool[nodePos].activeBranches > 0){
                    std::printf("\033[1;31mIN USE! %d \033[0m\n", ai_ctx.nodePool[nodePos].activeBranches);
                }
                Move move = availMoves[nValidMoves];
                selectedNode->branches = &insertNodeIntoPool(ai_ctx, nodePos, selectedNode, move);


                while (nValidMoves--)
                {
                    nodePos += 1;
                    move = availMoves[nValidMoves];
                    [[maybe_unused]] const auto& unusedNode =
                        insertNodeIntoPool(ai_ctx, nodePos, selectedNode, move);
                }
                std::printf("\033[1;37mnew branch:%p brch:%p \n", selectedNode-ai_ctx.nodePool, (&selectedNode->branches[0])-ai_ctx.nodePool);
                for (int i=0; false && i<selectedNode->createdBranches; ++i) // todo put this loop into the assert
                {
                    std::printf("\033[1;36m%p (%d) exp:%d mv:%d \n", (&selectedNode->branches[i])-ai_ctx.nodePool, selectedNode->branches[i].moveHere, 0, selectedNode->branches[i].moveHere);
                    //  std::printf("mvs: %d, avllen: %d, POS: %d, npos: %d \n", nValidMoves, availNodes.len, availNodes.posOfAvailChunk, nodePos);
                    aiAssert(selectedNode != &selectedNode->branches[i]);
                    aiAssert(selectedNode->branches && selectedNode->activeBranches>0);
                    aiAssert(selectedNode->branches[i].parent == selectedNode);
                }
                std::printf("\033[0m\n");
            }







            // 3a. Ensure all moves on root node are visited once:
            if (rootMovesRemaining > 0)
            {
                rootMovesRemaining -= 1;
                selectedNode = &root->branches[rootMovesRemaining];
                outcome = boardClone.doMove( selectedNode->moveHere );
                boardClone.switchPlayer();
            }
            // 3b. Pick (select) a node for analysis:
            else if (selectedNode->activeBranches > 0)
            {
                aiAssert(selectedNode->branches);
                // Should be rand to ensure fair distribution:
                const auto sample = rand() % selectedNode->activeBranches;
                // ^^^ What that means is that if we bias towards a specific kind of node: good,
                // bad or something else, then we will fail to discover unexpected possibilities!
                selectedNode = &selectedNode->branches[sample];
                //aiAssert(selectedNode->score < 1.f);
                outcome = boardClone.doMove( selectedNode->moveHere );
                boardClone.switchPlayer();
                //depth += 1;
            }


            //  bool stop = false;
            float score = 0.f; // -1: loss, 1: win, 0: draw // 0 means loss, 1 means draw, 2 means win!!!
            /* if (parent->activeBranches <= 0) {
            std::printf("\033[1;35m act:%d created:%d , fin?: %d \033[0m \n", parent->activeBranches, parent->createdBranches,  outcome!=Board::Outcome::running);
            // break;
            // continue;
            stop = true;
            if (outcome == Board::Outcome::draw) //[[likely]]
            // draw:
            score += 1.f;
            else if (boardOriginal->getCurrentPlayer() != boardClone->getCurrentPlayer())
            // win:
            score += 2.f;
            selectedNode = ppp;
            }*/
            //aiAssert(parent != selectedNode);   // Ensure the graph remains acyclic
            //aiAssert(parent->activeBranches>0); // Ensure parent node has at least one




            // 4. Determine branch score ("rollout"):
            SWORD branchscore = 0;
            bool disconnect = false;
            if (outcome != Outcome::fin) // <- This one
            #if 0
              if (outcome == Outcome::running) // <- Not this
            #endif
            {
                // This one:
                const SWORD polarity = boardClone.getCurrentPlayer()!=boardOriginal.getCurrentPlayer() ? -1 : 1;
                #if 0
                  // Not this:
                  const SWORD polarity = boardClone.getWinner()!=boardOriginal.getCurrentPlayer() ? -1 : 1;
                #endif
                branchscore = minimax(boardClone, MinimaxDepth);
                if ((branchscore&1) == 0)
                {
                    branchscore = 0;
                    // Simulate to get an estimation of the quality of this position:
                    score = simulate<SimDepth>(boardClone, rand);
                    score *= polarity-0.f;
                }
                else
                {
                    score = branchscore-0.f;
                    score *= polarity-0.f;
                    branchscore *= polarity;
                    disconnect = true;
                }

                //score += 2.f;//todo test!


                // todo: flag avg sim score for this turn
            }
            else
            {
                cutoffDepth = cutoffDepth < depth ? cutoffDepth : depth;
                selectedNode->dp = depth<selectedNode->dp ? depth : selectedNode->dp;

                // A terminal node is equivalent to a 100% simulation score:
                if (outcome == Outcome::draw)
                {
                    // Nothing?
                }
                // '!=' if "boardClone.getCurrentPlayer()" and '==' if "boardClone.getWinner()":
                else if (boardOriginal.getCurrentPlayer() == boardClone.getWinner())
                {
                    // win:
                    score = 1.f;
                    //score = boardClone->getCurrentPlayer() == boardOriginal->getCurrentPlayer() ? -1 : 1;
                   branchscore = 1;
                }
                else
                {
                    // lose:
                    score = -1.f;
                    //score = boardClone->getCurrentPlayer() == boardOriginal->getCurrentPlayer() ? 1 : -1;

                  branchscore = -1;
                }

                disconnect = true;

            }


            if (disconnect)
            {


                Node *child = selectedNode;
                Node *parent = selectedNode->parent;
                aiAssert(parent);
                aiAssert(selectedNode != root);                  // Ensure 'selectedNode' not root
                //aiAssert([&]{ return outcome==Board::Outcome::running ? parent->branches!=nullptr : true; }());
                //aiAssert([&]{ return outcome==Board::Outcome::running ? parent->activeBranches>0 : true; }());
                aiAssert([&]{ return parent->branches!=nullptr; }());
                // terminal nodes have no branches:
                aiAssert(parent->activeBranches!=0); //>0 || parent->activeBranches==Node::never_expanded);

                while (parent)
                {
                    std::printf("/// active:%d *brnch-start:%p root:%p parent:%p dep:%d sel:%p scr:%2.2f\n", parent->activeBranches, parent->branches-ai_ctx.nodePool, root-ai_ctx.nodePool, parent-ai_ctx.nodePool, 0, selectedNode-ai_ctx.nodePool, score);

                    std::printf("still active 'siblings': ");
                    for (int i=0; false&&i<parent->activeBranches; ++i)
                    {
                        std::printf("\033[0;33m%p scr:%2.2f  \033[0m", (&parent->branches[i])-ai_ctx.nodePool, parent->branches[i].score);
                    }
                    std::printf("%d \n", parent->activeBranches);


                    // todo: reassign wont branch up!!! (bug!!)
                    selectedNode = disconnectBranch(ai_ctx, parent, child);




                    if (parent->activeBranches != 0)
                    {
                        break; // Stop
                    }
                    else
                    {
                        //__builtin_trap();

                        // Apply minimax:
                        // We can still use minimax even if we haven't seen all moves on a node if they were
                        // not all created during step 2 (allocate). Still better than nothing...
                        /*Node *bla = parent; //child;

                        //while (bla && bla->activeBranches==0)
                        //if (bla->activeBranches<=0)



                        {
                        a(*bla);
                        mize tmp = a;
                        a = b;
                        b = tmp;

                        //bla->score += 10000.f * bla->mnx;

                        printf("\033[1;34m val %d \033[0m] ", bla->mnx);
                        bla = bla->parent;
                        }
                        */
                        child = parent;
                        aiAssert(parent != parent->parent);
                        parent = parent->parent; // Lolz
                    }
                }
            }




            // 5. Backprop/update tree:
            aiAssert([selectedNode]
                     {
                         // Floyd's Cycle
                         Node *fast = selectedNode;
                         Node *slow = selectedNode;
                         while (fast)
                         {
                             if (fast)
                             fast = fast->parent;
                             if (fast == slow)
                             {
                                 fast = nullptr; // 'break'
                                 return false;
                             }
                             if (fast)
                             fast = fast->parent;
                             slow = slow->parent;
                         }
                         return true;
                     }()
                    );

            float cur = 1.f;
            //int yyy = fib(depth);

            SWORD depthScore = 1;
           // for (Node *tmp=selectedNode; tmp; ) { tmp=tmp->parent; branchDepth+=1; cur=cur+0.002f; }
            std::printf("scre sel:%p score: %1.3f %d max:%d \n", selectedNode-ai_ctx.nodePool, score, iterations, MaxIterations);

            int child_dp = 9999;

            while (selectedNode != root)
            {
                int dp_temp = selectedNode->dp;
                selectedNode->dp = child_dp<dp_temp ? child_dp : dp_temp;
                child_dp = dp_temp;

                //selectedNode->visits += 1.f ;//- ((cur-0.f)/(depth-0.f)); // Games that have more than two possible outcomes (example:
                //selectedNode->visits += aiLog( 100 * ((cur-0.f)/(depth-0.f)) );

                // This 'if' is used to (optionally) stop counting scores for branches that are deeper
                // than the most immediate node where a turn ends:
                //if ([&]{ if constexpr ((cutoff_scoring&hyperparams)==cutoff_scoring) return branchDepth<cutoff; else return true; }())
                //if (depth <= cutoffDepth)
                {
                    selectedNode->visits += 1;
                    //selectedNode->visits += 1.f ;/// (depth-0.f);
                    // 'score' does not tell us if a position is a winner or not. It tells us
                    // how -interesting- a position is:
                    selectedNode->score += score;
                    selectedNode->branchScore += branchscore;
                    depthScore += 1;
                    //depth += 1;
                    //selectedNode->weight += 1; //(fib(cur)-0.f) / (fib(depth)-0.f); //(aiLog(cur* 10)/2) / (depth-0.f); //((cur-0.f)/(depth-0.f)) ; weight
                //    cur += 1.f;
                    //    if (fib(cur+1) <= yyy)
                    //    cur += 1;

                }
                depth -= 1;

                std::printf("sel:%p score: %1.3f act:%d crt:%d  \n", selectedNode-ai_ctx.nodePool, selectedNode->score, selectedNode->activeBranches, selectedNode->createdBranches);


                selectedNode = selectedNode->parent;
                //  if constexpr ((cutoff_scoring&hyperparams)==cutoff_scoring) { branchDepth -= 1; }


            }
        } // iterations











        // Finally, pick the branch with the best score:
        auto bestVisits = root->branches[0].visits;
      //  auto best = root->branches[0].visits / root->branches[0].score;
        auto bestScore = root->branches[0].score;
        int posVisits=0, posScore=0;
        for (int i=0; i<root->createdBranches; ++i)
        {
            // Add minimax score to final result:
            //if (root->branches[i].mnx == -1)
            //    root->branches[i].visits -= 10000;
            //else if (root->branches[i].mnx == 1)
            //     root->branches[i].visits += 10000;



            //  if constexpr ((count_visits_only&hyperparams)==count_visits_only)
            auto    expectedVisits = root->branches[i].visits;
            // else
            auto    expectedScore = root->branches[i].score;
          //  if (root->branches[i].score <0.001f)
            //    expected = 0;
            //if (expected < 0.001f)



            if (expectedVisits>bestVisits)// && root->branches[i].parent)
            {
                bestVisits = expectedVisits;
                posVisits = i;
            }
            if (expectedScore>bestScore)// && root->branches[i].parent)
            {
                bestScore = expectedScore;
                posScore = i;
            }
        }

        for (int i=0; i<root->createdBranches; ++i)
        {

            if (i==posScore) std::printf("\033[1;31m");
            if (i==posVisits) std::printf("\033[1;36m");
            std::printf("mv: %d bs:%d   ", root->branches[i].moveHere, root->branches[i].branchScore);
            std::printf("s:%4.1f v:%d       dp: %d       act:%d  s/v:%2.3f v/s:%2.5f \033[0m \n", root->branches[i].score, root->branches[i].visits, root->branches[i].dp, root->branches[i].activeBranches,
                root->branches[i].score/ root->branches[i].visits,root->branches[i].visits/root->branches[i].score);
        }
        std::printf(", gap: %f mv: %d \033[1;31mcutoff: %d\033[0m ACT:%d \n",  0, root->branches[posVisits].moveHere, cutoffDepth, root->activeBranches);

        struct MCTS_result
        {
            enum {chunk_overflow,end};
            int errors[end] = {0};
            Move move;
        };
        return MCTS_result{ .move = [root, posVisits, posScore, bestScore]
                                    {
                                        if (bestScore > 0.f)
                                            return root->branches[posScore].moveHere;
                                        return root->branches[posVisits].moveHere;
                                    }()
                          };
    }

} // namespace include_ai


#else // AI_HPP
  #error "double include"
#endif // AI_HPP








/****************************************/
/*                             Research */
/****************************************/
/*
    https://u.cs.biu.ac.il/~sarit/advai2018/MCTS.pdf


    todo: http://www.incompleteideas.net/609%20dropbox/other%20readings%20and%20resources/MCTS-survey.pdf
*/
