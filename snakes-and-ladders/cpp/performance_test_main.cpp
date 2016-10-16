
#include <random>
#include <chrono>
#include <iostream>

#include "..\include\snakes-and-ladders\types.h"
namespace snl = snakes_and_ladders;

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
    snl::board_t b(builder);
    snl::dice_t d_inner(6);

    auto const game_count = 1 << 23;
    auto const mean_rolls_per_game = 200; //measured no is ~100 doubling for safety margin
    auto const buffer_length = game_count * mean_rolls_per_game;

    auto start_time = std::chrono::high_resolution_clock().now();
    auto end_time = std::chrono::high_resolution_clock().now();

        start_time = std::chrono::high_resolution_clock().now();
        snl::buffered_dice_t d(std::move(d_inner), buffer_length);
        end_time = std::chrono::high_resolution_clock().now();

    auto drps = double{ buffer_length } / std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

    std::cout << "Time taken = " << (end_time - start_time).count() << " us\n";
    std::cout << "Dice rolls = " << buffer_length << "\n";
    std::cout << "DRPS       = " << drps << "\n";

    int i = game_count;
    snl::game_t g(b, 3);

    {
        start_time = std::chrono::high_resolution_clock().now();
        while (i--) {
            g.reset();
            while (g) {
                //auto player = g.current_player();
                //auto source_pos = g.player_position(player);

                auto roll = d.roll();
                g.move(std::get<0>(roll), std::get<1>(roll), std::get<2>(roll));
                
                //auto destination_pos = g.player_position(player);
                //std::cout << "Player " << (int)player << " moved from " << (int)source_pos << " to " << (int)destination_pos << "\n";
            }
            //std::cout << "Player " << (int)g.current_player() << " won\n\n" ;

        }
        end_time = std::chrono::high_resolution_clock().now();
    }
    auto gps = double{ game_count } / std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

    std::cout << "Time taken = " << (end_time - start_time).count() << " us\n";
    std::cout << "Games      = " << game_count << "\n";
    std::cout << "GPS        = " << gps  << std::endl << std::endl;
	return 0;
}
