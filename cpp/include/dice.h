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
    namespace detail {
        template<typename T>
        constexpr bool const type_larget_than_int_v = sizeof(T) >= sizeof(int);
    }

    /*! @brief This class represents a normal N sided dice. @see https://en.wikipedia.org/wiki/Dice
        @todo replace typename Integral with concept
    */
    template<typename Integer, typename = std::void_t<std::enable_if_t< std::is_integral<Integer>::value, void>>>
    class dice_t {
    public:
        /*! Value type representing the roll of a dice.
        */
        using roll_t = Integer;

    private:
        std::mt19937 engine;
        std::uniform_int_distribution<std::conditional_t<detail::type_larget_than_int_v<Integer>, Integer, int>> distribution;

    public:
        /*! @brief
        @param sides The number of sides.
        */
        dice_t(roll_t sides) :
            engine(std::random_device()()),
            distribution(1, sides)
        {}

        /*! @brief This function rolls the dice
        */
        roll_t roll() {
            return static_cast<Integer>(distribution(engine));
        }

        /*! @brief Returns the Number of sides of the dice
        */
        roll_t sides() const { return static_cast<Integer>(distribution.max()); }
    };

    /*! @brief A dice which can be rolled upto 3 times.
        @details This class represents a normal N sided dice with the following
        rolling rules:
            1. If a N is rolled repeat step 1 upto 3 times.
            2. If N is rolled 3 times, all 3 rolls are nullified.
            3. Return the sequence of 3 rolls.
    */
    template<typename Dice>
    class upto3_dice_t {
    public:
        /*! Value type representing a single roll of a Dice.
        */
        using single_roll_t = typename Dice::roll_t;

        /*! Value type reprenting 3 rolls of the underlying Dice.
        */
        using roll_t = std::tuple<single_roll_t, single_roll_t, single_roll_t>;

    private:
        Dice dice;
        static constexpr auto const nil = typename Dice::roll_t{};

    public:
        /*! @brief
            @param sides The number of sides.
        */
        upto3_dice_t(single_roll_t sides) :
            dice(sides)
        {}

        /*! @brief This function rolls the dice upto 3 times
        */
        roll_t roll() {
            auto r0 = dice.roll();
            if (r0 != dice.sides())
                return std::make_tuple(r0, nil, nil);

            auto r1 = dice.roll();
            if (r1 != dice.sides())
                return std::make_tuple(r0, r1, nil);

            auto r2 = dice.roll();
            if (r2 != dice.sides())
                return std::make_tuple(r0, r1, r2);

            return std::make_tuple(nil, nil, nil);//! returns `0` on rolling `{ sides, sides, sides }`.
        }

        /*! @brief Returns the Number of sides of the dice
        */
        auto sides() { return d.sides(); }
    };

    /*!
        @brief A simple buffered dice which wraps a normal dice with a asynchronously filled buffer.
            The roll_t type of the given dice must fit within a boost::lockfree::queue cell
    */
    template<typename Dice>
    class fixed_buffer_dice_t {
    public:
        /*! Value type representing the roll of a dice.
        */
        using roll_t = typename Dice::roll_t;

    private:
        static constexpr auto buffer_length = 128 * 1024 * 1024;

        std::unique_ptr<roll_t[]> read, write;
        std::atomic_ptrdiff_t read_index = buffer_length / 2;
        Dice d;
        std::future<void> writer;
        std::mutex swap_mutex;

    public:
        /*! Allocates a read & a write buffer and then asynchronously requests a fill_buffer().
        */
        fixed_buffer_dice_t(roll_t sides) :
            read(new roll_t[64 * 1024 * 1024]),
            write(new roll_t[64 * 1024 * 1024]),
            d(sides)
        {
            fill_buffer();
        }

        /*! @brief Performs a read operation on the the read buffer.
            Performs a swap operation when the read buffer is fully utilized.
            In most cases the previous async write operation will have completed and the writer.get() is unlikely to block.
            If you encounter frequent blocks increase the buffer length.
            This operation must be synchronized externally if performed upon a single object from multiple threads.
        */
        roll_t roll() {
            if (read_index == buffer_length / 2)
                swap_buffers();
            return read[read_index++];
        }


        /*! @brief Returns the Number of sides of the dice
        */
        auto sides() { return d.sides(); }

    private:
        /*! @internal
            @details We launch a asynchronous task to fill the write buffer.
        */
        void fill_buffer() {
            using std::begin; using std::end;

            writer = std::async(std::launch::async, [this]() {
                std::generate_n(write.get(), buffer_length / 2, [this]() { return d.roll(); });
            });
        }

        /*! @internal @brief Swap the two buffers.
        */
        void swap_buffers() {
            std::lock_guard<std::mutex> guard(swap_mutex);
            if (read_index != buffer_length / 2) return;

            writer.get();// wait on future value if required
            swap(read, write);// swap in place
            read_index = {};// reset the read index
            fill_buffer();// launch writer async
        }

    };


    namespace testing {

    }

}