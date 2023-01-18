#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 6969
#define SERVER_IP "127.0.0.1"
#define MAX_USERNAME_LENGTH 20
#define MAX_PASSWORD_LENGTH 20

int main() {
    int sockfd, authenticated = 0;
    struct sockaddr_in server_address;
    char buffer[1024] = {0}, username[MAX_USERNAME_LENGTH], password[MAX_PASSWORD_LENGTH];
    int operation, amount;
    MYSQL *conn;
    MYSQL_RES *res;
    MYSQL_ROW row;

    // Connect to MySQL server
    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, "localhost", "username", "password", "database", 0, NULL, 0)) {
        printf("Error: %s\n", mysql_error(conn));
        return 1;
    }

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    // Set server address
    memset(&server_address, '0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    if(inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    while(1) {
        if (!authenticated) {
            // Prompt for username and password
            printf("Enter username: ");
            scanf("%s", username);
            printf("Enter password: ");
            scanf("%s", password);

            // Check credentials against MySQL database
            sprintf(buffer, "SELECT * FROM users WHERE username='%s' AND password='%s'", username, password);
            if (mysql_query(conn, buffer)) {
                printf("Error: %s\n", mysql_error(conn));
                continue;
            }
            res = mysql_use_result(conn);
            if (mysql_num_rows(res) > 0) {
                printf("Authentication successful!\n");
                authenticated = 1;
            } else {
                printf("Invalid username or password!\n");
                continue;
            }
            mysql_free_result(res);
            } else {
                printf("\nSelect operation:\n1. Deposit\n2. Withdraw\n3. Check Balance\n4. Exit\n");
                scanf("%d", &operation);

                switch(operation) {
                    case 1: // Deposit
                        printf("Enter amount to deposit: ");
                        scanf("%d", &amount);
                        sprintf(buffer, "DEPOSIT %d %s", amount, username);
                        send(sockfd, buffer, strlen(buffer), 0);
                        recv(sockfd, buffer, sizeof(buffer), 0);
                        printf("Server: %s\n", buffer);
                        break;
                    case 2: // Withdraw
                        printf("Enter amount to withdraw: ");
                        scanf("%d", &amount);
                        sprintf(buffer, "WITHDRAW %d %s", amount, username);
                        send(sockfd, buffer, strlen(buffer), 0);
                        recv(sockfd, buffer, sizeof(buffer), 0);
                        printf("Server: %s\n", buffer);
                        break;
                    case 3: // Check balance
                        sprintf(buffer, "BALANCE %s", username);
                        send(sockfd, buffer, strlen(buffer), 0);
                        recv(sockfd, buffer, sizeof(buffer), 0);
                        printf("Server: %s\n", buffer);
                        break;
                    case 4: // Exit
                        close(sockfd);
                        mysql_close(conn);
                        exit(0);
                        break;
                    default:
                        printf("Invalid operation!\n");
                        break;
                }
            }
        }
    return 0;
}
