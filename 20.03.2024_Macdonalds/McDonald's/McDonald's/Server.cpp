#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
using namespace std;

#define MAX_CLIENTS 30
#define DEFAULT_BUFLEN 512
#define MONEY_CLIENT 50

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

SOCKET server_socket;

class ClientData {
public:
    SOCKET client_socket;
    int moneyClient;

    ClientData(SOCKET socket, int money) : client_socket(socket), moneyClient(money) {}
};

vector<ClientData> clients;

void OrderProcessingStore(SOCKET* client_sockets, int i, const char* client_message, ClientData& clientData) {
    int hamburger_price = 8;
    int sprite_price = 3;
    int potato_price = 6;

    int hamburger_time = 8;
    int sprite_time = 3;
    int potato_time = 6;

    int total_time = 0;
    int total_price = 0;
    int hamburger_count = 0;
    int sprite_count = 0;
    int potato_count = 0;

    int moneyClient = clientData.moneyClient;

    bool OK;

    if (moneyClient == MONEY_CLIENT) {
        OK = true;
    }
    else {
        OK = false;
    }

    if (OK) {
        string message = "Your money: " + to_string(moneyClient) + "\n";
        send(client_sockets[i], message.c_str(), message.size(), 0);
    }

    OK = false;

    istringstream iss(client_message);
    string word;
    while (iss >> word) {
        if (word == "hamburger") {
            hamburger_count++;
            total_time += hamburger_time;
            total_price += hamburger_price;
        }
        else if (word == "sprite") {
            sprite_count++;
            total_time += sprite_time;
            total_price += sprite_price;
        }
        else if (word == "potato") {
            potato_count++;
            total_time += potato_time;
            total_price += potato_price;
        }
    }

    if (hamburger_count != 0 || sprite_count != 0 || potato_count != 0) {
        if (moneyClient - total_price >= 0) {
            string response = "Your order:";
            if (hamburger_count > 0) {
                response += " Hamburger(s): " + to_string(hamburger_count);
            }
            if (sprite_count > 0) {
                response += " Sprite(s): " + to_string(sprite_count);
            }
            if (potato_count > 0) {
                response += " Potato(es): " + to_string(potato_count);
            }
            response += ".\nYour order will be ready in " + to_string(total_time) + " seconds.";

            send(client_sockets[i], response.c_str(), response.size(), 0);

            if (total_time > 0) {
                Sleep(total_time * 1000);
                response = "Your order is ready";
                send(client_sockets[i], response.c_str(), response.size(), 0);
            }
            clientData.moneyClient = moneyClient - total_price;
        }
        else {
            string response = "You don't have enough money\n";
            send(client_sockets[i], response.c_str(), response.size(), 0);
        }
    }
    else {
        string response = "You have not ordered anything\n";
        send(client_sockets[i], response.c_str(), response.size(), 0);
    }
    string message = "Your money: " + to_string(clientData.moneyClient) + "\n";
    send(client_sockets[i], message.c_str(), message.size(), 0);
}

void LowerCase(char* client_message) {
    int length = strlen(client_message);
    for (int i = 0; i < length; ++i) {
        client_message[i] = tolower(client_message[i]);
    }
}

int main() {
    cout << "Start server... DONE.\n";
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code: %d", WSAGetLastError());
        return 1;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket: %d", WSAGetLastError());
        return 2;
    }

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8888);

    if (bind(server_socket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed with error code: %d", WSAGetLastError());
        return 3;
    }

    listen(server_socket, MAX_CLIENTS);

    cout << "Server is waiting for incoming connections...\nPlease, start one or more client-side app.\n";

    fd_set readfds;
    SOCKET client_sockets[MAX_CLIENTS] = {};

    while (true) {
        FD_ZERO(&readfds);

        FD_SET(server_socket, &readfds);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET s = client_sockets[i];
            if (s > 0) {
                FD_SET(s, &readfds);
            }
        }

        if (select(0, &readfds, NULL, NULL, NULL) == SOCKET_ERROR) {
            printf("select function call failed with error code : %d", WSAGetLastError());
            return 4;
        }

        SOCKET new_socket;
        sockaddr_in address;
        int addrlen = sizeof(sockaddr_in);
        if (FD_ISSET(server_socket, &readfds)) {
            if ((new_socket = accept(server_socket, (sockaddr*)&address, &addrlen)) < 0) {
                perror("accept function error");
                return 5;
            }

            printf("New connection, socket fd is %d, ip is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    clients.push_back(ClientData(new_socket, MONEY_CLIENT));
                    printf("Adding to list of sockets at index %d\n", i);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET s = client_sockets[i];
            if (FD_ISSET(s, &readfds)) {
                sockaddr_in address;
                int addrlen = sizeof(sockaddr_in);

                char client_message[DEFAULT_BUFLEN];

                int client_message_length = recv(s, client_message, DEFAULT_BUFLEN, 0);
                client_message[client_message_length] = '\0';

                LowerCase(client_message);

                string check_exit = client_message;
                if (check_exit == "off") {
                    cout << "Client #" << i << " is off\n";
                    client_sockets[i] = 0;
                }
                else {
                    for (int j = 0; j < clients.size(); ++j) {
                        if (clients[j].client_socket == s) {
                            OrderProcessingStore(client_sockets, i, client_message, clients[j]);
                            break;
                        }
                    }
                }
            }
        }
    }

    WSACleanup();
}