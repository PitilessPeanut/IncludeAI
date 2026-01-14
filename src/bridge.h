#ifndef BRIDGE_HPP
#define BRIDGE_HPP

#ifdef __cplusplus
extern "C" {
#endif

    /* Called from outside: */
    int gaem_initGame(void *rawMem, const unsigned availMem);
    void gaem_launch(void *pEverything, void *rawMem);//, int seed);
    void gaem_step(void *rawMem);

    /* Calling outside: */
    //unsigned gaem_rand(void *pEverything);
    void gaem_scan(void *pEverything, void *dst);
    void gaem_draw(void *pEverything, const unsigned *updatesVrt, int size);
    void gaem_debugPrint(void *pEverything, const char *debugStr);


#ifdef __cplusplus
}
#endif


#else
  #error "double include"
#endif // BRIDGE_HPP

