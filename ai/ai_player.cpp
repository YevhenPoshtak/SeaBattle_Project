#include "ai_player.hpp"
#include "../game/board_size_menu.hpp"
#include "../game/board_layout.hpp"
#include "../game/ship_config.hpp"
#include "../game/game_settings.hpp"
#include "../game/game_logic_utils.hpp"
#include "../ui/animation.hpp"
#include <algorithm>
#include <ctime>
#include <cmath>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

extern GameSettings g_gameSettings;

AIPlayer::AIPlayer(AIDifficulty diff) 
    : User(false), 
      difficulty(diff), 
      hunting(false), 
      huntDirection(0) {
    
    int size = getBoardSize();
    opponentBoard.resize(size, std::vector<char>(size, '?'));
    
    lastHit.x = -1;
    lastHit.y = -1;
    
    attackSeed = time(NULL) + 12345;
    
    initializeAvailableShots();
    setupBoard();
}

void AIPlayer::connect() {
}

void AIPlayer::setupBoard() {
    myBoard.generateRandomBoardAuto(false);
}

void AIPlayer::initializeAvailableShots() {
    availableShots.clear();
    int size = getBoardSize();
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            AICoordinates coord;
            coord.x = j;
            coord.y = i;
            availableShots.push_back(coord);
        }
    }
    
    srand(attackSeed);
    for (size_t i = 0; i < availableShots.size(); i++) {
        size_t j = rand() % availableShots.size();
        std::swap(availableShots[i], availableShots[j]);
    }
    srand(time(NULL));
}

AICoordinates AIPlayer::getRandomShot() {
    srand(attackSeed);
    attackSeed = rand(); 
    
    if (availableShots.empty()) {
        AICoordinates coord;
        coord.x = rand() % getBoardSize();
        coord.y = rand() % getBoardSize();
        srand(time(NULL)); 
        return coord;
    }
    
    AICoordinates shot = availableShots.back();
    availableShots.pop_back();
    srand(time(NULL));
    return shot;
}

void AIPlayer::addAdjacentCells(int x, int y) {
    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};
    int size = getBoardSize();
    
    for (int i = 0; i < 4; i++) {
        int nx = x + dx[i];
        int ny = y + dy[i];
        
        if (nx >= 0 && nx < size && ny >= 0 && ny < size) {
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
    int size = getBoardSize();
    
    if (huntDirection == 1) { 
        if (y > 0 && opponentBoard[y-1][x] == '?') {
            AICoordinates coord;
            coord.x = x;
            coord.y = y - 1;
            targetQueue.push_back(coord);
        }
        if (y < size - 1 && opponentBoard[y+1][x] == '?') {
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
        if (x < size - 1 && opponentBoard[y][x+1] == '?') {
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
    
    srand(attackSeed);
    attackSeed = rand();
    
    if (!availableShots.empty()) {
        for (size_t i = 0; i < availableShots.size(); i++) {
            if ((availableShots[i].x + availableShots[i].y) % 2 == 0) {
                AICoordinates shot = availableShots[i];
                availableShots.erase(availableShots.begin() + i);
                srand(time(NULL)); 
                return shot;
            }
        }
        
        AICoordinates shot = availableShots.back();
        availableShots.pop_back();
        srand(time(NULL)); 
        return shot;
    }
    
    AICoordinates coord;
    coord.x = rand() % getBoardSize();
    coord.y = rand() % getBoardSize();
    srand(time(NULL)); 
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
    
    if (boardPiece != 'w' && boardPiece != 'x' && boardPiece != 'o') {
        char ship = boardPiece;
        myBoard.boardArray[coords.y][coords.x] = 'x';
        myBoard.markShipAsHit(ship);
        
        if (myBoard.isShipSunk(ship)) {
            return toupper(ship);
        } else {
            return 'h';
        }
    } else if (boardPiece == 'w') {
        myBoard.boardArray[coords.y][coords.x] = 'o';
        return 'm';
    } else {
        return 'm';
    }
}

Gameboard& AIPlayer::getBoard() {
    return myBoard;
}

bool AIPlayer::hasLost() {
    int size = getBoardSize();
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            char cell = myBoard.boardArray[i][j];
            if (cell == 'a' || cell == 'b' || cell == 'd' || 
                cell == 's' || cell == 'p') {
                return false; 
            }
        }
    }
    return true; 
}

void AIPlayer::gameLoop(SOCKET client_socket) {
    (void)client_socket;
    
    clear();
    
    int size = getBoardSize();
    int shots = g_gameSettings.shotsPerTurn;
    
    const char* diffName = (difficulty == EASY) ? "Easy" : "Smart";
    
    Gameboard playerBoard;

    int boardResult = 0;
    while (boardResult != 1) {
        boardResult = playerBoard.generateRandomBoard(true);  
        
        if (boardResult == 0) {
            boardResult = playerBoard.generateManualBoard();
        }
    }

    BoardLayout layout = calculateBoardLayout(size);
    
    int boardWidth = size * 4 + 8;
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    
    clear();
    
    char aiTitle[50];
    const char* yourBoardTitle;
    
    if (size >= 20) {
        sprintf(aiTitle, "AI-%s", diffName);
        yourBoardTitle = "You";
    } else if (size >= 15) {
        sprintf(aiTitle, "AI (%s)", diffName);
        yourBoardTitle = "Your";
    } else {
        sprintf(aiTitle, "AI Board (%s)", diffName);
        yourBoardTitle = "Your Board";
    }
    
    int yourBoardLen = strlen(yourBoardTitle);
    int aiTitleLen = strlen(aiTitle);
    
    int leftPad1 = (boardWidth - yourBoardLen) / 2;
    int rightPad1 = boardWidth - leftPad1 - yourBoardLen;
    int leftPad2 = (boardWidth - aiTitleLen) / 2;
    int rightPad2 = boardWidth - leftPad2 - aiTitleLen;
    
    move(layout.startY, layout.board1StartX);
    for (int i = 0; i < leftPad1; i++) printw("-");
    printw("%s", yourBoardTitle);
    for (int i = 0; i < rightPad1; i++) printw("-");
    
    move(layout.startY, layout.separatorX);
    printw("~~~~~");
    
    move(layout.startY, layout.board2StartX);
    for (int i = 0; i < leftPad2; i++) printw("-");
    printw("%s", aiTitle);
    for (int i = 0; i < rightPad2; i++) printw("-");
    printw("\n");

    move(layout.startY + 1, layout.board1StartX);
    for (int i = 0; i < boardWidth; i++) printw("_");
    move(layout.startY + 1, layout.separatorX);
    printw("~~~~~");
    move(layout.startY + 1, layout.board2StartX);
    for (int i = 0; i < boardWidth; i++) printw("_");
    printw("\n");

    move(layout.startY + 2, layout.board1StartX);
    printw("|  | A |");
    for (int i = 1; i < size; i++) {
        printw(" %c |", 'A' + i);
    }
    
    move(layout.startY + 2, layout.separatorX);
    printw("~~~~~");
    
    move(layout.startY + 2, layout.board2StartX + 4);
    printw("|  | A |");
    for (int i = 1; i < size; i++) {
        printw(" %c |", 'A' + i);
    }
    printw("\n");

    for (int i = 0; i < size; i++) {
        move(layout.startY + 3 + i, layout.board1StartX);
        printw("|%2d|", i + 1);
        for (int j = 0; j < size; j++) {
            printw("   |");
        }
        
        move(layout.startY + 3 + i, layout.separatorX);
        printw("~~~~~");
        
        move(layout.startY + 3 + i, layout.board2StartX + 4);
        printw("|%2d|", i + 1);
        for (int j = 0; j < size; j++) {
            printw("   |");
        }
    }

    move(layout.startY + 3 + size, layout.board1StartX);
    for (int i = 0; i < boardWidth; i++) printw("-");
    move(layout.startY + 3 + size, layout.separatorX);
    printw("~~~~~");
    move(layout.startY + 3 + size, layout.board2StartX);
    for (int i = 0; i < boardWidth; i++) printw("-");
    printw("\n");
    
    attron(A_UNDERLINE);
    mvprintw(1, 1, "instructions");
    attroff(A_UNDERLINE);
    
    mvprintw(2, 1, "w/↑ - up      a/← - left");
    mvprintw(3, 1, "s/↓ - down    d/→ - right");
    mvprintw(4, 1, "space/enter - select target");
    mvprintw(5, 1, "f - fire all shots");
    mvprintw(6, 1, "q - quit game");
    
    refresh();

    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (playerBoard.boardArray[i][j] != 'w') {
                attron(COLOR_PAIR(2));
                move(layout.startY + 3 + i, layout.board1StartX + 5 + (4 * j));
                addch(playerBoard.boardArray[i][j]);
            }
        }
    }
    attron(COLOR_PAIR(1));
    
    coordinates cursor;
    cursor.y = layout.startY + 3;
    cursor.x = layout.board2StartX + 9;
    int maxCursorX = cursor.x + (size - 1) * 4;
    int maxCursorY = cursor.y + size - 1;
    
    move(cursor.y, cursor.x);
    refresh();
    
    int maxHits = getTotalShipCells(size);
    int totalShips = getTotalShips(size);
    int playerHits = 0;
    int aiHits = 0;
    bool playerTurn = true;
    
    int playerShipsRemaining = totalShips;
    int aiShipsRemaining = totalShips;
    
    std::vector<std::vector<char>> aiKnownBoard(size, std::vector<char>(size, ' '));
    
    int grid_x = 0, grid_y = 0;
    
    struct PendingShot {
        int x, y;
        bool used;
    };
    
    std::vector<PendingShot> playerShots;
    playerShots.resize(shots);
    for (int i = 0; i < shots; i++) {
        playerShots[i].used = false;
    }
    
    int shotsSelected = 0;
    bool selectingMode = true;
    
    int animFrame = 0;
    int animStartY = maxY - 6;  
    
    int playerStatsY = layout.startY + 3 + size + 2;  
    int aiStatsY = layout.startY + 3 + size + 5;      
    
    while (playerHits < maxHits && aiHits < maxHits) {
        move(0, maxX - 35);
        clrtoeol();
        attron(COLOR_PAIR(5) | A_BOLD);
        printw("YOUR SHIPS: %d", playerShipsRemaining);
        attroff(A_BOLD);
        
        move(0, maxX - 15);
        attron(COLOR_PAIR(6) | A_BOLD);
        printw("AI: %d", aiShipsRemaining);
        attroff(A_BOLD);
        attron(COLOR_PAIR(1));
        
        if (animStartY > layout.startY + 3 + size + 5) {
            int animationStartX = 10;
            int animationEndX = maxX - 10;
    
            for (int clearY = animStartY - 4; clearY <= animStartY + 6; clearY++) {
                move(clearY, animationStartX);
                for (int x = animationStartX; x < animationEndX; x++) {
                    addch(' ');
                }
            }
            
            int cycleFrame = animFrame % 80;
            
            int yellowShipX = 15 + (cycleFrame / 2);
            int blueShipX = maxX - 25 - (cycleFrame / 2);
            
            attron(COLOR_PAIR(3));
            for (int i = 0; i < maxX; i += 2) {
                int waveY = animStartY + 4 + (int)(sin((i + animFrame * 0.5) * 0.2) * 1.5);
                if (waveY >= 0) {
                    mvaddch(waveY, i, '~');
                    if (i + 1 < maxX) {
                        mvaddch(waveY, i + 1, '~');
                    }
                }
            }
            attroff(COLOR_PAIR(3));
            
            int phase = (cycleFrame / 20);
            
            if (phase == 0 || phase == 1) {
                attron(COLOR_PAIR(5));
                mvprintw(animStartY - 2, yellowShipX, "    _~_");
                mvprintw(animStartY - 1, yellowShipX, "   /___\\");
                mvprintw(animStartY,     yellowShipX, "  |=====|>");
                mvprintw(animStartY + 1, yellowShipX, " /~~~~~~~\\");
                mvprintw(animStartY + 2, yellowShipX, "~~~~~~~~~~~");
                attroff(COLOR_PAIR(5));
                
                attron(COLOR_PAIR(6));
                mvprintw(animStartY - 2, blueShipX, " _~_");
                mvprintw(animStartY - 1, blueShipX, "/___\\");
                mvprintw(animStartY,     blueShipX, "<|=====|");
                mvprintw(animStartY + 1, blueShipX, "/~~~~~~~\\");
                mvprintw(animStartY + 2, blueShipX, "~~~~~~~~~~~");
                attroff(COLOR_PAIR(6));
                
            } else if (phase == 2) {
                attron(COLOR_PAIR(5));
                mvprintw(animStartY - 2, yellowShipX, "    _~_");
                mvprintw(animStartY - 1, yellowShipX, "   /___\\");
                mvprintw(animStartY,     yellowShipX, "  |=====|>");
                mvprintw(animStartY + 1, yellowShipX, " /~~~~~~~\\");
                mvprintw(animStartY + 2, yellowShipX, "~~~~~~~~~~~");
                attroff(COLOR_PAIR(5));
                
                int projectileProgress = cycleFrame % 20;
                int projectileX = yellowShipX + 11 + (projectileProgress * (blueShipX - yellowShipX - 15) / 20);
                if (projectileX < blueShipX - 2) {
                    attron(COLOR_PAIR(2));
                    mvprintw(animStartY, projectileX, "===>");
                    attroff(COLOR_PAIR(2));
                }
                
                attron(COLOR_PAIR(6));
                mvprintw(animStartY - 2, blueShipX, " _~_");
                mvprintw(animStartY - 1, blueShipX, "/___\\");
                mvprintw(animStartY,     blueShipX, "<|=====|");
                mvprintw(animStartY + 1, blueShipX, "/~~~~~~~\\");
                mvprintw(animStartY + 2, blueShipX, "~~~~~~~~~~~");
                attroff(COLOR_PAIR(6));
                
            } else if (phase == 3) {
                attron(COLOR_PAIR(5));
                mvprintw(animStartY - 2, yellowShipX, "    _~_");
                mvprintw(animStartY - 1, yellowShipX, "   /___\\");
                mvprintw(animStartY,     yellowShipX, "  |=====|>");
                mvprintw(animStartY + 1, yellowShipX, " /~~~~~~~\\");
                mvprintw(animStartY + 2, yellowShipX, "~~~~~~~~~~~");
                attroff(COLOR_PAIR(5));
                
                int explosionFrame = cycleFrame % 20;
                if (explosionFrame < 10) {
                    attron(COLOR_PAIR(4));
                    mvprintw(animStartY - 3, blueShipX - 2, "  * * *");
                    mvprintw(animStartY - 2, blueShipX - 2, " * * * *");
                    mvprintw(animStartY - 1, blueShipX - 2, "* BOOM *");
                    mvprintw(animStartY,     blueShipX - 2, "* * * * *");
                    mvprintw(animStartY + 1, blueShipX - 2, " * * * *");
                    mvprintw(animStartY + 2, blueShipX - 2, "  * * *");
                    attroff(COLOR_PAIR(4));
                } else {
                    attron(COLOR_PAIR(1));
                    mvprintw(animStartY - 1, blueShipX, " . . .");
                    mvprintw(animStartY,     blueShipX, ". . . .");
                    mvprintw(animStartY + 1, blueShipX, " . . .");
                    attroff(COLOR_PAIR(1));
                }
            }
        }
        
        if (playerTurn) {
            if (selectingMode) {
                move(1, 82);
                clrtoeol();
                attron(COLOR_PAIR(6));
                char msg[70];
                sprintf(msg, "Select %d (or less) targets (%d/%d) - F to fire",
                        shots, shotsSelected, shots);
                printw("%s", msg);
                attron(COLOR_PAIR(1));
                move(cursor.y, cursor.x);
                refresh();
                
                #ifdef _WIN32
                    Sleep(50);
                #else
                    usleep(50000);
                #endif
                
                animFrame++;
                if (animFrame >= 80) animFrame = 0;
                
                nodelay(stdscr, TRUE);
                int key = getch();
                nodelay(stdscr, FALSE);
                
                if (key == ERR) continue;
                
                flushinp();
                
                switch (key) {
                    case KEY_LEFT:
                    case 'a':
                    case 'A':
                        if (cursor.x > layout.board2StartX + 9 && grid_x > 0) {
                            cursor.x -= 4;
                            grid_x--;
                        }
                        break;
                    case KEY_RIGHT:
                    case 'd':
                    case 'D':
                        if (cursor.x < maxCursorX && grid_x < size - 1) {
                            cursor.x += 4;
                            grid_x++;
                        }
                        break;
                    case KEY_UP:
                    case 'w':
                    case 'W':
                        if (cursor.y > layout.startY + 3 && grid_y > 0) {
                            cursor.y -= 1;
                            grid_y--;
                        }
                        break;
                    case KEY_DOWN:
                    case 's':
                    case 'S':
                        if (cursor.y < maxCursorY && grid_y < size - 1) {
                            cursor.y += 1;
                            grid_y++;
                        }
                        break;
                    case ' ':
                    case 10:
                        if (aiKnownBoard[grid_y][grid_x] == ' ' && shotsSelected < shots) {
                            bool alreadySelected = false;
                            for (int i = 0; i < shotsSelected; i++) {
                                if (playerShots[i].x == grid_x && playerShots[i].y == grid_y) {
                                    alreadySelected = true;
                                    break;
                                }
                            }
                            
                            if (!alreadySelected) {
                                playerShots[shotsSelected].x = grid_x;
                                playerShots[shotsSelected].y = grid_y;
                                playerShots[shotsSelected].used = true;
                                shotsSelected++;
                                
                                attron(COLOR_PAIR(5) | A_BOLD);
                                move(cursor.y, cursor.x - 1);
                                addch('[');
                                move(cursor.y, cursor.x);
                                addch('+');
                                move(cursor.y, cursor.x + 1);
                                addch(']');
                                attron(COLOR_PAIR(1));
                            }
                        }
                        break;
                    case 'f':
                    case 'F':
                        if (shotsSelected > 0) {
                            selectingMode = false;
                        }
                        break;
                    case 'q':
                    case 'Q':
                        return;
                }
                
                move(cursor.y, cursor.x);
                refresh();
                
            } else {
                move(1, 82);
                clrtoeol();
                attron(COLOR_PAIR(4) | A_BOLD);
                printw("                    FIRING!                                ");
                attron(COLOR_PAIR(1));
                refresh();
                
                for (int i = 0; i < 3; i++) {
                    move(playerStatsY + i, layout.board1StartX);
                    clrtoeol();
                }
                
                attron(A_UNDERLINE | COLOR_PAIR(2));
                mvprintw(playerStatsY, layout.board1StartX, "Your volley:");
                attroff(A_UNDERLINE | COLOR_PAIR(2));
                attron(COLOR_PAIR(1));
                
                char playerStatsLine[256] = "";
                
                for (int i = 0; i < shotsSelected; i++) {
                    AICoordinates attack;
                    attack.x = playerShots[i].x;
                    attack.y = playerShots[i].y;
                    
                    char result = receiveAttack(attack);
                    
                    int shot_y = layout.startY + 3 + attack.y;
                    int shot_x = layout.board2StartX + 9 + (4 * attack.x);
                    
                    move(shot_y, shot_x - 1);
                    addch(' ');
                    move(shot_y, shot_x + 1);
                    addch(' ');
                    
                    char shotInfo[16];
                    char resultChar = (result == 'h') ? 'X' : (result == 'm') ? 'O' : 'S';
                    sprintf(shotInfo, "%c%d-%c", 'A' + attack.x, attack.y + 1, resultChar);
                    
                    if (i > 0) strcat(playerStatsLine, ", ");
                    strcat(playerStatsLine, shotInfo);
                    
                    if (result == 'h') {
                        attron(COLOR_PAIR(4));
                        move(shot_y, shot_x);
                        addch('X');
                        aiKnownBoard[attack.y][attack.x] = 'h';
                        playerHits++;
                    } else if (result == 'm') {
                        attron(COLOR_PAIR(3));
                        move(shot_y, shot_x);
                        addch('O');
                        aiKnownBoard[attack.y][attack.x] = 'm';
                    } else {
                        attron(COLOR_PAIR(4) | A_BOLD);
                        move(shot_y, shot_x);
                        addch('S');
                        aiKnownBoard[attack.y][attack.x] = 's';
                        playerHits++;
                        aiShipsRemaining--;
                        
                        for (int row = 0; row < size; row++) {
                            for (int col = 0; col < size; col++) {
                                if (aiKnownBoard[row][col] == 'h') {
                                    aiKnownBoard[row][col] = 's';
                                    int cell_y = layout.startY + 3 + row;
                                    int cell_x = layout.board2StartX + 9 + (4 * col);
                                    attron(COLOR_PAIR(4) | A_BOLD);
                                    move(cell_y, cell_x);
                                    addch('S');
                                }
                            }
                        }
                    }
                    attron(COLOR_PAIR(1));
                    
                    attron(COLOR_PAIR(1));
                    mvprintw(playerStatsY + 1, layout.board1StartX, "%s", playerStatsLine);
                    attroff(COLOR_PAIR(1));
                    
                    refresh();
                    
                    #ifdef _WIN32
                        Sleep(400);
                    #else
                        usleep(400000);
                    #endif
                }
                
                if (playerHits >= maxHits) {
                    drawFirework(true);
                    return;
                }
                
                shotsSelected = 0;
                selectingMode = true;
                playerTurn = false;
                
                for (int i = 0; i < size; i++) {
                    for (int j = 0; j < size; j++) {
                        if (aiKnownBoard[i][j] == ' ') {
                            continue;
                        }
                        int cell_y = layout.startY + 3 + i;
                        int cell_x = layout.board2StartX + 9 + (4 * j);
                        
                        if (aiKnownBoard[i][j] == 's') {
                            attron(COLOR_PAIR(4) | A_BOLD);
                            move(cell_y, cell_x);
                            addch('S');
                        } else if (aiKnownBoard[i][j] == 'h') {
                            attron(COLOR_PAIR(4));
                            move(cell_y, cell_x);
                            addch('X');
                        } else if (aiKnownBoard[i][j] == 'm') {
                            attron(COLOR_PAIR(3));
                            move(cell_y, cell_x);
                            addch('O');
                        }
                        attron(COLOR_PAIR(1));
                    }
                }
            }
        } else {
            move(1, 98);
            clrtoeol();
            attron(COLOR_PAIR(5));
            printw(" AI's turn...                           ");
            attron(COLOR_PAIR(1));
            refresh();
            
            #ifdef _WIN32
                Sleep(1000);
            #else
                usleep(1000000);
            #endif
            
            for (int i = 0; i < 3; i++) {
                move(aiStatsY + i, layout.board1StartX);
                clrtoeol();
            }
            
            attron(A_UNDERLINE | COLOR_PAIR(4));
            mvprintw(aiStatsY, layout.board1StartX, "AI volley:");
            attroff(A_UNDERLINE | COLOR_PAIR(4));
            attron(COLOR_PAIR(1));
            
            char aiStatsLine[256] = "";
            
            for (int shot = 0; shot < shots; shot++) {
                AICoordinates aiMove = makeMove();
                char result = 'w';
                char boardPiece = playerBoard.boardArray[aiMove.y][aiMove.x];
                
                if (boardPiece == 'w') {
                    result = 'm';
                    playerBoard.boardArray[aiMove.y][aiMove.x] = 'o';
                } else if (boardPiece != 'x' && boardPiece != 'o') {
                    char ship = boardPiece;
                    playerBoard.boardArray[aiMove.y][aiMove.x] = 'x';
                    playerBoard.markShipAsHit(ship);
                    aiHits++;
                    
                    if (playerBoard.isShipSunk(ship)) {
                        result = toupper(ship);
                        playerShipsRemaining--;
                    } else {
                        result = 'h';
                    }
                } else {
                    if (boardPiece == 'o') {
                        result = 'm';
                    } else {
                        result = 'h';
                    }
                }
                
                int shot_y = layout.startY + 3 + aiMove.y;
                int shot_x = layout.board1StartX + 5 + (4 * aiMove.x);
                
                char shotInfo[16];
                char resultChar = (result == 'm') ? 'O' : (result == 'h') ? 'X' : 'S';
                sprintf(shotInfo, "%c%d-%c", 'A' + aiMove.x, aiMove.y + 1, resultChar);
                
                if (shot > 0) strcat(aiStatsLine, ", ");
                strcat(aiStatsLine, shotInfo);
                
                if (result == 'm') {
                    attron(COLOR_PAIR(3));
                    move(shot_y, shot_x);
                    addch('O');
                } else {
                    attron(COLOR_PAIR(4));
                    move(shot_y, shot_x);
                    addch('X');
                }
                attron(COLOR_PAIR(1));
                
                receiveAttackResult(aiMove, result);
                
                attron(COLOR_PAIR(1));
                mvprintw(aiStatsY + 1, layout.board1StartX, "%s", aiStatsLine);
                attroff(COLOR_PAIR(1));
                
                refresh();
                
                #ifdef _WIN32
                    Sleep(400);
                #else
                    usleep(400000);
                #endif
            }
            
            if (aiHits >= maxHits) {
                drawFirework(false);
                return;
            }
            
            playerTurn = true;
        }
    }
}

void AIPlayer::playGame(AIDifficulty difficulty) {
    clear();
    
    int size = getBoardSize();
    int shots = selectShotsPerTurn(size);
    g_gameSettings.shotsPerTurn = shots;
    
    clear();
    const char* diffName = (difficulty == EASY) ? "Easy" : "Smart";
    mvprintw(2, 2, "Playing against %s AI", diffName);
    mvprintw(3, 2, "Board: %dx%d | Shots: %d per turn", size, size, shots);
    refresh();
    
    #ifdef _WIN32
        Sleep(2000);
    #else
        usleep(2000000);
    #endif
    
    AIPlayer ai(difficulty);
    ai.gameLoop(0); 
}