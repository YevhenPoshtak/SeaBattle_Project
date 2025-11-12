#include "user/user.hpp"
#include "ai/ai_player.hpp"
#include <signal.h>

using namespace std;

void setupWindow();
int connectToServer(char *type);
char startMenu();
void playAgainstAI(AIDifficulty difficulty);
unsigned long resolveName(const char *name);

SOCKET matchmaking_socket = INVALID_SOCKET;

void signal_handler(int sig) {
    endwin();
    if (matchmaking_socket != INVALID_SOCKET) {
        closesocket(matchmaking_socket);
    }
    
    #ifdef _WIN32
        WSACleanup();
    #endif
    
    exit(0);
}

int main(int argc, char **argv) {
    
    #ifdef _WIN32
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            printf("WSAStartup failed: %d\n", iResult);
            return 1;
        }
    #endif

    srand(time(NULL)); 

    setupWindow();
    
    char ret = 'e';
    while (ret != 'h' && ret != 'c' && ret != 'a' && ret != 's') {
        for (int i = 8; i < 30; i++) {
            mvprintw(i, 0, "\n");
        }
        refresh();
        
        ret = startMenu();
        
        if (ret == 'a') {
            playAgainstAI(EASY);
            ret = 'e'; 
            
        } else if (ret == 's') {
            playAgainstAI(SMART);
            ret = 'e'; 
            
        } else if (ret == 'h') {
            for (int i = 7; i < 30; i++) {
                mvprintw(i, 0, "\n");
            }
            refresh();
            mvprintw(8, 2, "Enter a host (display) name: ");
            char *str = (char *) malloc(100);
            echo();
            flushinp();
            getnstr(str, 100);
            noecho();
            
            if (strcmp(str, "") != 0) {
                char strToSend[106];
                strcpy(strToSend, "host ");
                strcat(strToSend, str);
                
                mvprintw(10, 2, "Connecting to matchmaking server...\n");
                refresh();
                
                int matchResult = connectToServer(strToSend);
                
                if (matchResult == 0) {
                    mvprintw(13, 2, "Matchmaking server unavailable.\n");
                    mvprintw(14, 2, "Continue without matchmaking? (y/n): ");
                    refresh();
                    
                    flushinp();
                    char choice = getch();
                    
                    if (choice != 'y' && choice != 'Y') {
                        free(str);
                        ret = 'e';
                        continue;
                    }
                    
                    for (int i = 7; i < 30; i++) {
                        mvprintw(i, 0, "\n");
                    }
                    refresh();
                }
            } else {
                free(str);
                ret = 'e';
                continue;
            }
            
            free(str);
            Host *h = new Host();
            h->connect();
            delete h;
            
        } else if (ret == 'c') {
            for (int i = 7; i < 30; i++) {
                mvprintw(i, 0, "\n");
            }
            refresh();
            char *str = (char *) malloc(100);
            echo();
            mvprintw(8, 2, "Enter host IP address: ");
            getnstr(str, 100);
            noecho();
            
            Client *c = new Client(str);
            c->connect();
            delete c;
            free(str);
        }
    }

    endwin();
    
    #ifdef _WIN32
        WSACleanup();
    #endif
    
    return 0;
}

void playAgainstAI(AIDifficulty difficulty) {
    clear();
    
    const char* diffName = (difficulty == EASY) ? "Easy" : "Smart";
    mvprintw(2, 2, "Playing against %s AI", diffName);
    refresh();
    
    AIPlayer ai(difficulty);
    
    Gameboard playerBoard;
    
    move(23, 1);
    printw("Would you like to create your board manually? Enter: y/n\n");
    refresh();
    
    int manualBoard = -1;
    int randomBoard = -1;
    
    while ((manualBoard == -1) && (randomBoard == -1)) {
        flushinp();
        switch (getch()) {
            case 'y':
                manualBoard = playerBoard.generateManualBoard();
                break;
            case 'n':
                randomBoard = playerBoard.generateRandomBoard(true);
                break;
            default:
                move(24, 1);
                printw("Invalid Input please enter y/n for manual board\n");
                refresh();
                break;
        }
    }
    
    bool settingBoard = true;
    while (settingBoard) {
        if (manualBoard == 0) {
            for (int i = 8; i < 30; i++) {
                mvprintw(i, 0, "\n");
            }
            manualBoard = -1;
            randomBoard = playerBoard.generateRandomBoard(true);
        } else if (randomBoard == 0) {
            randomBoard = -1;
            manualBoard = playerBoard.generateManualBoard();
        } else {
            settingBoard = false;
        }
    }
    
    clear();
    move(6, 0);
    printw(
        "   --------------------Your Board--------------~~~~~----------------AI Board (%s)-----------\n"
        "   ____________________________________________~~~~~____________________________________________\n"
        "   |  | A | B | C | D | E | F | G | H | I | J |~~~~~|  | A | B | C | D | E | F | G | H | I | J |\n"
        "   | 1|   |   |   |   |   |   |   |   |   |   |~~~~~| 1|   |   |   |   |   |   |   |   |   |   |\n"
        "   | 2|   |   |   |   |   |   |   |   |   |   |~~~~~| 2|   |   |   |   |   |   |   |   |   |   |\n"
        "   | 3|   |   |   |   |   |   |   |   |   |   |~~~~~| 3|   |   |   |   |   |   |   |   |   |   |\n"
        "   | 4|   |   |   |   |   |   |   |   |   |   |~~~~~| 4|   |   |   |   |   |   |   |   |   |   |\n"
        "   | 5|   |   |   |   |   |   |   |   |   |   |~~~~~| 5|   |   |   |   |   |   |   |   |   |   |\n"
        "   | 6|   |   |   |   |   |   |   |   |   |   |~~~~~| 6|   |   |   |   |   |   |   |   |   |   |\n"
        "   | 7|   |   |   |   |   |   |   |   |   |   |~~~~~| 7|   |   |   |   |   |   |   |   |   |   |\n"
        "   | 8|   |   |   |   |   |   |   |   |   |   |~~~~~| 8|   |   |   |   |   |   |   |   |   |   |\n"
        "   | 9|   |   |   |   |   |   |   |   |   |   |~~~~~| 9|   |   |   |   |   |   |   |   |   |   |\n"
        "   |10|   |   |   |   |   |   |   |   |   |   |~~~~~|10|   |   |   |   |   |   |   |   |   |   |\n"
        "   --------------------------------------------~~~~~--------------------------------------------\n",
        diffName);
    
    attron(A_UNDERLINE);
    move(21, 1);
    printw("instructions");
    attroff(A_UNDERLINE);
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (playerBoard.boardArray[i][j] != 'w') {
                attron(COLOR_PAIR(2));
                move(3 + i + 6, 8 + (4 * j));
                addch(playerBoard.boardArray[i][j]);
            }
        }
    }
    attron(COLOR_PAIR(1));
    
    int cursor_y = 9, cursor_x = 57;
    move(cursor_y, cursor_x);
    refresh();
    
    int playerHits = 0;
    int aiHits = 0;
    bool playerTurn = true;
    
    char aiKnownBoard[BOARD_SIZE][BOARD_SIZE]; 
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            aiKnownBoard[i][j] = ' ';
        }
    }
    
    int grid_x = 0, grid_y = 0;
    
    while (playerHits < 17 && aiHits < 17) {
        if (playerTurn) {
            move(20, 52);
            attron(COLOR_PAIR(6));
            printw("                Your turn!                     ");
            attron(COLOR_PAIR(1));
            move(cursor_y, cursor_x);
            refresh();
            
            flushinp();
            int key = getch();
            
            switch (key) {
                case KEY_LEFT:
                case 'a':
                    if (cursor_x > 57 && grid_x > 0) {
                        cursor_x -= 4;
                        grid_x--;
                    }
                    break;
                case KEY_RIGHT:
                case 'd':
                    if (cursor_x < 91 && grid_x < 9) {
                        cursor_x += 4;
                        grid_x++;
                    }
                    break;
                case KEY_UP:
                case 'w':
                    if (cursor_y > 9 && grid_y > 0) {
                        cursor_y -= 1;
                        grid_y--;
                    }
                    break;
                case KEY_DOWN:
                case 's':
                    if (cursor_y < 18 && grid_y < 9) {
                        cursor_y += 1;
                        grid_y++;
                    }
                    break;
                case 'q':
                    return;
                case ' ':
                case 10:
                    if (aiKnownBoard[grid_y][grid_x] != ' ') {
                        move(20, 52);
                        attron(COLOR_PAIR(5));
                        printw("     Already attacked there! Try again        ");
                        attron(COLOR_PAIR(1));
                        refresh();
                        break;
                    }
                    
                    AICoordinates attack;
                    attack.x = grid_x;
                    attack.y = grid_y;
                    char result = ai.receiveAttack(attack);
                    
                    if (result == 'h' || result == 'a' || result == 'b' || 
                        result == 's' || result == 'd' || result == 'p') {
                        attron(COLOR_PAIR(4));
                        move(cursor_y, cursor_x);
                        addch('X');
                        aiKnownBoard[grid_y][grid_x] = 'h';
                        playerHits++;
                        
                        if (result != 'h') {
                            move(22, 52);
                            printw("AI ship sunk!");
                        }
                    } else {
                        attron(COLOR_PAIR(3));
                        move(cursor_y, cursor_x);
                        addch('O');
                        aiKnownBoard[grid_y][grid_x] = 'm';
                    }
                    attron(COLOR_PAIR(1));
                    refresh();
                    
                    playerTurn = false;
                    break;
            }
            
            move(cursor_y, cursor_x);
            refresh();
            
        } else {
            move(20, 52);
            attron(COLOR_PAIR(5));
            printw("                AI's turn...                   ");
            attron(COLOR_PAIR(1));
            refresh();
        
            #ifdef _WIN32
                Sleep(1000);
            #else
                usleep(1000000);
            #endif
            
            AICoordinates aiMove = ai.makeMove();
            char result = 'w';
            
            if (playerBoard.boardArray[aiMove.y][aiMove.x] == 'w') {
                result = 'm';
                playerBoard.boardArray[aiMove.y][aiMove.x] = 'o';
                attron(COLOR_PAIR(3));
            } else if (playerBoard.boardArray[aiMove.y][aiMove.x] != 'x' && 
                       playerBoard.boardArray[aiMove.y][aiMove.x] != 'o') {
                char ship = playerBoard.boardArray[aiMove.y][aiMove.x];
                result = 'h';
                playerBoard.boardArray[aiMove.y][aiMove.x] = 'x';
                aiHits++;
                attron(COLOR_PAIR(4));
                
                bool sunk = true;
                for (int i = 0; i < BOARD_SIZE; i++) {
                    for (int j = 0; j < BOARD_SIZE; j++) {
                        if (playerBoard.boardArray[i][j] == ship) {
                            sunk = false;
                            break;
                        }
                    }
                    if (!sunk) break;
                }
                
                if (sunk) {
                    result = ship;
                    move(22, 2);
                    printw("Your ship sunk!");
                }
            }
            
            move(3 + aiMove.y + 6, 8 + (4 * aiMove.x));
            addch('X');
            attron(COLOR_PAIR(1));
            
            ai.receiveAttackResult(aiMove, result);
            
            move(21, 2);
            printw("AI attacked: %c%d", 'A' + aiMove.x, aiMove.y + 1);
            refresh();
            
            playerTurn = true;
        }
    }
    
    clear();
    if (playerHits >= 17) {
        attron(COLOR_PAIR(6));
        mvprintw(10, 30, "YOU WON!");
    } else {
        attron(COLOR_PAIR(4));
        mvprintw(10, 30, "AI WON!");
    }
    attron(COLOR_PAIR(1));
    mvprintw(12, 20, "Press any key to return to menu...");
    refresh();
    getch();
}

int connectToServer(char *type) {
    struct sockaddr_in server_address;
    unsigned short serverPort = 9999;

    matchmaking_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (matchmaking_socket == INVALID_SOCKET) {
        mvprintw(7, 2, "Error creating socket\n");
        refresh();
        return 0;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    const char *serverName = "130.240.154.146";
    server_address.sin_addr.s_addr = resolveName(serverName);
    server_address.sin_port = htons(serverPort);

    #ifdef _WIN32
    DWORD timeout = 3000;
    setsockopt(matchmaking_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(matchmaking_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    #else
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    setsockopt(matchmaking_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(matchmaking_socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    #endif

    if (::connect(matchmaking_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        int error = GET_SOCKET_ERROR();
        #ifdef _WIN32
        if (error == WSAECONNREFUSED || error == WSAETIMEDOUT) {
        #else
        if (error == ECONNREFUSED || error == ETIMEDOUT) {
        #endif
            mvprintw(11, 2, "Cannot connect to matchmaking server (offline/timeout)\n");
        } else {
            mvprintw(11, 2, "Connection error. Code: %d\n", error);
        }
        refresh();
        closesocket(matchmaking_socket);
        matchmaking_socket = INVALID_SOCKET;
        return 0;
    }

    send(matchmaking_socket, type, strlen(type), 0);
    if (*type == 'h') {
        mvprintw(12, 2, "Waiting for client via matchmaking...\n");
        char *received_msg = (char *) malloc(1024);
        int len = recv(matchmaking_socket, received_msg, 1024, 0);
        if (len > 0) {
            mvprintw(13, 2, received_msg);
            refresh();
        }
        free(received_msg);
        closesocket(matchmaking_socket);
        matchmaking_socket = INVALID_SOCKET;
    } else {
        mvprintw(12, 2, "Searching for hosts...\n");
        refresh();
        char *received_msg = (char *) malloc(1024);
        int len = recv(matchmaking_socket, received_msg, 1024, 0);
        if (len > 0) {
            mvprintw(13, 2, received_msg);
            refresh();
        }
        free(received_msg);
        closesocket(matchmaking_socket);
        matchmaking_socket = INVALID_SOCKET;
    }

    return 1;
}

unsigned long resolveName(const char *name) {
    struct hostent *host;
    if ((host = gethostbyname(name)) == NULL) {
        return 0;
    }
    return *((unsigned long *) host->h_addr_list[0]);
}

void setupWindow() {
    WINDOW *win = initscr();
    if (win == NULL) {
        printf("ERROR: initscr() failed!\n");
        exit(1);
    }
    
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    
    signal(SIGINT, signal_handler);
    attron(COLOR_PAIR(1));
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    printw("  ___            ___          _    _    _        \n"
       " / __) ___  __ _| _ ) __ _ __| |_ | |_ | | ___   \n"
       " \\__ \\/ -_)/ _` | _ \\/ _` (_-<  _||  _|| |/ -_)  \n"
       " |___/\\___| \\__,_|___/\\__,_/__/\\__| \\__||_|\\___|  \n"
       "\n"
           "  -------------------------------------------------- \n"
           " |         Choose your game mode below              |\n"
           "  -------------------------------------------------- \n"
           "\n\n");
    refresh();
}

char startMenu() {
    string options[5] = {"1) vs Easy AI", "2) vs Smart AI", "3) Host (Multiplayer)", "4) Client (Multiplayer)", "5) Quit"};
    char ret = 'e';
    int i = 0, prev = 0;
    curs_set(0);
    
    for (int j = 0; j < 5; j++) {
        if (j == 0) {
            attron(A_STANDOUT);
        }
        mvprintw(10 + j, 4, options[j].c_str());
        if (j == 0) {
            attroff(A_STANDOUT);
        }
    }
    refresh();
    
    bool isActiveMenu = true;
    while (isActiveMenu) {
        prev = i;
        flushinp();
        int c = getch();
        
        switch (c) {
            case KEY_UP:
                i--;
                i = (i < 0) ? 4 : i;
                break;
            case KEY_DOWN:
                i++;
                i = (i > 4) ? 0 : i;
                break;
            case 10:
                switch (i) {
                    case 0:
                        ret = 'a'; 
                        isActiveMenu = false;
                        break;
                    case 1:
                        ret = 's'; 
                        isActiveMenu = false;
                        break;
                    case 2:
                        ret = 'h'; 
                        isActiveMenu = false;
                        break;
                    case 3:
                        ret = 'c'; 
                        isActiveMenu = false;
                        break;
                    case 4:
                        endwin();
                        #ifdef _WIN32
                            WSACleanup();
                        #endif
                        exit(0);
                }
        }
        
        attroff(A_STANDOUT);
        mvprintw(10 + prev, 4, options[prev].c_str());
        attron(A_STANDOUT);
        mvprintw(10 + i, 4, options[i].c_str());
        attroff(A_STANDOUT);
    }
    curs_set(2);
    return ret;
}