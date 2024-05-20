#include "path.hpp"


/****************************************/
/*                                Tests */
/****************************************/
    static_assert([]
                  {
                      int dist[36] = {
                          99,10,99,20,99, 1,
                          99,99, 5,99,99,99,
                          99,99,99, 1,99,99,
                           5,99,99,99, 7,99,
                          99,99,99,99,99,99,
                          99,99,99,99,99,99
                      };
                      int path[36] = {
                          99,99,99,99,99,99,
                          99,99,99,99,99,99,
                          99,99,99,99,99,99,
                          99,99,99,99,99,99,
                          99,99,99,99,99,99,
                          99,99,99,99,99,99
                      };
                      FloydWarshall<6>(dist, path);
                      int reconstructed[6] = {0};
                      int from=3, to=5;
                      bool ok = Floyd_reconstruct<6>(reconstructed, from, to, dist, path, 99);
                      ok = ok && (reconstructed[5]==5);
                      ok = ok && (reconstructed[4]==0);
                      ok = ok && (reconstructed[3]==3);
                      from=5; to=3;
                      ok = ok && (Floyd_reconstruct<6>(reconstructed, from, to, dist, path, 99) == false);
                      for (int i=0; i<6; ++i)
                          reconstructed[i] = 0;
                      from=1; to=4;
                      ok = Floyd_reconstruct<6>(reconstructed, from, to, dist, path, 99);
                      ok = ok && (reconstructed[5]==4);
                      ok = ok && (reconstructed[4]==3);
                      ok = ok && (reconstructed[3]==2);
                      ok = ok && (reconstructed[2]==1);
                      return ok;
                  }()
                 );

    static_assert([]
                  {
                      bool ok = true;
                      return ok;
                  }()
                 );
                 
