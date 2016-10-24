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

#include <chrono>
#include <iostream>

#define SNL_TEST 1

#include "include\dice.h"
#include "include\types.h"

namespace snl = snakes_and_ladders;
namespace tlg = the_learning_games;

int main() {
    auto const builder = snl::board_builder_t(10)
        .add_jump(97, 78)
        .add_jump(94, 74)
        .add_jump(92, 72)
        .add_jump(86, 23)
        .add_jump(79, 99)
        .add_jump(70, 90)
        .add_jump(63, 59)
        .add_jump(61, 18)
        .add_jump(53, 33)
        .add_jump(50, 66)
        .add_jump(20, 41)
        .add_jump(16, 6)
        .add_jump(8, 30)
        .add_jump(3, 14)
        .add_jump(1, 38)
        .finalize();
    snl::board_t board(builder);

    auto const game_count = 1 << 22;
    tlg::upto3_dice_t<
        tlg::fixed_buffer_dice_t<
        tlg::dice_t<std::int8_t> > > dice(6);
    auto const buffer_length = 64 * 1024 * 1024;// 64 MB

    auto start_time = std::chrono::high_resolution_clock().now();
    dice.roll();
    auto end_time = std::chrono::high_resolution_clock().now();

    auto time_taken = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    auto drps = 1000. * double{ buffer_length } / time_taken;

    std::cout << "Time taken = " << time_taken << " ms\n";
    std::cout << "Dice rolls = " << buffer_length << "\n";
    std::cout << "DRPS       = " << drps << "\n";

    auto counter = game_count;
    snl::game_t game(board, 3);

    //player_strategy_t strategy;

    start_time = std::chrono::high_resolution_clock().now();
    while (counter--) {
        game.reset();
        while (game) {
            auto roll = dice.roll();
            game.move(std::get<0>(roll), std::get<1>(roll), std::get<2>(roll));
        }
    }
    end_time = std::chrono::high_resolution_clock().now();

    time_taken = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    auto gps = 1000. * double{ game_count } / time_taken;

    std::cout << "Time taken = " << time_taken << " ms\n";
    std::cout << "Games      = " << game_count << "\n";
    std::cout << "GPS        = " << gps << std::endl << std::endl;
    return 0;
}

