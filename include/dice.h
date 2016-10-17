/*MIT License

Copyright(c) 2016 Kedar Bodas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/*! @author Kedar Bodas
    @version 0.0.1
    @date 2016
    @copyright MIT License
*/
#pragma once

#include <cstdint>

#include <random>
#include <tuple>
#include <vector>
#include <future>
#include <cassert>

namespace the_learning_games {

    /*! @brief A N-sided dice which can be rolled upto 3 times.
        @details This class represents a normal N sided dice with the following
        rolling rules:
            1. If a N is rolled the dice is rolled again.
            2. If N is rolled 3 times, all 3 rolls are nullified.
            3. Return the sequence of upto 3 rolls.
        @details This class can be used to represent upto a 127 sided dice.
    */
    class dice_t {
        std::mt19937 engine;
        std::uniform_int_distribution<int> distribution;

    public:
        using single_roll_t = std::int8_t;//! Value type representing the rolls of a dice. int8_t restricts us to a 127 sided dice.
        using roll_t = std::tuple<single_roll_t, single_roll_t, single_roll_t>;//! A single_roll_t fold'ed upto3 times.
        
        /*! @brief
            @param sides The number of sides.
            @pre `1 < sides < 128`.
        */
        dice_t(single_roll_t sides) :
            engine(std::random_device()()),
            distribution(1, sides)
        {
            assert(1 < sides && sides <= 127);
        }

        /*! @internal
            @brief This function rolls the dice upto 3 times
        */
        roll_t roll() {
            auto r0 = static_cast<single_roll_t>(distribution(engine));
            if (r0 != distribution.max())
                return std::make_tuple(r0, single_roll_t{}, single_roll_t{});

            auto r1 = static_cast<single_roll_t>(distribution(engine));
            if (r1 != distribution.max())
                return std::make_tuple(r0, r1, single_roll_t{});

            auto r2 = static_cast<single_roll_t>(distribution(engine));
            if (r2 != distribution.max())
                return std::make_tuple(r0, r1, r2);

            return std::make_tuple(single_roll_t{}, single_roll_t{}, single_roll_t{});//! @internal return `0` on rolling `{ sides, sides, sides }`.
        }
    };

    /*!
        @brief A simple buffered dice which wraps a normal dice with a asynchronously filled buffer.
    */
    class buffered_dice_t {
        std::vector<dice_t::roll_t> read, write;
        std::atomic<int> current_index;
        std::mutex swap_mutex;
        std::future<void> writer;
        dice_t d;

    public:
        /*! Allocates a read & a write buffer and then asynchronously requests a fill_buffer().
        */
        buffered_dice_t(dice_t&& d, int buffer_length) :
            d(std::move(d)),
            read(buffer_length >> 1),
            write(buffer_length >> 1),
            current_index(buffer_length >> 1)
        {
            fill_buffer();
        }

        /*! @brief Performs a read operation on the the read buffer.
            Performs a swap operation when the read buffer is fully utilized.
            In most cases the previous async write operation will have completed and the writer.get() is unlikely to block.
            If you encounter frequent blocks increase the buffer length
        */
        dice_t::roll_t roll() {
            using std::swap;

            if (current_index != read.size())
                return read[current_index++];//read from cache

            writer.get();// retrieve the other buffer
            {
                std::lock_guard<std::mutex> lock(swap_mutex);
                swap(read, write);// swap in place
                current_index = {};// reset the read index
            }
            fill_buffer();// launch writer async
            return read[current_index++];
        }

    private:
        /*! @internal
            @details We launch a asynchronous tasklet to fill the write vector.
        */
        void fill_buffer() {
            writer = std::async(std::launch::async, [this]() {std::generate_n(write.begin(), write.size(), [this]() { return d.roll(); }); } );
        }
    };
}