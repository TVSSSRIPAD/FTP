#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <pthread.h>
#include <bits/stdc++.h>
#include <dirent.h>

using namespace std;

#define BACKLOG 10 // the no of connections queue will hold
#define MAXMSGSIZE 150
#define MAX_FILE_SIZE 500
#define PORT "5000" // the port users will be connecting to
#define MAX_USERS 100
#define BUFFERSIZE 8192

vector<pair<string, string>> users;
vector<bool> usersLogin;

vector<pthread_t> threadIDs(MAX_USERS);
int activeUsers = 0;
typedef struct threadInfo
{
    int sock_conn_fd;
    pthread_t tid;
} threadInfo;

int verify(char *username, char *password)
{

    for (int i = 0; i < users.size(); i++)
    {
        if (username == users[i].first)
        {
            if (password == users[i].second)
            {
                // cout << i + 1 << "oo\n";
                return i + 1;
            }
        }
    }
    return -1;
}

void sendFile(char *filename, int sockfd, bool isBinaryMode)
{
    int fd = open(filename, O_RDONLY);
    struct stat file_stat;
    int remain_data, sent_bytes = 0, len;
    char file_size[256];

    if (fd == -1)
    {
        fprintf(stderr, "Error opening file --> %s\n", strerror(errno));
        len = send(sockfd, strerror(errno), BUFFERSIZE, 0);
        // cout << len << "islen\n";
        return;
    }
    else if (fstat(fd, &file_stat) < 0)
    {
        fprintf(stderr, "Error fstat --> %s\n", strerror(errno));
        len = send(sockfd, strerror(errno), BUFFERSIZE, 0);
        // cout << len << "islen\n";
        return;
    }
    else
    {
        send(sockfd, "OK", 3, 0);
    }
    fprintf(stdout, "File Size: \n%d bytes\n", (int)file_stat.st_size);
    sprintf(file_size, "%d", (int)file_stat.st_size);

    /* Sending file size */
    len = send(sockfd, file_size, sizeof(file_size), 0);
    if (len < 0)
    {
        fprintf(stderr, "Error on sending greetings --> %s", strerror(errno));
    }

    remain_data = file_stat.st_size;
    int size = file_stat.st_size;

    char data[BUFFERSIZE] = {0};
    unsigned char bBuffer[BUFFERSIZE];

    FILE *fp = fopen(filename, "r");

    if (isBinaryMode)
    {
        fp = fopen(filename, "rb"); // r for read, b for binary
        //
        while (remain_data > 0 && (len = fread(bBuffer, 1, sizeof(data) > remain_data ? remain_data : sizeof(data), fp)))
        {
            if ((sent_bytes = send(sockfd, bBuffer, len, 0)) == -1)
            {
                perror("Error in sending File");
                // exit(1);
            }
            // cout << "len " << len << "\n";
            // cout << "sent_bytes " << sent_bytes << "\n";
            fprintf(stdout, "=========================================================\n");
            remain_data -= sent_bytes;
            fprintf(stdout, "Sent: %d bytes. Remaining data : %d bytes\n", sent_bytes, remain_data);
            fprintf(stdout, "Sending in progress. %.4f%% completed \n", (float)(size - remain_data) / size * 100);
            fprintf(stdout, "=========================================================\n\n");
            memset(data, 0, BUFFERSIZE);
        }
        return;
        close(fd);
    }
    else
    {
        while (remain_data > 0 && (len = fread(data, 1, sizeof(data) > remain_data ? remain_data : sizeof(data), fp)))
        {
            if ((sent_bytes = send(sockfd, data, len, 0)) == -1)
            {
                perror("Error in sending File");
                // exit(1);
            }

            fprintf(stdout, "=========================================================\n");
            remain_data -= sent_bytes;
            fprintf(stdout, "Sent: %d bytes. Remaining data : %d bytes\n", sent_bytes, remain_data);
            fprintf(stdout, "Sending in progress. %.4f%% completed \n", (float)(size - remain_data) / size * 100);
            fprintf(stdout, "=========================================================\n\n");
            memset(data, 0, BUFFERSIZE);
        }

        close(fd);
        return;
    }
}

void recvFile(char *fname, int sockfd, bool isBinaryMode)
{
    ssize_t len;
    struct sockaddr_in remote_addr;
    char buffer[BUFFERSIZE];
    int file_size;
    FILE *received_file;
    int remain_data = 0;

    int numbytes = recv(sockfd, buffer, BUFFERSIZE, 0);
    buffer[numbytes] = '\0';
    // cout << "num" << numbytes << "ll\n";
    if (buffer[0] != 'O' || buffer[1] != 'K')
    {
        cout << "Server - " << buffer << "?\n";
        return;
    }

    /* Receiving file size */
    recv(sockfd, buffer, BUFFERSIZE, 0);
    file_size = atoi(buffer);
    //fprintf(stdout, "\nFile size : %d\n", file_size);

    received_file = fopen(fname, "w");
    if (received_file == NULL)
    {
        fprintf(stderr, "Failed to open file  --> %s\n", strerror(errno));
        // exit(1);
    }

    remain_data = file_size;

    if (isBinaryMode)
    {
        unsigned char buffer[BUFFERSIZE];
        received_file = fopen(fname, "wb"); // w for write, b for binary

        while ((remain_data > 0) && ((len = recv(sockfd, buffer, BUFFERSIZE > remain_data ? remain_data : BUFFERSIZE, 0)) > 0))
        {
            // fprintf(received_file, "%s", buffer);
            // cout << len << "o\n";
            fwrite(buffer, 1, len, received_file);
            remain_data -= len;
            bzero(buffer, BUFFERSIZE);
            // fprintf(stdout, "Receive %d bytes and we hope :- %d bytes\n", len, remain_data);
            fprintf(stdout, "=========================================================\n");
            fprintf(stdout, "Receiving in progress. %d bytes received and %d bytes are left\n", (int)len, remain_data);
            fprintf(stdout, "%.4f%% completed \n", (float)(file_size - remain_data) / file_size * 100);
            fprintf(stdout, "=========================================================\n\n");
        }
    }
    else
    {
        while ((remain_data > 0) && ((len = recv(sockfd, buffer, BUFFERSIZE > remain_data ? remain_data : BUFFERSIZE, 0)) > 0))
        {
            // fprintf(received_file, "%s", buffer);
            fwrite(buffer, sizeof(char), len, received_file);
            remain_data -= len;
            bzero(buffer, BUFFERSIZE);
            // fprintf(stdout, "Receive %d bytes and we hope :- %d bytes\n", len, remain_data);
            fprintf(stdout, "=========================================================\n");
            fprintf(stdout, "Receiving in progress. %d bytes received and %d bytes are left\n", (int)len, remain_data);
            fprintf(stdout, "%.4f%% completed \n", (float)(file_size - remain_data) / file_size * 100);
            fprintf(stdout, "=========================================================\n\n");
        }
    }
    fclose(received_file);
}
void sendDirFiles(string directory, int sock_conn_fd)
{
    DIR *d;
    int numbytes;
    struct dirent *dir;
    d = opendir(&directory[0]);
    if (d)
    {
        string fnames;
        while ((dir = readdir(d)) != NULL)
        {
            string name = dir->d_name;
            if (!strcmp(dir->d_name, "."))
                continue;
            if (!strcmp(dir->d_name, ".."))
                continue;
            struct stat st_buf;
            int status = stat(&name[0], &st_buf);
            if (S_ISDIR(st_buf.st_mode))
            {
                // cout << name << "is a dir\n";
                name += "/";
            }
            fnames = fnames + name + "\n";
            // cout << name << "\n";
        }
        if (fnames.size() == 0)
        {
            fnames = "You have not uploaded any files to server till now\n";
        }
        if ((numbytes = send(sock_conn_fd, &fnames[0], fnames.size(), 0)) == -1)
        {
            perror("send");
            // exit(1);
        }
        closedir(d);
    }
}
// get IPv4  sockaddr  ;
void *getAddress(struct sockaddr *sa)
{
    return &(((struct sockaddr_in *)sa)->sin_addr);
}

void *threadFn(void *arg)
{
    threadInfo *myInfo = (threadInfo *)arg;
    int sock_conn_fd = myInfo->sock_conn_fd;
    pthread_t myID = threadIDs[myInfo->tid];
    int id;
    bool isBinaryMode = 0;

    char msg_send[MAXMSGSIZE], msg_recv[MAXMSGSIZE];
    int numbytes;
    int isUserSignedIn = 0;

    // Ask the client to enter UserName & password
    if ((numbytes = send(sock_conn_fd, "Welcome to FTP server. Enter username & password to continue..\n", 64, 0)) == -1)
    {
        perror("send");
        // exit(1);
    }
    while (!isUserSignedIn)
    {
        // receive the username
        if ((numbytes = recv(sock_conn_fd, msg_recv, MAXMSGSIZE, 0)) == -1)
        {
            perror("recv");
            // exit(1);
        }
        msg_recv[numbytes] = '\0';
        char *username = msg_recv;

        char ttt[1024];
        for (int i = 0; msg_recv[i] && i < 1024; i++)
        {
            ttt[i] = msg_recv[i];
        }
        // printf("%d bytes rec\n", numbytes);
        // receive the question
        if ((numbytes = recv(sock_conn_fd, msg_recv, MAXMSGSIZE, 0)) == -1)
        {
            perror("recv");
            // exit(1);
        }
        msg_recv[numbytes] = '\0';
        char *password = msg_recv;

        // cout << password << " p\n";
        // cout << ttt << " u\n";

        // printf("%d bytes rec\n", numbytes);
        id = verify(ttt, password);
        isUserSignedIn = (id == -1) ? 0 : 1;
        // printf("?%sis un l%sispass\n", username, password);
        if (!isUserSignedIn)
        {
            // printf("YY %d\n", isUserSignedIn);
            if ((numbytes = send(sock_conn_fd, "Incorrect name or password. Please enter correct credentials\n", 62, 0)) == -1)
            {
                perror("send");
                // exit(1);
            }
        }
        else if (usersLogin[id])
        {
            isUserSignedIn = 0;
            if ((numbytes = send(sock_conn_fd, "User already loggedin. Please logout before logging in back.\n", 62, 0)) == -1)
            {
                perror("send");
                // exit(1);
            }
        }
        else
        {
            if ((numbytes = send(sock_conn_fd, "LoggedIn successfully!                                       ", 62, 0)) == -1)
            {
                perror("send");
                // exit(1);
            }

            usersLogin[id] = 1;
            break;
        }
    }

    // cout << "SentY\n";

    while (1)
    {
        if ((numbytes = send(sock_conn_fd, "Enter 1 for GET\n 2 for PUT\n 3 for Binary MODE\n4 for ASCII Mode\n 5 to See Server FIles\n6 to See your Uploaded files onn Server\n7 to Exit\n", 137, 0)) == -1)
        {
            perror("send");
            // exit(1);
        }
        // cout << "Here\n";
        if ((numbytes = recv(sock_conn_fd, msg_recv, MAXMSGSIZE - 1, 0)) == -1)
        {
            perror("recv");
            // exit(1);
        }
        msg_recv[numbytes] = '\0';
        int op = atoi(msg_recv);
        // cout << "op " << op << "\n";

        if (op == 1 || op == 2)
        {

            cout << "Waiting for filename\n";
            if ((numbytes = recv(sock_conn_fd, msg_recv, MAXMSGSIZE - 1, 0)) == -1)
                perror("recv");
            msg_recv[numbytes] = '\0';

            cout << "Filename is " << msg_recv << "\n";

            if (op == 1)
            {
                string path(msg_recv, msg_recv + numbytes);
                string dirpath = "./client" + to_string(id);
                string fullpath = dirpath + "/" + path;
                sendFile(&fullpath[0], sock_conn_fd, isBinaryMode);
            }
            else
            {
                string path(msg_recv, msg_recv + numbytes);
                string dirpath = "./client" + to_string(id);
                string fullpath = dirpath + "/" + path;
                // if(result == -1 || !result)
                recvFile(&fullpath[0], sock_conn_fd, isBinaryMode);
            }
        }
        else if (op == 3)
        {
            isBinaryMode = 1;
            cout << "Binary Mode is turned on for id: " << id << "\n";
        }
        else if (op == 4)
        {
            isBinaryMode = 0;
            cout << "Binary Mode is turned off for id:" << id << "\n ";
        }
        else if (op == 5)
        {
            string cdir = ".";
            sendDirFiles(cdir, sock_conn_fd);
        }
        else if (op == 6)
        {
            string dirpath = "./client" + to_string(id);
            sendDirFiles(dirpath, sock_conn_fd);
        }
        else if (op == 7)
        {
            usersLogin[id] = 0;
            break;
        }
    }

    close(sock_conn_fd);
    for (auto i = threadIDs.begin(); i != threadIDs.end(); ++i)
    {
        if (*i == myID)
        {
            threadIDs.erase(i);
            break;
        }
    }
    activeUsers--;
    // exit(1);
}

bool addUser(string username, string password)
{
    struct stat st = {0};
    int id = users.size();
    for (auto x : users)
    {
        if (x.first == username)
        {
            cout << "User with given username already exits\n";
            return 0;
        }
    }

    users.push_back({username, password});
    usersLogin.push_back(0);
    string dirpath = "./client" + to_string(id + 1);
    if (stat(&dirpath[0], &st) == -1)
    {
        mkdir(&dirpath[0], 0700);
    }

    return 1;
}

void *addUserThread(void *)
{
    string uname, pass;
    while (1)
    {
        // cout << "Enter username:\n";
        cin >> uname;
        // cout << "Enter password:\n";
        cin >> pass;
        bool res = addUser(uname, pass);
        if (res)
            cout << "User added successfully\n";
        else
            cout << "Add User failed. Please try again\n";
    }
}
int main()
{
    // Name : T.V.S.S.SRIPAD
    // Roll Number : 18CS01008
    int sockfd, sock_conn_fd; // listen on sock_fd, new connection on sock_conn_fd
    struct addrinfo hints, *servinfo, *loopvar;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;

    int yes = 1, rv;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    //using IPv4
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (loopvar = servinfo; loopvar != NULL; loopvar = loopvar->ai_next)
    {
        if ((sockfd = socket(loopvar->ai_family, loopvar->ai_socktype, loopvar->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, loopvar->ai_addr, loopvar->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (loopvar == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    users.push_back({"sai", "sai"});
    users.push_back({"i", "i"});
    usersLogin.push_back(0);
    usersLogin.push_back(0);
    string p1 = "client1", p2 = "client2";
    struct stat st = {0};
    if (stat(&p1[0], &st) == -1)
    {
        mkdir(&p1[0], 0700);
    }
    if (stat(&p2[0], &st) == -1)
    {
        mkdir(&p2[0], 0700);
    }
    pthread_t userThreadID;
    pthread_create(&userThreadID, NULL, addUserThread, NULL);

    if (listen(sockfd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    cout << "Server: waiting for connection...\n";

    while (1)
    {
        sin_size = sizeof their_addr;

        // Accept connections
        sock_conn_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (sock_conn_fd == -1)
        {
            perror("accept");
        }

        inet_ntop(their_addr.ss_family, getAddress((struct sockaddr *)&their_addr), s, sizeof s);
        //Printing address of client
        cout << "Server: got connection from " << s << " \n";

        if (activeUsers == MAX_USERS)
        {
            threadIDs.resize(MAX_USERS + 1);
        }
        //fork a child process for communicating with each client
        threadInfo t;
        t.sock_conn_fd = sock_conn_fd;
        t.tid = activeUsers;
        pthread_create(&threadIDs[activeUsers], NULL, threadFn, (void *)&t);

        activeUsers++;
        // close(sock_conn_fd);
    }

    return 0;
}
