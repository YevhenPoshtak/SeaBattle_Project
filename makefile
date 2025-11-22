CXX = g++
CXXFLAGS = -g -Wall -std=c++11 -DWINDOWS -I.
LDFLAGS = -lpdcurses -lws2_32

TARGET = battleship.exe

SOURCES = main.cpp \
          user/user.cpp \
          network/host.cpp \
          network/client.cpp \
          game/game_board.cpp \
          game/game_piece.cpp \
          game/board_size_menu.cpp \
          ai/ai_player.cpp \
          ui/config.cpp \
          ui/ui_helper.cpp \
          ui/animation.cpp \
          utils/system_utils.cpp

OBJS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo Compilation successful!

%.o: %.cpp
	@echo Compiling $<...
	@$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f user/*.o
	rm -f game/*.o
	rm -f ai/*.o
	rm -f ui/*.o
	rm -f network/*.o
	rm -f utils/*.o
	rm -f $(TARGET)
	@echo Clean complete

rebuild: clean all

info:
	@echo Source files: $(SOURCES)
	@echo Object files: $(OBJS)
	@echo Target: $(TARGET)

debug: CXXFLAGS += -DDEBUG -O0
debug: rebuild

release: CXXFLAGS += -O2 -DNDEBUG
release: rebuild

.PHONY: all clean rebuild info debug release