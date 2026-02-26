#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


using namespace std;

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
int recv(int sockfd, void *buf, int len) {}
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
    //if (argc != 2)
    //{
    //    cout << "Too little or too much line arguments, please do 2 arguments" << endl;
    //    return 1;
    //}
//
    //int chunkSize = atoi(argv[1]);

    // Ensure the chunk size is between 1 - 1000 (0B makes no sense).
    //if (chunkSize <= 0 || chunkSize > 1000)
    //{
    //    cout << "Invalid chunk size\n";
     //   return 1;
    //}

    // Connect to the server/port.
    int sockfd = lookup_and_connect(SERVER, PORT);
    if (sockfd < 0)
    {
        return 1;
    }

   
    close(sockfd);
    return 0;
}

