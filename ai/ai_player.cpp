#include "ai_player.hpp"
#include <algorithm>
#include <ctime>

AIPlayer::AIPlayer(AIDifficulty diff) : difficulty(diff), hunting(false), huntDirection(0) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            opponentBoard[i][j] = '?'; 
        }
    }
    
    lastHit.x = -1;
    lastHit.y = -1;
    
    initializeAvailableShots();
    setupBoard();
}

void AIPlayer::setupBoard() {
    myBoard.generateRandomBoard(false);
}

void AIPlayer::initializeAvailableShots() {
    availableShots.clear();
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            AICoordinates coord;
            coord.x = j;
            coord.y = i;
            availableShots.push_back(coord);
        }
    }
    
    for (size_t i = 0; i < availableShots.size(); i++) {
        size_t j = rand() % availableShots.size();
        std::swap(availableShots[i], availableShots[j]);
    }
}

AICoordinates AIPlayer::getRandomShot() {
    if (availableShots.empty()) {
        AICoordinates coord;
        coord.x = rand() % BOARD_SIZE;
        coord.y = rand() % BOARD_SIZE;
        return coord;
    }
    
    AICoordinates shot = availableShots.back();
    availableShots.pop_back();
    return shot;
}

void AIPlayer::addAdjacentCells(int x, int y) {
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    
    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        
        if (nx >= 0 && nx < BOARD_SIZE && ny >= 0 && ny < BOARD_SIZE) {
            if (opponentBoard[ny][nx] == '?') {
                AICoordinates coord;
                coord.x = nx;
                coord.y = ny;
                targetQueue.push_back(coord);
            }
        }
    }
}

void AIPlayer::addDirectionalCells(int x, int y) {
    if (huntDirection == 1) { 
        if (y > 0 && opponentBoard[y-1][x] == '?') {
            AICoordinates coord;
            coord.x = x;
            coord.y = y - 1;
            targetQueue.push_back(coord);
        }
        if (y < BOARD_SIZE - 1 && opponentBoard[y+1][x] == '?') {
            AICoordinates coord;
            coord.x = x;
            coord.y = y + 1;
            targetQueue.push_back(coord);
        }
    } else if (huntDirection == 2) { 
        if (x > 0 && opponentBoard[y][x-1] == '?') {
            AICoordinates coord;
            coord.x = x - 1;
            coord.y = y;
            targetQueue.push_back(coord);
        }
        if (x < BOARD_SIZE - 1 && opponentBoard[y][x+1] == '?') {
            AICoordinates coord;
            coord.x = x + 1;
            coord.y = y;
            targetQueue.push_back(coord);
        }
    }
}

AICoordinates AIPlayer::getSmartShot() {
    if (!targetQueue.empty()) {
        AICoordinates shot = targetQueue.back();
        targetQueue.pop_back();
        return shot;
    }
    
    if (!availableShots.empty()) {
        for (size_t i = 0; i < availableShots.size(); i++) {
            if ((availableShots[i].x + availableShots[i].y) % 2 == 0) {
                AICoordinates shot = availableShots[i];
                availableShots.erase(availableShots.begin() + i);
                return shot;
            }
        }
        
        AICoordinates shot = availableShots.back();
        availableShots.pop_back();
        return shot;
    }
    
    AICoordinates coord;
    coord.x = rand() % BOARD_SIZE;
    coord.y = rand() % BOARD_SIZE;
    return coord;
}

AICoordinates AIPlayer::makeMove() {
    if (difficulty == EASY) {
        return getRandomShot();
    } else {
        return getSmartShot();
    }
}

void AIPlayer::receiveAttackResult(AICoordinates coords, char result) {
    opponentBoard[coords.y][coords.x] = result;
    
    if (difficulty == SMART) {
        if (result == 'h' || result == 'a' || result == 'b' || 
            result == 's' || result == 'd' || result == 'p') {
            hunting = true;
            
            if (lastHit.x != -1) {
                if (lastHit.x == coords.x) {
                    huntDirection = 1; 
                } else if (lastHit.y == coords.y) {
                    huntDirection = 2; 
                }
            }
            
            lastHit = coords;
            
            if (result != 'h') {
                hunting = false;
                huntDirection = 0;
                targetQueue.clear();
                lastHit.x = -1;
                lastHit.y = -1;
            } else {
                if (huntDirection != 0) {
                    addDirectionalCells(coords.x, coords.y);
                } else {
                    addAdjacentCells(coords.x, coords.y);
                }
            }
        } else if (result == 'm') {
            if (hunting && !targetQueue.empty()) {
            }
        }
    }
    
    for (size_t i = 0; i < availableShots.size(); i++) {
        if (availableShots[i].x == coords.x && availableShots[i].y == coords.y) {
            availableShots.erase(availableShots.begin() + i);
            break;
        }
    }
}

char AIPlayer::receiveAttack(AICoordinates coords) {
    char boardPiece = myBoard.boardArray[coords.y][coords.x];
    
    if (boardPiece != 'w' && boardPiece != 'x') {
        myBoard.boardArray[coords.y][coords.x] = 'x'; 
        
        bool shipSunk = true;
        for (int i = 0; i < BOARD_SIZE; i++) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                if (myBoard.boardArray[i][j] == boardPiece) {
                    shipSunk = false;
                    break;
                }
            }
            if (!shipSunk) break;
        }
        
        if (shipSunk) {
            return boardPiece; 
        } else {
            return 'h'; 
        }
    } else if (boardPiece == 'w') {
        myBoard.boardArray[coords.y][coords.x] = 'o'; 
        return 'm';
    } else {
        return 'x';
    }
}

Gameboard& AIPlayer::getBoard() {
    return myBoard;
}

bool AIPlayer::hasLost() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            char cell = myBoard.boardArray[i][j];
            if (cell == 'a' || cell == 'b' || cell == 'd' || 
                cell == 's' || cell == 'p') {
                return false; 
            }
        }
    }
    return true; 
}