
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
#include <netdb.h>

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
    
    
    // Create a socket to connect to the server
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    if (client_socket < 0) {
        cout << "ERROR: Could not create socket" << endl;
        return 1;
    }
    
    
    // Set up the server address
    struct sockaddr_in server_address;
    
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_number);
    
    
    // Convert the hostname to an IP address
    // First try if it's already an IP address
    if (inet_aton(hostname.c_str(), &server_address.sin_addr) == 0) {
        
        // If not an IP, try to resolve hostname using DNS
        // I hope this works for all hostnames
        struct hostent *host_entry;
        host_entry = gethostbyname(hostname.c_str());
        
        if (host_entry == NULL) {
            cout << "ERROR: RESOLVE ISSUE" << endl;
            close(client_socket);
            return 1;
        }
        
        // Copy the first IP address from the host entry
        // This should give us the IP address to connect to
        memcpy(&server_address.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
    }
    
    
    // Try to connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        cout << "ERROR: CANT CONNECT TO " << hostname << endl;
        close(client_socket);
        return 1;
    }
    
    
    // Read the protocols that the server supports
    string protocol_line;
    
    bool found_our_protocol = false;
    
    
    while (true) {
        protocol_line = "";
        
        char c;
        
        // Read one character at a time until we get a newline
        while (true) {
            int bytes_read = recv(client_socket, &c, 1, 0);
            
            if (bytes_read <= 0) {
                cout << "Connection error while reading protocols" << endl;
                close(client_socket);
                return 1;
            }
            
            if (c == '\n') {
                break;
            }
            
            protocol_line = protocol_line + c;
        }
        
        // If we get an empty line, we're done reading protocols
        if (protocol_line.empty()) {
            break;
        }
        
        
        // Check if this is the protocol we want
        // We only support "TEXT TCP 1.0"
        if (protocol_line == "TEXT TCP 1.0") {
            found_our_protocol = true;
        }
        
    }
    
    
    // Check if the server supports our protocol
    if (!found_our_protocol) {
        cout << "ERROR: MISSMATCH PROTOCOL" << endl;
        close(client_socket);
        return 1;
    }
    
    
    // Send OK to the server
    string ok_message = "OK\n";
    
    int send_result = send(client_socket, ok_message.c_str(), ok_message.length(), 0);
    
    if (send_result <= 0) {
        cout << "Failed to send OK message" << endl;
        close(client_socket);
        return 1;
    }
    
    
    // Read the assignment from the server
    string assignment_line = "";
    
    char c;
    
    while (true) {
        int bytes_read = recv(client_socket, &c, 1, 0);
        
        if (bytes_read <= 0) {
            cout << "Failed to read assignment" << endl;
            close(client_socket);
            return 1;
        }
        
        
        if (c == '\n') {
            break;
        }
        
        assignment_line = assignment_line + c;
    }
    
    
    // Parse the assignment line to get operation and values
    string operation;
    string value1_string;
    string value2_string;
    
    
    // Find the first space
    int space1 = assignment_line.find(' ');
    
    if (space1 == string::npos) {
        cout << "Invalid assignment format" << endl;
        close(client_socket);
        return 1;
    }
    
    
    // Find the second space
    int space2 = assignment_line.find(' ', space1 + 1);
    
    if (space2 == string::npos) {
        cout << "Invalid assignment format" << endl;
        close(client_socket);
        return 1;
    }
    
    
    operation = assignment_line.substr(0, space1);
    
    value1_string = assignment_line.substr(space1 + 1, space2 - space1 - 1);
    
    value2_string = assignment_line.substr(space2 + 1);
    
    
    cout << "ASSIGNMENT: " << operation << " " << value1_string << " " << value2_string << endl;
    
    
    // TODO: Calculate result based on operation
    cout << "Calculation not implemented yet" << endl;
    
    // Close the socket
    close(client_socket);
    
    return 0;
}