#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Target HTTP server.
#define SERVER "www.ecst.csuchico.edu"
#define PORT "80"

using namespace std;

int lookup_and_connect(const char *host, const char *service)
{
    struct addrinfo hints;
    struct addrinfo *rp, *result;
    int s;
    /* Translate host name into peer's IP address */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    if ((s = getaddrinfo(host, service, &hints, &result)) != 0)
    {
        fprintf(stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }
    /* Iterate through the address list and try to connect */
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        if ((s = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1)
        {
            continue;
        }
        if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1)
        {
            break;
        }
        close(s);
    }
    if (rp == NULL)
    {
        perror("stream-talk-client: connect");
        return -1;
    }
    freeaddrinfo(result);
:while () {
  
}    return s;
}

void search()
{

}

void join()
{

}

void publish()
{

}

/**
 * Send the entire buffer to a socket
 *
 * @return Total bytes sent, or -1 on error.
 */
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

int main()
{
  string command;
  cout << "Enter a command: " << endl;
  cin >> command;
  switch(command)
  {
    case "SEARCH":
      //run search
      break;
    case "JOIN":
      //run JOIN
    break;
      case "PUBLISH":
      break;
    case "EXIT":
      return 0;
    break;
  }
}

int main(int argc, char *argv[])
{
    // Ensure the arguments is only ./h1-counter and then the chunk size.
    if (argc != 2)
    {
        cout << "Too little or too much line arguments, please do 2 arguments" << endl;
        return 1;
    }

    int chunkSize = atoi(argv[1]);

    // Ensure the chunk size is between 1 - 1000 (0B makes no sense).
    if (chunkSize <= 0 || chunkSize > 1000)
    {
        cout << "Invalid chunk size\n";
        return 1;
    }

    // Connect to the server/port.
    int sockfd = lookup_and_connect(SERVER, PORT);
    if (sockfd < 0)
    {
        return 1;
    }

    // HTTP/1.0 request (simple: no Host header needed for many servers).
    const char request[] = "GET /~kkredo/file.html HTTP/1.0\r\n\r\n";

    // Send the request.
    if (send_all(sockfd, request, static_cast<int>(strlen(request))) == -1)
    {
        cout << "Error sending request\n";
        close(sockfd);
        return 1;
    }

    if (recv_all(sockfd, chunkSize) == -1)
    {
        cout << "Error recieving file\n";
        close(sockfd);
        return -1;
    }

    close(sockfd);
    return 0;
}

