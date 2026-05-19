#ifndef PATH_HPP
#define PATH_HPP

#include <concepts>

/****************************************/
/*                           PathEngine */
/* Cache efficient, ultra-fast          */
/* "Dijkstra" for small graphs.         */
/****************************************/
    template <int MaxDistance, class Board, std::signed_integral IntType = int, IntType InfCost = 99>
    class PathEngine
    {
    private:
        static_assert(MaxDistance < InfCost);
        static constexpr int DIAMOND_SIZE = 2 * (MaxDistance+1) * ((MaxDistance+1) + 1) + 1;
        static constexpr int QUEUE_SIZE = DIAMOND_SIZE < Board::MAX_AREA ? DIAMOND_SIZE : Board::MAX_AREA;
        IntType cached_neighbors[Board::MAX_AREA][4] = {}; // 'constexpr' requires initialization
        IntType costs[Board::MAX_AREA] = {};
        IntType predecessor[Board::MAX_AREA]; // for path reconstruction
        int generation[Board::MAX_AREA] = {0};
        int current_generation = 0;
    public:
        PathEngine() = default;
        PathEngine(const PathEngine&) = delete;
        PathEngine& operator=(const PathEngine&) = delete;

        constexpr void precompute(const Board& board)
        {
            for (int pos=0; pos<Board::MAX_AREA; ++pos)
            {
                const int x = pos % board.mapWidth;
                const int y = pos / board.mapWidth;
                cached_neighbors[pos][0] = (y>0)                 ? (pos-board.mapWidth) : -1; // North
                cached_neighbors[pos][1] = (y<board.mapHeight-1) ? (pos+board.mapWidth) : -1; // South
                cached_neighbors[pos][2] = (x>0)                 ? (pos-1) : -1; // West
                cached_neighbors[pos][3] = (x<board.mapWidth-1)  ? (pos+1) : -1; // East
            }
        }

        constexpr void floodFill(IntType start, const Board& board)
        {
            // Asking for path to -1 forces Dijkstra to explore everything
            // until MaxDistance is reached or the queue is empty:
            dijkstra(start, -1, board);
        }

        constexpr IntType getExploredCost(IntType node) const
        {
            // This is to stop having to reset 'costs' to InfCost every time before floodFill():
            if (generation[node] != current_generation) return InfCost;
            return costs[node];
        }

        constexpr IntType dijkstra(IntType start, IntType target, const Board& board)
        {
            current_generation++; // Increment generation to avoid clearing 'costs' and 'generation' arrays.

            IntType queue[QUEUE_SIZE];
            IntType head = 0, tail = 0;
            queue[tail++] = start;
            generation[start] = current_generation;
            costs[start] = 0;
            predecessor[start] = -1;
            while (head != tail)
            {
                IntType lowest_idx = head;
                for (IntType i=head+1; i<tail; ++i)
                {
                    if (costs[queue[i]] < costs[queue[lowest_idx]])
                        lowest_idx = i;
                }
                // Swap lowest to front and pop:
                const IntType current = queue[lowest_idx];
                queue[lowest_idx] = queue[head];
                ++head;

                if (current == target)
                    return costs[current];  // Dijkstras first pop of target is optimal

                if (costs[current] > MaxDistance)
                    continue; // Prune...

                for (int i=0; i<4; ++i)
                {
                    const IntType neighbor = cached_neighbors[current][i];
                    if (neighbor == -1) continue; // Out of bounds.

                    const IntType next_cost = costs[current] + board.getCost(neighbor, current);
                    const bool unvisited = (generation[neighbor] != current_generation);
                    if (unvisited || next_cost < costs[neighbor])
                    {
                        costs[neighbor] = next_cost;
                        predecessor[neighbor] = current;
                        if (unvisited)
                        {
                            generation[neighbor] = current_generation;
                            queue[tail++] = neighbor;
                        }
                    }
                }
            }
            return InfCost;
        }

        constexpr int reconstruct(IntType start, IntType target, IntType *out_path) const
        {
            int length = 0;
            IntType cur = target;
            while (cur != start)
            {
                out_path[length++] = cur;
                cur = predecessor[cur];
            }
            out_path[length++] = start;
            // Reverse path:
            for (int i=0, j = length-1; i<j; ++i, --j)
            {
                const IntType tmp = out_path[i];
                out_path[i] = out_path[j];
                out_path[j] = tmp;
            }
            return length;
        }
    };


/****************************************/
/*                       Floyd-Warshall */
/* github.com/Wenox/fast-fw/            */
/****************************************/
    template <int NumVertices, int MaxLength, typename G, typename P, typename IntType>
    constexpr void FloydWarshall(G& costs, P& path, P& steps, const IntType Inf)
    {
        for (int k=0; k<NumVertices; ++k)
        for (int i=0; i<NumVertices; ++i)
        for (int j=0; j<NumVertices; ++j)
        {
            const auto in = i*NumVertices;
            const auto k_in = k*NumVertices;

            const auto ik_steps = steps[in+k];
            const auto kj_steps = steps[k_in+j];

            // If either sub-path is already invalid, or their combined length is too long,
            // then this candidate path i->k->j is not viable.
            if (ik_steps==Inf || kj_steps==Inf || (ik_steps+kj_steps) > MaxLength)
                continue; // Prune...

            const auto new_cost = costs[in+k] + costs[(k*NumVertices)+j];
            const auto index = j+in;
            if (costs[index] > new_cost)
            {
                costs[index] = new_cost;
                path[index] = k;
                steps[index] = ik_steps + kj_steps;
            }
        }
    }

    class FloydDummyPaths
    {
    private:
        struct Proxy
        {
            template <typename T>
            void operator=(const T& /*value*/) const {} // Do nothing.
        };
    public:
        Proxy operator[](int /*index*/) { return {}; }
    };

    template <int NumVertices, typename G>
    constexpr void FloydWarshall(G& dist)
    {
        FloydDummyPaths path;
        FloydWarshall<NumVertices>(dist, path);
    }

    template <int Width, typename Int, typename G, typename P>
    constexpr int Floyd_reconstruct(Int *dst, const int from, const int to, const G& costs, const P& path, const Int blocked)
    {
        if (/*path[(from*Width)+to]==blocked  <- todo: delete this ||*/ costs[(from*Width)+to]==blocked)
            return -1;
        int y=0;
        dst[0] = to;
        int z = path[(from*Width)+to];
        while (z != blocked)
        {
            // Fill gaps:
            const int len = Floyd_reconstruct<Width>(&dst[y], z, dst[y], costs, path, blocked);
            y += len-1;
            // Next step in path:
            //dst[y] = z; // redundant
            z = path[(from*Width)+z];
        }
        dst[(y++) + 1] = from;
        return y+1;
    }




#else
  #error "double include"
#endif // PATH_HPP
