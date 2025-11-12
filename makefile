CXX = g++
CXXFLAGS = -g -Wall -std=c++11 -DWINDOWS
LDFLAGS = -lpdcurses -lws2_32

TARGET = battleship.exe

SOURCES = main.cpp \
          user/user.cpp \
          user/host.cpp \
          user/client.cpp \
          game/game_board.cpp \
          game/game_piece.cpp \
          ai/ai_player.cpp

OBJS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Компіляція завершена успішно!"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Очищення завершено"

rebuild: clean all

info:
	@echo "Вихідні файли: $(SOURCES)"
	@echo "Об'єктні файли: $(OBJS)"
	@echo "Ціль: $(TARGET)"

.PHONY: all clean rebuild info