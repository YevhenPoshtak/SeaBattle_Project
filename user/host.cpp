#include "user.hpp"

void Host::connect() {
    SOCKET host_socket;
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);
    refresh();

    host_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (host_socket == INVALID_SOCKET) {
        endwin();
        cout << "Error creating host socket. Error = " << GET_SOCKET_ERROR() << "\n";
        closesocket(host_socket); 
        exit(1);
    }

    int on = 1;
    if (setsockopt(host_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on)) < 0) { 
        endwin();
        cout << "Something went wrong when setting socket options... " << GET_SOCKET_ERROR() << "\n";
        exit(1);
    }

    if (bind(host_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        endwin();
        cout << "Error trying to bind and connect socket. Error = " << GET_SOCKET_ERROR() << "\n";
        closesocket(host_socket); 
        exit(1);
    }

    if (listen(host_socket, 5) < 0) {
        endwin();
        cout << "Error trying to listen. Error = " << GET_SOCKET_ERROR() << "\n";
        closesocket(host_socket); 
        exit(1);
    }

    printw("  Waiting for a client to connect...\n");
    refresh();

    while (1) {
        struct sockaddr_in client_address;
        socklen_t address_size = sizeof(client_address);
        if ((client_socket = accept(host_socket, (struct sockaddr *) &client_address, &address_size)) < 0) { 
            printw("Error trying to accept client connection.\n");
            refresh();
            continue;
        }

        if (acceptClientConnection()) {
            const char *msg = "Connection accepted by host\n";
            send(client_socket, msg, strlen(msg), 0);
            
            int y, x;
            getyx(stdscr, y, x);
            
            for (int i = 7; i <= y + 2; i++) {
                move(i, 0);
                clrtoeol();
            }
            
            move(0, 0);  
            refresh();
            
            this->gameLoop(client_socket);
            mvprintw(6, 0, "\n\n");
            closesocket(client_socket); 
            closesocket(host_socket); 
            return;
        }else {
            closesocket(client_socket); 
            printw("Connection was rejected, closing client socket\n");
            refresh();
        }
    }
}

bool Host::acceptClientConnection() {
    printw("\n  Would you like to accept the connection, Y/N?\n");
    refresh();

    while (1) {
        flushinp();
        char user_input;
        user_input = getch();
        if (user_input == 'Y' || user_input == 'y') {
            printw("  accepting connection! Have fun!\n");
            refresh();
            return true;
        } else if (user_input == 'N' || user_input == 'n') {
            printw("Connection refused... waiting for a new connection request\n");
            refresh();
            return false;
        }
    }
}