
#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H
#include <vector>
#include <array>
#include <iostream>

struct Player{
    virtual ~Player() = default;
    Player(char mov);
    virtual bool operator==(const Player& other)const = 0;
    virtual int read_data(char buf[], size_t buflen) = 0;
    virtual int write_data(const char buf[], size_t buflen) const = 0;
    char move;
};

struct HumanPlayer: Player{
    HumanPlayer(char mov, int fd);
    HumanPlayer(const HumanPlayer& other) = delete ;
    HumanPlayer& operator=(const HumanPlayer&) = delete;
    bool operator==(const Player& other) const override;
    int read_data(char buf[], size_t buflen) override;
    int write_data(const char buf[], size_t buflen) const override;
    ~HumanPlayer();
private:
    int fd;
    std::string Pbuf; //buffered input for player used for framing
};

struct BotPlayer: public Player{
    BotPlayer(char mov);
    bool operator==(const Player& other) const override;
    int read_data(char buf[], size_t bufflen) override;
    int write_data(const char buf[], size_t buflen) const override;
    ~BotPlayer();
private:
    std::array<int, 9> board;
    std::array<int, 9> best_positions;
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

void send_message(Player& p, const std::string message);

#endif
