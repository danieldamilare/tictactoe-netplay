
#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H
#include <vector>
#include <array>
#include <iostream>
struct Player{
    std::string Pbuf; //buffered input for player used for framing
    Player(char mov, int fd);
    Player(const Player& other) = delete ;
    Player& operator=(const Player&) = delete;
    bool operator==(const Player& other) const;
    int read_data(char buf[], size_t buflen);
    int write_data(const char buf[], size_t buflen);
    ~Player();
    char move;
private:
    int fd;
};

struct Game{
    const static std::vector<std::vector<int>> win;
    Game(Player& f, Player& s);
    void print_board() const;
    void play_game();
private:
    std::array<char, 9> board;
    char buf[BUFSIZ];
    Player& first, &second;
    char check_winner() const;
    bool game_over() const;
    bool is_valid(int n) const;
    int read_move(Player& cur, Player& nxt_pl);
    void handle_result();
    int handle_input(Player * cur, Player * next_pl);
};
#endif
