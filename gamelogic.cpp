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

Player::Player(char mov): move{mov} {}
HumanPlayer::HumanPlayer(char mov, int fd):Player(mov), fd{fd}{
    Pbuf.reserve(1024);
}

bool HumanPlayer::operator==(const Player& other) const{
    if (auto * p = dynamic_cast<const HumanPlayer*>(&other)){
        return p->fd == fd && p->move == move;
    }
    return false;
}
HumanPlayer::~HumanPlayer(){
        if(fd >= 0){ close(fd);}
}
    // uses buflen -1, store '\0' in the last byte
int HumanPlayer::read_data(char buf[], size_t buflen){ /* buflenn is always the uppe
                                                     rbound amount to read*/
    while(true){
        size_t pos = Pbuf.find('\n');
        if (pos != std::string::npos){
            size_t len = std::min(buflen-1, pos+1);
            std::copy_n(Pbuf.begin(), len, buf);
            buf[len] = 0;
            Pbuf.erase(0, pos+1);
            LOG("Receiving data for player{}: {}", HumanPlayer::move, buf);
            return len;
        }
            char rb[512];
            int result = recv(fd, rb, 512, 0);
            if (result <= 0){
                if(result < 0) perror("recv");
                LOG("HumanPlayer disconnected or error reading input");
                return result;
            } 
            Pbuf.append(rb, result);
    }
}

int HumanPlayer::write_data(const char buf[], size_t buflen) const{
    LOG("Writing data for player: {}: {}", move, buf);
    size_t rem_bytes{buflen};
    int cur{};
    const char * str = buf;

    while (rem_bytes > 0){
        cur = send(fd, str, rem_bytes, 0);
        if (cur == -1) { return cur; }
        rem_bytes -= cur;
        str = str + cur;
    }
    return static_cast<int>(buflen - rem_bytes);
}

Game::Game(Player& f, Player& s): first{f}, second{s}{
    send_message(first, "ready:X");
    send_message(second, "ready:O");
}

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

void send_message(Player& p, const std::string message){
    std::string framed = message + '\n';
    p.write_data(framed.c_str(), framed.length());
}

int Game::handle_input(Player * cur, Player * next_pl){
      while(1){
            int bytelen  = read_move(*cur, *next_pl);
            std::string res = buf;
            if (bytelen <= 0 ){
                LOG("{}", bytelen==0? "Player disconnected": "Error reading from Player");
                send_message(*next_pl, "mesg:Player quit the game");
                send_message(*cur, "mesg:You quit the game");
                send_message(*next_pl, "over:disconnect");
                send_message(*cur, "over:disconnect");
                return false;
            }
            if (res == "quit\n"){
                LOG("Player quit");
                send_message(*next_pl, "mesg:Player quit the game");
                send_message(*next_pl, "over:disconnect");
                send_message(*cur, "over:disconnect");
                return false;
            }
            LOG("RES: {}", res);

            if (res.length() == 2 && std::isdigit(res.at(0)) && res.at(1) == '\n'){ //Protocol demands all input to be bounded by newline
                LOG("Valid format: Single digit");

                if(is_valid(res.at(0) - '0')){
                    board[res[0] - '0'] = cur->move;
                    return true;
                }

                else {
                    LOG("Invalid move by res: {}", res);
                    send_message(*cur, "err:Invalid Move - Position already taken or out of range");
                }
            } else{
                LOG("Invalid format: Expected Single digit: {}", res);
                send_message(*cur, "mesg:Enter a digit between 0–8 to make a move. Type 'quit' to exit.");
            }
        }
      return false;
}

void Game::play_game(){
    Player* cur = &first;
    Player* next_pl = &second;

    while (!game_over()){
#ifdef LOGSOCK
        print_board();
#endif
        if (!handle_input(cur, next_pl)) return;
        std::string msg = std::string{"play:"} + cur->move + ":" + buf[0] ;
        send_message(*cur, msg);
        send_message(*next_pl, msg);
        std::swap(cur, next_pl);
    }
    handle_result();
    send_message(*cur, "over:disconnect");
    send_message(*next_pl, "over:disconnect");
}

void Game::handle_result(){
    LOG("Game Ended");
    char x{};

    if ((x = check_winner())){
        LOG("{} is the winner", x);
        std::string s1 = "over:won\n", s2 = "over:lost\n";
        if (first.move == x){
            send_message(first, s1);
            send_message(second, s2);
        } else{
            send_message(second, s1);
            send_message(first, s2);
        }
    } else{
        LOG("Game ended in a tie");
        send_message(first,"over:tie");
        send_message(second,"over:tie");
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
    send_message(cur, "move");
    int bytelen = cur.read_data(buf, BUFSIZ);
    return  bytelen;
}
        
const std::vector<std::vector<int>>Game::win = {
    {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
    {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
    {0, 4, 8}, {2, 4, 6}
};
