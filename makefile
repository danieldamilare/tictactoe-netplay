CC = g++
CFLAGS = -Wall -g -fsanitize=undefined,address -std=c++20	
SRC =  $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
OBJ = $(SRC:.cpp=.o)
TARGET = tictactoe_server

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ -o $@ $(CFLAGS)

%.o:%.cpp $(HEADERS)
	$(CC) $(CFLAGS) $< -c -o $@

debug: CFLAGS += -DLOGSOCK
debug: clean all

clean:
	rm -rf $(OBJ) $(TARGET)

.PHONY: all debug clean
