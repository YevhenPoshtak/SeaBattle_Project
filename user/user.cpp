#include "user.hpp"
#include <stdio.h>
#include <cmath>
#include "../ui/config.hpp"
#include "../game/board_layout.hpp"
#include "../game/ship_config.hpp"
#include "../game/game_settings.hpp"
#include "../game/board_size_menu.hpp"
#include "../ui/animation.hpp"
#include "../game/game_logic_utils.hpp"
#include "../utils/text_utils.hpp"

extern GameSettings g_gameSettings;

std::queue<string> logQueue;

int aircraft_count = 5;
int battleship_count = 4;
int submarine_count = 3;
int destroyer_count = 3;
int patrol_count = 2;

void User::gameLoop(SOCKET client_socket) {
    clear();
    
    int size;
    int shots;
    
    if (isHost) {
        size = getBoardSize();
        shots = selectShotsPerTurn(size);
        
        send(client_socket, (const char*)&size, sizeof(int), 0);
        send(client_socket, (const char*)&shots, sizeof(int), 0);
        
        g_gameSettings.shotsPerTurn = shots;
        
        clear();
        mvprintw(2, 2, "Multiplayer Game (Host)");
        mvprintw(3, 2, "Board: %dx%d | Shots: %d per turn", size, size, shots);
        mvprintw(4, 2, "Waiting for client to setup board...");
        refresh();
    } else {
        clear();
        mvprintw(2, 2, "Multiplayer Game (Client)");
        mvprintw(3, 2, "Waiting for host settings...");
        refresh();
        
        if (recv(client_socket, (char*)&size, sizeof(int), MSG_WAITALL) <= 0) {
            attrset(A_NORMAL); 
            clear(); 
            mvprintw(5, 2, "Error: Connection lost!");
            mvprintw(6, 2, "Press any key to exit...");
            refresh();
            flushinp(); 
            getch();
            clear(); 
            closesocket(client_socket);
            return;
        }
        
        if (recv(client_socket, (char*)&shots, sizeof(int), MSG_WAITALL) <= 0) {
            attrset(A_NORMAL); 
            clear(); 
            mvprintw(5, 2, "Error: Connection lost!");
            mvprintw(6, 2, "Press any key to exit...");
            refresh();
            flushinp(); 
            getch();
            clear(); 
            closesocket(client_socket);
            return;
        }
        
        g_gameSettings.shotsPerTurn = shots;
        
        clear();
        mvprintw(2, 2, "Multiplayer Game (Client)");
        mvprintw(3, 2, "Board: %dx%d | Shots: %d per turn", size, size, shots);
        mvprintw(4, 2, "Host has chosen the settings!");
        refresh();
        
        #ifdef _WIN32
            Sleep(2000);
        #else
            usleep(2000000);
        #endif
    }
    
    setBoardSize(size);
    
    Gameboard myBoard;
    int shotsPerTurn = g_gameSettings.shotsPerTurn;
    int maxHits = getTotalShipCells(size);
    int totalShips = getTotalShips(size);

    int boardResult = 0;
    while (boardResult != 1) {
        boardResult = myBoard.generateRandomBoard(isHost);
        
        if (boardResult == 0) {
            boardResult = myBoard.generateManualBoard();
        }
    }

    BoardLayout layout = calculateBoardLayout(size);
    
    int boardWidth = size * 4 + 8;
    int maxY, maxX;
    getmaxyx(stdscr, maxY, maxX);
    
    clear();
    
    const char* yourBoardTitle;
    const char* oppBoardTitle;
    
    if (size >= 20) {
        yourBoardTitle = "You";
        oppBoardTitle = "Opp";
    } else if (size >= 15) {
        yourBoardTitle = "Your";
        oppBoardTitle = "Opponent";
    } else {
        yourBoardTitle = "Your Board";
        oppBoardTitle = "Opp. Board";
    }
    
    int yourBoardLen = strlen(yourBoardTitle);
    int oppBoardLen = strlen(oppBoardTitle);
    
    int leftPad1 = (boardWidth - yourBoardLen) / 2;
    int rightPad1 = boardWidth - leftPad1 - yourBoardLen;
    int leftPad2 = (boardWidth - oppBoardLen) / 2;
    int rightPad2 = boardWidth - leftPad2 - oppBoardLen;
    
    move(layout.startY, layout.board1StartX);
    for (int i = 0; i < leftPad1; i++) printw("-");
    printw("%s", yourBoardTitle);
    for (int i = 0; i < rightPad1; i++) printw("-");
    
    move(layout.startY, layout.separatorX);
    printw("~~~~~");
    
    move(layout.startY, layout.board2StartX);
    for (int i = 0; i < leftPad2; i++) printw("-");
    printw("%s", oppBoardTitle);
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
            if (myBoard.boardArray[i][j] != 'w') {
                attron(COLOR_PAIR(2));
                move(layout.startY + 3 + i, layout.board1StartX + 5 + (4 * j));
                addch(myBoard.boardArray[i][j]);
            }
        }
    }
    attron(COLOR_PAIR(1));
    
    cursor.y = layout.startY + 3;
    cursor.x = layout.board2StartX + 9;
    int maxCursorX = cursor.x + (size - 1) * 4;
    int maxCursorY = cursor.y + size - 1;
    
    move(cursor.y, cursor.x);
    refresh();
    
    int playerHits = 0;
    int enemyHits = 0;
    bool playerTurn = isHost;
    
    int myShipsRemaining = totalShips;
    int enemyShipsRemaining = totalShips;
    
    std::vector<std::vector<char>> enemyKnownBoard(size, std::vector<char>(size, ' '));
    
    int grid_x = 0, grid_y = 0;
    
    struct PendingShot {
        int x, y;
        bool used;
    };
    
    std::vector<PendingShot> playerShots;
    playerShots.resize(shotsPerTurn);
    for (int i = 0; i < shotsPerTurn; i++) {
        playerShots[i].used = false;
    }
    
    int shotsSelected = 0;
    bool selectingMode = true;
    
    int animFrame = 0;
    int animStartY = maxY - 6; 
    
    int playerStatsY = layout.startY + 3 + size + 2;  
    int enemyStatsY = layout.startY + 3 + size + 5;   
    
    while (playerHits < maxHits && enemyHits < maxHits) {
        move(0, maxX - 35);
        clrtoeol();
        attron(COLOR_PAIR(5) | A_BOLD);
        printw("YOUR SHIPS: %d", myShipsRemaining);
        attroff(A_BOLD);
        
        move(0, maxX - 15);
        attron(COLOR_PAIR(6) | A_BOLD);
        printw("ENEMY: %d", enemyShipsRemaining);
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
                        shotsPerTurn, shotsSelected, shotsPerTurn);
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
                        if (enemyKnownBoard[grid_y][grid_x] == ' ' && shotsSelected < shotsPerTurn) {
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
                        closesocket(client_socket);
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
                
                send(client_socket, (const char*)&shotsSelected, sizeof(int), 0);

                for (int i = 0; i < shotsSelected; i++) {
                    coordinates shot;
                    shot.x = playerShots[i].x;
                    shot.y = playerShots[i].y;
                    
                    send(client_socket, (const char*)&shot, sizeof(shot), 0);
                    char answer;
                    if (recv(client_socket, &answer, sizeof(char), MSG_WAITALL) <= 0) {
                        attrset(A_NORMAL); 
                        clear(); 
                        mvprintw(5, 2, "Error: Connection lost!");
                        mvprintw(6, 2, "Press any key to exit...");
                        refresh();
                        flushinp();
                        getch();
                        clear(); 
                        closesocket(client_socket);
                        return;
                    }
                    
                    int shot_y = layout.startY + 3 + shot.y;
                    int shot_x = layout.board2StartX + 9 + (4 * shot.x);
                    
                    move(shot_y, shot_x - 1);
                    addch(' ');
                    move(shot_y, shot_x + 1);
                    addch(' ');
                    
                    char shotInfo[16];
                    char resultChar = (answer == 'h') ? 'X' : (answer == 'm') ? 'O' : 'S';
                    sprintf(shotInfo, "%c%d-%c", 'A' + shot.x, shot.y + 1, resultChar);
                    
                    if (i > 0) strcat(playerStatsLine, ", ");
                    strcat(playerStatsLine, shotInfo);
                    
                    if (answer == 'h') {
                        attron(COLOR_PAIR(4));
                        move(shot_y, shot_x);
                        addch('X');
                        enemyKnownBoard[shot.y][shot.x] = 'h';
                        playerHits++;
                    } else if (answer == 'm') {
                        attron(COLOR_PAIR(3));
                        move(shot_y, shot_x);
                        addch('O');
                        enemyKnownBoard[shot.y][shot.x] = 'm';
                    } else {
                        attron(COLOR_PAIR(4) | A_BOLD);
                        move(shot_y, shot_x);
                        addch('S');
                        enemyKnownBoard[shot.y][shot.x] = 's';
                        playerHits++;
                        enemyShipsRemaining--;
                        
                        for (int row = 0; row < size; row++) {
                            for (int col = 0; col < size; col++) {
                                if (enemyKnownBoard[row][col] == 'h') {
                                    enemyKnownBoard[row][col] = 's';
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
                    closesocket(client_socket);
                    return;
                }
                
                shotsSelected = 0;
                selectingMode = true;
                playerTurn = false;
                
                for (int i = 0; i < size; i++) {
                    for (int j = 0; j < size; j++) {
                        if (enemyKnownBoard[i][j] == ' ') {
                            continue;
                        }
                        int cell_y = layout.startY + 3 + i;
                        int cell_x = layout.board2StartX + 9 + (4 * j);
                        
                        if (enemyKnownBoard[i][j] == 's') {
                            attron(COLOR_PAIR(4) | A_BOLD);
                            move(cell_y, cell_x);
                            addch('S');
                        } else if (enemyKnownBoard[i][j] == 'h') {
                            attron(COLOR_PAIR(4));
                            move(cell_y, cell_x);
                            addch('X');
                        } else if (enemyKnownBoard[i][j] == 'm') {
                            attron(COLOR_PAIR(3));
                            move(cell_y, cell_x);
                            addch('O');
                        }
                        attron(COLOR_PAIR(1));
                    }
                }
            }
        } else {
            move(1, 90);
            clrtoeol();
            attron(COLOR_PAIR(5));
            printw("         Enemy's turn...                           ");
            attron(COLOR_PAIR(1));
            refresh();
            
            #ifdef _WIN32
                Sleep(1000);
            #else
                usleep(1000000);
            #endif
            
            for (int i = 0; i < 3; i++) {
                move(enemyStatsY + i, layout.board1StartX);
                clrtoeol();
            }
            
            attron(A_UNDERLINE | COLOR_PAIR(4));
            mvprintw(enemyStatsY, layout.board1StartX, "Enemy volley:");
            attroff(A_UNDERLINE | COLOR_PAIR(4));
            attron(COLOR_PAIR(1));
            
            char enemyStatsLine[256] = "";
            
            int incomingShotsCount = 0;
            if (recv(client_socket, (char*)&incomingShotsCount, sizeof(int), MSG_WAITALL) <= 0) {
                attrset(A_NORMAL); 
                clear(); 
                mvprintw(5, 2, "Error: Connection lost!");
                mvprintw(6, 2, "Press any key to exit...");
                refresh();
                flushinp();
                getch();
                clear(); 
                closesocket(client_socket);
                return;
            }
            
            for (int shot = 0; shot < incomingShotsCount; shot++) {
            
                coordinates attack_coords;
                if (recv(client_socket, (char*)&attack_coords, sizeof(attack_coords), MSG_WAITALL) <= 0) {
                    attrset(A_NORMAL); 
                    clear(); 
                    mvprintw(5, 2, "Error: Connection lost!");
                    mvprintw(6, 2, "Press any key to exit...");
                    refresh();
                    flushinp();
                    getch();
                    clear(); 
                    closesocket(client_socket);
                    return;
                }
                
                char result = 'w';
                char boardPiece = myBoard.boardArray[attack_coords.y][attack_coords.x];
                
                if (boardPiece == 'w') {
                    result = 'm';
                    myBoard.boardArray[attack_coords.y][attack_coords.x] = 'o';
                } else if (boardPiece != 'x' && boardPiece != 'o') {
                    char ship = boardPiece;
                    myBoard.boardArray[attack_coords.y][attack_coords.x] = 'x';
                    myBoard.markShipAsHit(ship);
                    enemyHits++;
                    
                    if (myBoard.isShipSunk(ship)) {
                        result = toupper(ship);
                        myShipsRemaining--;
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
                
                int shot_y = layout.startY + 3 + attack_coords.y;
                int shot_x = layout.board1StartX + 5 + (4 * attack_coords.x);
                
                char shotInfo[16];
                char resultChar = (result == 'm') ? 'O' : (result == 'h') ? 'X' : 'S';
                sprintf(shotInfo, "%c%d-%c", 'A' + attack_coords.x, attack_coords.y + 1, resultChar);
                
                if (shot > 0) strcat(enemyStatsLine, ", ");
                strcat(enemyStatsLine, shotInfo);
                
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
                
                send(client_socket, &result, sizeof(char), 0);
                
                attron(COLOR_PAIR(1));
                mvprintw(enemyStatsY + 1, layout.board1StartX, "%s", enemyStatsLine);
                attroff(COLOR_PAIR(1));
                
                refresh();
                
                #ifdef _WIN32
                    Sleep(400);
                #else
                    usleep(400000);
                #endif
            }
            
            if (enemyHits >= maxHits) {
                drawFirework(false);        
                closesocket(client_socket);
                return;
            }
            
            playerTurn = true;
        }
    }
}

void User::displayBoard(Gameboard board) {
    int size = board.getBoardSize();
    BoardLayout layout = calculateBoardLayout(size);
    
    int boardWidth = size * 4 + 8;
    
    clear();
    
    const char *yourBoardTitle, *oppBoardTitle;
    if (size >= 20) {
        yourBoardTitle = "You";
        oppBoardTitle = "Opp";
    } else if (size >= 15) {
        yourBoardTitle = "Your";
        oppBoardTitle = "Opponent";
    } else {
        yourBoardTitle = "Your Board";
        oppBoardTitle = "Opp. Board";
    }
    
    int yourBoardLen = strlen(yourBoardTitle);
    int oppBoardLen = strlen(oppBoardTitle);
    
    int leftPad1 = (boardWidth - yourBoardLen) / 2;
    int rightPad1 = boardWidth - leftPad1 - yourBoardLen;
    int leftPad2 = (boardWidth - oppBoardLen) / 2;
    int rightPad2 = boardWidth - leftPad2 - oppBoardLen;
    
    move(layout.startY, layout.board1StartX);
    for (int i = 0; i < leftPad1; i++) printw("-");
    printw("%s", yourBoardTitle);
    for (int i = 0; i < rightPad1; i++) printw("-");
    
    move(layout.startY, layout.separatorX);
    printw("~~~~~");
    
    move(layout.startY, layout.board2StartX);
    for (int i = 0; i < leftPad2; i++) printw("-");
    printw("%s", oppBoardTitle);
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
    
    move(layout.startY + 2, layout.board2StartX);
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
        
        move(layout.startY + 3 + i, layout.board2StartX);
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
    mvprintw(5, 1, "F - fire all shots");
    mvprintw(6, 1, "q - quit game");
    
    attron(A_UNDERLINE);
    mvprintw(layout.instructionsY, layout.logStartX, "log");
    attroff(A_UNDERLINE);
    
    refresh();
    
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (board.boardArray[i][j] != 'w') {
                attron(COLOR_PAIR(2));
                move(layout.startY + 3 + i, layout.board1StartX + 4 + (4 * j));
                addch(' ');
                addch(board.boardArray[i][j]);
                addch(' ');
            } else {
                attron(COLOR_PAIR(3));
                move(layout.startY + 3 + i, layout.board1StartX + 4 + (4 * j));
                addch(' ');
                addch(' ');
                addch(' ');
            }
        }
    }
    attron(COLOR_PAIR(1));
    cursor.y = layout.startY + 3;
    cursor.x = layout.board2StartX + 5;
    move(cursor.y, cursor.x);
    refresh();
}

void User::printClientIP(struct sockaddr_in their_address) {
    char s[INET6_ADDRSTRLEN];
    inet_ntop(their_address.sin_family, &their_address.sin_addr, s, sizeof(s));
    cout << "Connection established with " << s << "\n";
}

void User::handleFullBoard(char_coordinates cc) {
    int size = getBoardSize();
    BoardLayout layout = calculateBoardLayout(size);
    
    int y_coor = layout.startY + 3 + cc.y;
    int x_coor = layout.board2StartX + 5 + (4 * cc.x);
    
    move(y_coor, x_coor);
    char c = inch() & A_CHARTEXT;
    if (c == 'm' || cc.c == 'w') {
        attron(COLOR_PAIR(3));
    } else {
        attron(COLOR_PAIR(2));
    }
    if (c == ' ') {
        if (cc.c == 'w') {
            addch(' ');
        } else {
            addch(cc.c);
        }
    }
    if (c == 'h') {
        attron(COLOR_PAIR(4));
        addch(cc.c);
        attron(COLOR_PAIR(2));
    }

    move(y_coor, x_coor - 1);
    addch(' ');
    move(y_coor, x_coor + 1);
    addch(' ');
    attron(COLOR_PAIR(1));
}

char User::handleAttack(coordinates attack_coords, SOCKET client_socket, Gameboard& board) {
    int size = getBoardSize();
    BoardLayout layout = calculateBoardLayout(size);

    string print_msg = "Enemy attack: " +
                       string(1, 'A' + attack_coords.x) +
                       to_string(attack_coords.y + 1) + " = ";
    char result;
    string result_str;
    string attackStatus = "";
    char boardPiece = board.boardArray[attack_coords.y][attack_coords.x];
    
    int y_pos = layout.startY + 3 + attack_coords.y;
    int x_pos = layout.board1StartX + 5 + (4 * attack_coords.x);
    
    if (boardPiece != 'w' && boardPiece != 'o' && boardPiece != 'x') {
        char originalShip = boardPiece;
        board.boardArray[attack_coords.y][attack_coords.x] = 'x';
        board.markShipAsHit(originalShip);
        
        if (board.isShipSunk(originalShip)) {
            result = toupper(originalShip);  
            attackStatus = "Your ship sunk!";
            result_str = "SUNK!";
        } else {
            result = 'h'; 
            result_str = "Hit";
        }
        
        attron(COLOR_PAIR(4));
        move(y_pos, x_pos - 1);
        addch(' ');
        move(y_pos, x_pos);
        addch('X');
        move(y_pos, x_pos + 1);
        addch(' ');
        attron(COLOR_PAIR(1));
        
        send(client_socket, &result, sizeof(char), 0);
    } else {
        result = 'm';
        result_str = "Miss";
        
        attron(COLOR_PAIR(3));
        move(y_pos, x_pos - 1);
        addch(' ');
        move(y_pos, x_pos);
        addch('O');
        move(y_pos, x_pos + 1);
        addch(' ');
        attron(COLOR_PAIR(1));
        
        send(client_socket, &result, sizeof(char), 0);
    }
    
    messageLog(print_msg + result_str);
    if (attackStatus != "") {
        messageLog(attackStatus);
    }
    return result;
}

void User::messageLog(string message) {
    int size = getBoardSize();
    BoardLayout layout = calculateBoardLayout(size);
    
    logQueue.push(message);
    
    while (logQueue.size() > 10) {
        logQueue.pop();
    }

    int logY = layout.instructionsY + 1;
    for (int i = 0; i < 10; i++) {
        move(logY + i, layout.logStartX);
        clrtoeol();
    }
    
    std::queue<string> tempQueue = logQueue;
    int lineNum = 0;
    while (!tempQueue.empty() && lineNum < 10) {
        attron(COLOR_PAIR(1));
        mvprintw(logY + lineNum, layout.logStartX, "%s", tempQueue.front().c_str());
        attroff(COLOR_PAIR(1));
        tempQueue.pop();
        lineNum++;
    }
    
    refresh();
}