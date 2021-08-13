#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <bits/stdc++.h>
using namespace std;

#define PORT "5000" // the port client will be connecting to
#define BUFFERSIZE 8192
#define MAXMSGSIZE 150 // max number of bytes we can get at once

void sendFile(char *filename, int sockfd, bool isBinaryMode)
{
    int fd = open(filename, O_RDONLY);
    struct stat file_stat;
    int remain_data, sent_bytes = 0, len;
    char file_size[256];

    struct timeval start, finish, optime;
    gettimeofday(&start, NULL);
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
                exit(1);
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
        gettimeofday(&finish, NULL);

        timersub(&start, &finish, &optime);

        cout << "Time taken for Send is  " << optime.tv_sec << "seconds " << optime.tv_usec << "microseconds\n";
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
                exit(1);
            }

            fprintf(stdout, "=========================================================\n");
            remain_data -= sent_bytes;
            fprintf(stdout, "Sent: %d bytes. Remaining data : %d bytes\n", sent_bytes, remain_data);
            fprintf(stdout, "Sending in progress. %.4f%% completed \n", (float)(size - remain_data) / size * 100);
            fprintf(stdout, "=========================================================\n\n");
            memset(data, 0, BUFFERSIZE);
        }
        gettimeofday(&finish, NULL);
        timersub(&finish, &start, &optime);

        cout << "Time taken for Send is  " << optime.tv_sec << "seconds " << optime.tv_usec << "microseconds\n";
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
    struct timeval start, finish, optime;
    gettimeofday(&start, NULL);
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
        exit(1);
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

    gettimeofday(&finish, NULL);
    timersub(&finish, &start, &optime);

    cout << "Time taken for Receive is  " << optime.tv_sec << "seconds " << optime.tv_usec << "microseconds\n";
    fclose(received_file);
}

// get IPv4  sockadd
void *getAddress(struct sockaddr *sa)
{
    return &(((struct sockaddr_in *)sa)->sin_addr);
}

int main(int argc, char *argv[])
{
    // Name : T.V.S.S.SRIPAD
    // Roll Number : 18CS01008
    int sockfd;
    struct addrinfo hints, *servinfo, *loopvar;
    int rv;
    char s[INET6_ADDRSTRLEN];
    bool isBinaryMode = 0;
    memset(&hints, 0, sizeof hints);
    //using IPv4
    hints.ai_family = AF_INET;

    hints.ai_socktype = SOCK_STREAM;

    //getAddressInfo
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for (loopvar = servinfo; loopvar != NULL; loopvar = loopvar->ai_next)
    {
        if ((sockfd = socket(loopvar->ai_family, loopvar->ai_socktype, loopvar->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, loopvar->ai_addr, loopvar->ai_addrlen) == -1)
        {
            perror("client: connect");
            close(sockfd);
            continue;
        }

        break;
    }

    if (loopvar == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(loopvar->ai_family, getAddress((struct sockaddr *)loopvar->ai_addr), s, sizeof s);
    cout << "Client: connecting to " << s << "\n";

    int bytes, numbytes;
    char msg_send[MAXMSGSIZE], msg_recv[MAXMSGSIZE], msg_recv1[MAXMSGSIZE];
    int isUserSignedIn = 0;

    if ((numbytes = recv(sockfd, msg_recv, MAXMSGSIZE - 1, 0)) == -1)
        perror("recv");
    msg_recv[numbytes] = '\0';

    cout << "Server - " << msg_recv << " \n";

    while (!isUserSignedIn)
    {
        cout << "Enter username \n";
        cin >> msg_send;
        // send UserName to server
        if ((numbytes = send(sockfd, msg_send, sizeof(msg_send), 0)) == -1)
        {
            perror("send");
            exit(1);
        }
        // printf("Enter username is %s\n", msg_send);
        cout << "Enter password \n";

        cin >> msg_send;

        // Ask client for password
        if ((numbytes = send(sockfd, msg_send, sizeof(msg_send), 0)) == -1)
        {
            perror("send");
            exit(1);
        }

        if ((numbytes = recv(sockfd, msg_recv, 62, 0)) == -1)
            perror("recv");
        msg_recv[numbytes] = '\0';
        cout << "Server - " << msg_recv << " \n";
        string msg(msg_recv, msg_recv + numbytes);

        if (strcmp(&(msg.substr(0, 22))[0], "LoggedIn successfully!") == 0)
        {
            isUserSignedIn = 1;
            break;
        }
        // printf("Stilll %d", isUserSignedIn);
    }
    while (1)
    {
        // send the question
        // cout << " I am waiting..\n";
        if ((numbytes = recv(sockfd, msg_recv, 137, 0)) == -1)
            perror("recv");
        msg_recv[numbytes] = '\0';
        cout << "Server - " << msg_recv << " \n";
        int i;
        cin >> i;
        // cout << "i is" << i << "\n";
        msg_send[0] = '0' + i;
        msg_send[1] = '\0';
        if ((numbytes = send(sockfd, msg_send, strlen(msg_send), 0)) == -1)
            perror("send");

        // receive the result
        if (i == 1 || i == 2)
        {
            cout << "Enter filename:\n";
            int j = 0;
            string fname;
            cin >> fname;
            for (j = 0; j < MAXMSGSIZE - 1; j++)
            {
                msg_send[j] = (char)fname[j];
            }
            msg_send[j] = '\0';
            // cout << msg_send << "is sent\n";
            if ((numbytes = send(sockfd, msg_send, strlen(msg_send), 0)) == -1)
                perror("send");
            cout << "File Name sent\n";
            if (i == 1)
            {
                // recvFile(msg_send, sockfd);
                recvFile(msg_send, sockfd, isBinaryMode);
            }
            else
            {
                // sendFile(msg_send, sockfd);
                sendFile(msg_send, sockfd, isBinaryMode);
            }
        }
        else if (i == 3)
        {
            isBinaryMode = 1;
            cout << "Binary Mode is turned On\n";
        }
        else if (i == 4)
        {
            isBinaryMode = 0;
            cout << "Binary Mode is turned Off. You are in ASCII mode\n";
        }
        else if (i == 5 || i == 6)
        {
            if ((numbytes = recv(sockfd, msg_recv, MAXMSGSIZE - 1, 0)) == -1)
                perror("recv");
            msg_recv[numbytes] = '\0';
            if (i == 5)
                cout << "Available files on Server are:\n";
            else
                cout << "Your uploaded files on Server are:\n";
            cout << msg_recv;
        }
        else if (i == 7)
        {
            break;
        }
    }
    close(sockfd);
    freeaddrinfo(servinfo);

    return 0;
}