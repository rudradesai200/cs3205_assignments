#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define BUF_SIZE 2048
#define NUSERS 1000
#define USERIDLIMIT 100

// Data Structures
typedef struct Email
{
    struct User *from, *to;
    char /* *subject ,*/ *content, *dt;
    struct Email *last, *next;
} Email;

typedef struct User
{
    char userid[USERIDLIMIT];
    struct Email *current_email, *email_head;
    int nmsgs;
} User;

// Global Declarations
int current_users = 0;
int current_emails = 0;
int new_socket;
User *users[NUSERS], *current_user;

// Helper functions

/** Appends cnt number of strings and returns a final string.*/
char *str_append(int cnt, char *str, ...)
{
    char *retstr;
    retstr = calloc(BUF_SIZE, sizeof(char));
    va_list arg;
    va_start(arg, str);
    for (int i = 0; i < cnt; i++)
    {
        strcat(retstr, str);
        str = va_arg(arg, char *);
    }
    va_end(arg);
    return retstr;
}

/** Returns 'mail_root/users/uid.txt' */
char *get_filename(char *uid) { return str_append(3, "mail_root/users/", uid, ".txt"); }

/** Converts email to string in a specified format */
char *email_to_str(struct Email *email)
{
    char *data = calloc(BUFSIZ, sizeof(char));
    strcpy(data, str_append(3, "From: ", email->from->userid, "\n"));
    strcat(data, str_append(3, "To: ", email->to->userid, "\n"));
    strcat(data, str_append(3, "Date: ", email->dt, "\n"));
    strcat(data, str_append(3, "Contents:\n", email->content, "###\n"));
    return data;
}

/** Returns the useridx of the user stored in users array.
 * If not present returns -1.
 */
int isUserPresent(char *userid)
{
    for (int i = 0; i < current_users; i++)
        if (strcmp(users[i]->userid, userid) == 0)
            return i;
    return -1;
}

/** Reads an email from the passed fp  */
Email *read_email_from_file(FILE *fp)
{
    if (fp == NULL)
        return NULL;
    else
    {
        char *tstr = calloc(BUFSIZ, sizeof(char));
        char *content = calloc(BUFSIZ, sizeof(char));
        int read;
        size_t len = 0;
        Email *em = calloc(1, sizeof(Email));

        // Check the first line. Return NULL if not present
        if (fscanf(fp, "From: %s\n", tstr) != 1)
            return NULL;
        em->from = users[isUserPresent(tstr)];

        // Read recipient of the email
        fscanf(fp, "To: %s\n", tstr);
        em->to = users[isUserPresent(tstr)];

        // Read date of the email
        fgets(tstr, BUFSIZ * sizeof(char), fp);
        em->dt = calloc(BUFSIZ, sizeof(char));
        strcpy(em->dt, &tstr[6]);
        em->dt[strlen(em->dt) - 1] = '\0';

        // Ignore content: header
        fscanf(fp, "%s\n", tstr);

        // Read content lines
        while ((read = getline(&tstr, &len, fp)) != -1)
        {
            if (strcmp(tstr, "###\n") == 0)
                break;
            strcat(content, tstr);
        }
        em->content = calloc(BUFSIZ, sizeof(char));
        strcpy(em->content, content);

        return em;
    }
}

/** Fetch emails for the current_user from its spool file.
 * Uses the read_email_from_file function.
*/
void fetch_emails()
{
    FILE *fp = fopen(get_filename(current_user->userid), "r");
    Email *curr = NULL, *email = NULL;
    int i = 0;

    current_user->current_email = NULL;
    while ((email = read_email_from_file(fp)) != NULL)
    {
        // Join links for this email to the next one.
        if (curr != NULL)
            curr->next = email;
        email->last = curr;
        curr = email;
        if (i == 0)
            current_user->email_head = curr;
        i++;
    }

    // Join the list with the head
    if (curr != NULL)
    {
        (curr->next) = (current_user->email_head);
        (current_user->email_head->last) = curr;
    }

    // Set the current_email to the head
    current_user->current_email = current_user->email_head;
    fclose(fp);
}

/** Stores the whole user object in the fp */
ssize_t store_user(struct User *user, FILE *fp) { return fwrite(user, sizeof(struct User), 1, fp); }

/** Reads the whole user object from the fp */
ssize_t read_user(FILE *fp, struct User *inp) { return fread(inp, sizeof(struct User), 1, fp); }

/** Creates a user and add it in the users array */
int addUser(char *userid)
{
    if (isUserPresent(userid) != -1)
        return -1;

    users[current_users] = calloc(1, sizeof(struct User));
    strncpy(users[current_users]->userid, userid, USERIDLIMIT - 1);
    users[current_users]->nmsgs = 0;
    fclose(fopen(get_filename(userid), "w"));
    current_users++;
    return 0;
}

/** Returns a space separated list of userids */
char *list_users()
{
    char *userslist = calloc(BUF_SIZE, sizeof(char));
    for (int i = 0; i < current_users; i++)
    {
        strcat(userslist, users[i]->userid);
        strcat(userslist, " ");
    }
    return userslist;
}

/** initializes the users array from the mail_root/users.txt file */
void initialize()
{
    FILE *fp = fopen("mail_root/users.txt", "r");
    struct User inp;
    while (read_user(fp, &inp))
    {
        users[current_users] = calloc(1, sizeof(struct User));
        memcpy(users[current_users], &inp, sizeof(struct User));
        users[current_users]->current_email = NULL;
        users[current_users++]->email_head = NULL;
    }
    fclose(fp);
}

/** Writes all the emails back for the usr to it's own spool file */
void write_spool_back(User *usr)
{
    if (usr->email_head != NULL)
    {
        Email *curr = usr->email_head->next;
        fclose(fopen(get_filename(usr->userid), "w"));
        FILE *fp = fopen(get_filename(usr->userid), "w");
        char *str = email_to_str(usr->email_head);

        fwrite(str, 1, strlen(str), fp);
        while ((curr != usr->email_head))
        {
            str = email_to_str(curr);
            fwrite(str, 1, strlen(str), fp);
            curr = curr->next;
        }
        fclose(fp);
    }
    else
        fclose(fopen(get_filename(usr->userid), "w"));
}

/** Stores all the users and their spool files. */
void storeback()
{
    FILE *fp;
    for (int i = 0; i < current_users; i++)
    {
        write_spool_back(users[i]);
        fp = fopen("users.txt", "w+");
        store_user(users[i], fp);
        fclose(fp);
    }
    fclose(fp);
}

/** Receives command and writes it in the buffer. */
int get_command(char *buffer, int to_print)
{
    memset(buffer, 0, BUF_SIZE * sizeof(char));
    int valread = read(new_socket, buffer, BUF_SIZE);
    if (to_print)
    {
        printf("\033[0;33mReceived message :\033[0;0m %s\n", buffer);
        if (strcmp(buffer, "Quit") == 0)
        {
            printf("Connection ended\n");
            printf("===============================================\n");
            // storeback();
            current_user = NULL;
            return -1;
        }
    }
    return 0;
}

void delete_mail()
{
    if (current_user->current_email == current_user->current_email->next)
    {
        current_user->current_email = NULL;
        current_user->email_head = NULL;
    }
    else
    {
        Email *curr = current_user->current_email;
        curr->last->next = curr->next;
        curr->next->last = curr->last;
        if (current_user->current_email == current_user->email_head)
            current_user->email_head = current_user->email_head->next;
        current_user->current_email = current_user->current_email->next;
        free(curr);
    }
    current_user->nmsgs--;
}

/** SEND function */
Email *SEND(int toidx)
{
    Email *em = calloc(1, sizeof(Email));
    em->from = current_user;
    em->to = users[toidx];
    char *buffer = calloc(BUF_SIZE, sizeof(char));
    em->content = calloc(BUF_SIZE, sizeof(char));
    while (1)
    {
        get_command(buffer, -1);
        if (strcmp(&buffer[strlen(buffer) - 4], "###\n") == 0)
        {
            strncat(em->content, buffer, strlen(buffer) - 4);
            strcat(em->content, "\n");
            break;
        }
        else
        {
            strcat(em->content, buffer);
        }
    }
    // strcpy(em->dt, "Today");
    if (em->to->email_head != NULL)
    {
        em->last = em->to->email_head->last;
        em->next = em->to->email_head;
        em->to->email_head->last->next = em;
        em->to->email_head->last = em;
        em->to->email_head = em;
    }
    else
    {
        em->next = em;
        em->last = em;
        em->to->email_head = em;
    }
    if (em->to->current_email == NULL)
    {
        em->to->current_email = em->to->email_head;
    }
    em->to->nmsgs++;

    time_t curtime;
    time(&curtime);
    char *dstr = ctime(&curtime);
    dstr[strlen(dstr) - 1] = '\0';
    em->dt = calloc(BUF_SIZE, sizeof(char));
    strcpy(em->dt, dstr);
    return em;
}

void command_processor(char *str)
{
    // printf("command : %s\n", str);
    if (strcmp(str, "LSTU") == 0)
        strcpy(str, str_append(3, "OK\n", list_users(), "\n"));
    else if (strncmp(str, "ADDU", 4) == 0)
    {
        char *userid = &str[5];
        int err = addUser(userid);
        memset(str, 0, BUF_SIZE * sizeof(char));
        if (err == 0)
            strcpy(str, "OK\n");
        else
            strcpy(str, "Not OK : Userid already present\n");
    }
    else if (strncmp(str, "USER", 4) == 0)
    {
        char *userid = &str[5];
        int useridx = isUserPresent(userid);
        memset(str, 0, BUF_SIZE * sizeof(char));
        if (useridx == -1)
            strcpy(str, "Not OK : Userid doesn not exist\n");
        else
        {
            char tstr[10];
            current_user = users[useridx];
            sprintf(tstr, "%d", current_user->nmsgs);
            strcpy(str, str_append(4, "OK\n", "Total Messages : ", tstr, "\n"));
            fetch_emails();
        }
    }
    else if (strcmp(str, "DONEU") == 0)
    {
        if (current_user == NULL)
            strcpy(str, "Not OK : User not set\n");
        else
        {
            write_spool_back(current_user);
            current_user = NULL;
            strcpy(str, "OK\n");
        }
    }
    else if (strcmp(str, "READM") == 0)
    {
        if (current_user == NULL)
            strcpy(str, "Not OK : User not set\n");
        else if ((current_user->current_email == NULL))
            strcpy(str, "OK\nNo More Email\n");
        else
        {
            strcpy(str, str_append(2, "OK\n", email_to_str((current_user->current_email))));
            current_user->current_email = current_user->current_email->next;
        }
    }
    else if (strcmp(str, "DELM") == 0)
    {
        if (current_user == NULL)
            strcpy(str, "Not OK : User not set\n");
        else if ((current_user->current_email == NULL))
            strcpy(str, "OK\nNo More Email\n");
        else
        {
            delete_mail();
            strcpy(str, "OK\n");
        }
    }
    else if (strncmp(str, "SEND", 4) == 0)
    {
        char *to = calloc(BUF_SIZE, sizeof(char));
        strcpy(to, &str[5]);
        int toidx = isUserPresent(to);
        if (toidx == -1)
            strcpy(str, str_append(3, "Not OK : No userid = ", to, "\n"));
        else
        {
            strcpy(str, "OK\n");
            // printf("Sent message : %s", str);
            send(new_socket, str, strlen(str), 0);
            Email *em = SEND(toidx);
            strcpy(str, "OK\n");
            write_spool_back(em->to);
        }
    }
    else
    {
        strcpy(str, "Not OK : Illegal command received\n");
    }
}

void network_interface(int port)
{
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char *buffer;
    char *hello = "OK";
    buffer = calloc(BUF_SIZE, sizeof(char));

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                                 (socklen_t *)&addrlen)) < 0)
        {
            // storeback();
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("===============================================\n");
        char str[1024];
        inet_ntop(AF_INET, &(address.sin_addr), str, INET_ADDRSTRLEN);
        printf("\033[0;32mConnected with %s\033[0;0m\n", str);

        while (1)
        {
            int ret = get_command(buffer, 1);
            if (ret == -1)
                break;
            command_processor(buffer);
            // printf("Sent message : %s", buffer);
            send(new_socket, buffer, strlen(buffer), 0);
        }
    }
    return;
}

int main(int argc, char const *argv[])
{
    // Arg Parse
    if (argc != 2)
    {
        printf("Usage:\n./emailserver <port:int>\n");
    }
    else
    {
        int port;
        sscanf(argv[1], "%d", &port);
        initialize();
        network_interface(port);
    }
    return 0;
}
