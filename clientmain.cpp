
// This is a TCP client program that connects to a server
// It talks to the server and does math calculations
// Written for the network programming assignment

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// I'm including this library for the calculator functions
#include <calcLib.h>

using namespace std;

int main(int argc, char *argv[]) {
    // Check if the user provided the right arguments
    if (argc != 2) {
        cout << "Usage: ./client <host:port>" << endl;
        return 1;
    }
    
    
    // Parse the host and port from the command line
    string input = argv[1];
    
    string hostname;
    string port_string;
    
    // Find the colon to separate host and port
    // This should work for most cases I think
    int colon_pos = input.find(':');
    if (colon_pos == string::npos) {
        cout << "Invalid host:port format" << endl;
        return 1;
    }
    
    hostname = input.substr(0, colon_pos);
    port_string = input.substr(colon_pos + 1);
    
    int port_number = atoi(port_string.c_str());
    
    cout << "Host " << hostname << ", and port " << port_string << "." << endl;
    
    // TODO: Add socket connection
    cout << "Socket connection not implemented yet" << endl;
    
    return 0;
}