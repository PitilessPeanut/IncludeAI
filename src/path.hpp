#ifndef PATH_HPP
#define PATH_HPP


/****************************************/
/*                       Floyd-Warshall */
/* github.com/Wenox/fast-fw/            */
/****************************************/
    template <int NumVertices, typename G, typename P>
    constexpr void FloydWarshall(G *dist, P *path)
    {
        for (int k=0; k<NumVertices; ++k)
        for (int i=0; i<NumVertices; ++i)
        for (int j=0; j<NumVertices; ++j)
        {
            const auto in = i*NumVertices;
            const auto distance = dist[in+k] + dist[(k*NumVertices)+j];
            const auto index = j+in;
            if (dist[index] > distance)
            {
                dist[index] = distance;
                path[index] = k;
            }
        }
    }

    template <int Width, typename Int, typename G, typename P>
    constexpr bool Floyd_reconstruct(Int *dst, const int from, const int to, const G *dist, const P *path, const Int blocked)
    {
        if (path[(from*Width)+to]==blocked && dist[(from*Width)+to]==blocked)
            return false;
        int y=Width-1;
        dst[y--] = to;
        int z = path[(from*Width)+to];
        while (z != blocked)
        {
            dst[y--] = z;
            z = path[(from*Width)+z];
        }
        dst[y] = from;
        return true;
    }
    

#else
  #error "double include"
#endif // PATH_HPP

