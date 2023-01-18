#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <mysql.h>

#define PORT 8888

int main() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char *token;
    char username[20];
    int operation, amount;

    // Connect to MySQL server
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "localhost", "username", "password", "database", 0, NULL, 0)) {
        printf("Error: %s\n", mysql_error(conn));
        return 1;
    }

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

    // Forcefully attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    while(1) {
        valread = read( new_socket , buffer, 1024);
        token = strtok(buffer, " ");
        strcpy(username,token + strlen(token) +1);
        if(strcmp(token,"DEPOSIT")==0) {
            token = strtok(NULL, " ");
            amount = atoi(token);
            sprintf(buffer, "Deposit of %d done successfully", amount);
            sprintf(buffer,"UPDATE users SET balance = balance + %d WHERE username = '%s'",amount,username);
            if (mysql_query(conn, buffer)) {
                sprintf(buffer, "Error: %s\n", mysql_error(conn));
                send(new_socket, buffer, strlen(buffer), 0);
            } else {
                sprintf(buffer, "Deposit of %d done successfully", amount);
                send(new_socket, buffer, strlen(buffer), 0);
            }
        } else if(strcmp(token,"WITHDRAW")==0) {
            token = strtok(NULL, " ");
            amount = atoi(token);
            sprintf(buffer,"SELECT balance FROM users WHERE username = '%s'",username);
            if (mysql_query(conn, buffer)) {
                sprintf(buffer, "Error: %s\n", mysql_error(conn));
                send(new_socket, buffer, strlen(buffer), 0);
            } else {
                res = mysql_use_result(conn);
                row = mysql_fetch_row(res);
                if (atoi(row[0]) < amount) {
                    sprintf(buffer, "Insufficient balance");
                } else {
                    sprintf(buffer,"UPDATE users SET balance = balance - %d WHERE username = '%s'",amount,username);
                    if (mysql_query(conn, buffer)) {
                        sprintf(buffer, "Error: %s\n", mysql_error(conn));
                    } else {
                        sprintf(buffer, "Withdraw of %d done successfully", amount);
                    }
                }
                mysql_free_result(res);
                send(new_socket, buffer, strlen(buffer), 0);
            }
        } else if(strcmp(token,"BALANCE")==0) {
            sprintf(buffer,"SELECT balance FROM users WHERE username = '%s'",username);
            if (mysql_query(conn, buffer)) {
                sprintf(buffer, "Error: %s\n", mysql_error(conn));
                send(new_socket, buffer, strlen(buffer), 0);
            } else {
                res = mysql_use_result(conn);
                row = mysql_fetch_row(res);
                sprintf(buffer, "Your balance is %s", row[0]);
                mysql_free_result(res);
                send(new_socket, buffer, strlen(buffer), 0);
            }
        }
    }
    mysql_close(conn);
    return 0;
}
