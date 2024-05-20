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
        // A generic 'move'. Can represent anything:
        SDWORD moveIdx; // todo change name
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

        virtual int getMovesCnt() const = 0;
        
        virtual Move getMove(int idx) = 0;

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
        SWORD    mnx = -999; // minimax score
       // SWORD    visits = 1;  
       float visits = 1.f; //1.f;
        //float weight = 1.f;
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
/*                          Hyperparams */
/****************************************/
    enum : UQWORD { count_visits_only=1, cutoff_scoring=2, minimax=4 };

    
/****************************************/
/*                       Find best move */
/****************************************/
    struct MCTS_result
    {
        enum {chunk_overflow,end};
        int errors[end] = {0};
        Move move;
    };


/****************************************/
/*                      mcts variations */
/****************************************/
    #define MAKE_MCTS_CUTOFF(nodes,rots,expl_base,expl_fract,intstr,itype, cntvis)\
      MCTS_result mcts_##nodes##_##rots##_##expl_base##p##expl_fract##cntvis##__##intstr(\
          Board *boardEmpty, const Board *boardOriginal, Ai_ctx<nodes, itype>& ai_ctx, const Simulator *simulator); \
      MCTS_result mcts_##nodes##_##rots##_##expl_base##p##expl_fract##cntvis##c__##intstr(\
          Board *boardEmpty, const Board *boardOriginal, Ai_ctx<nodes, itype>& ai_ctx, const Simulator *simulator);

    #define MAKE_MCTS_COUNT_VISITS(nodes,rots,expl_base,expl_fract,intstr,itype) \
      MAKE_MCTS_CUTOFF(nodes,rots,expl_base,expl_fract,intstr,itype, __) \
      MAKE_MCTS_CUTOFF(nodes,rots,expl_base,expl_fract,intstr,itype, __v)
  
    #define MAKE_MCTS(nodes,rots,expl_base,expl_fract)\
      MAKE_MCTS_COUNT_VISITS(nodes,rots,expl_base,expl_fract, u32,UDWORD) \
      MAKE_MCTS_COUNT_VISITS(nodes,rots,expl_base,expl_fract, u64,UQWORD) 
      

    MAKE_MCTS(500, 1500, 0,80)
    MAKE_MCTS(500, 1500, 1,20)
    MAKE_MCTS(500, 1500, 2,20)


#else
  #error "double include"
#endif // AI_HPP






