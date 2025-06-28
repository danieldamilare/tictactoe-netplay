#include <iostream>
#include <string>
#include <array>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>
#include <algorithm>

struct Player{
private:
    char move;
    int fd;
public:
    Player(){

    }
    Player(char mov, int fd): move{mov}, fd{fd}{

    }
    bool 
    ~Player(){
        close(fd);
    }
    int read_data(char buf[], size_t buflen){
        int result = recv(fd, buf, buflen, 0);
        return result;
    }
    int write_data(const char buf[], size_t buflen){
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

    int is_active(){
        char buf[2];
       return  recv(fd, buf, 0, MSG_PEEK|MSG_DONTWAIT);
    }
};

class Game{
    std::array<char, 9> board{};
    char buf[BUFSIZ];
    const static std::vector<std::vector<int>> win{
            {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
            {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
            {0, 4, 8}, {2, 4, 6}
        };

    Player first, second;
    Game(Player f, Player s): first{f}, second{s}{
    }

    void print_board(){
        for (int i = 0; i < 9; i++){
            if (i % 3 == 0)
                std::cout << "+---+---+---+\n";
            std::cout << "| " << (board[i] == 0? ' ': board[i]) << " |";
            if ((i+1) % 4 == 0) std::cout  << "\n";
        }
    }
    char check_winner(){
        for (auto w: win){
            if (board[w[0]] == board[w[1]] == board[w[2]]) return board[w[0]];
        }
        return '\0';
    }

    bool game_over(){
        return (check_winner() != 0 || 
                std::find(board.begin(), board.end(), '\0') == board.end());
    }

    void play_game(){
        int turn = 0;
        Player cur;
        bool abrupt = false;
        while (!game_over()){
            cur = turn? second: first;
            cur.write_data("move", 4);
            int status = cur.read_data(buf, BUFSIZ);
            if (status == 0){
                //try to write to both players informing them of the disconnection
                if (first.move == cur.move)
            }
        }
    }
}
