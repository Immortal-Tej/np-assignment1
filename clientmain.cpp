
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
    
    
    // Set a timeout for reading so we don't hang forever
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 second timeout
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    
    // Read the first line from the server
    string first_line = "";
    
    char c;
    
    // Read one character at a time until we get a newline
    while (true) {
        int bytes_read = recv(client_socket, &c, 1, 0);
        
        if (bytes_read <= 0) {
            cout << "ERROR: MISSMATCH PROTOCOL" << endl;
            close(client_socket);
            return 1;
        }
        
        if (c == '\n') {
            break;
        }
        
        first_line = first_line + c;
    }
    
    
    // Check if the first line is exactly what we need
    // We only support "TEXT TCP 1.0"
    if (first_line != "TEXT TCP 1.0") {
        cout << "ERROR: MISSMATCH PROTOCOL" << endl;
        close(client_socket);
        return 1;
    }
    
    
    // Now read the rest of the protocol lines until empty line
    // But limit how many lines we read to avoid hanging
    int max_protocol_lines = 10;
    int lines_read = 0;
    
    
    while (lines_read < max_protocol_lines) {
        string protocol_line = "";
        
        // Read one character at a time until we get a newline
        while (true) {
            int bytes_read = recv(client_socket, &c, 1, 0);
            
            if (bytes_read <= 0) {
                // Timeout or connection error, but we already got TEXT TCP 1.0
                break;
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
        
        lines_read++;
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
    
    
    
    // Calculate the result based on the operation
    string result_string;
    
    
    if (operation[0] == 'f') {
        
        // This is a floating point operation
        double value1 = atof(value1_string.c_str());
        double value2 = atof(value2_string.c_str());
        
        double result;
        
        
        if (operation == "fadd") {
            result = value1 + value2; // Addition
            
        } else if (operation == "fsub") {
            result = value1 - value2; // Subtraction
            
        } else if (operation == "fmul") {
            result = value1 * value2; // Multiplication
            
        } else if (operation == "fdiv") {
            result = value1 / value2; // Division
            
        } else {
            result = 0.0; // Default case for unknown operation
        }
        
        
        // Formating the floating point
        char buffer[100];
        sprintf(buffer, "%8.8g", result);
        
        result_string = string(buffer) + "\n";
        
    } else {
        
        // This is an integer operation
        long long value1 = atoll(value1_string.c_str());
        long long value2 = atoll(value2_string.c_str());
        
        long long result;
        
        
        if (operation == "add") {
            result = value1 + value2; // Addition
            
        } else if (operation == "sub") {
            result = value1 - value2; // Subtraction
            
        } else if (operation == "mul") {
            result = value1 * value2; // Multiplication
            
        } else if (operation == "div") {
            result = value1 / value2; // Integer division
            
        } else {
            result = 0; // Default case for unknown operation
        }
        
        
        // Convert result to string
        char buffer[100];
        sprintf(buffer, "%lld", result);
        
        result_string = string(buffer) + "\n";
        
    }
    
    
    // Send the result back to the server
    int send_result2 = send(client_socket, result_string.c_str(), result_string.length(), 0);
    
    if (send_result2 <= 0) {
        cout << "Failed to send result" << endl;
        close(client_socket);
        return 1;
    }
    
    
    // Read the server's response (OK or ERROR)
    string server_response = "";
    
    
    while (true) {
        int bytes_read = recv(client_socket, &c, 1, 0);
        
        if (bytes_read <= 0) {
            cout << "Failed to read server response" << endl;
            close(client_socket);
            return 1;
        }
        
        
        if (c == '\n') {
            break;
        }
        
        server_response = server_response + c;
    }
    
    
    // Remove the newline from result_string for display
    string display_result = result_string;
    
    if (!display_result.empty() && display_result.back() == '\n') {
        display_result.pop_back();
    }
    
    
    // Print the final result
    cout << server_response << " (myresult=" << display_result << ")" << endl;
    
    
    close(client_socket);
    
    
    return 0; // Success!
}