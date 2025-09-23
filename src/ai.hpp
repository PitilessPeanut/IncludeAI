#ifndef AI_HPP
#define AI_HPP

#include "asmtypes.hpp"
#include "bitalloc.hpp"
#include "similarity.hpp"
//#include
#include <concepts>
#include <cmath>
#include <cstdio>
#if defined(AI_DEBUG)
  #include <assert.h>
#endif

namespace include_ai {


/****************************************/
/*       Util functions / configuration */
/****************************************/
// sqrt
    inline constexpr float aiSqrt(auto x) { return std::sqrt(x-0.f); }

// log
    inline constexpr float aiLog(auto x) { return std::log(x-0.f); }

// abs
    inline constexpr auto aiAbs(auto x) { return std::abs(x); }

// min/max
    inline float aiMin(float x, float y) { return std::fminf(x, y); }
    inline float aiMax(float x, float y) { return std::fmaxf(x, y); }
    template <typename T>
    inline constexpr T aiMin(T x, T y) { return x < y ? x : y; }
    template <typename T>
    inline constexpr T aiMax(T x, T y) { return x > y ? x : y; }

// assert/debug
    #if defined(AI_DEBUG)
      #define aiAssert(x) assert(x)
      #define aiDebug(x) x
    #else
      #define aiAssert(x)
      #define aiDebug(x)
    #endif


/****************************************/
/*   A 'move'/'action' by a user/player */
/* within the game context              */
/****************************************/
    template <typename T>
    concept GameMove = std::movable<T> || std::copyable<T>;


/****************************************/
/*            outcome of a single round */
/****************************************/
    enum class Outcome { running, draw, fin };


/****************************************/
/*          A view of the game from the */
/* perspective of a player as a Concept */
/*                                      */
/* This 'view' can be either the actual */
/* game state (complete information) or */
/* a guess/estimation of the game state */
/* percieved by one player (incomplete  */
/* information)                         */
/****************************************/
  //  template <typename T>
  //  concept convertible_to_ptrFloat =
  //      std::convertible_to<T, HALF *> ||
  //      std::convertible_to<T, DOUBLE *> ||
   //     std::convertible_to<T, FLOAT *>;

    template <typename T>
    concept Gameview =
        !std::copy_constructible<T> &&
        !std::is_copy_assignable<T>::value &&
        !std::copyable<T> &&
        requires (T obj, const T cobj)
        {
            {cobj.clone()} -> std::same_as<T>;
            {obj.generateMovesAndGetCnt(nullptr)} -> std::convertible_to<int>;
            {obj.doMove(int())} -> std::same_as<Outcome>;
            {obj.switchPlayer()};
            {cobj.getCurrentPlayer()} -> std::equality_comparable;
            {cobj.getWinner()} -> std::equality_comparable;
           // {obj.getNetworkInputs()} -> convertible_to_ptrFloat;
        };


/****************************************/
/*       Node (should be converted from */
/*        an 'array of structs' into a  */
/*       'struct of arrays'...)         */
/* Attn: The Node does *not* store a    */
/* copy of the Board, instead the game  */
/* state must be kept in sync during    */
/* tree expansion (HEADACHE ATTACK!!!)  */
/****************************************/
    template <GameMove Move>
    struct Node
    {
        static constexpr SWORD never_expanded = -1;
        static constexpr SWORD removed = -2; // Debug
        SWORD    activeBranches = never_expanded; // Must be signed!
        SWORD    createdBranches = 0;             // Must be signed!
        Node    *parent = nullptr;
        Node    *branches = nullptr;  // Must be initialized to NULL
        FLOAT    score = 0.f; //0.01f;    // Start with some positive value to prevent divide by zero error
        Move     moveHere;
        SWORD    visits = 1; // Must be '1' to stop 'nan' // todo if changed also chage in ucbselect()!
        FLOAT    UCBscore;         // Placeholder used during UCB calc.
        #ifdef INCLUDEAI__ADD_SCORE_FOR_TERMINAL_NODES
          FLOAT    terminalScore = 0.f; // Keeping another separate score for terminal nodes may not make sense,
                                        // since a terminal condition may pop up at any level of the tree,
                                        // which means that by the time we are back at the root, a wrong node
                                        // could end up having a higher score than the correct one...
        #endif
SWORD branchScore = 0;
int shallowestTerminalDepth = 9999;

        constexpr Node()
          : moveHere(0)
        {}

        constexpr Node(Node *newParent, Move move)
          : activeBranches(never_expanded),
            parent(newParent),
            // Ownership is implicit: the 'owner' is the player who makes this move!
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
            //    branchScore = other.branchScore;
                shallowestTerminalDepth = other.shallowestTerminalDepth;
                for (int i=0; i<other.createdBranches; ++i)
                    branches[i].parent = this;
            }
            return *this;
        }
    };

    static_assert(sizeof(Node<SQWORD>) <= 64);


/****************************************/
/*                           Ai context */
/****************************************/
    template <int NumNodes, GameMove MoveType, BitfieldIntType BitfieldType, class Pattern, int MaxPatterns>
    struct Ai_ctx
    {
        static constexpr int numNodes = NumNodes;
        BitAlloc<NumNodes, BitfieldType> bitalloc;
        Node<MoveType> nodePool[NumNodes];

        static constexpr int maxPatterns = MaxPatterns;
        int storedPatterns = 0;
        Pattern patterns[maxPatterns];
        float maxPatternSimilarity = 1.f;

        Ai_ctx() {}
        Ai_ctx(const Ai_ctx&) = delete;
        Ai_ctx& operator=(const Ai_ctx&) = delete;

        template <Gameview Board>
        void collectPattern(Board& current)
        {
            const Pattern *ptrInputs = current.getNextPattern();
            while (ptrInputs != nullptr)
            {
                if (storedPatterns < maxPatterns)
                {
                    Pattern& target = patterns[storedPatterns];
                    for (int i=0; i<target.patternSize; ++i)
                        target.pattern[i] = ptrInputs->pattern[i];
                    storedPatterns += 1;
                }
                else
                {
                    maxPatternSimilarity = diversify(patterns, *ptrInputs, maxPatterns);
                }
                ptrInputs = current.getNextPattern();
            }
        }

        template <Gameview Board>
        void train(Board& current)
        {
            const Pattern *ptrInputs = current.getNextPattern();
            // todo
        }

        template <Gameview Board>
        float query(Board& current) const
        {
            // todo
            const float win  = 1;// inputs[0];
            const float lose = 1; //-inputs[1];
            return (win+lose) / 2.f;
        }
    };


/****************************************/
/*    Search tree node helper functions */
/****************************************/
    template <int NumNodes, GameMove MoveType, BitfieldIntType BitfieldType, class Pattern, int NumPatterns>
    inline Node<MoveType> *disconnectBranch(Ai_ctx<NumNodes, MoveType, BitfieldType, Pattern, NumPatterns>& ai_ctx,
                                            Node<MoveType> *parent,
                                            const Node<MoveType> *removeMe
                                           )
    {
        aiAssert(parent);
        aiAssert(parent->activeBranches > 0);
        aiAssert(parent->branches);
        aiAssert(parent == removeMe->parent);
        aiAssert((removeMe-ai_ctx.nodePool)>=0 && (removeMe-ai_ctx.nodePool)<ai_ctx.numNodes);
        aiAssert((parent->parent==nullptr) || (removeMe->activeBranches <= 0));

        std::printf("disc pos: %d len: %d parnode: %p \n", removeMe-ai_ctx.nodePool, 1, parent-ai_ctx.nodePool);


        const auto posOfChild = removeMe - parent->branches;
        Node<MoveType>& swapDst = parent->branches[posOfChild]; // Bypass the 'const'

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


        const MoveType removedMoveHere = swapDst.moveHere;
        const auto     removedVisits   = swapDst.visits;
        const auto     removedScore    = swapDst.score;
        const auto     removedBranchScore = swapDst.branchScore;
        const auto    removedShallowestTerminalDepth = swapDst.shallowestTerminalDepth;

        Node<MoveType>& swapSrc = parent->branches[parent->activeBranches-1];

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
        swapDst.shallowestTerminalDepth = swapSrc.shallowestTerminalDepth;


        // Establish new "parent" for each branch node after swap
        // (the "parent" was prev. &swapSrc):
        //for (int i=0; i<swapDst.createdBranches; ++i)
        for (int i=0; i<swapDst.activeBranches; ++i)
            swapDst.branches[i].parent = &swapDst;

        #ifdef INCLUDEAI__PREVENT_PARENT_NODES_FROM_GETTING_UPDATED_CORRECTLY
        #else
          swapSrc.activeBranches = Node<MoveType>::removed;
          swapSrc.moveHere       = removedMoveHere;
          swapSrc.visits         = removedVisits;
          swapSrc.score          = removedScore;
          swapSrc.branches       = nullptr;
          swapSrc.branchScore = removedBranchScore;
          swapSrc.shallowestTerminalDepth = removedShallowestTerminalDepth; // Important!
        #endif

          //for (int i=0; i<swapSrc.createdBranches; ++i)
            //swapSrc.branches[i].parent = &swapSrc;


        parent->activeBranches -= 1;
        aiAssert(swapSrc.parent == parent);
        return &swapSrc;
    }


/****************************************/
/*                               Memory */
/****************************************/
    template <typename Ctx, GameMove MoveType>
    inline constexpr auto& insertNodeIntoPool(Ctx& ctx, const int pos, Node<MoveType> *node, MoveType move)
    {
        aiAssert(pos < ctx.numNodes);
        return ctx.nodePool[pos] = Node(node, move);
    }


/****************************************/
/*                            Simulator */
/* MaxRandSims should be '%3 != 0'      */
/****************************************/
    template <int MaxRandSims, Gameview Board, typename Rndfunc>
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
                typename Board::StorageForMoves storageForMoves;
                const int nAvailMovesForThisTurn = boardSim.generateMovesAndGetCnt(storageForMoves);
                if (nAvailMovesForThisTurn == 0)
                    break;
                const int idx = rand() % nAvailMovesForThisTurn;
                outcome = boardSim.doMove( storageForMoves[idx] );
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
/****************************************/
    constexpr SWORD MinimaxWin              =   1;
    constexpr SWORD MinimaxDraw             =   0;
    constexpr SWORD MinimaxLose             =  -1;
    constexpr SWORD MinimaxInit             =  -2;
    // Both "undeterminable" and "indeterminable" are correct and can be used interchangeably in many
    // contexts. However, "indeterminable" is more commonly used in formal or academic writing. (ai)
    constexpr SWORD MinimaxIndeterminable99 = -99; // Debug
    constexpr SWORD MinimaxIndeterminable   =   0;

    template <Gameview Board, GameMove MoveType>
    constexpr SWORD minimax(const Board& current, const MoveType move, SWORD alpha, SWORD beta, const int depth)
    {
        if (depth==0) { return MinimaxIndeterminable; }

        Board clone = current.clone();
        clone.switchPlayer();
        const Outcome outcome = clone.doMove(move);
        if (outcome != Outcome::running)
        {
            if (outcome == Outcome::draw)
                return MinimaxDraw;
            else if (clone.getWinner() != current.getCurrentPlayer())
                return MinimaxLose;
            else
                return MinimaxWin;
        }
        typename Board::StorageForMoves storageForMoves;
        int nMoves = clone.generateMovesAndGetCnt(storageForMoves);
        nMoves -= 1;
        SWORD minimaxScore = MinimaxInit;
        while (nMoves >= 0)
        {
            const MoveType moveHere = storageForMoves[nMoves];
            const SWORD mnx = -minimax(clone, moveHere, -beta, -alpha, depth-1);
            minimaxScore = aiMax(minimaxScore, mnx);
            alpha        = aiMax(alpha, mnx);
            // Alpha-Beta Pruning (Thank you AI!!!!):
            if (beta <= alpha) {
                break; // Cut off the search
            }
            nMoves -= 1;
        }
        return minimaxScore==MinimaxInit ? MinimaxDraw : minimaxScore;
    }

    template <Gameview Board, GameMove MoveType>
    inline constexpr SWORD minimax(const Board& current, const int MaxDepth)
    {
        Board clone = current.clone();
        clone.switchPlayer();
        typename Board::StorageForMoves storageForMoves;
        int nMoves = clone.generateMovesAndGetCnt(storageForMoves);
        nMoves -= 1;
        SWORD best = MinimaxInit;
        while (nMoves >= 0)
        {
            const MoveType moveHere = storageForMoves[nMoves];
            const SWORD mnx = -minimax(clone, moveHere, MinimaxLose-1, MinimaxWin+1, MaxDepth);
            if (mnx > best)
                best = mnx;
            nMoves -= 1;
        }
        return best==MinimaxInit ? MinimaxDraw : best;
    }


/****************************************/
/*                               result */
/****************************************/
    template <GameMove MoveType>
    struct MCTS_result
    {
        enum { simulations, minimaxes, maxPatternSimilarity, end };
        int statistics[end] = {0};
        MoveType best;
    };

    template <GameMove MoveType>
    class MCTS_Future
    {
    public:
        bool have_result() const
        {
            return false;
        }

        MoveType getResult() const
        {
        }
    };


/****************************************/
/*                                 mcts */
/****************************************/
    template <int MaxIterations,
              int SimDepth,
              int MinimaxDepth,
              GameMove MoveType,
              BitfieldIntType BitfieldType,
              Gameview Board,
              typename AiCtx,
              typename Rndfunc
             >
    constexpr MCTS_result<MoveType> mcts(const Board& boardOriginal, AiCtx& ai_ctx, Rndfunc rand)
    {
        [[maybe_unused]] auto UCBselectBranch =
            [](const Node<MoveType>& node) -> Node<MoveType> *
            {
                for (int i=0; i<node.activeBranches; ++i)
                {
                    aiAssert(node.branches);
                    Node<MoveType>& arm = node.branches[i];
                    if (arm.visits == 1)
                    {
                        // UCB requires each slot-machine 'arm' to be tried at least once (https://u.cs.biu.ac.il/~sarit/advai2018/MCTS.pdf):
                        return &arm; // Prevent x/0
                    }

                    // todo: epl/expl shld be adjusted to utilize max node use: check rate of node use vs nodes released and adjust based on that!
                    // todo2: score prefer if > 0?? (same like at the end????)
                    const float exploit = arm.score / (arm.visits-0.f); // <- This
                    #if 0
                      const float exploit = std::abs(arm.score) / (arm.visits-0.f); // <- Not this
                    #endif
                    constexpr float exploration_C = 1.618f;// * 2; // useless?
                    constexpr float Hoeffdings_multiplier = 1.f; //2.f; // (http://www.incompleteideas.net/609%20dropbox/other%20readings%20and%20resources/MCTS-survey.pdf)
                    const float explore = exploration_C * aiSqrt((Hoeffdings_multiplier * aiLog(node.visits)) / arm.visits);
                    /*

                        def Q(self):  # returns float
        return self.total_value / (1 + self.number_visits)

    def U(self):  # returns float
        return (math.sqrt(self.parent.number_visits)
                * self.prior / (1 + self.number_visits))

    def best_child(self, C):
        return max(self.children.values(),
                   key=lambda node: node.Q() + C*node.U())

                    */
                    arm.UCBscore = exploit + explore;
                    #ifdef INCLUDEAI__FORCE_REVISIT_IDENTICAL_SCORE
                      for (int j=0; j<node.activeBranches; ++j)
                      {
                        if (&node.branches[j]!=&arm && node.branches[j].UCBscore==arm.UCBscore && node.branches[j].visits==arm.visits)
                            return &arm;
                      }
                    #endif
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
            [](const Node<MoveType>& node) -> Node<MoveType> *
            {
                for (int i=0; i<node.activeBranches; ++i)
                {
                    Node<MoveType>& branch = node.branches[i];
                    if (branch.visits == 0)
                        return &branch;
                }
                return &node.branches[0];
            };

        [[maybe_unused]] auto pickRandom =
            [](const Node<MoveType>& node) -> Node<MoveType> *
            {
                return nullptr; // todo!!
            };


        Node<MoveType> *placeholder = nullptr; // Prevent gcc from deducting the wrong type... 🙄
        Node<MoveType> *root = &insertNodeIntoPool(ai_ctx, 0, placeholder, MoveType{});
        ai_ctx.bitalloc.clearAll();
        [[maybe_unused]] const auto throwaway = ai_ctx.bitalloc.largestAvailChunk(1);
        for (int i=0; i<AiCtx::numNodes; ++i)
        {
            ai_ctx.nodePool[i].activeBranches = -1;
            ai_ctx.nodePool[i].createdBranches = 0;
        }

        int cutoffDepth = 9999;
        FLOAT threshold = 1.f; // 'threshold' above which the result of the neuralnet is used, not minimax or randroll
        SWORD rootMovesRemaining;
        MCTS_result<MoveType> mcts_result;
        for (int iterations=0; root->activeBranches!=0 && iterations<MaxIterations; ++iterations)
        {
            Node<MoveType> *selectedNode = root;
            Board boardClone = boardOriginal.clone();
            Outcome outcome = Outcome::running;
            int depth = 1;




            // 1. Traverse tree and select leaf:
            Node<MoveType> *parentOfSelected;
            while ((selectedNode->activeBranches > 0) && (rootMovesRemaining == 0))
            {
                parentOfSelected = selectedNode;
                aiAssert(selectedNode->branches);
                selectedNode = UCBselectBranch(*selectedNode);
                //aiAssert(selectedNode->branchScore == 0);
                aiAssert(selectedNode->parent == parentOfSelected);
                const MoveType moveHere = selectedNode->moveHere;
                [[maybe_unused]] const auto outcome = boardClone.doMove(moveHere);
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
            if (selectedNode->activeBranches==Node<MoveType>::never_expanded && outcome==Outcome::running)
            {
                typename Board::StorageForMoves storageForMoves;
                int nValidMoves = boardClone.generateMovesAndGetCnt(storageForMoves);
                const auto availNodes = ai_ctx.bitalloc.largestAvailChunk(nValidMoves);
                nValidMoves = availNodes.length; // This line is critical!
                int nodePos = availNodes.posOfAvailChunk;
                if (nodePos == -1) [[unlikely]]
                {
                    // No more nodes available, stopping condition!
                    // todo: record this in the result!
                    break;
                }
                if (selectedNode == root)
                    rootMovesRemaining = nValidMoves;

                //std::printf("np:%d, avl:%d \n", nodePos, nValidMoves);

                if ((nodePos+nValidMoves) >= ai_ctx.numNodes) [[unlikely]] // This can happen at the end
                {
                    // todo: out of mem is stopping condition!
                    std::printf("\033[1;35mexceeded! %d vs  %d \n\033[0m", ai_ctx.numNodes, nodePos+nValidMoves);
                    // Can't be salvaged. Once we are out of nodes we can't release already
                    // alloc'd branches (cuz we must reach a terminal node for that). We are stuck:
                    aiAssert(false);
                    break;
                    //nValidMoves = nValidMoves - ((nodePos+nValidMoves)-nValidMoves);
                    //nValidMoves -= 1;
                }

                // If this gets triggered there is a bug in the game. There can NEVER
                // be a situation where a player can't move but game is still running!!!!:
                aiAssert(nValidMoves > 0); // In one example checking for draw condition was missing, leading to this getting triggered...

                //aiAssert((nodePos+nValidMoves) >= 0); &&
                //aiAssert((nodePos+nValidMoves) <= ai_ctx.numNodes);
                //if ((nodePos+nValidMoves) < 0 || (nodePos+nValidMoves) >= ai_ctx.numNodes) [[unlikely]]
                //    continue;
                selectedNode->activeBranches  = nValidMoves;
                selectedNode->createdBranches = nValidMoves;
                nValidMoves -= 1;

                aiAssert(ai_ctx.nodePool[nodePos].activeBranches <= 0);
                MoveType move = storageForMoves[nValidMoves];
                selectedNode->branches = &insertNodeIntoPool(ai_ctx, nodePos, selectedNode, move);


                while (nValidMoves--)
                {
                    nodePos += 1;
                    move = storageForMoves[nValidMoves];
                    [[maybe_unused]] const auto& unusedNode =
                        insertNodeIntoPool(ai_ctx, nodePos, selectedNode, move);
                }
                //std::printf("\033[1;37mnew branch:%p brch:%p \033[0m \n", selectedNode-ai_ctx.nodePool, (&selectedNode->branches[0])-ai_ctx.nodePool);
                for (int i=0; false && i<selectedNode->createdBranches; ++i) // todo put this loop into the assert
                {
                    std::printf("\033[1;36m%p (%d) exp:%d mv:%d \033[0m \n", (&selectedNode->branches[i])-ai_ctx.nodePool, selectedNode->branches[i].moveHere, 0, selectedNode->branches[i].moveHere);
                    aiAssert(selectedNode != &selectedNode->branches[i]);
                    aiAssert(selectedNode->branches && selectedNode->activeBranches>0);
                    aiAssert(selectedNode->branches[i].parent == selectedNode);
                }
            }






            // 3a. Ensure all moves on root node are visited once:
            if (rootMovesRemaining > 0)
            {
                rootMovesRemaining -= 1;
                selectedNode = &root->branches[rootMovesRemaining];
                outcome = boardClone.doMove( selectedNode->moveHere );
                boardClone.switchPlayer();
                if (selectedNode->moveHere == 59)
                {
                    std::printf("root move: %d \n", selectedNode->moveHere);
                }
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
            bool disconnect = false;
            if (outcome != Outcome::fin) // <- This one
            #if INCLUDEAI__BRANCH_ON_WRONG_TERMINAL_CONDITION
              if (outcome == Outcome::running) // <- Not this
            #endif
            {
                #ifndef INCLUDEAI__CONFUSE_CURRENT_PLAYER_WITH_WINNER
                  // correct:
                  const SWORD polarity = boardClone.getCurrentPlayer()!=boardOriginal.getCurrentPlayer() ? -1 : 1;
                #else
                  // incorrect:
                  const SWORD polarity = boardClone.getWinner()!=boardOriginal.getCurrentPlayer() ? -1 : 1;
                #endif

                const FLOAT neuroscore = ai_ctx.query(boardClone);
                //if (true && aiAbs(neuroscore) < threshold) // todo!
                {
                    const SWORD branchscore = minimax<Board, MoveType>(boardClone, MinimaxDepth) * polarity;
                    aiAssert(-abs(branchscore) != MinimaxIndeterminable99);
                    if (branchscore == MinimaxIndeterminable)
                    {
                        // Simulate to get an estimation of the quality of this position:
                        score = simulate<SimDepth>(boardClone, rand);
                        score *= polarity-0.f;
                        mcts_result.statistics[MCTS_result<MoveType>::simulations] += 1;
                    }
                    else
                    {
                        score = branchscore-0.f;
                        const bool sameSign = (score * neuroscore) > 0;
                        if (sameSign)
                            threshold = aiMin(neuroscore, threshold);
                        #ifdef INCLUDEAI__INSTANT_LOBOTOMY
                          disconnect = true;
                        #endif
                        mcts_result.statistics[MCTS_result<MoveType>::minimaxes] += 1;
                    }
                }
                //else
                {
                  //  score = neuroscore * (polarity-0.f);
                }
            }
            else
            {
                cutoffDepth = cutoffDepth < depth ? cutoffDepth : depth;
                selectedNode->shallowestTerminalDepth = depth<selectedNode->shallowestTerminalDepth ? depth : selectedNode->shallowestTerminalDepth;

                // A terminal node is equivalent to a 100% simulation score:
                if (outcome != Outcome::draw)
                {
                    constexpr float win = 1.f, lose = -1.f;
                    #ifndef INCLUDEAI__CONFUSE_LAST_PLAYER_WITH_WINNER
                      // This one:
                      score = boardOriginal.getCurrentPlayer() == boardClone.getWinner() ? win : lose;
                    #else
                      // Not this:
                      score = boardOriginal.getCurrentPlayer() != boardClone.getCurrentPlayer() ? win : lose;
                    #endif
                }
                disconnect = true;
            }


            if (disconnect)
            {


                Node<MoveType> *child = selectedNode;
                Node<MoveType> *parent = selectedNode->parent;
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


                    Node<MoveType> *todo = selectedNode->parent;
                    selectedNode = disconnectBranch(ai_ctx, parent, child);
                    aiAssert(selectedNode->parent == parent);
                    //aiAssert(todo == selectedNode->parent); // 100% fail!!!
                    //selectedNode->parent = todo; // fix this goes in  disconnectBranch()




                    if (parent->activeBranches != 0)
                    {
                        break; // Stop
                    }
                    else
                    {
                        child = parent;
                        aiAssert(parent != parent->parent);
                        parent = parent->parent; // Lolz
                    }
                }
            }

            // Floyd's Cycle. Ensure the graph remains acyclic (debug only!):
            #if defined(AI_DEBUG)
              aiAssert([selectedNode]
                       {
                           Node<MoveType> *fast = selectedNode;
                           Node<MoveType> *slow = selectedNode;
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
            #endif






            // 5. Backprop/update tree:
            int child_dpt = 9999;
            while (selectedNode != root)
            {
                int dpt_temp = selectedNode->shallowestTerminalDepth;
                selectedNode->shallowestTerminalDepth = child_dpt<dpt_temp ? child_dpt : dpt_temp;
                child_dpt = dpt_temp;

                //selectedNode->visits += 1.f ;//- ((cur-0.f)/(depth-0.f)); // Games that have more than two possible outcomes (example:
                //selectedNode->visits += aiLog( 100 * ((cur-0.f)/(depth-0.f)) );

                // This 'if' is used to (optionally) stop counting scores for branches that are deeper
                // than the most immediate node where a turn ends:
                //if ([&]{ if constexpr ((cutoff_scoring&hyperparams)==cutoff_scoring) return branchDepth<cutoff; else return true; }())
                //if (depth <= cutoffDepth)
                {
                    // 'visits' does not tell us if a position is a winner or not. It tells us
                    // how -interesting- a position is:
                    selectedNode->visits += 1;
                    selectedNode->score += score;

                    //depth += 1;
                    //selectedNode->weight += 1; //(fib(cur)-0.f) / (fib(depth)-0.f); //(aiLog(cur* 10)/2) / (depth-0.f); //((cur-0.f)/(depth-0.f)) ; weight
                //    cur += 1.f;
                    //    if (fib(cur+1) <= yyy)
                    //    cur += 1;

                }
                depth -= 1;

                //std::printf("sel:%p score: %1.3f act:%d crt:%d  \n", selectedNode-ai_ctx.nodePool, selectedNode->score, selectedNode->activeBranches, selectedNode->createdBranches);


                selectedNode = selectedNode->parent;
                //  if constexpr ((cutoff_scoring&hyperparams)==cutoff_scoring) { branchDepth -= 1; }


            }
        } // iterations


        // This is the part NOT discussed in the AlphaZero paper (or ANY mcts paper for that matter):
        // What to do when terminal and non-terminal nodes appear in the tree mixed together????
        int shallowestTerminal = 9999;
        for (int i=0; i<root->createdBranches; ++i)
        {
            Node<MoveType>& branch = root->branches[i];
            if (branch.shallowestTerminalDepth < shallowestTerminal)
                shallowestTerminal = branch.shallowestTerminalDepth;
        }

        #ifndef INCLUDEAI__DISABLE_DETECTION_OF_INSTANT_WIN
          if (shallowestTerminal != 9999)
          {
              for (int i=0; i < root->createdBranches; ++i)
              {
                  Node<MoveType>& branch = root->branches[i];
                  if (branch.shallowestTerminalDepth == shallowestTerminal)
                  {
                      if (branch.score > 0.f)
                          continue;
                  }
                  else
                  {
                      branch.score = -1000.f; // -iters! Todo!!
                  }
              }
          } // (shallowestTerminal != 9999)
        #endif

        // Yes, scoring is complex!!!:
        auto bestScore = root->branches[0].score;
        auto bestVisits = root->branches[0].visits;
        int posScore=0, posVisits=0;
        for (int i=1; i<root->createdBranches; ++i)
        {
            auto expectedScore = root->branches[i].score;
            auto expectedVisits = root->branches[i].visits;
            if (expectedScore > bestScore)
            {
                bestScore = expectedScore;
                posScore = i;
            }
            else if (expectedScore == bestScore)
            {
                if (expectedVisits > root->branches[posScore].visits)
                    posScore = i;
            }

            if (expectedVisits > bestVisits)
            {
                bestVisits = expectedVisits;
                posVisits = i;
            }
            else if (expectedVisits == bestVisits)
            {
                if (expectedScore > root->branches[posVisits].score)
                    posVisits = i;
            }
        }










            for (int i=0; i<root->createdBranches; ++i)
            {

              //  if (i==bestScore) std::printf("\033[1;36m");
                //std::printf("mv: %c %d  bs:%d   ", root->branches[i].moveHere, root->branches[i].moveHere, root->branches[i].branchScore);
                // v/s == exploration E!
                //std::printf("s:%4.3f v:%d  dp: %d   act:%d  s/v:%2.3f v/s:%2.5f \033[0m \n", root->branches[i].score, root->branches[i].visits, root->branches[i].shallowestTerminalDepth, root->branches[i].activeBranches, root->branches[i].score/ root->branches[i].visits,root->branches[i].visits/root->branches[i].score);
            }
            std::printf("threshold: %f mv: %d \033[1;31mcutoff: %d\033[0m ACT:%d maxPatternsim %f \n",
                          threshold, root->branches[posScore].moveHere, cutoffDepth, root->activeBranches, ai_ctx.maxPatternSimilarity);

            mcts_result.statistics[MCTS_result<MoveType>::maxPatternSimilarity] = ai_ctx.maxPatternSimilarity*100;
            mcts_result.best = [root, posVisits, posScore, bestScore]
                               {
                                   if (bestScore > 0.f)
                                       return root->branches[posScore].moveHere;
                                   return root->branches[posVisits].moveHere;
                               }();
            return mcts_result;

    }

    template <int MaxIterations,
              int SimDepth,
              int MinimaxDepth,
              GameMove MoveType,
              BitfieldIntType BitfieldType,
              Gameview Board,
              typename AiCtx,
              typename Rndfunc
             >
    MCTS_Future<MoveType> mcts_async(const Board& boardOriginal, AiCtx& ai_ctx, Rndfunc rand)
    {
        // https://github.com/cdwfs/cds_sync/blob/master/cds_sync.h
        using atomic = int;
        static atomic rootMovesRemaining;
        if (rootMovesRemaining == 0)
        {
            typename Board::StorageForMoves storageForMoves;
            Board boardClone = boardOriginal.clone();
            rootMovesRemaining = boardClone.generateMovesAndGetCnt(storageForMoves);
        }
        while (rootMovesRemaining)
        {
            typename Board::StorageForMoves storageForMoves;
            Board boardClone = boardOriginal.clone();
          //  ??? = boardClone.generateMovesAndGetCnt(storageForMoves);
            // rootMovesRemaining -= 1;
        }
    }

} // namespace include_ai


#else // AI_HPP
  #error "double include"
#endif // AI_HPP
