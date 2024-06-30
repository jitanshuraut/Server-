#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <system_error>
#include <sys/sysinfo.h>
#include <unistd.h>
// NetWork Libaries
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

using namespace std;

// MAX size of Buffer
const size_t k_max_message = 4096;

// Priting Message
void message(const char *message)
{
    cerr << message << endl;
}

// Aborting with printing Message
void Killed(const char *message)
{
    int err = errno;
    cerr << "[" << err << "] " << message << endl;
    abort();
}

// Read Data from Buffer (Transferring) (Client -> server)
static int32_t read_Buffer(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0)
        {
            return -1; // error, or unexpected EOF (End of Line)
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}

// Wirting to Buffer (Server -> client)
static int32_t write_Buffer(int fd, const char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0)
        {
            return -1; // error
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
    }
    return 0;
}



static int32_t request(int connfd)
{
    // 4 bytes header
    char rbuf[4 + k_max_message + 1];
    errno = 0;
    int32_t err = read_Buffer(connfd, rbuf, 4);
    if (err)
    {
        if (errno == 0)
        {
            message("EOF");
        }
        else
        {
            message("read() error");
        }
        return err;
    }
 

    uint32_t len = 0;
    memcpy(&len, rbuf, 4); // assume little endian
    if (len > k_max_message)
    {
        message("too long");
        return -1;
    }

    // request body
    err = read_Buffer(connfd, &rbuf[4], len);
    if (err)
    {
        message("read() error");
        return err;
    }

    rbuf[4 + len] = '\0';

    cout << "client says: " << &rbuf[4] << endl;
    char op;
    double a, b;
    if (sscanf(&rbuf[4], "%c %lf %lf", &op, &a, &b) != 3)
    {
        message("Invalid command format");
        return -1;
    }

    double result;
    switch (op)
    {
    case '+':
        result = a + b;
        break;
    case '-':
        result = a - b;
        break;
    case '*':
        result = a * b;
        break;
    case '/':
        if (b == 0)
        {
            message("Division by zero");
            return -1;
        }
        result = a / b;
        break;
    default:
        message("Invalid operation");
        return -1;
    }

    string reply = "Result: " + to_string(result);
    len = reply.size();
    char wbuf[4 + len];
    memcpy(wbuf, &len, 4);
    memcpy(&wbuf[4], reply.c_str(), len);

   
    return write_Buffer(connfd, wbuf, 4 + len);
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        Killed("socket()");
    }
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0); // wildcard address 0.0.0.0 
    int rv = bind(fd, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr));
    if (rv)
    {
        Killed("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv)
    {
        Killed("listen()");
    }

    cout<<"Listing "<<endl;
    while (true)
    {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, reinterpret_cast<struct sockaddr *>(&client_addr), &socklen);
        if (connfd < 0)
        {
            continue; // error
        }

        while (true)
        {

            int32_t err = request(connfd);
            if (err)
            {
                break;
            }
        }
        close(connfd);
    }

    return 0;
}
