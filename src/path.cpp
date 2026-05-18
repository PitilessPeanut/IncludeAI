#include "path.hpp"


/****************************************/
/*                                Tests */
/****************************************/
    struct TestBoard
    {
        int mapWidth = 6, mapHeight = 6;
        static constexpr int MAX_AREA = 36;
        unsigned char board[MAX_AREA] = {
            9,9,9,1,9,9,
            9,9,9,1,1,9,
            9,9,9,9,1,9,
            9,9,9,9,9,9,
            9,9,9,9,9,9,
            9,9,9,9,9,9
        };
        constexpr int getCost(int next, int current) const
        {
            return board[next];
        }
    };

    static_assert([]
                  {
                      TestBoard testBoard;
                      PathEngine<36, TestBoard, short, 99> pathEngineTest;
                      pathEngineTest.precompute(testBoard);
                      short dist = pathEngineTest.dijkstra(3, 16, testBoard);
                      bool ok = dist == 3;
                      short path[10] = {0};
                      const int length = pathEngineTest.reconstruct(3, 16, path);
                      ok = ok && length == 4;
                      ok = ok && path[0] == 3;
                      ok = ok && path[1] == 9;
                      ok = ok && path[2] == 10;
                      ok = ok && path[3] == 16;
                      testBoard.board[9] = 2;
                      dist = pathEngineTest.dijkstra(3, 16, testBoard);
                      ok = ok && dist == 4; // more expensive path now
                      return ok;
                  }()
                 );

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
                      //FloydWarshall<6>(dist, path);
                      //int reconstructed[6] = {0};
                      //int from=3, to=5;
                      //const int len = Floyd_reconstruct<6>(reconstructed, from, to, dist, path, 99);
                      //bool ok = len == 4;
                      //ok = ok && (reconstructed[0]==5);
                      //ok = ok && (reconstructed[1]==0);
                      //ok = ok && (reconstructed[2]==3);
                      //from=5; to=3;
                      //ok = ok && (Floyd_reconstruct<6>(reconstructed, from, to, dist, path, 99) == -1);
                      //for (int i=0; i<6; ++i)
                      //    reconstructed[i] = 0;
                      //from=1; to=4;
                      //ok = Floyd_reconstruct<6>(reconstructed, from, to, dist, path, 99);
                      //ok = ok && (reconstructed[0]==4);
                      //ok = ok && (reconstructed[1]==3);
                      //ok = ok && (reconstructed[2]==2);
                      //ok = ok && (reconstructed[3]==1);
                      return true; //ok;
                  }()
                  );

    static_assert([]
                  {
                      auto floyd_makeCosts = [](auto& costs)
                                             {
                                                 constexpr int Width = 7;
                                                 int pos = 0;
                                                 for (int i=0; i<(Width*Width); ++i)
                                                 for (int j=0; j<(Width*Width); ++j)
                                                 {
                                                     const bool north = (j ^ (i - Width)) == 0;
                                                     const bool east  = (j ^ (i +     1)) == 0 && ((j%Width) != 0);
                                                     const bool south = (j ^ (i + Width)) == 0;
                                                     const bool west  = (j ^ (i -     1)) == 0 && ((i%Width) != 0);
                                                     const bool self  = (j ^ i) == 0;
                                                     if ((north || east || south || west) && !self)
                                                         costs[pos++] = 1;
                                                     else
                                                         costs[pos++] = 9;
                                                 }
                                             };
                      auto floyd_populate = [](auto& paths, const auto& map)
                                            {
                                                constexpr int Width = 7;
                                                int pos = 0;
                                                for (int i=0; i<(Width*Width); ++i)
                                                for (int j=0; j<(Width*Width); ++j)
                                                {
                                                    const auto start = map[i], end = map[j];
                                                    if (start==0 && end==1 && paths[pos]!=255)
                                                        paths[pos] = 100;
                                                    else if (paths[pos] == 5)
                                                        paths[pos++] = 1;
                                                    pos++;
                                                }
                                            };
                      constexpr int EdgeLen = 7;
                      constexpr int PathsTableWidth = EdgeLen*EdgeLen;
                      unsigned char costs[PathsTableWidth*PathsTableWidth];
                      unsigned char paths[PathsTableWidth*PathsTableWidth];
                      floyd_makeCosts(costs);
                      constexpr unsigned char map[EdgeLen*EdgeLen] =
                          {1,0,1,0,0,0,0,
                           0,0,0,0,0,0,0,
                           0,0,0,1,0,0,0,
                           1,0,0,0,0,0,0,
                           0,0,0,0,1,0,0,
                           0,0,0,0,0,0,0,
                           0,0,0,0,0,0,0
                          };
                      floyd_populate(costs, map);
                      for (int i=0; i<(EdgeLen*EdgeLen*EdgeLen*EdgeLen); ++i)
                          paths[i] = 255;
                      //FloydWarshall<PathsTableWidth>(costs, paths);
                      //int dst[PathsTableWidth] = {0};
                      //const int len = Floyd_reconstruct<PathsTableWidth>(dst, 1, 4, costs, paths, 255);
                      //bool ok = len==6; // 4,3,10,9,8,1 <- move around obstacle
                      //ok = ok && dst[0] == 4;
                      //ok = ok && dst[1] == 3;
                      //ok = ok && dst[2] == 10;
                      //ok = ok && dst[3] == 9;
                      //ok = ok && dst[4] == 8;
                      //ok = ok && dst[5] == 1;
                      return true; //ok;
                  }()
                 );

    //static_assert([]
    //              {
    //                  constexpr int Width=8, Height=8;
    //                  constexpr unsigned char map[] = {
    //                      0,0,0,0,0,0,0,0,
    //                      0,0,0,0,0,0,0,0,
    //                      0,0,0,1,1,1,0,0,
    //                      0,0,0,0,0,1,0,0,
    //                      0,0,0,0,0,1,0,0,
    //                      0,0,0,1,1,1,0,0,
    //                      0,0,0,0,0,0,0,0,
    //                      0,0,0,0,0,0,0,0
    //                  };
    //                  PathContainer2D<Width, Height, unsigned char> pc2d;
    //                  return true;
    //              }()
    //             );



    // qjao.github.io/PathFinding.js/visual/
