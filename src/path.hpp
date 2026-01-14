#ifndef PATH_HPP
#define PATH_HPP


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


/****************************************/
/*          Cache for Paths on a 2d map */
/****************************************/
    template <int Width, int Height, int MaxLength, typename IntType>
    class PathContainer2D
    {
    private:
        static constexpr IntType Inf = ~static_cast<IntType>(0);
        static constexpr IntType Area = Width*Height;
        IntType costsTable[Area*Area] = {0};
        IntType pathsTable[Area*Area];
    public:
        explicit PathContainer2D(const IntType *map)
        {
            rebuild(map);
        }

        void rebuild(const IntType *map)
        {
            IntType stepsTable[Area*Area];

            // clip edges and prepare costs:
            int pos = 0;
            for (int i=0; i<Area; ++i)
            for (int j=0; j<Area; ++j)
            {
                const bool north = j == (i - Width);
                const bool east  = j == (i +     1) && ((i + 1) % Width != 0);
                const bool south = j == (i + Width);
                const bool west  = j == (i -     1) && ((i % Width) != 0);
                const bool self  = j == i;
                if ((north || east || south || west) && !self)
                {
                    costsTable[pos] = map[j];
                    stepsTable[pos] = 1; // It's one step to an adjacent tile.
                }
                else
                {
                    costsTable[pos] = Inf;
                    stepsTable[pos] = Inf;
                }
                pathsTable[pos] = Inf; // reset paths
                pos += 1;
            }

            FloydWarshall<Area, MaxLength>(costsTable, pathsTable, stepsTable, Inf);

            // Inf cost if steps > MaxLength:
            for (int i=0; i<(Area*Area); ++i)
            {
                if (stepsTable[i] > MaxLength)
                    costsTable[i] = Inf;
            }
        }

        int getCost(const int start_x, const int start_y, const int end_x, const int end_y, const IntType *currentMap)
        {
            const int tableIdxStart = (start_y*Width) + start_x;
            const int tableIdxEnd   = (end_y  *Width) + end_x;
            return costsTable[(tableIdxStart*Area) + tableIdxEnd];
        }
    };


#else
  #error "double include"
#endif // PATH_HPP
