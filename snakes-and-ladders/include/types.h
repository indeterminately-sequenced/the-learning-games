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
#include <stdexcept>

#include <algorithm>
#include <numeric>
#include <iterator>
#include <utility>

#include <vector>

namespace snakes_and_ladders {
    using cell_iterator_t = std::int16_t;
    using cell_offset_t = std::int16_t;
    using player_id_t = std::int16_t;
    using length_t = std::int8_t;

    //! represents the Haskell type EitherOr running finished
    enum class game_state_t : bool {
        finished = true,
        running = false
    };

    inline bool operator! (game_state_t v) { return !static_cast<bool>(v); }

    struct cell_t {//! A snakes and ladders cell
        cell_offset_t const next;
    };


    /*! @param c The cell.
        @brief Get the offset to the next cell relative to the current cell
        

            1. 0 if empty cell.
            2. Positive if c is the start of a ladder. The value represents the length of the ladder.
            3. Negative if c is the mouth of a snake. The value represents the length of the snake.
    */
    inline cell_offset_t next(cell_t const& c) {
        return c.next;
    }

    struct board_builder_t {
        using jump_t = std::pair<cell_iterator_t, cell_iterator_t>;
        using jump_list_t = std::vector<jump_t>;

        board_builder_t(length_t side) : side_(side) {}

        board_builder_t& add_jump(cell_iterator_t from, cell_iterator_t to) {
            if (from < cell_iterator_t{} || to < cell_iterator_t{}) throw std::logic_error("pre: source or destination less than start");
            if (from >= cell_iterator_t{ side_ } *side_ || to >= cell_iterator_t{ side_ } *side_) throw std::logic_error("pre: source or destination greater than end");
            if (std::abs(to - from) < cell_offset_t{ 2 }) throw std::logic_error("pre: jump length less than two");
            if ((to - from > cell_offset_t{}) && (from == cell_iterator_t{})) throw std::logic_error("pre: ladder at start");
            if ((to - from < cell_offset_t{}) && (from == cell_iterator_t{ side_ } *side_ - 1)) throw std::logic_error("pre: snake at end");
            //! @internal @todo throw on Jump not in same row

            jumps_.emplace_back(from, to);
            return *this;
        }

        jump_list_t const& jumps() const {
            return jumps_;
        }

        length_t side() const {
            return side_;
        }

        board_builder_t const& finalize() {
            using std::begin; using std::end;

            std::sort(begin(jumps_), end(jumps_));//sort required to simplify arena construction.
            return *this;
        }

    private:
        length_t const side_;
        jump_list_t jumps_;
    };

    struct board_t {
        explicit board_t(board_builder_t const& builder) :
            arena([&]() {//! @internal Pseudo constructor to construct a const complex member object
            using std::begin; using std::end;

            auto last_cell = cell_iterator_t{ builder.side() } *builder.side();
            auto current_cell = cell_iterator_t{};

            std::vector<cell_t> rc;
            rc.reserve(last_cell - current_cell + 1);//!< @internal Allocate the actual object

            for (auto const jump : builder.jumps()) {
                // Fill cells upto jump start
                std::generate_n(std::back_inserter(rc), std::min(last_cell - current_cell, jump.first - current_cell), []() -> cell_t { return{ 0 }; });
                //set jump source
                rc.emplace_back(cell_t{ jump.second - jump.first });
                current_cell = jump.first;
            }
            while (current_cell != last_cell) {//fill remaining cells
                rc.emplace_back(cell_t{ cell_offset_t{} });
                current_cell++;
            }
            return rc;
        }())//! @internal Execute the pseudo constructor. The move is probably elided.
        {}

        cell_iterator_t start_position() const {//!< @brief begin iterator
            return{};
        }

        cell_iterator_t end_position() const {//! @brief end iterator - 1
            return static_cast<cell_iterator_t>(arena.size() - 1);
        }

        bool is_jump_cell(cell_iterator_t c) const {//! @brief `!( is_snake || is_ladder)`
            return static_cast<bool>(arena[c].next);
        }

        cell_iterator_t advance(cell_iterator_t position, cell_offset_t count) const {//! @brief equivalent to modified std::advance(cell_iterator_t, count)
            if (position + count > end_position())
                return position;//The no advance until exact end variation
                //return end_position() - (position + count - end_position());// The reflect at end variation. ignores jumps?
            return position + count + next(arena[position + count]);
        }
    private:
        const std::vector<cell_t> arena;
    };

    class game_t {
        board_t const &board;
        player_id_t current_player_;
        std::vector<cell_iterator_t> players;
        game_state_t state_;

    public:
        using player_id_t = std::int8_t;

        game_t(board_t const &board, int n_players) :
            board(board),
            players(n_players, board.start_position()),
            current_player_{},
            state_(game_state_t::running)
        {}

        void reset() {
            state_ = game_state_t::running;//Set state to running
            std::fill(begin(players), end(players), cell_offset_t{ board.start_position() });//Set all players at beginning of board
        }

        explicit operator bool() const {
            return state_ == game_state_t::running;
        }

        player_id_t current_player() const {
            return current_player_;
        }

        auto const& all_player_positions() const {
            return players;
        }

        auto player_position(player_id_t id) const {
            return players[id];
        }

        /*! @param player The id of the player to remove.
            @brief Removes player from the players vector and returns the new id's for the remaining players.
        */
        std::vector<player_id_t> remove_player(player_id_t player) {
            std::vector<player_id_t> new_players(players.size());
            std::iota(begin(new_players), end(new_players), player_id_t{});//set index to self

            new_players.erase(new_players.begin() + player);//remove player index 
            players.erase(players.begin() + player);//remove player

            return new_players;
        }
        /*! 1. Moves the current_player() by upto 3 steps sequencially.
            2. Sets the game state to end and returns if a player reaches the end.
            3. Increments the current_player_ index.
        */
        void move(cell_offset_t first, cell_offset_t second, cell_offset_t third) {
            using std::begin; using std::end;

            cell_offset_t moves[] = { first, second, third };

            auto exec_single_step = [this](cell_offset_t offset) {
                players[current_player_] = board.advance(players[current_player_], offset);//advance(iterator, offset) equivalent on snl::board_t::arena
                while (board.is_jump_cell(players[current_player_])) {
                    players[current_player_] = board.advance(players[current_player_], 0);//take all jumps at target cells
                }
                if (players[current_player_] == board.end_position()) {//taken only at end
                    state_ = game_state_t::finished;
                    return state_;
                }
                return game_state_t::running;
            };

            if (std::all_of(begin(moves), end(moves), exec_single_step))// Maybe (execute 3) steps. Signal the game end state.
                current_player_ = complete_turn();//If game didn't end advance the current_player.
            return;
        }

    private:
        /*! @internal players is a Ring of size players.size(). current_player_ is a index on that ring.
            complete_turn() effectively implements +1 on that index.
        */
        player_id_t complete_turn() const {
            if (current_player_ + 1 == players.size())
                return player_id_t{};
            return current_player_ + 1;
        }
    };
}