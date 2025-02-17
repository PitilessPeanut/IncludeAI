# &#35;include&#60;AI&#62;

This is a new, custom, hand-written implementation of a **Zero-Knowledge Reinforcement Learning** algorithm that, unlike AlphaZero, does NOT suffer from the [shallow trap](https://nullprogram.com/blog/2017/04/27) problem and should (given enough resources) be able to play games with *complete information*[^1] perfectly.

Behold this MIND BLOWING [stb](https://github.com/nothings/stb/)-style *#include*-only single-header library focusing on General Game Playing, aka. *Effective Decision Making with Uncertain or Incomplete Information* (EDMUII) which aims to implement [Artificial General Intelligence](https://en.wikipedia.org/wiki/Artificial_general_intelligence) as a single-header drop-in library with an emphasis on quick deployment and ease-of-use.
*General Game Playing*, despite sounding fun, is the second most difficult problem in the universe, far surpassing quantum physics, elliptic curve arithmetic and axionic singularity modulation, but slightly "easier" than mapping homomorphic endofunctors to submanifolds of a Hilbert space[^2]. As such, it remains an ongoing research project! Thread carefully 🔬


### Building
1. 'python3 generate.py'
2. profit

### Using
1. '#define INCLUDEAI_IMPLEMENTATION' in ONE source file before `#include`ing includeai.hpp
2. Inspect the examples (in the `example` folder) on how to use
3. Profit...



[^1]: Like Chess, not Poker: No secrets, dice or cards
[^2]: The worst part about that sentence is that it apparently "means" something: https://www.reddit.com/r/cpp/comments/1gj0j1a/comment/lvcby4j/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1
