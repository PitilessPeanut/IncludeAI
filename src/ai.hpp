#ifndef AI_HPP
#define AI_HPP

#include "asmtypes.hpp"
#include "node_allocator.hpp"


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
        SWORD    visits = 0;
        Node    *parent = nullptr;
        Node    *branches = nullptr;
        float    score = 0.f;
        float    UCBscore;    // Placeholder used during UCB calc.
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
    MCTS_result mcts___5000_2000_1p20__u32(Board *boardEmpty, const Board *boardOriginal, Ai_ctx<  5000, UDWORD>& ai_ctx, const Simulator *simulator);
    MCTS_result mcts__15000_4000_1p20__u64(Board *boardEmpty, const Board *boardOriginal, Ai_ctx< 15000, UQWORD>& ai_ctx, const Simulator *simulator);
    MCTS_result mcts_100000_4000_1p20__u64(Board *boardEmpty, const Board *boardOriginal, Ai_ctx<100000, UQWORD>& ai_ctx, const Simulator *simulator);


#else
  #error "double include"
#endif // AI_HPP






