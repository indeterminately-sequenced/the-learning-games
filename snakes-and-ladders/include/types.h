#pragma once

#include <cstdint>

#include <iterator>
#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>
#include <tuple>

namespace snakes_and_ladders {
    using cell_id_t = std::int16_t;
    using cell_offset_t = std::int16_t;
    using player_id_t = std::int16_t;
    using length_t = std::int8_t;

    enum class game_state_t : bool {
        finished = true,
        running = false
    };

    inline bool operator! (game_state_t v) { return !static_cast<bool>(v); }

    struct cell_t {
        cell_offset_t const next;// A snakes and ladders cell
    };


    inline cell_offset_t next(cell_t const& c) {// get the increment for next
        return c.next;
    }

    struct board_builder_t {
        using jump_t = std::pair<cell_id_t, cell_id_t>;
        using jump_list_t = std::vector<jump_t>;

        board_builder_t(length_t side) : side_(side) {}

        board_builder_t& add_jump(cell_id_t from, cell_id_t to) {//Do what you will. This writes itself
            if (from < cell_id_t{} || to < cell_id_t{}) throw std::logic_error("pre: source or destination less than start");
            if (from >= cell_id_t{ side_ } *side_ || to >= cell_id_t{ side_ } *side_) throw std::logic_error("pre: source or destination greater than end");
            if (std::abs(to - from) < cell_offset_t{ 2 }) throw std::logic_error("pre: jump length less than two");
            if ((to - from > cell_offset_t{}) && (from == cell_id_t{})) throw std::logic_error("pre: ladder at start");
            if ((to - from < cell_offset_t{}) && (from == cell_id_t{ side_ } *side_ - 1)) throw std::logic_error("pre: snake at end");

            jumps_.emplace_back(from, to);
            return *this;
        }

        jump_list_t const& jumps() const {//Read
            return jumps_;
        }
        length_t side() const {//Read
            return side_;
        }

        board_builder_t const& finalize() {//ready to rock
            using std::begin; using std::end;

            std::sort(begin(jumps_), end(jumps_));//sort required to simplyfy arena construction.
            return *this;
        }

    private:
        length_t const side_;
        jump_list_t jumps_;
    };

    struct board_t {
        board_t(board_builder_t const& builder) :
            arena([&]() {//pseudo delegated constructor to construct a const complex member object
            using std::begin; using std::end;

            auto last_cell = cell_id_t{ builder.side() } *builder.side();
            auto current_cell = cell_id_t{};

            std::vector<cell_t> rc;
            rc.reserve(last_cell - current_cell + 1);//allocate the actual object

            for (auto const jump : builder.jumps()) {
                // Fill cells upto jump start
                std::generate_n(std::back_inserter(rc), std::min(last_cell - current_cell, jump.first - current_cell), []() -> cell_t { return{ 0 }; });
                //set jump source
                rc.emplace_back(cell_t{ jump.second - jump.first });
                current_cell = jump.first;
            }
            while (current_cell < last_cell) {//fill remaining cells
                rc.emplace_back(cell_t{ cell_offset_t{} });
                current_cell++;
            }
            rc.emplace_back(cell_t{ cell_offset_t{} });// set last cell
            return rc;
        }())//exec()!! The move is probably elided. This should just slide into the const object
        {}

        cell_id_t start_position() const {//begin iterator
            return{};
        }

        cell_id_t end_position() const {//end iterator - 1
            return static_cast<cell_id_t>(arena.size() - 1);
        }

        bool is_jump_cell(cell_id_t c) const {// get the increment for next
            return static_cast<bool>(arena[c].next);
        }

        cell_id_t advance(cell_id_t position, cell_offset_t count) const {//equivalent to modified std::advance(cell_iterator_t, count)
            if (position + count > end_position())
                return position;//The no advance until exact end variation
                //return end_position() - (position + count - end_position());// The reflect at end variation. ignores jumps?
            return position + count + next(arena[position + count]);//writes itself
        }
    private:
        const std::vector<cell_t> arena;
    };

    struct game_t {
        using player_id_t = std::int8_t;

        game_t(board_t const &board, int n_players) :
            board(board),
            players(n_players, board.start_position()),
            current_player_{},
            state_(game_state_t::running)
        {}//Look ma no body!!

        void reset() {
            state_ = game_state_t::running;//Set state to running
            std::fill(begin(players), end(players), cell_offset_t{ board.start_position() });//Set all players at beginning of board
        }

        explicit operator bool() const {
            return state_ == game_state_t::running;
        }

        player_id_t current_player() const {//This is a Read -> atomic 
            return current_player_;
        }

        auto const& all_player_positions() const {//This is a Read -> atomic 
            return players;
        }

        auto player_position(player_id_t id) const {//This is a Read -> atomic 
            return players[id];
        }

        std::vector<player_id_t> remove_player(player_id_t player) {////This is a Write -> atomic 
            std::vector<player_id_t> new_players(players.size());
            std::iota(begin(new_players), end(new_players), player_id_t{});//set index to self

            new_players.erase(new_players.begin() + player);//remove player index 
            players.erase(players.begin() + player);//The Write

            return new_players;
        }

        void move(cell_offset_t first, cell_offset_t second, cell_offset_t third) {
            using std::begin; using std::end;

            cell_offset_t moves[] = { first, second, third };

            auto exec_single_step = [this](cell_offset_t offset) {
                players[current_player_] = board.advance(players[current_player_], offset);//Increment the iterator
                while (board.is_jump_cell(players[current_player_])) {
                    players[current_player_] = board.advance(players[current_player_], 0);
                }
                if (players[current_player_] == board.end_position()) {//End check taken only once
                    state_ = game_state_t::finished;//Write state
                    return state_;
                }
                return game_state_t::running;
            };

            if (std::all_of(begin(moves), end(moves), exec_single_step))//The Write(s) 
                current_player_ = complete_turn();//The Optional Write. Always taken other than on finished game
            return;
        }

    private:
        player_id_t complete_turn() const {// This is the successor function on a Ring structure in Mathematics
            if (current_player_ + 1 == players.size())
                return player_id_t{};
            return current_player_ + 1;
        }
        board_t const &board;
        std::vector<cell_offset_t> players;
        player_id_t current_player_;
        game_state_t state_;
    };
}