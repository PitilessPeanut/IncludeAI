#### Decision Making AI

This is a new, custom, hand-written implementation of a **Zero-Knowledge Reinforcement Learning** algorithm that, unlike AlphaZero, does NOT suffer from the [shallow trap](https://nullprogram.com/blog/2017/04/27) problem and should be able to play *solved* games perfectly.

General Game Playing, aka. *Effective Decision Making under Uncertain or Incomplete Information* (EDMUII), is the most difficult[^1] problem in the universe, far surpassing quantum physics, elliptic curve arithmetic and axionic singularity modulation. As such, this is an ongoing research project! Please be considerate 🔬
 

### Building

This was supposed to be a "header only" library, but I have given up on trying to implement it that way. While a proper build system is missing I recommend to simply copy all the files into your project and provide a ^world.hpp^ file to the linker: 
- gcc/clang: `-I../location_of_world.hpp`
- msvc: `properties->VC++ Directories->Include Directories`


[^1]: While still falling towards the "somewhat possible" side on the possible/impossible dividing line... 
