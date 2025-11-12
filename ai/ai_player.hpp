#ifndef AI_PLAYER_HPP
#define AI_PLAYER_HPP

#include "../game/game_board.hpp"
#include <vector>
#include <cstdlib>
#include <ctime>

enum AIDifficulty {
    EASY,  
    SMART   
};

struct AICoordinates {
    int x, y;
};

class AIPlayer {
private:
    AIDifficulty difficulty;
    Gameboard myBoard;
    char opponentBoard[BOARD_SIZE][BOARD_SIZE]; 
    
    std::vector<AICoordinates> availableShots;
    
    std::vector<AICoordinates> targetQueue;
    AICoordinates lastHit;
    bool hunting; 
    int huntDirection; 
    
    void initializeAvailableShots();
    AICoordinates getRandomShot();
    AICoordinates getSmartShot();
    void addAdjacentCells(int x, int y);
    void addDirectionalCells(int x, int y);
    
public:
    AIPlayer(AIDifficulty diff);
    
    void setupBoard();
    AICoordinates makeMove();
    void receiveAttackResult(AICoordinates coords, char result);
    char receiveAttack(AICoordinates coords);
    Gameboard& getBoard();
    bool hasLost();
};

#endif 