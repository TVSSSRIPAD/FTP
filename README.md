# FTP
A basic FTP server created with TCP sockets

Functionalities supported by my implementation:-
 - User authentication system.
 - Function to add users on server. (A separate thread is created for adding users on server) 
 - Multiple Clients using Threads.
 - Progress Bar (Statistics like percentage completed, No.of bytes received, No.of bytes remaining, Total time taken for an operation etc of the current operation) are printed on Client side.
 - GET and PUT methods are supported. Default Mode is ASCII Mode but can be changed to Binary Mode and back to ASCII Mode also.
 - The client can also see the list of files uploaded by him to the server.
 - Different directories are created for different clients on the server side, so that files of one user do not get mixed up with files of others.
 - Error handling is done for get and put methods.
 
 
Instructions to Run program:-

Server file is server.cpp

Command to run:- 
1) g++ -o ser server.cpp -lpthread
2) ./ser

Client file is client.cpp [Located inside Client Folder]

Command to run:-
1) cd ./client
2) g++ -o cli client.cpp
3) ./cli

Directory Structure:- 
The Root Directory consists of FTPServer file (server.cpp). 
Immediately after running the server, 2 folders Client1 and Client2 are created (for default users). Upon addition of further users, separate folders are created for each user. Root directory also contains of client folder which consists of FTP-Client file i.e., client.cpp .
