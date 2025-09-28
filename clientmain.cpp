
// Client program that connects to server and does math calculations

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include <calcLib.h>

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cout << "Usage: ./client <host:port>" << endl;
        return 1;
    }
    
    // Split the input into host and port parts
    string input = argv[1];
    
    string hostname;
    string port_string;
    
    // Check if this is IPv6 format with brackets like [::1]:5000
    if (input[0] == '[') {
        size_t bracket_end = input.find(']');
        if (bracket_end == string::npos || bracket_end + 1 >= input.length() || input[bracket_end + 1] != ':') {
            cout << "Invalid bracketed IPv6 format" << endl;
            return 1;
        }
        hostname = input.substr(1, bracket_end - 1);
        port_string = input.substr(bracket_end + 2);
    } else {
        // Find the last colon for host:port splitting
        size_t colon_pos = input.rfind(':');
        if (colon_pos == string::npos) {
            cout << "Invalid host:port format" << endl;
            return 1;
        }
        
        hostname = input.substr(0, colon_pos);
        port_string = input.substr(colon_pos + 1);
    }
    
    cout << "Host " << hostname << ", and port " << port_string << "." << endl;
    
    // Get address info to support both IPv4 and IPv6
    struct addrinfo hints;
    struct addrinfo *result;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;    // Allow both IPv4 and IPv6
    hints.ai_socktype = SOCK_STREAM;
    
    int addr_result = getaddrinfo(hostname.c_str(), port_string.c_str(), &hints, &result);
    
    if (addr_result != 0) {
        cout << "ERROR: RESOLVE ISSUE" << endl;
        return 1;
    }
    
    // Try connecting to each resolved address until one works
    int client_socket = -1;
    struct addrinfo *current_addr;
    
    for (current_addr = result; current_addr != NULL; current_addr = current_addr->ai_next) {
        
        client_socket = socket(current_addr->ai_family, current_addr->ai_socktype, current_addr->ai_protocol);
        
        if (client_socket < 0) {
            continue;
        }
        
        if (connect(client_socket, current_addr->ai_addr, current_addr->ai_addrlen) == 0) {
            break; // Connected successfully!
        }
        
        // This address didn't work, try the next one
        close(client_socket);
        client_socket = -1;
    }
    
    freeaddrinfo(result);
    
    if (client_socket < 0) {
        cout << "ERROR: CANT CONNECT TO " << hostname << endl;
        return 1;
    }
    
    // Set timeout so we don't wait forever
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // Read protocol line from server
    string first_line = "";
    char c;
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
    
    // Make sure server supports our protocol
    if (first_line != "TEXT TCP 1.0") {
        cout << "ERROR: MISSMATCH PROTOCOL" << endl;
        close(client_socket);
        return 1;
    }
    
    // Read remaining protocol lines until empty line
    int max_protocol_lines = 10;
    int lines_read = 0;
    
    while (lines_read < max_protocol_lines) {
        string protocol_line = "";
        
        while (true) {
            int bytes_read = recv(client_socket, &c, 1, 0);
            
            if (bytes_read <= 0) {
                break;
            }
            
            if (c == '\n') {
                break;
            }
            
            protocol_line = protocol_line + c;
        }
        
        // Empty line means we're done with protocol negotiation
        if (protocol_line.empty()) {
            break;
        }
        
        lines_read++;
    }
    
    // Tell server we accept the protocol
    string ok_message = "OK\n";
    
    int send_result = send(client_socket, ok_message.c_str(), ok_message.length(), 0);
    
    if (send_result <= 0) {
        cout << "Failed to send OK message" << endl;
        close(client_socket);
        return 1;
    }
    
    // Get the math problem from server
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
    
    // Parse the operation and two numbers
    string operation;
    string value1_string;
    string value2_string;
    
    int space1 = assignment_line.find(' ');
    
    if (space1 == string::npos) {
        cout << "Invalid assignment format" << endl;
        close(client_socket);
        return 1;
    }
    
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
    
    // Do the math calculation
    string result_string;
    
    // Check if this is a float operation (starts with 'f')
    if (operation[0] == 'f') {
        double value1 = atof(value1_string.c_str());
        double value2 = atof(value2_string.c_str());
        double result;
        
        if (operation == "fadd") {
            result = value1 + value2;
        } else if (operation == "fsub") {
            result = value1 - value2;
        } else if (operation == "fmul") {
            result = value1 * value2;
        } else if (operation == "fdiv") {
            result = value1 / value2;
        } else {
            result = 0.0;
        }
        
        // Format the float result
        char buffer[100];
        sprintf(buffer, "%8.8g", result);
        result_string = string(buffer) + "\n";
        
    } else {
        // Integer operation
        long long value1 = atoll(value1_string.c_str());
        long long value2 = atoll(value2_string.c_str());
        long long result;
        
        if (operation == "add") {
            result = value1 + value2;
        } else if (operation == "sub") {
            result = value1 - value2;
        } else if (operation == "mul") {
            result = value1 * value2;
            
        } else if (operation == "div") {
            result = value1 / value2;
        } else {
            result = 0;
        }
        
        // Convert to string
        char buffer[100];
        sprintf(buffer, "%lld", result);
        result_string = string(buffer) + "\n";
    }
    
    // Send answer back to server
    int send_result2 = send(client_socket, result_string.c_str(), result_string.length(), 0);
    
    if (send_result2 <= 0) {
        cout << "Failed to send result" << endl;
        close(client_socket);
        return 1;
    }
    
    // Get server's response
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
    
    // Clean up result for display
    string display_result = result_string;
    if (!display_result.empty() && display_result.back() == '\n') {
        display_result.pop_back();
    }
    
    cout << server_response << " (myresult=" << display_result << ")" << endl;
    
    close(client_socket);
    return 0;
}