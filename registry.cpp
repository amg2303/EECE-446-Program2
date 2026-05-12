// Alejandro Gutierrez and Sam Farnsley
// EECE 446 Program 4
// Spring 2026
//


#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_FILES        10
#define MAX_FILENAME_LEN 100
#define MAX_PENDING      10

// Action byte values (same protocol as Programs 2 & 3)
#define ACTION_JOIN      0
#define ACTION_PUBLISH   1
#define ACTION_SEARCH    2

struct peer_entry {
    uint32_t id;
    int socket_descriptor;
    char files[MAX_FILES][MAX_FILENAME_LEN];
    int file_count;
    struct sockaddr_in address;
};

static int recv_all(int s, void *buf, int len)
{
    int total = 0;
    char *p = (char *)buf;
    while (total < len) {
        int n = recv(s, p + total, len - total, 0);
        if (n <= 0) return n;
        total += n;
    }
    return total;
}


static int send_all(int s, const char *buf, int len)
{
    int total = 0;
    while (total < len) {
        int n = send(s, buf + total, len - total, 0);
        if (n == -1) return -1;
        total += n;
    }
    return total;
}

// Bind and listen on the given port string.
// Returns the listening socket fd, or -1 on error.
static int bind_and_listen(const char *port)
{
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    addrinfo *result = nullptr;
    int s = getaddrinfo(nullptr, port, &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }

    int sockfd = -1;
    for (addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) continue;

        int yes = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) break;

        close(sockfd);
        sockfd = -1;
    }
    freeaddrinfo(result);

    if (sockfd == -1) { perror("bind"); return -1; }

    if (listen(sockfd, MAX_PENDING) == -1) {
        perror("listen");
        close(sockfd);
        return -1;
    }
    return sockfd;
}

// Return the highest fd currently set in fs.
static int find_max_fd(const fd_set *fs)
{
    int ret = 0;
    for (int i = FD_SETSIZE - 1; i >= 0 && ret == 0; --i)
        if (FD_ISSET(i, fs)) ret = i;
    return ret;
}

// Find a peer_entry by socket descriptor; returns nullptr if not found.
static peer_entry *find_peer_by_socket(std::vector<peer_entry> &peers, int s)
{
    for (auto &p : peers)
        if (p.socket_descriptor == s) return &p;
    return nullptr;
}



static void handle_join(int s, std::vector<peer_entry> &peers)
{
    // Read the 4-byte peer ID in network byte order
    uint8_t buf[4];
    if (recv_all(s, buf, 4) <= 0) return;

    uint32_t peer_id = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] <<  8) |  (uint32_t)buf[3];

    // Look up if we already have an entry for this socket.
    // If not, create a new one.
    peer_entry *pe = find_peer_by_socket(peers, s);
    if (pe == nullptr) {
        peers.push_back(peer_entry{});
        pe = &peers.back();
        pe->socket_descriptor = s;
        pe->file_count        = 0;

        // Use getpeername to learn the peers ip address and port number.
        socklen_t len = sizeof(pe->address);
        if (getpeername(s, (struct sockaddr *)&pe->address, &len) == -1)
            perror("getpeername");
    }
    pe->id = peer_id;

    printf("TEST] JOIN %u\n", peer_id);
    fflush(stdout);
}


static void handle_publish(int s, std::vector<peer_entry> &peers)
{
    // Read the 4-byte file count
    uint8_t buf[4];
    if (recv_all(s, buf, 4) <= 0) return;
 
    uint32_t file_count =
        ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
        ((uint32_t)buf[2] <<  8) |  (uint32_t)buf[3];
 
    peer_entry *pe = find_peer_by_socket(peers, s);
    if (pe == nullptr) return;
 
    // Read each null-terminated filename
    pe->file_count = 0;
    for (uint32_t i = 0; i < file_count && i < MAX_FILES; i++) {
        int j = 0;
        char c;
        while (recv_all(s, &c, 1) == 1 && c != '\0' && j < MAX_FILENAME_LEN - 1) {
            pe->files[i][j++] = c;
        }
        pe->files[i][j] = '\0';
        pe->file_count++;
    }
 
    // Print output
    printf("TEST] PUBLISH %d", pe->file_count);
    for (int i = 0; i < pe->file_count; i++)
        printf(" %s", pe->files[i]);
    printf("\n");
    fflush(stdout);
}



static void handle_search(int s, std::vector<peer_entry> &peers)
{
    // Read filename one byte at a time
    char filename[MAX_FILENAME_LEN];
    int i = 0;
    char c;
    while (recv_all(s, &c, 1) == 1 && c != '\0' && i < MAX_FILENAME_LEN - 1) {
        filename[i++] = c;
    }
    filename[i] = '\0';
 
    // Search all peers for the file
    // Search all peers for the file, excluding the peer that sent the SEARCH
    peer_entry *found = nullptr;
    for (auto &p : peers) {
        if (p.socket_descriptor == s) continue; // skip the searching peer
        for (int f = 0; f < p.file_count; f++) {
            if (strcmp(p.files[f], filename) == 0) {
                found = &p;
                break;
            }
        }
        if (found) break;
    }
 
    // Build and send response. All zeros if not found
    char response[10] = {};
    if (found) {
        uint32_t id = found->id; 
        uint32_t ip = htonl(found->address.sin_addr.s_addr);
        uint16_t port = found->address.sin_port;        // already network order
 
        response[0] = (id   >> 24) & 0xFF;
        response[1] = (id   >> 16) & 0xFF;
        response[2] = (id   >>  8) & 0xFF;
        response[3] =  id          & 0xFF;
 
        response[4] = (ip   >> 24) & 0xFF;
        response[5] = (ip   >> 16) & 0xFF;
        response[6] = (ip   >>  8) & 0xFF;
        response[7] =  ip          & 0xFF;
 
        response[8] = (port >>  8) & 0xFF;
        response[9] =  port        & 0xFF;
    }
    send_all(s, response, 10);
 
    // Print output
    char ip_str[INET_ADDRSTRLEN] = "0.0.0.0";
    uint16_t port = 0;
    uint32_t id   = 0;
    if (found) {
        inet_ntop(AF_INET, &found->address.sin_addr, ip_str, INET_ADDRSTRLEN);
        port = ntohs(found->address.sin_port);
        id   = found->id;
    }
    printf("TEST] SEARCH %s %u %s:%u\n", filename, id, ip_str, port);
    fflush(stdout);
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Start listening for peer connections on the given port
    int listen_sock = bind_and_listen(argv[1]);
    if (listen_sock < 0) return 1;

    // peers stores state for every currently connected peer
    std::vector<peer_entry> peers;

    // all_sockets tracks every active fd — listen socket + all peer sockets
    fd_set all_sockets;
    FD_ZERO(&all_sockets);
    FD_SET(listen_sock, &all_sockets);
    int max_sock = listen_sock;

    while (true) {
        // copy all_sockets into call_set before every select call —
        // select modifies the set in place so we never pass all_sockets directly
        fd_set call_set = all_sockets;

        // Single select call site.
        // No timeout (NULL) — registry has nothing to do without a socket ready.
        // No FD_SETSIZE as argument — use max_sock + 1 per the spec.
        int num_ready = select(max_sock + 1, &call_set, nullptr, nullptr, nullptr);
        if (num_ready < 0) {
            perror("select");
            return -1;
        }

        // Check every fd from 3 (skip stdin/stdout/stderr) up to max_sock
        for (int s = 3; s <= max_sock; ++s) {
            if (!FD_ISSET(s, &call_set)) continue;

            if (s == listen_sock) {
                // A new peer is connecting — accept it
                int new_sock = accept(listen_sock, nullptr, nullptr);
                if (new_sock < 0) { perror("accept"); continue; }

                // Track it in all_sockets and update max_sock if needed
                FD_SET(new_sock, &all_sockets);
                if (new_sock > max_sock) max_sock = new_sock;

            } else {
                // An existing peer socket has data (or has closed)
                // Read the 1-byte action field first
                uint8_t action;
                int n = recv(s, &action, 1, 0);

                if (n <= 0) {
                    // Peer closed connection — clean up its state
                    peer_entry *pe = find_peer_by_socket(peers, s);
                    if (pe != nullptr) {
                        // Remove by swapping with the last entry
                        *pe = peers.back();
                        peers.pop_back();
                    }
                    FD_CLR(s, &all_sockets);
                    close(s);
                    max_sock = find_max_fd(&all_sockets);
                    continue;
                }

                // Dispatch to the appropriate handler based on action byte
                switch (action) {
                    case ACTION_JOIN:    handle_join(s, peers);    break;
                    case ACTION_PUBLISH: handle_publish(s, peers); break;
                    case ACTION_SEARCH:  handle_search(s, peers);  break;
                    default:
                        fprintf(stderr, "Unknown action byte: %u\n", action);
                        break;
                }
            }
        }
    }
}
