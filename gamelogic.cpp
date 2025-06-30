#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <array>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <algorithm>
#include "gamelogic.h"
#include "log.h"

Player::Player(char mov, int fd): move{mov}, fd{fd}{}

bool Player::operator==(const Player& other) const{
    return other.fd == fd && other.move == move;
}
Player::~Player(){
        if(fd >= 0) close(fd);
}
    // uses buflen -1, store '\0' in the last byte
int Player::read_data(char buf[], size_t buflen){
    int result = recv(fd, buf, buflen-1, 0);
    LOG("Receiving data for player{}: {}", Player::move, buf);
    if (result > 0) buf[result] = '\0';
    return result;
}

int Player::write_data(const char buf[], size_t buflen){
    LOG("Writing data for player{}: {}", move, buf);
    int rem_bytes{static_cast<int>(buflen)};
    int cur{};
    const char * s = buf;

    while (rem_bytes > 0){
        cur = send(fd, s, rem_bytes, 0);
        if (cur == -1) return cur;
        rem_bytes -= cur;
        s = s + cur;
    }
    return buflen - rem_bytes;
}

Game::Game(Player& f, Player& s): first{f}, second{s}{}

void Game::print_board() const{
    std::cout << "+---+---+---+\n";
    for (int i = 0; i < 9; i++){
        std::cout << "| " << (board[i] == 0? ' ': board[i]) << " " ;
        if ((i+1) % 3 == 0){
            std::cout  << "|\n";
            std::cout << "+---+---+---+\n";
        }
    }
}

void Game::play_game(){
    Player* cur = &first;
    Player* next_pl = &second;

    while (!game_over()){
#ifdef LOGSOCK
        print_board();
#endif
        while(1){
            int bytelen  = read_move(*cur, *next_pl);
            std::string res = buf;
            if (bytelen == 0 ||(res.length() == 4 && res == "quit")){
                std::string msg = "mesg:Player quit the gamej";
                next_pl->write_data(msg.c_str(), msg.length());
                return;
            }
            LOG("RES: {}", res);
            if (res.length() == 1 && std::isdigit(res.at(0))){
                LOG("Res length == 1 && isdigit(0)");
                if(is_valid(res.at(0) - '0')) break;
                else {
                    LOG("Invalid move by res: {}", res);
                    std::string s{"Err:Invalid Move - Invalid Position"};
                    cur->write_data(s.c_str(), s.length());
                }
            } else{
                LOG("Res length is less than 1 or the first char is not digit");
                std::string s{"Err:Invalid Move - Please Enter digit 0 - 8"};
                cur->write_data(s.c_str(), s.length());
            }
        }
        board[buf[0] - '0'] = cur->move;

        std::string msg = std::string{"play:"} + cur->move + ":" + buf[0];
        cur->write_data(msg.c_str(), msg.length());
        next_pl->write_data(msg.c_str(), msg.length());
        std::swap(cur, next_pl);
    }
    LOG("Game Ended");
    char x{};
    if ((x = check_winner())){
        LOG("{} is the winner", x);
        std::string s1 = "mesg:You Won", s2 = "mesg:You lose";
        if (first.move == x){
            first.write_data(s1.c_str(), s1.length());
            second.write_data(s2.c_str(), s2.length());
        } else{
            second.write_data(s1.c_str(), s1.length());
            first.write_data(s2.c_str(), s2.length());
        }
    } else{
        LOG("Game ended in a tie");
        std::string s1 = "mesg:Tie";
        first.write_data(s1.c_str(), s1.length());
        second.write_data(s1.c_str(), s1.length());
    }
}


char Game::check_winner() const{
    for (auto w: win){
        if (board[w[0]] != '\0' && board[w[0]] == board[w[1]]
                && board[w[0]] == board[w[2]]) return board[w[0]];
    }
    return '\0';
}

bool Game::game_over() const{
    return (check_winner() != 0 || 
            std::find(board.begin(), board.end(), '\0') == board.end());
}
bool Game::is_valid(int n) const {
    return n < 9 && board[n] == '\0';
}

int Game::read_move(Player& cur, Player& nxt_pl){
    cur.write_data("move", 4);
    int bytelen = cur.read_data(buf, BUFSIZ);
    return  bytelen;
}
        
const std::vector<std::vector<int>>Game::win = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
    {0, 4, 8}, {2, 4, 6}
};
