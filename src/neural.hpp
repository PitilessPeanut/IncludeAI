#ifndef NEURAL_HPP
#define NEURAL_HPP

#include "asmtypes.hpp"
#include <cmath>
#include <assert.h>
#include <iostream>
//#include <arm_neon.h>
//#include <immintrin.h>


inline float sigmoid(float x) { return 1.0f / (1.0f + std::exp(-x)); }
inline float sigmoid_derivative(float x) { return x * (1.0f - x); }
inline float relu(float x) { return x > 0 ? x : 0.0001*x; }
inline float relu_derivative(float x) { return x > 0 ? 1 : 0.0001*x; }
using std::tanh;
inline float tanh_derivative(float x) { float t = std::tanh(x); return 1.0f - t * t; }


/****************************************/
/*  feed forward 32 (this one is worse) */
/****************************************/
    template <int InputSize, int OutputSize, int Max_layers, typename Rng>
    class FeedForward32
    {
    public:
        template <int Size>
        struct BitArray
        {
            UQWORD data[(Size+63) / 64] = {0};
            bool operator[](int idx) const
            {
                const int word = idx / 64;
                const int bit  = idx % 64;
                return (data[word] >> (63 - bit)) & 1;
            }
            void set(int idx, bool value)
            {
                const int word = idx / 64;
                const int bit  = idx % 64;
                if (value)
                    data[word] |= (UQWORD(1) << (63 - bit));
                else
                    data[word] &= ~(UQWORD(1) << (63 - bit));
            }
            bool operator==(const BitArray& other) const
            {
                for (int i = 0; i < (Size+63) / 64; ++i)
                {
                    if (data[i] != other.data[i])
                        return false;
                }
                return true;
            }
        };
    private:
        static constexpr int columns = InputSize+(Max_layers*2)+OutputSize;
        int nLayers = Max_layers;
        FLOAT weights[columns * columns];
        FLOAT biases[columns * Max_layers];
        BitArray<columns*columns> topologies[ Max_layers ];
        FLOAT activations[columns * Max_layers];
    private:
        static FLOAT u64_to_float(const UQWORD i)
        {
            union { UQWORD i; DOUBLE d; } u;
            // This generates a double in [1.0, 2.0) by setting the exponent bits
            // and using the top 52 bits of the UQWORD for the mantissa.
            u.i = (0x3FFULL << 52) | (i >> 12);
            return static_cast<FLOAT>(u.d - 1.0);
        }
    private:
        template <FLOAT (*Act)(FLOAT)>
        void forward567(const int layer, const FLOAT *input, int inputSize)
        {
            #if 0
            FLOAT tops[columns*columns];
            for (int dst = 0; dst < columns; ++dst)
            {
                for (int src = 0; src < inputSize; ++src)
                {
                    const int i = dst * columns + src;
                    tops[dst * columns + src] = topologies[layer][i];
                }
            }

            for (int dst = 0; dst < columns; ++dst)
                {
                    // AVX accumulator for the dot product (8 floats initialized to 0.0)
                    __m256 v_sum = _mm256_setzero_ps();
                    int src = 0;
                    const int weight_row_start = dst * columns;


                    //__m256 v_connected[inputSize/8];
                    //int pos = 0;
                    //for (int src2=src; src2 <= inputSize - 8; src2 += 8)
                    //{
                    //    const float connected_mask[8] = {
                    //        (float)topologies[layer][weight_row_start + src2 + 0],
                    //        (float)topologies[layer][weight_row_start + src2 + 1],
                    //        (float)topologies[layer][weight_row_start + src2 + 2],
                    //        (float)topologies[layer][weight_row_start + src2 + 3],
                    //        (float)topologies[layer][weight_row_start + src2 + 4],
                    //        (float)topologies[layer][weight_row_start + src2 + 5],
                    //        (float)topologies[layer][weight_row_start + src2 + 6],
                    //        (float)topologies[layer][weight_row_start + src2 + 7]
                    //    };
                    //    v_connected[pos++] = _mm256_loadu_ps(connected_mask);
                    //}
                    //pos = 0;

                    for (; src <= inputSize - 8; src += 8)
                    {
                        // Build the connection mask for 8 elements
                        //const float connected_mask[8] = {
                        //    (float)topologies[layer][weight_row_start + src + 0],
                        //    (float)topologies[layer][weight_row_start + src + 1],
                        //    (float)topologies[layer][weight_row_start + src + 2],
                        //    (float)topologies[layer][weight_row_start + src + 3],
                        //    (float)topologies[layer][weight_row_start + src + 4],
                        //    (float)topologies[layer][weight_row_start + src + 5],
                        //    (float)topologies[layer][weight_row_start + src + 6],
                        //    (float)topologies[layer][weight_row_start + src + 7]
                        //};

                        const __m256 v_connected = _mm256_loadu_ps(&tops[weight_row_start+src]);
                        //const __m256 v_connected = _mm256_loadu_ps(connected_mask);

                        // Load 8 contiguous inputs and weights (use 'loadu' for unaligned access)
                        const __m256 v_input = _mm256_loadu_ps(&input[src]);
                        const __m256 v_weight = _mm256_loadu_ps(&weights[weight_row_start + src]);

                        __m256 v_prod = _mm256_mul_ps(v_connected, v_weight);
                        v_sum = _mm256_fmadd_ps(v_prod, v_input, v_sum);
                    }

                    // Horizontal sum of the 8 floats in the v_sum vector
                    // This is a standard, efficient pattern for AVX
                    __m128 v_low = _mm256_castps256_ps128(v_sum);
                    __m128 v_high = _mm256_extractf128_ps(v_sum, 1);
                    v_low = _mm_add_ps(v_low, v_high);
                    __m128 h_sum = _mm_hadd_ps(v_low, v_low);
                    h_sum = _mm_hadd_ps(h_sum, h_sum);
                    float sum = _mm_cvtss_f32(h_sum);

                    // Handle any remaining elements (if inputSize is not a multiple of 8)
                    for (; src < inputSize; ++src)
                    {
                        const int i = weight_row_start + src;
                        if (topologies[layer][i])
                        {
                            sum += input[src] * weights[i];
                        }
                    }

                    // Apply bias and activation function
                    activations[(layer * columns) + dst] = Act(sum + biases[(layer * columns) + dst]);
                }
            #endif
        }


        template <FLOAT (*Act)(FLOAT)>
        void forward3455(const int layer, const FLOAT *input, int inputSize)
        {
            int cntW = 0;
            float sparseWeights[columns+columns];
            float sparseSrc[columns+columns];
            float sparseDst[columns+columns];
            for (int dst = 0; dst < columns; ++dst)
            {
                for (int src = 0; src < inputSize; ++src)
                {
                    const int i = dst * columns + src;
                    if (topologies[layer][i] != 0)
                    {
                        sparseWeights[cntW] = weights[i];
                      //  sparseSrc[cntSrc++] = src;
                    }
                }
            }





            for (int dst = 0; dst < columns; ++dst)
            {
                float sum = 0.0f;
                for (int src = 0; src < inputSize; ++src)
                {
                    const int i = dst * columns + src;
                    const float connected = topologies[layer][i];
                    sum += input[src] * weights[i] * connected;
                }
                activations[(layer*columns) + dst] = Act(sum + biases[(layer*columns) + dst]);
            }
        }



        template <FLOAT (*Act)(FLOAT)>
        void forward(const int layer, const FLOAT *input, int inputSize)
        {
            for (int dst = 0; dst < columns; ++dst)
            {
                FLOAT tops[columns];
                for (int src = 0; src < inputSize; ++src)
                {
                    const int i = dst * columns + src;
                    tops[src] = topologies[layer][i];
                }

                FLOAT sum = 0.0f;
                for (int src = 0; src < inputSize; ++src)
                {
                    const int i = dst * columns + src;
                    sum += input[src] * weights[i] * tops[src];
                }
                activations[(layer*columns) + dst] = Act(sum + biases[(layer*columns) + dst]);
            }
        }



            //for (int dst = 0; dst < columns; ++dst)
            //    {
            //        // Vector accumulator for the dot product.
            //        float32x4_t v_sum = vdupq_n_f32(0.0f);
            //        int src = 0;
            //        const int weight_row_start = dst * columns;
            //
            //        // Process the inner loop in chunks of 4 using NEON.
            //        for (; src <= inputSize - 4; src += 4)
            //        {
            //            // 1. Load 4 contiguous inputs. (FAST)
            //            const float32x4_t v_input = vld1q_f32(&input[src]);
            //
            //            // 2. Load 4 contiguous weights. (FAST)
            //            const float32x4_t v_weight = vld1q_f32(&weights[weight_row_start + src]);
            //
            //            // 3. Handle the 'connected' bits. This remains a bottleneck.
            //            //    We still need to build the mask on the fly.
            //            const float connected_mask[4] = {
            //                (float)topologies[layer][weight_row_start + src + 0],
            //                (float)topologies[layer][weight_row_start + src + 1],
            //                (float)topologies[layer][weight_row_start + src + 2],
            //                (float)topologies[layer][weight_row_start + src + 3]
            //            };
            //            const float32x4_t v_connected = vld1q_f32(connected_mask);
            //
            //            // 4. Multiply weights and inputs.
            //            float32x4_t v_prod = vmulq_f32(v_input, v_weight);
            //            v_sum = vmlaq_f32(v_sum, v_prod, v_connected);
            //        }
            //
            //        // 7. Horizontally add the 4 lanes of the sum vector to get a single float.
            //        float sum = vgetq_lane_f32(v_sum, 0) + vgetq_lane_f32(v_sum, 1) +
            //                    vgetq_lane_f32(v_sum, 2) + vgetq_lane_f32(v_sum, 3);
            //
            //        // 8. Handle any remaining elements (if inputSize is not a multiple of 4).
            //        for (; src < inputSize; ++src)
            //        {
            //            const int i = weight_row_start + src;
            //            if (topologies[layer][i])
            //            {
            //                sum += input[src] * weights[i];
            //            }
            //        }
            //
            //        // 9. Apply bias and activation function.
            //        activations[(layer * columns) + dst] = Act(sum + biases[(layer * columns) + dst]);
            //    }


        template <FLOAT (*Deriv)(FLOAT)>
        void backward(const int topologyIdx, FLOAT *deltas, const FLOAT *inputs, const FLOAT *activations, FLOAT learning_rate)
        {
            FLOAT hidden_error[columns] = {0.0f};
            for (int input_idx = 0; input_idx < columns; ++input_idx)
            {
                const FLOAT common = inputs[input_idx] * learning_rate;
                for (int hidden_idx = 0; hidden_idx < columns; ++hidden_idx)
                {
                    const int weight_idx = input_idx * columns + hidden_idx;
                    const auto connected = topologies[topologyIdx][weight_idx];
                    hidden_error[hidden_idx] += inputs[input_idx] * weights[weight_idx] * connected;
                    weights[weight_idx] += activations[hidden_idx] * common * connected;
                }
            }
            for (int h = 0; h < columns; ++h)
            {
                deltas[h] = hidden_error[h] * Deriv(activations[h]);
                const int biasIdx = (topologyIdx - 1) * columns + h;
                biases[biasIdx] += deltas[h] * learning_rate;
            }
        }

    public:
        explicit FeedForward32(Rng& rng, bool randomizeTopology = true)
        {
            // Initialize weights/biases with random values in range [-1.0, 1.0]
            // should be ~symmetric around zero
            for (int i = 0; i < columns * columns; ++i)
                weights[i] = 2.0f * u64_to_float(rng()) - 1.0f;
            for (int i = 0; i < columns * Max_layers; ++i)
                biases[i] = 2.0f * u64_to_float(rng()) - 1.0f;

            // Initialize DAG:
            int from = InputSize*columns;
            const int hiddenSize = (columns-(InputSize+OutputSize)) / (nLayers-1);
            for (int i=0; i<hiddenSize; ++i)
            {
                for (int j=0; j<InputSize; ++j)
                    topologies[0].set(from + j, true);
                from += columns;
            }
            from += InputSize;
            for (int layer=1; layer<nLayers-1; ++layer)
            {
                for (int i=0; i<hiddenSize; ++i)
                {
                    for (int j=0; j<hiddenSize; ++j)
                        topologies[1].set(from + j, true);
                    from += columns;
                }
                from += hiddenSize;
            }
            for (int i=0; i<OutputSize; ++i)
            {
                for (int j=0; j<hiddenSize; ++j)
                    topologies[nLayers-1].set(from + j, true);
                from += columns;
            }

            // Randomize connections a bit:
            for (int layer=0; randomizeTopology && layer<nLayers; ++layer)
            {
                for (int i=0; i<columns*columns; ++i)
                {
                    if (rng() % 100 < 15) // 15% chance to flip
                    {
                        const bool current = topologies[layer][i];
                        topologies[layer].set(i, !current);
                    }
                }
            }
        }

        FLOAT *evaluate(const FLOAT *inputs)
        {
            forward<relu>(0, inputs, InputSize);
            for (int i=1; i<nLayers-1; ++i)
                forward<relu>(i, &activations[(i-1)*columns], columns);
            // todo: tahn or softmax?
            forward<tanh>(nLayers-1, &activations[(nLayers-2)*columns], columns);
            return &activations[(nLayers-1)*columns];
        }

        FLOAT train(const FLOAT *inputs, const FLOAT *targets, FLOAT learning_rate)
        {
            forward<relu>(0, inputs, InputSize);
            for (int i=1; i<nLayers-1; ++i)
                forward<relu>(i, &activations[(i-1)*columns], columns);
            // last layer:
            // todo: tahn or softmax?
            forward<tanh>(nLayers-1, &activations[(nLayers-2)*columns], columns);

            // Output to last hidden (sigmoid):
            FLOAT squared_error_sum = 0.0f;
            FLOAT output_delta[columns] = {0.0f};
            const int lastLayerIdx = (nLayers-1) * columns;
            for (int j=0; j<OutputSize; ++j)
            {
                const int output_neuron_idx = (columns - OutputSize) + j;
                const FLOAT output_error = targets[j] - activations[lastLayerIdx + output_neuron_idx];
                output_delta[output_neuron_idx] = output_error * tanh_derivative(activations[lastLayerIdx + output_neuron_idx]);
                biases[lastLayerIdx + output_neuron_idx] += output_delta[output_neuron_idx] * learning_rate;

                // this line has nothing to do with anything, just for the return at the end:
                squared_error_sum += output_error * output_error;
            }


            // Hidden (relu):
            FLOAT *next_layer_deltas = output_delta;
            FLOAT delta_buffer[columns];
            for (int l = nLayers-1; l > 1; --l)
            {
                backward<relu_derivative>(l, delta_buffer, next_layer_deltas, &activations[(l-1)*columns], learning_rate);
                next_layer_deltas = delta_buffer;
            }


            // Hidden to input (sigmoid):
            backward<relu_derivative>(1, delta_buffer, next_layer_deltas, activations, learning_rate);


            // Update weights (hidden to input):
            for (int hidden_idx = 0; hidden_idx < columns; ++hidden_idx)
            {
                const FLOAT common = delta_buffer[hidden_idx] * learning_rate;
                for (int input_idx = 0; input_idx < InputSize; ++input_idx)
                {
                    const int weight_idx = hidden_idx * columns + input_idx;
                    const auto connected = topologies[0][weight_idx];
                    weights[weight_idx] += inputs[input_idx] * common * connected;
                }
            }
            return squared_error_sum / OutputSize; // mean squared error
        }
    };


/****************************************/
/*           feed forward 16 (IEEE 754) */
/****************************************/
template <int InputSize, int OutputSize, int Max_layers, typename Rng>
using FeedForward16 = FeedForward32<InputSize, OutputSize, Max_layers, Rng>;
// FLOAT masterCopy


/****************************************/
/*                     "Neural" Network */
/****************************************/
#if defined(__AVX512FP16__) || defined(__ARM_NEON) || defined(__wasm_simd128__)
  template <int InputSize, int OutputSize, int Max_layers, typename Rng>
  using Neural = FeedForward16<InputSize, OutputSize, Max_layers, Rng>;
#else
  // Of course not... 🙄
  template <int InputSize, int OutputSize, int Max_layers, typename Rng>
  using Neural = FeedForward32<InputSize, OutputSize, Max_layers, Rng>;
#endif



#else // NEURAL_HPP
  #error "double include"
#endif // NEURAL_HPP
