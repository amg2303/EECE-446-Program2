//Alejandro Gutierrez and Sam Farnsley
//EECE 446 Program 2
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
#include <filesystem>
#include <cstdint>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>


using namespace std;

int recv_helper(int s, void *buf, int response_size)
{
    int total_received = 0;
    char *buffer = (char *)buf;

    while (total_received < response_size)
    {
        int n = recv(s, buffer + total_received,
                     response_size - total_received, 0);

        if (n == 0)
            break;

        if (n < 0)
            return -1;

        total_received += n;
    }

    return total_received;
}

int send_all(int sockfd, const char *buf, int len)
{
    int total = 0;
    while (total < len)
    {
        int sent = send(sockfd, buf + total, len - total, 0);
        if (sent == -1)
        {
            return -1;
        }
        total += sent;
    }
    return total;
}

int lookup_and_connect(const char *host, const char *service)
{
    addrinfo hints{};              // zero-initialize without memset
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *result = nullptr;
    int s = getaddrinfo(host, service, &hints, &result);
    if (s != 0)
    {
        cerr << "getaddrinfo: " << gai_strerror(s) << "\n";
        return -1;
    }

    int sockfd = -1;
    for (addrinfo *rp = result; rp != nullptr; rp = rp->ai_next)
    {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) continue;

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(result);
    if (sockfd == -1) perror("connect");
    return sockfd;
}

void search(int sockfd)
{
    string filename;
    cout << "Enter a file name: ";
    cin >> filename;
    vector<char> buffer;

    buffer.push_back(2);

    for (char c : filename)
    {
        buffer.push_back(c);
    }

    buffer.push_back('\0');

    if (send_all(sockfd, buffer.data(), buffer.size()) == -1)
    {
        cout << "Error sending SEARCH request\n";
        return;
    }

    uint8_t response[10];

    if (recv_helper(sockfd, response, 10) == -1)
    {
        cout << "Error receiving SEARCH response\n";
        return;
    }

    //parsing the peer id without memcpy (I had to look up how to do this lol)
    uint32_t peer_id =
        (static_cast<uint32_t>(response[0]) << 24) |
        (static_cast<uint32_t>(response[1]) << 16) |
        (static_cast<uint32_t>(response[2]) << 8)  |
        (static_cast<uint32_t>(response[3]));

    //Doing same for ip    
    uint32_t ip =
        (static_cast<uint32_t>(response[4]) << 24) |
        (static_cast<uint32_t>(response[5]) << 16) |
        (static_cast<uint32_t>(response[6]) << 8)  |
        (static_cast<uint32_t>(response[7]));

    //Same thing for port #
    uint16_t port =
        (static_cast<uint16_t>(response[8]) << 8) |
        (static_cast<uint16_t>(response[9]));

    if (peer_id == 0)
    {
        cout << "File not indexed by registry\n";
        return;
    }

    struct in_addr addr;
    addr.s_addr = htonl(ip);

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr, ip_str, INET_ADDRSTRLEN);

    cout << "File found at\n";
    cout << "Peer " << peer_id << endl;
    cout << ip_str << ":" << port << endl;
}

void join(int sockfd, uint32_t peer_id)
{
    char buffer[5];
    buffer[0] = 0; 

    buffer[1] = (peer_id >> 24) & 0xFF;
    buffer[2] = (peer_id >> 16) & 0xFF;
    buffer[3] = (peer_id >> 8) & 0xFF;
    buffer[4] = peer_id & 0xFF;

    if (send_all(sockfd, buffer, 5) == -1)
    {
        cout << "Error sending JOIN request\n";
    }
}

void publish(int s)
{
    char action_code = 0x01;
    uint32_t count = 0;
    vector<string> files;
    int size = 5;

    for (const auto& iterator : filesystem::directory_iterator("./SharedFiles/"))
    {
        if (iterator.is_regular_file())
        {
            string current_file = iterator.path().filename().string();
            files.push_back(current_file);
            count++;
        }
    }

    for (string temp_file : files)
    {
        size += temp_file.size() + 1;
    }

    char* buffer = new char[size];

    buffer[0] = action_code;

    buffer[1] = (count >> 24) & 0xFF;
    buffer[2] = (count >> 16) & 0xFF;
    buffer[3] = (count >> 8)  & 0xFF;
    buffer[4] = count & 0xFF;

    int boundary = 5;

    for (string file_name : files)
    {
        strcpy(buffer + boundary, file_name.c_str());
        boundary += file_name.size() + 1;
    }

    send_all(s, buffer, size);
    delete[] buffer;
}

int main(int argc, char *argv[])
{
    // Ensure the arguments is only ./h1-counter and then the chunk size.
    if (argc != 4)
    {
        cout << "Too little or too much line arguments, please do 2 arguments" << endl;
        return 1;
    }

    const char* host = argv[1];
    const char* port = argv[2];
    uint32_t peer_id = atoi(argv[3]);

    // Ensure the chunk size is between 1 - 1000 (0B makes no sense).
    //if (chunkSize <= 0 || chunkSize > 1000)
    //{
    //    cout << "Invalid chunk size\n";
    //    return 1;
    //}

    // Connect to the server/port.
    int sockfd = lookup_and_connect(host, port);
    if (sockfd < 0)
    {
        return 1;
    }

    string command;
    while(true)
{
    cout << "Enter a command: ";
    cin >> command;

    if(command == "SEARCH")
    {
        search(sockfd);
    }
    else if(command == "JOIN")
    {
        join(sockfd, peer_id);
    }
    else if(command == "PUBLISH")
    {
        publish(sockfd);
    }
    else if(command == "EXIT")
    {
        close(sockfd);
        return 0;
    }
}
}
