#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

char *userid;

// IT needs that the server replies with no message.
void send_message(char *content)
{
    printf("Type Message: ");
    int read;
    char token[1048] = {0};
    while (fgets(token, sizeof(token), stdin) != NULL)
    {
        strcat(content, token);
        if (strcmp(token + strlen(token) - 4, "###\n") == 0)
            break;
    }
}
void user_input_interface(char *command)
{
    printf("Sub-Prompt-%s> ", userid);
    scanf("%s", command);
    if (strcmp(command, "Read") == 0)
    {
        strcpy(command, "READM");
    }
    else if (strcmp(command, "Delete") == 0)
    {
        strcpy(command, "DELM");
    }
    else if (strcmp(command, "Send") == 0)
    {
        strcpy(command, "SEND ");
        char token[1048] = {0};
        scanf("%s", token);
        strcat(command, token);
    }
    else if (strcmp(command, "Done") == 0)
    {
        strcpy(command, "DONEU");
    }
    else
    {
        printf("Incorrect command received.\nAvailable options - Read, Delete, Send <receiverid>, Done\n");
        return user_input_interface(command);
    }
}
void general_input_interface(char *command)
{
    printf("Main-Prompt> ");
    scanf("%s", command);
    if (strcmp(command, "Quit") == 0)
        strcpy(command, "Quit");
    else if (strcmp(command, "Listusers") == 0)
        strcpy(command, "LSTU");
    else if (strcmp(command, "Adduser") == 0)
    {
        strcpy(command, "ADDU ");
        char token[1048] = {0};
        scanf("%s", token);
        strcat(command, token);
    }
    else if (strcmp(command, "SetUser") == 0)
    {
        strcpy(command, "USER ");
        char token[1048] = {0};
        scanf("%s", token);
        strcat(command, token);
    }
    else
    {
        printf("Incorrect command received.\nAvailable options - Listusers, Adduser <userid>, SetUser <userid>, Quit\n");
        return general_input_interface(command);
    }
}

int network_interface(char *hostname, int port)
{
    char *command;
    printf("Trying server %s on port %d\n", hostname, port);
    int sock = 0, valread, userflag = 0;
    struct sockaddr_in serv_addr;

    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, hostname, &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    command = malloc(1024 * sizeof(char));

    while (1)
    {
        // Take input
        memset(command, 0, strlen(command));
        if (userflag == 0)
            general_input_interface(command);
        else if (userflag == 1)
            user_input_interface(command);
        else
        {
            send_message(command);
            userflag -= 2;
        }

        // Send the command to the server
        int lcom = send(sock, command, strlen(command), 0);

        // Check for Quit
        if (strcmp(command, "Quit") == 0)
            break;

        // Get the response
        memset(buffer, 0, strlen(buffer));
        if ((strncmp(command, "SEND", 4) != 0))
            valread = read(sock, buffer, 1024);

        // printf("%s\n%s", command, buffer);
        // Take actions according to the response
        if ((strncmp(command, "USER", 4) == 0) && (strncmp(buffer, "OK", 2) == 0))
        {
            userflag = 1;
            userid = malloc(sizeof(char));
            strcpy(userid, &command[5]);
        }
        if ((strncmp(command, "DONEU", 5) == 0) && (strncmp(buffer, "OK", 2) == 0))
        {
            userflag = 0;
            userid = NULL;
        }
        if ((strncmp(command, "SEND", 4) == 0))
        {
            userflag += 2;
        }
        else
            printf("%s", buffer);
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage:\n./emailclient <host | ip :str> <port:int>\n");
    }
    else
    {
        int port;
        char *IPbuffer;
        struct hostent *host_entry;
        char *hostname;
        host_entry = gethostbyname(argv[1]);
        hostname = inet_ntoa(*((struct in_addr *)
                                   host_entry->h_addr_list[0]));

        userid = malloc(sizeof(char));
        sscanf(argv[2], "%d", &port);
        return network_interface(hostname, port);
    }
    return 0;
}
