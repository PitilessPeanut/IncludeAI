#ifndef AI_HPP
#define AI_HPP

#include "asmtypes.hpp"


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
    template <int NumNodes, class IntType>
    struct Ai_ctx;

    struct Node
    {
    };


/****************************************/
/*                           Ai context */
/****************************************/
    template <int NumNodes, class IntType>
    struct Ai_ctx
    {
        //static constexpr int numNodes =

    //    NodeAllocator<, IntType> nodeAllocator;
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
    template <int NumNodes, int MaxIterations, typename IntType>
    Move mcts(Board *boardClone, const Board *boardOriginal, Ai_ctx<NumNodes, IntType>& ai_ctx, const Simulator *simulator)
    {


    return Move{0,nullptr};
    }


#else
  #error "double include"
#endif // AI_HPP
