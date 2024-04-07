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
/*                                 Node */
/****************************************/
  //  template <int NumNodes, class IntType>
  //  struct Ai_ctx;

    struct Node
    {
        static constexpr int never_expanded = -1;
        int      activeBranches  = never_expanded;
        int      createdBranches = 0;
        Node    *parent;
        Node    *branches = nullptr;
        SWORD    visits = 0;
        float    score = 0.f;
        float    UCBscore;    // Placeholder used during UCB calc.
        Move     moveHere;
        
        constexpr Node()
          : parent(nullptr),
            moveHere({0, nullptr})
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
        Move move;
    };

    // mcts_NODES_ROTATIONS_EXPLRCONSTANT_INTSIZE()
    MCTS_result mcts__500_2000_1p20__u32(Board *boardEmpty, const Board *boardOriginal, Ai_ctx<500, UDWORD>& ai_ctx, const Simulator *simulator);
    MCTS_result mcts_1500_4000_1p20__u64(Board *boardEmpty, const Board *boardOriginal, Ai_ctx<500, UDWORD>& ai_ctx, const Simulator *simulator);


#else
  #error "double include"
#endif // AI_HPP






