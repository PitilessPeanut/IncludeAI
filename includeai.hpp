/*
    You must add '#define INCLUDEAI_IMPLEMENTATION' before #include'ing this in ONE source file.
    Like this:
        #define INCLUDEAI_IMPLEMENTATION
        #include "includeai.hpp"
*/

#ifndef INCLUDEAI_HPP
#define INCLUDEAI_HPP

#ifdef INCLUDEAI_IMPLEMENTATION


#include <cmath>
#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
  #include <immintrin.h>
#endif


#ifndef AI_HPP
#define AI_HPP



/****************************************/
/* A 'move' by a user/player within the */
/* game context                         */
/****************************************/
    struct Move
    {
        // A move represented as an index into a table of possible moves, or used directly:
        SDWORD moveIdx;
        // Pointer to another possible move by the same player (used during node expansion):
        Move *next;
    };


/****************************************/
/*                     Board base class */
/****************************************/
    class Board
    {
    public:
        enum class Outcome { running, draw, fin, invalid };

        Board() = default;

        Board(const Board&) = delete;
        Board& operator=(const Board&) = delete;

        virtual void cloneInto(Board *dst) const = 0;

        virtual Move *getAnotherMove() = 0;

        virtual Outcome doMove(const Move) = 0;

        virtual void switchPlayer() = 0;

        virtual int getCurrentPlayer() const = 0;

        virtual ~Board() = default;
    };


/****************************************/
/*      Node (should fit into 64 bytes) */
/****************************************/
    struct Node
    {
        static constexpr int never_expanded = -1;
        int      activeBranches  = never_expanded; // Must be signed!
        int      createdBranches = 0;              // Must be signed!
       // SWORD    visits = 1;  
       float visits = 1.f; //1.f;
        float weight = 1.f;
        Node    *parent = nullptr;
        Node    *branches = nullptr;
        float    score = 0.f; //0.01f;    // Start with some positive value to prevent divide by zero error
        float    UCBscore;         // Placeholder used during UCB calc.
        Move     moveHere;
        
        constexpr Node()
          : moveHere({0, nullptr})
        {}
        
        constexpr Node(Node *newParent, Move move)
          : activeBranches(never_expanded),
            parent(newParent),
            moveHere(move) // Ownership is implicit (the 'owner' is the player how makes this move!)
        {}
    
        Node(const Node&)            = delete;
        Node& operator=(const Node&) = delete;
        Node(Node&&)                 = delete;
        Node& operator=(Node&&);
    };


/****************************************/
/*                           Ai context */
/****************************************/
    template <int NumNodes, class IntType>
    struct Ai_ctx
    {
        static constexpr int numNodes = NumNodes;
        NodeAllocator<NumNodes, IntType> nodeAllocator;    
        Node nodePool[NumNodes];
        
        Ai_ctx() {}
        Ai_ctx(const Ai_ctx&) = delete;
        Ai_ctx& operator=(const Ai_ctx&) = delete;
    };


/****************************************/
/*                            Simulator */
/****************************************/
    struct Simulator
    {
        virtual float simulate(const Board *original) const = 0;
    };


/****************************************/
/*                       Find best move */
/****************************************/
    struct MCTS_result
    {
        enum {chunk_overflow,end};
        int errors[end] = {0};
        Move move;
    };

    // usage: mcts_NODES_ROTATIONS_EXPLRCONSTANT__INTSIZE
    MCTS_result mcts___500_2000_1p20__u32(Board *boardEmpty, const Board *boardOriginal, Ai_ctx<  500, UDWORD>& ai_ctx, const Simulator *simulator);
    MCTS_result mcts__1500_4000_1p20__u64(Board *boardEmpty, const Board *boardOriginal, Ai_ctx< 1500, UQWORD>& ai_ctx, const Simulator *simulator);
    MCTS_result mcts_10000_4000_1p20__u64(Board *boardEmpty, const Board *boardOriginal, Ai_ctx<10000, UQWORD>& ai_ctx, const Simulator *simulator);


#else
  #error "double include"
#endif // AI_HPP








#ifndef NODE_ALLOCATOR_HPP
#define NODE_ALLOCATOR_HPP



/****************************************/
/*                                Tools */
/****************************************/
// ctz
    template <typename Int>
    constexpr int ctz_comptime(Int val)
    {
        Int c=0;  // output: c will count val's trailing zero bits,
                  // so if val is 1101000, then c will be 3
        if (val)
        {
            val = (val ^ (val - 1)) >> 1;  // Set val's trailing 0s to 1s and zero rest
            for (; val; c++)
                val >>= 1;
        }
        else
        {
            c = CHARBITS * sizeof(val);
        }
        return c;
    }
    
    int ctz_runtime(const SDWORD);
    int ctz_runtime(const UDWORD);
    int ctz_runtime(const UQWORD);
    int ctz_runtime(const  UBYTE);
    

// rotate
    template <typename Int>
    constexpr Int rotl_comptime(Int x, int amount)
    {
        // _rotl64 // todo: msvc
        return ((x << amount) | (x >> ((sizeof(Int)*CHARBITS) - amount)));
    }

    SDWORD rotl_runtime(const SDWORD x, int amount);
    UDWORD rotl_runtime(const UDWORD x, int amount);
    SQWORD rotl_runtime(const UQWORD x, int amount);
    UBYTE  rotl_runtime(const  UBYTE x, int amount);


// min/max
    template <typename T>
    constexpr T ptMin(T x, T y) { return x < y ? x : y; }
    template <typename T>
    constexpr T ptMax(T x, T y) { return x > y ? x : y; }

    
/****************************************/
/*                               Config */
/****************************************/
    enum class Mode { FAST, TIGHT };


/****************************************/
/*                                 Impl */
/****************************************/
    template <int NumberOfBuckets, class Inttype, Mode mode=Mode::FAST, bool comptime=false>
    struct NodeAllocator
    {
    private:
        static constexpr int Intbits = sizeof(Inttype)*CHARBITS;
    public:
        Inttype bucketPool[NumberOfBuckets] = {0};
    public:
        constexpr auto largestAvailChunk(const int desiredSize)
        {
            struct Pos
            {
                int posOfAvailChunk;
                int len;
            };

            auto ctz = [](auto val)
                       {
                           if constexpr (comptime)
                               return ctz_comptime(val);
                           else
                               return ctz_runtime(val);
                       };

            auto rotl = [](auto x, int amount)
                        {
                            if constexpr (comptime)
                                return rotl_comptime(x, amount);
                            else
                                return rotl_runtime(x, amount);
                        };
           
            int discoveredSize = ptMin(desiredSize, Intbits*NumberOfBuckets);
            while (discoveredSize>0)
            {
                if (discoveredSize <= Intbits)
                {
                    for (int bucketNo=0; bucketNo<NumberOfBuckets; ++bucketNo)
                    {
                        if (ctz(bucketPool[bucketNo]) >= discoveredSize)
                        {
//                            const bool isEmpty = !bucketPool[bucketNo];
//                            const Inttype availTail = ctz(bucketPool[bucketNo]) * !isEmpty;
//
//                            const Inttype leftMask = ~((everyBitSet << availTail) * !isEmpty);
//                            const Inttype toggleBits = everyBitSet << ((availTail-discoveredSize) * (availTail>=discoveredSize));
//                            bucketPool[bucketNo] = bucketPool[bucketNo] | (leftMask&toggleBits);
                            
                            constexpr Inttype everyBitSet = ~0;
                            const int availTail = ctz(bucketPool[bucketNo]);
                            Inttype mask = everyBitSet << discoveredSize; // the 'discoveredSize' here is always <= than Intbits 
                            mask = rotl(mask, availTail-discoveredSize);
                            bucketPool[bucketNo] = bucketPool[bucketNo] | ~mask;
                            
                            return Pos{ .posOfAvailChunk = static_cast<int>((bucketNo*Intbits) + (Intbits-availTail))
                                      , .len = static_cast<int>(discoveredSize)
                                      };
                        }
                    }
                }
                
                if constexpr (mode == Mode::FAST)
                {
                    const int bucketsRequired = discoveredSize/Intbits;
                    for (int bucketNo=0; bucketNo<(NumberOfBuckets-bucketsRequired); ++bucketNo)
                    {
                        Inttype *iter = &bucketPool[bucketNo + bucketsRequired+1];
                        Inttype *iter2 = iter;
                        bool avail = true;
                        while (iter != &bucketPool[bucketNo])
                        {
                            iter--;
                            avail = avail && *iter == 0;
                        }
                        
                        if (avail)
                        {
                            while (iter2 != &bucketPool[bucketNo])
                            {
                                iter2--;
                                *iter2 = -1;
                            }
                            constexpr Inttype everyBitSet = ~0;
                            bucketPool[bucketNo+bucketsRequired] = everyBitSet << (Intbits - (discoveredSize%Intbits));
                            return Pos{ .posOfAvailChunk = static_cast<int>(bucketNo*Intbits)
                                      , .len = static_cast<int>(discoveredSize)
                                      };
                        }
                    }
                }
                else if constexpr (mode == Mode::TIGHT)
                {
                    
//                    const int headBitsUsed = ctz(bucketPool[bucketNo]);
//                    int remainingBits = discoveredSize - headBitsUsed;
//                    int emptyBuckets = 0;
//
//                    bool avail = true;
//                    while (remainingBits > 0 && avail)
//                    {
//                        emptyBuckets += 1;
//                        remainingBits -= Intbits;
//                        avail = avail && bucketPool[bucketNo+emptyBuckets] == 0;
//                    }
//
//                    if (avail)
//                    {
//                        constexpr Inttype everyBitSet = -1;
//                        bucketPool[bucketNo] = bucketPool[bucketNo] | ~(everyBitSet << headBitsUsed);
//                        bucketPool[bucketNo+emptyBuckets] = everyBitSet << (Intbits - ((discoveredSize-headBitsUsed)%Intbits));
//                        const int usedBuckets = emptyBuckets+1;
//                        while (emptyBuckets--)
//                            bucketPool[bucketNo+emptyBuckets] = -1;
//                        return Pos{ .pos = ((bucketNo*usedBuckets)*Intbits)
//                                         + ((discoveredSize-headBitsUsed)%Intbits)
//                                  , .len = discoveredSize
//                                  };
//                    }
                }
                discoveredSize -= 1;
            } // while (discoveredSize>0)

            return Pos{ .posOfAvailChunk = 0, .len = 0 };
        }
        
        constexpr void free(const int pos, const int len)
        {
            constexpr Inttype everyBitSet = ~0;
            int startBucket = pos/Intbits;
            int endBucket = (pos+len)/Intbits;
            const Inttype maskHead = everyBitSet << (Intbits - (pos%Intbits));
            const Inttype maskTail = everyBitSet << (Intbits - ((pos+len)%Intbits));
            
            if (startBucket == endBucket)
            {
                bucketPool[startBucket] = (maskHead | ~maskTail) & bucketPool[startBucket];
            }
            else
            {
                // Start
                bucketPool[startBucket] = maskHead & bucketPool[startBucket];

                // End
                bucketPool[endBucket] = ~maskTail & bucketPool[endBucket];

                // Middle
                endBucket -= 1;
                while (&bucketPool[startBucket++] != &bucketPool[endBucket])
                    bucketPool[startBucket] = 0;
            }
        }
        
        constexpr void clearAll()
        {
            for (int i=0; i<NumberOfBuckets; ++i)
                bucketPool[i] = 0;
        }
    };


#else
  #error "double include"
#endif // NODE_ALLOCATOR_HPP



#if defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
  #include <immintrin.h>
#endif

 
/****************************************/
/*                                Tools */
/****************************************/
// ctz
    int ctz_runtime(const UBYTE  val) 
    { 
        #ifdef _WIN32
          return _tzcnt_u32( (unsigned int)val );
        #else
          return 0 ? (sizeof(UBYTE )*CHARBITS) : __builtin_ctz(val); 
        #endif
    }
    
    int ctz_runtime(const UDWORD val) 
    { 
        #ifdef _WIN32
          return _tzcnt_u32( val );
        #else
          return 0 ? (sizeof(UDWORD)*CHARBITS) : __builtin_ctzl(val); 
        #endif
    }
    
    int ctz_runtime(const UQWORD val) 
    { 
        #ifdef _WIN32
          return _tzcnt_u64( val ); 
        #else
          return 0 ? (sizeof(UQWORD)*CHARBITS) : __builtin_ctzll(val); 
        #endif
    }
    
    int ctz_runtime(const SDWORD val) 
    { 
        #ifdef _WIN32
          return _tzcnt_u32( (unsigned int)val ); 
        #else
          return 0 ? (sizeof(SDWORD)*CHARBITS) : __builtin_ctz(val); 
        #endif
    }


// rotate
    SDWORD rotl_runtime(const SDWORD x, int amount) { return rotl_comptime(x, amount); }
    UDWORD rotl_runtime(const UDWORD x, int amount) { return rotl_comptime(x, amount); }
    SQWORD rotl_runtime(const UQWORD x, int amount) { return rotl_comptime(x, amount); }
    UBYTE  rotl_runtime(const  UBYTE x, int amount) { return rotl_comptime(x, amount); }
    
        
/****************************************/
/*                                Tests */
/****************************************/
    static_assert([]
                  {
                      constexpr bool comptime = true;
                      NodeAllocator<500, UQWORD, Mode::FAST, comptime> test;
                      const auto availNodes = test.largestAvailChunk(1);
                      return (availNodes.len==1) && (availNodes.posOfAvailChunk==0);
                  }()
                 );

    static_assert([]
                  {
                      constexpr bool comptime = true;
                      NodeAllocator<5, UBYTE, Mode::FAST, comptime> testAllocator;
                      auto pos = testAllocator.largestAvailChunk(3);
                      bool ok = pos.posOfAvailChunk==0 && pos.len==3;
                      pos = testAllocator.largestAvailChunk(3);
                      ok = ok && pos.posOfAvailChunk==3 && pos.len==3;
                      pos = testAllocator.largestAvailChunk(2);
                      ok = ok && pos.posOfAvailChunk==6 && pos.len==2;
                      ok = ok && testAllocator.bucketPool[0] == 255;
                      testAllocator.free(3, 3);
                      ok = ok && testAllocator.bucketPool[0] == 0b11100011;
                      pos = testAllocator.largestAvailChunk(5);
                      ok = ok && pos.posOfAvailChunk==8 && pos.len==5;
                      ok = ok && testAllocator.bucketPool[1] == 0b11111000;
                      pos = testAllocator.largestAvailChunk(18);
                      ok = ok && pos.posOfAvailChunk==16 && pos.len==18;
                      ok = ok && testAllocator.bucketPool[0] == 0b11100011;
                      ok = ok && testAllocator.bucketPool[1] == 0b11111000;
                      ok = ok && testAllocator.bucketPool[2] == 255;
                      ok = ok && testAllocator.bucketPool[3] == 255;
                      ok = ok && testAllocator.bucketPool[4] == 0b11000000;
                      pos = testAllocator.largestAvailChunk(7);
                      ok = ok && pos.posOfAvailChunk==34 && pos.len==6;
                      testAllocator.free(10, 24);
                      ok = ok && testAllocator.bucketPool[1] == 0b11000000;
                      ok = ok && testAllocator.bucketPool[2] == 0;
                      ok = ok && testAllocator.bucketPool[3] == 0;
                      ok = ok && testAllocator.bucketPool[4] == 0b00111111;
                      testAllocator.free(1, 1);
                      ok = ok && testAllocator.bucketPool[0] == 0b10100011;
                      testAllocator.free(12, 5);
                      ok = ok && testAllocator.bucketPool[1] == 0b11000000;
                      ok = ok && testAllocator.bucketPool[2] == 0;
                      testAllocator.free(30, 6);
                      ok = ok && testAllocator.bucketPool[3] == 0;
                      ok = ok && testAllocator.bucketPool[4] == 0b00001111;
//                      posB = testAllocator.largestAvailChunk(20);
     //   ok = ok && pos.pos==31 ; //&& pos.len<20;
       // ok = ok && testAllocator.bucketPool[4] == 0b00011111;
                        
                      return ok;
                  }()
                 );

                 


#include <cmath>
#include <cstdio>


/****************************************/
/*       Util functions / configuration */
/****************************************/
// sqrt
    inline float aiSqrt(float x) { return std::sqrt(x); }

// log
    inline float aiLog(float x) { return std::log(x); }

// rand
    auto aiRand = []{ return pcgRand<UQWORD>(); };
    
// assert
   // #if defined(AI_ASSERT)
      #include <assert.h>
      #define aiAssert(x) assert(x)
  //  #else
  //    #define aiAssert(x)
   // #endif


/****************************************/
/*                               Memory */
/****************************************/
    template <typename Ctx>
    inline constexpr auto& getFresh(Ctx& ctx, const int pos, Node *node, Move move)
    {
        aiAssert(pos < ctx.numNodes);
        return ctx.nodePool[pos] = Node(node, move);
    }


/****************************************/
/*                                 Node */
/****************************************/
    //static_assert(sizeof(Node) <= 64);//todo back!

    Node& Node::operator=(Node&& other)
    {
        if (this != &other)
        {
            activeBranches  = other.activeBranches;
            createdBranches = other.createdBranches;
            parent          = other.parent;
            branches        = other.branches;
            visits          = other.visits;
            score           = other.score;
            moveHere        = other.moveHere;
            for (int i=0; i<other.createdBranches; ++i)
                branches[i].parent = this;
        }
        return *this;
    }

    template <int NumNodes, typename IntType>
    void discard(Ai_ctx<NumNodes, IntType>& ai_ctx, Node& removeMe)
    {
        /*const auto pos = ai_ctx.nodePool - &removeMe;
        if (pos<0 && pos>=NumNodes)
        {
        std::printf("%d  .... \n", pos);
        aiAssert(false);
        }*/
        //ai_ctx.nodeAllocator.free(pos, 1);
        if (!removeMe.branches)
            return; // We are done
        for (int i=0; i<removeMe.createdBranches; ++i)
            discard(ai_ctx, removeMe.branches[i]);            
        ai_ctx.nodeAllocator.free(removeMe.branches - ai_ctx.nodePool, removeMe.createdBranches);
        removeMe.branches = nullptr;
    }
    
    template <int NumNodes, typename IntType>
    void disconnectBranch(Ai_ctx<NumNodes, IntType>& ai_ctx, Node *parent, const Node *removeMe)
    {
       // aiAssert(parent->activeBranches > 0);
        aiAssert(parent == removeMe->parent);
        aiAssert((removeMe-ai_ctx.nodePool)>=0 && (removeMe-ai_ctx.nodePool)<ai_ctx.numNodes);

        std::printf("disc %d len: %d \n", removeMe-ai_ctx.nodePool, 1);

        
        const auto posOfChild = removeMe - parent->branches;
        Node& swapDst = parent->branches[posOfChild]; // Bypass the 'const'
        
        // Discard all child branches of each of the child nodes of the node
        // we want to remove (recursive):
        for (int i=0; i<swapDst.createdBranches; ++i)
            discard(ai_ctx, swapDst.branches[i]);
        ai_ctx.nodeAllocator.free(removeMe - ai_ctx.nodePool, removeMe->createdBranches);

        // But don't discard() the child node, that is the slot we will swap into:
        // discard(ai_ctx, swapDst); // incorrect!!

        // Don't swap w/ itself if the to-be-removed node is last:
        if (&swapDst == &parent->branches[parent->activeBranches-1])
        {
            parent->activeBranches -= 1;
            return;
        }
        const Move  removedMoveHere = swapDst.moveHere;
        const float removedVisits   = swapDst.visits;
        const float removedScore    = swapDst.score;

        Node& swapSrc = parent->branches[parent->activeBranches-1];

        swapDst.activeBranches  = swapSrc.activeBranches;
        swapDst.createdBranches = swapSrc.createdBranches;
        swapDst.moveHere        = swapSrc.moveHere;
        swapDst.visits          = swapSrc.visits;
        swapDst.score           = swapSrc.score;
        swapDst.branches        = swapSrc.branches;
        // Establish new "parent" for each branch node after swap:
        for (int i=0; i<swapDst.createdBranches; ++i)
            swapDst.branches[i].parent = &swapDst;

        swapSrc.moveHere = removedMoveHere;
        swapSrc.visits   = removedVisits;
        swapSrc.score    = removedScore;
        swapSrc.branches = nullptr;
        
        parent->activeBranches -= 1;
    }
    

/****************************************/
/*                       Find best move */
/****************************************/
    template <int NumNodes, int MaxIterations, int C, typename IntType>
    constexpr MCTS_result mcts(Board *boardEmpty, const Board *boardOriginal, Ai_ctx<NumNodes, IntType>& ai_ctx, const Simulator *simulator)
    {
        [[maybe_unused]] auto UCBselectBranch =
            [](const Node& node) -> Node *
            {
                for (int i=0; i<node.activeBranches; ++i)
                {
                    aiAssert(node.branches);
                    Node& branch = node.branches[i];
                    if (branch.visits < .1f)
                    {
                        //std::printf("\033[1;31m LOW VISITS!\033[0m \n");
                        return &branch; // Prevent x/0
                    }
                    branch.UCBscore = branch.score / (branch.visits);// * 2.f);
                    
                    // If this is too large we will get an "overemphasis" on visits and the search
                    // could start "jumping around" instead of locking into a promising branch:
                    constexpr float exploration_C = (C-0.f) / 100.f; // convert back to float
                    
                    branch.UCBscore += exploration_C * aiSqrt((1 * aiLog(node.visits)) / branch.visits);
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
                    if (branch.visits < 1.f)
                        return &branch;
                }
                return &node.branches[0];
            };
        
        [[maybe_unused]] auto pickRandom =
            [](const Node& node) -> Node *
            {
                return nullptr;
            };

        
        Node Root; // todo: will be useless if we keep the trree xross rounds! (remove!)
        Node *root = &Root; //getFresh(ai_ctx, 0, nullptr, Move{0, nullptr});
        ai_ctx.nodeAllocator.clearAll();
                 const auto throwaway = ai_ctx.nodeAllocator.largestAvailChunk(1);
        aiAssert(root->parent == nullptr);
        int iterations=0; // todo back 
        for (; /*root->activeBranches!=0 &&*/ iterations<MaxIterations; ++iterations)
        {
            Node *selectedNode = root;
            boardOriginal->cloneInto(boardEmpty);
            Board *boardClone = boardEmpty;
            Board::Outcome outcome = Board::Outcome::running;
            int depth = 1;
            
            
            
            
            // 1. Traverse tree and select leaf:
            while (selectedNode->activeBranches > 0)
            {
                selectedNode = UCBselectBranch(*selectedNode);
                
                const Move moveHere = selectedNode->moveHere;
                outcome = boardClone->doMove(moveHere);
                boardClone->switchPlayer();
                depth += 1;
            }
            //if (selectedNode->activeBranches != Node::never_expanded) { std::printf("------never exp. ---%d \n", selectedNode==root); break; } // insuff nodes!
            
            
            
            
            // 2. Add (allocate/expand) branch/child nodes to leaf:
            if (selectedNode->activeBranches==Node::never_expanded && outcome==Board::Outcome::running)
            {
                int nValidMoves = 0;
                Move *first = boardClone->getAnotherMove();
                Move *move = first;
                if (!first)
                    continue;
                while (move)
                {
                    move->next = boardClone->getAnotherMove();
                    move = move->next;
                    nValidMoves += 1;
                }
                const auto availNodes = ai_ctx.nodeAllocator.largestAvailChunk(nValidMoves);
                nValidMoves = availNodes.len; // This line is critical!
                int nodePos = availNodes.posOfAvailChunk;
                if ((nodePos+nValidMoves) >= ai_ctx.numNodes) [[unlikely]] // This can happen at the end
                {
                std::printf("........................npos %d mves %d iii: %d \n", nodePos, nValidMoves, iterations);
                    // todo: flag!
                    //iterations = MaxIterations;
                    // not enough nodes!!
                    break; // Top loop!
                    //continue;
                }
                aiAssert((nodePos+nValidMoves) >= 0 && (nodePos+nValidMoves) <= ai_ctx.numNodes);
                selectedNode->activeBranches  = nValidMoves;
                selectedNode->createdBranches = nValidMoves;
                selectedNode->branches = &getFresh(ai_ctx, nodePos, selectedNode, *first);
                first = first->next;
                while (first)
                {
                    nodePos += 1;
                    [[maybe_unused]] const auto& unusedNode =
                        getFresh(ai_ctx, nodePos, selectedNode, *first);
                    first = first->next;
                }
                for (int i=0; i<selectedNode->createdBranches; ++i) // todo put this loop into the assert
                {
                  //  std::printf("mvs: %d, avllen: %d, POS: %d, npos: %d \n", nValidMoves, availNodes.len, availNodes.posOfAvailChunk, nodePos);
                    aiAssert(selectedNode != &selectedNode->branches[i]);
                }
            }
            
            
            
            
            // 3. Pick (select) a node for analysis:
            Node *parent = selectedNode;
            if (selectedNode->activeBranches > 0)
            {
                // Must be rand, otherwise it's not 'monte-carlo':
                const auto X = aiRand() % selectedNode->activeBranches; 
                selectedNode = &selectedNode->branches[X];
                //aiAssert(selectedNode->score < 1.f);
                outcome = boardClone->doMove( selectedNode->moveHere );    
                boardClone->switchPlayer(); // Segfault if this line missing!
            }
            else
            {
                std::printf("\033[1;34m switching \033[0m \n");
                if (selectedNode->parent)
                parent = selectedNode->parent;
                //continue;
            }
            aiAssert(parent);                   // Ensure 'selectedNode' not root
            if (parent->activeBranches <= 0) {
                std::printf("\033[1;35m act:%d created:%d , fin?: %d \033[0m \n", parent->activeBranches, parent->createdBranches,  outcome!=Board::Outcome::running);
                break;
                //continue;
            }
            aiAssert(parent != selectedNode);   // Ensure the graph remains acyclic
            aiAssert(parent->activeBranches>0); // Ensure parent node has at least one
                                                // active child node (the 'selectedNode' one!)




            // 4. Determine branch score ("rollout"):
            float score = 0.f; // 0 means loss, 1 means draw, 2 means win!!! 
            if (outcome == Board::Outcome::running) [[likely]]
            {
                // Simulate to get an estimation of the quality of this position:
                score += simulator->simulate(boardClone);
                // todo: flag avg sim score for this turn
            }
            else
            {
                // A terminal node is equivalent to a 100% score!
                if (outcome == Board::Outcome::draw) //[[likely]]
                {
                    // draw:
                    score += 1.f;
                }
                else if (boardOriginal->getCurrentPlayer() != boardClone->getCurrentPlayer())
                {
                    // win:
                    score += 2.f;
                }
                else
                {
                    // lose:
                }

                Node *child = selectedNode;
                while (parent)// && parent!=root)//todo: remove 2nd oart after fix!
                {
                    disconnectBranch(ai_ctx, parent, child);
                    if (parent->activeBranches != 0)
                    {
                        parent = nullptr; // Stop
                    }
                    else
                    {
                        child = parent;
                        aiAssert(parent != parent->parent);
                        parent = parent->parent; // Lolz
                    }
                }
            }
            


            
            // 5. Backprop/update tree:
            
            Node *fast = selectedNode;
            Node *slow = selectedNode;
            while (fast)
            {
                // Floyd's Cycle
                if (fast)
                    fast = fast->parent;
                if (fast == slow)
                {
                    selectedNode = nullptr;
                    // todo: log!
                    std::printf("loop \n");
                    break;
                    fast = nullptr; // 'break'
                }
                if (fast)
                    fast = fast->parent;
                slow = slow->parent;
            }
            int cur = 2;
            int yyy = fib(depth);
            while (selectedNode)
            {
                //selectedNode->visits += 1.f ;//- ((cur-0.f)/(depth-0.f)); // Games that have more than two possible outcomes (example:
                //selectedNode->visits += aiLog( 100 * ((cur-0.f)/(depth-0.f)) );
                selectedNode->visits += 1;
                selectedNode->weight += (fib(cur)-0.f) / (fib(depth)-0.f); //(aiLog(cur* 10)/2) / (depth-0.f); //((cur-0.f)/(depth-0.f)) ; weight
                if (fib(cur+1) <= yyy)
                    cur += 1;
                                           // win, loss, draw) are, in some way, fundamentally flawed
                                           // in their design. A "visit" should yield a clear result:
                                           // win or loss. To account for another "intermediate" state
                                           // we would have to introduce another "half-win" outcome, but
                                           // there is no such thing as a "half-visit"! To compensate,
                                           // we double the value of each win as well as each visit.
                                           // The value of each visit must be equivalent to the range 
                                           // of possible game outcomes! (2 if there are 3 outcomes; 
                                           // 3 if 4, etc.)
                selectedNode->score += score;
                selectedNode = selectedNode->parent;
                
                if (root->parent != nullptr)
                {
                    std::printf("err \n");
                }
            }
        } // iterations
        
        // The scoring process is VASTLY more complex than what is described in the alphazero paper! (see explanation below ⬇️)


// todo!        


        // Minimax overrides branch results!
        


// todo!!!!




        // Finally, pick the branch with the best score:
        aiAssert(root->branches);
        float best = root->branches[0].score / root->branches[0].visits;
        float worst = best;
        int pos=0;
        for (int i=0; i<root->createdBranches; ++i)
        {
            const float expected = root->branches[i].score /  root->branches[i].visits;
            //if (expected < 0.001f)
            //if (root->branches[i].visits < 0.01f)
            //{
            //    pos = i;
            //    break;
            //}
            //else 
            if (expected>best)// && root->branches[i].parent)
            {
                best = expected;
                pos = i;
            }
         //   if (expected<worst)
          //      worst = expected;
            std::printf("s:%f v:%f [%d]       exp:%f       w:%f \n", root->branches[i].score, root->branches[i].visits, root->branches[i].moveHere.moveIdx, expected, root->branches[i].weight);
        }
        std::printf("mv: %d, gap: %f rnds: %d \n", root->branches[pos].moveHere.moveIdx, worst/best, iterations);
        return { .move = root->branches[pos].moveHere
               };
    }

    MCTS_result mcts___500_2000_1p20__u32(Board *boardEmpty, const Board *boardOriginal, Ai_ctx<  500, UDWORD>& ai_ctx, const Simulator *simulator)
    { return mcts<  500, 2000, 120, UDWORD>(boardEmpty, boardOriginal, ai_ctx, simulator); }

    MCTS_result mcts__1500_4000_1p20__u64(Board *boardEmpty, const Board *boardOriginal, Ai_ctx< 1500, UQWORD>& ai_ctx, const Simulator *simulator)
    { return mcts< 1500, 4000, 120, UQWORD>(boardEmpty, boardOriginal, ai_ctx, simulator); }

    MCTS_result mcts_10000_4000_1p20__u64(Board *boardEmpty, const Board *boardOriginal, Ai_ctx<10000, UQWORD>& ai_ctx, const Simulator *simulator)
    { return mcts<10000, 4000, 120, UQWORD>(boardEmpty, boardOriginal, ai_ctx, simulator); }




/****************************************/
/*                             Research */
/****************************************/
/*  The main reason why scoring is difficult is because we have two conflicting goals competing with another:
    1) win as fast as possible
    2) don't lose
    The system must account for a situation where a winning and a losing branch have equal score: If we
    prioritize loss-aversion the ai will drag the game into a draw. If we prioritize aggression then 
    it will miss an opponents move that may lead to an immediate loss! 
    How do we account for such a situation??

*/


#endif // INCLUDEAI_IMPLEMENTATION


#endif // INCLUDEAI_HPP
/*
*/


