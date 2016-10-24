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
    //! A jumping random access iterator on the board_t::arena.
    using cell_iterator_t = std::int16_t;

    //! A offset to the cell position relative to another cell on the board_t::arena.
    using cell_offset_t = std::int16_t;

    //! Represents a player in snl::game_t.
    using player_id_t = std::int16_t;

    //! Represents the length of a side of the board_t::arena.
    using length_t = std::int8_t;

    /*! @brief Represents the state of the game.
        @internal @ingroup Haskell_Comments equivalent to a Haskell Either running finished
    */
    enum class game_state_t : bool {
        finished = true,
        running = false
    };

    //! @brief A hack to convert enum class to bool
    inline bool operator! (game_state_t v) { return !static_cast<bool>(v); }

    /*! @brief A builder class to simplify board_t construction.
    */
    class board_builder_t {
    public:
        //! A pair of cells specifying the jump.
        using jump_t = std::pair<cell_iterator_t, cell_iterator_t>;

        //! A list of jumps.
        using jump_list_t = std::vector<jump_t>;

    private:
        length_t const side_;
        jump_list_t jumps_;

    public:

        /*! @brief Construct a board_builder_t
            @param side The length of the board_t to construct.
        */
        board_builder_t(length_t side) : side_(side) {}

        /*! @brief Add a jump to the list of jumps.
            @param from The source cell of the jump.
            @param to The target cell of the jump.
            @throws std::logic_error The exception string contains the Snakes & Ladders rule which was violated.
            @return Returns a const reference to *this.
        */
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

        /*! @brief @return Returns the list of jumps.
        */
        jump_list_t const& jumps() const {
            return jumps_;
        }

        /*! @brief @return Returns the side length of the game board.
        */
        length_t side() const {
            return side_;
        }

        /*! @brief Sorts the jump list in preparation for constructing a board_t.
            @return Returns a const reference to *this.
        */
        board_builder_t const& finalize() {
            using std::begin; using std::end;

            std::sort(begin(jumps_), end(jumps_));//sort required to simplify arena construction.
            return *this;
        }
    };

    /*! @brief The snakes and ladders game board.
    This board essentially consists of a const array of cells. Players begin at cell 0 which represents the starting state before any dice rolls.
    The game proceeds from 1 to N*N where N is the side length of the game board @see https://en.wikipedia.org/wiki/Snakes_and_Ladders .
    */
    class board_t {

        /*! @internal @brief A snakes and ladders cell.
        */
        struct cell_t {
            
            /*! @brief next is the offset to the next cell relative to the current cell

            1. 0 if empty cell.
            2. Positive if c is the start of a ladder. The value represents the length of the ladder.
            3. Negative if c is the mouth of a snake. The value represents the length of the snake.
            */
            cell_offset_t const next;
        };

        const std::vector<cell_t> arena;//! @internal The actual game board

        /*! @internal @brief Pseudo constructor to construct the arena.
            @param builder Provides side length & a list of jumps in sorted order
            Constructs a sparse DFA based upon the supplied jumps
        */
        static std::vector<cell_t> make_arena(board_builder_t const& builder) {
            using std::begin; using std::end;

            auto last_cell = cell_iterator_t{ builder.side() } *builder.side();
            auto current_cell = cell_iterator_t{};

            std::vector<cell_t> rc;
            rc.resize(last_cell - current_cell + 1, cell_t{ 0 });//! @internal Allocate the actual object setting all cell.next to 0

            for (auto const jump : builder.jumps()) {
                cell_t jump_source{ jump.second - jump.first };
                //set jump source cell.next to the length of the jump
                (*(rc.data() + jump.first)).~cell_t();// is this neccessary?
                new(rc.data() + jump.first) cell_t{ jump.second - jump.first };
            }
            return rc;
        }

    public:
        /*! @brief Constructs a board_t based upon the parameter pack supplied by builder
            @param builder The parameter pack containing the arena dimensions & the jumps in sorted order
        */
        explicit board_t(board_builder_t const& builder) :
            arena(make_arena(builder))
        {}

        cell_iterator_t begin() const {//! @brief Returns iterator to the start position of the arena
            return{};
        }

        cell_iterator_t end() const {//! @brief Returns iterator pointing one past the end position of the arena.
            return static_cast<cell_iterator_t>(arena.size());
        }

        bool is_jump_cell(cell_iterator_t c) const {//! @brief `!( is_snake(c) || is_ladder(c))`
            return arena[c].next != 0;
        }

        /*! @brief std::advance(cell_iterator_t, count) equivalent on snl::board_t::arena
        */
        cell_iterator_t advance(cell_iterator_t position, cell_offset_t count) const {
            if (position + count >= end())
                return position;//! @internal @ingroup Snakes_And_Ladders Once a player is less than dice.sides() steps from the end they may move only in a sequence of exact dice rolls.
                //return take_all_jumps(end_position() - (position + count - end_position()));//! @internal @ingroup Snakes_And_Ladders The reflect at end variation.
            return take_all_jumps(position + count);
        }

    private:
        /*! @internal @brief Takes all jumps present in a chain starting at position
        */
        cell_iterator_t take_all_jumps(cell_iterator_t position) const {
            while (is_jump_cell(position)) {
                position = position + arena[position].next;
            }
            return position;
        }
    };

    /*! @brief The game_t class represents the state of a game in progress.
        It provides a single non const member function move() which advances the state of the game. The game_t class is explicitly
        convertible to bool to simplify checking the termination condition.
    */
    class game_t {
        board_t const &board;
        player_id_t current_player_;
        std::vector<cell_iterator_t> players;
        game_state_t state_;

    public:
        /*! @brief Constructs a n_player game state on board.
            @param board A board_t instance on which the game will be simulated.
            @param n_players The number of players in the game.
        */
        game_t(board_t const &board, player_id_t n_players) :
            board(board),
            players(n_players, board.begin()),
            current_player_{},
            state_(game_state_t::running)
        {}

#ifdef SNL_TEST
        void reset() {
            state_ = game_state_t::running;//Set state to running
            std::fill(begin(players), end(players), cell_offset_t{ board.begin() });//Set all players at beginning of board
        }
#endif

        /*! @brief Returns true if the game is running.
        */
        explicit operator bool() const {
            return state_ == game_state_t::running;
        }

        /*! @brief Returns the player whose move is next. If the game is finished then returns the winner of the game.
        */
        player_id_t current_player() const {
            return current_player_;
        }

        /*! @brief Returns a const referance to the internal std::vector of player positions.
        */
        auto const& all_player_positions() const {
            return players;
        }

        /*! @brief Returns the position of the player on the game board
        */
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

            cell_offset_t moves[] = { first, second, third };//! @todo send sum of all 3 to the validate_move function

            auto exec_single_step = [this](cell_offset_t offset) {
                players[current_player_] = board.advance(players[current_player_], offset);

                if (players[current_player_] == board.end() - 1) {//taken only at end
                    state_ = game_state_t::finished;
                    return state_;
                }
                return game_state_t::running;
            };

            if (std::none_of(begin(moves),
                end(moves),
                [&exec_single_step](cell_offset_t offset) { return !(!(exec_single_step(offset))); }))//! @internal @ingroup Haskell_Comments Equivalent to a Haskell TakeWhile. Signal the game end state.
                complete_turn();//If game didn't end advance the current_player.
            return;
        }

    private:
        /*! @internal @ingroup Algebraic_Structures players is a Ring of size players.size(). current_player_ is a index on that ring.
            complete_turn() effectively implements the successor function on a Ring.
        */
        void complete_turn() {
            ++current_player_;
            if (current_player_ == players.size()) {
                current_player_ = player_id_t{};
            }
        }
    };
}