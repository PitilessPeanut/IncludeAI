#include "bridge.h"
#include "allocs.hpp"
#include <world.hpp>


/****************************************/
/*                                Debug */
/****************************************/
    void debugPrint(const char *debugStr, void *pEverything)
    {
        static void *device;
        if (!device && (pEverything!=nullptr))
            device = pEverything;

        gaem_debugPrint(device, debugStr);
    }


/****************************************/
/*                               Random */
/****************************************/
/*    unsigned getRand(void *pEverything)
    {
        static void *device;
        if (!device && (pEverything!=nullptr))
            device = pEverything;

        return gaem_rand(device);
    }*/


/****************************************/
/*                            Init game */
/****************************************/
    using ptrSize = decltype(sizeof(1));

    inline void *operator new(ptrSize /* ignore */, void *where) noexcept
    {
        return where;
    }

    int gaem_initGame(void *rawMem, const unsigned availMem)
    {
        setupAlloc(rawMem, availMem);

        // todo: write into mem to ensure it HAS been alloc'd
        // (https://lemire.me/blog/2021/10/27/in-c-how-do-you-know-if-the-dynamic-allocation-succeeded/)

        auto iptr = reinterpret_cast<ptrSize>(rawMem);
        const bool isAligned = !(iptr % alignof(World));

        if (isAligned && (sizeof(World) <= availMem))
        {
            World *mem = (World *)alloc(sizeof(World));
            if (mem != nullptr)
                new (mem) World();
        }
        else
            return (int)false;

        return (int)true;
    }


/****************************************/
/*                           Start game */
/****************************************/
    void gaem_launch(void *pEverything, void *rawMem) //, int seed)
    {
        //pcg64Rand(seed);

        World& world = *(World *)rawMem;
        world.device.pEverything = pEverything;
        world.device.scan = gaem_scan;
        world.device.draw = gaem_draw;

        //[[maybe_unused]] const unsigned unused = getRand(pEverything);
        
        debugPrint("Started", pEverything); // Provide ptr to device (pEverything) here
    }


/****************************************/
/*                            Step game */
/****************************************/
    void gaem_step(void *rawMem)
    {
        World& world = *(World *)rawMem;
        world.step();
    }

