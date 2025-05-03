#ifndef SIMILARITY_HPP
#define SIMILARITY_HPP

/****************************************/
/*                              pattern */
/****************************************/
    template <int PatternSize>
    struct Pattern
    {
        static constexpr int patternSize = PatternSize;
        bool pattern[PatternSize] = {0}; // todo: bit array!
    };


/****************************************/
/*                              helpers */
/****************************************/
    float sqrt_runtime(float x);

    template <bool comptime=false>
    constexpr float similarity_sqrt(float x)
    {
        if constexpr (comptime)
        {
            if (x <= 0) { return 0; }
            float guess = x / 2.;
            float prev_guess = 0.;
            // Iterate until the guess stabilizes:
            for (int i = 0; i < 100; ++i)
            {
                prev_guess = guess;
                guess = (guess + x / guess) / 2.;
                // Check for convergence:
                if (prev_guess == guess) { break; }
            }
            return guess;
        }
        else
        {
            return sqrt_runtime(x);
        }
    }

    template <typename ValType, int PatternSize, bool comptime=false>
    constexpr float calculateNormalizedDotProduct(const ValType *hay, const ValType *needle)
    {
        float dotProduct = 0.0f;
        float normA = 0.0f;
        float normB = 0.0f;

        for (int j=0; j<PatternSize; ++j)
        {
            dotProduct += hay[j] * needle[j];
            normA += hay[j] * hay[j];
            normB += needle[j] * needle[j];
        }

        normA = similarity_sqrt<comptime>(normA);
        normB = similarity_sqrt<comptime>(normB);

        float similarity = 0.0f;
        if (normA > 0 && normB > 0) {
            similarity = dotProduct / (normA * normB);
        }
        return similarity;
    }


    template <int PatternSize>
    constexpr float calculateJaccardSimilarity(const bool *hay, const bool *needle)
    {
        int intersection = 0;
        int unionCount = 0;

        for (int i = 0; i < PatternSize; ++i)
        {
            // For binary data, intersection is the count of positions where both are 1
            if (hay[i] && needle[i]) {
                intersection++;
            }

            // Union is the count of positions where at least one is 1
            if (hay[i] || needle[i]) {
                unionCount++;
            }
        }

        // Avoid division by zero
        if (unionCount == 0) {
            return 0.0f; // Both arrays are all zeros
        }

        // Jaccard similarity = size of intersection / size of union
        return static_cast<float>(intersection) / unionCount;
    }


/****************************************/
/*                   similarity scoring */
/*   - Why cosine and not Jaccard? -    */
/* In chess, the overall pattern/       */
/* structure is often more important    */
/* than raw counts. Cosine similarity   */
/* better captures the "shape" or       */
/* configuration of the position        */
/****************************************/
    template <typename Pattern,  bool comptime=false>
    constexpr float diversify(Pattern *dst, const Pattern& input, const int maxPatterns)
    {
        // Find a pair that is MOST similar:
        float maxSimilarity = -1.0f;
        int idx1 = 0, idx2 = 0;
        for (int i=0; i<maxPatterns; ++i)
        {
            for (int j=i+1; j<maxPatterns; ++j)
            {
                const float similarity = calculateNormalizedDotProduct<bool, Pattern::patternSize>(dst[i].pattern, dst[j].pattern);
                if (similarity>maxSimilarity)
                {
                    maxSimilarity = similarity;
                    idx1 = i;
                    idx2 = j;
                }
            }
        }

        // Decide which of the pair to replace
        const float sim1 = calculateNormalizedDotProduct<bool, Pattern::patternSize, comptime>(dst[idx1].pattern, input.pattern);
        const float sim2 = calculateNormalizedDotProduct<bool, Pattern::patternSize, comptime>(dst[idx2].pattern, input.pattern);

        // Select the pattern that has higher similarity with input (we'll replace the one that's more redundant)
        const int replaceIdx = (sim1 < sim2) ? idx2 : idx1;

        // Replace only if the new pattern would decrease the maximum similarity
        // Compute similarities of replacement with all other patterns
        float maxNewSimilarity = -1.0f;
        for (int i = 0; i < maxPatterns; ++i)
        {
            if (i == replaceIdx) continue;
            const float potentialSim = calculateNormalizedDotProduct<bool, Pattern::patternSize>(dst[i].pattern, input.pattern);
            maxNewSimilarity = potentialSim > maxNewSimilarity ? potentialSim : maxNewSimilarity;
        }

        // Replace if doing so would reduce the maximum similarity in the collection
        if (maxNewSimilarity < maxSimilarity)
        {
            for (int i = 0; i < dst[replaceIdx].patternSize; ++i)
                dst[replaceIdx].pattern[i] = input.pattern[i];

            return maxNewSimilarity; // Return the new maximum similarity
        }
        else
        {
            // Not replacing - new pattern would increase similarity:
            return maxSimilarity; // Keep the current maximum similarity
        }
    }


#else // SIMILARITY_HPP
  #error "double include"
#endif // SIMILARITY_HPP
