#pragma once

#include <type_traits>
#include <random>
#include <tuple>
#include <vector>

namespace the_learning_games {

    struct dice_t {
        using single_roll_t = std::int8_t;
        using roll_t = std::tuple<single_roll_t, single_roll_t, single_roll_t>;
        dice_t(single_roll_t sides) :
            engine(std::random_device()()),
            distribution(1, sides)
        {}// we have a n_sided dice. No body

        roll_t roll() {// a normal n_sided dice roll folded by upto3
            auto r0 = static_cast<single_roll_t>(distribution(engine));
            if (r0 != distribution.max())
                return std::make_tuple(r0, single_roll_t{}, single_roll_t{});

            auto r1 = static_cast<single_roll_t>(distribution(engine));
            if (r1 != distribution.max())
                return std::make_tuple(r0, r1, single_roll_t{});

            auto r2 = static_cast<single_roll_t>(distribution(engine));
            if (r2 != distribution.max())
                return std::make_tuple(r0, r1, r2);

            return std::make_tuple(single_roll_t{}, single_roll_t{}, single_roll_t{});// 0 on n,n,n;
        }
    private:
        std::mt19937 engine;
        std::uniform_int_distribution<int> distribution;
    };

    struct buffered_dice_t {
        buffered_dice_t(dice_t&& d, int buffer_length) :
            d(std::move(d)),
            buffer(buffer_length)
        {//we have memory
            fill_buffer();//init
        }

        dice_t::roll_t roll() {
            if (current_index != buffer.size())
                return buffer[current_index++];//read from cache
            fill_buffer();//really crappy cache starts here. This stalls for a fashionably long time at sizeof(cache)
            current_index = {};
            return buffer[current_index++];
        }
    private:

        void fill_buffer() {// do as you will. This writes itself.
            std::generate_n(buffer.begin(), buffer.size(), [this]() { return d.roll(); });
        }

        std::vector<dice_t::roll_t> buffer;
        int current_index = int{};
        dice_t d;
    };
}