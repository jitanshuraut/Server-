#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <system_error>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <stdio.h>

using namespace std;
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

// Read Data from Buffer (Transferring) (Server -> Client)
int32_t read_Buffer(int fd, char *buf, size_t n)
{

    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);

        if (rv <= 0)
        {
            return -1; // error, or unexpected EOF
        }
        assert(static_cast<size_t>(rv) <= n);
        n -= static_cast<size_t>(rv);
        buf += rv;
        cout << "BUFFER: [read_Buffer] " << endl;
        cout << buf << endl;
    }
    return 0;
}

// Wirting to Buffer (Client -> Server)
static int32_t write_Buffer(int fd, const char *buf, size_t n)
{
    cout << "in:" << endl;
    cout << buf << endl;
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
        cout << "BUFFER: [write_Buffer] " << endl;
        cout << rv << endl;
    }
    return 0;
}


static int32_t query(int fd, const char *text)
{

    uint32_t len = static_cast<uint32_t>(strlen(text));
    if (len > k_max_message)
    {
        return -1;
    }

    char wbuf[4 + k_max_message];

    memcpy(wbuf, &len, 4); // assume little endian
    memcpy(&wbuf[4], text, len);
    if (int32_t err = write_Buffer(fd, wbuf, 4 + len))
    {
        return err;
    }

    // 4 bytes header
    char rbuf[4 + k_max_message + 1];
    errno = 0;
    int32_t err = read_Buffer(fd, rbuf, 4);
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

    memcpy(&len, rbuf, 4); // assume little endian
    if (len > k_max_message)
    {
        message("too long");
        return -1;
    }
    err = read_Buffer(fd, &rbuf[4], len);
    if (err)
    {
        message("read() error");
        return err;
    }

    rbuf[4 + len] = '\0';
    cout << "server says: " << &rbuf[4] << endl;
    return 0;
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        Killed("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); 
    int rv = connect(fd, reinterpret_cast<const struct sockaddr *>(&addr), sizeof(addr));
    if (rv)
    {
        Killed("connect");
    }

    cout<<"connected"<<endl;
    // multiple requests
    int flag = 1;
    while (flag)
    {
        int n;
        cout << "want to continue?" << endl;
        cin >> n;
        cin.ignore();
        if (n == 1)
        {
            char str[100]; 

            scanf("%99[^\n]", str); 
            cin.ignore();           

            cout << str << endl;

            int32_t err = query(fd, str);
            if (err)
            {
                goto L_DONE;
            }
        }
        else
        {
            flag = 0;
        }
    }

L_DONE:
    close(fd);
    return 0;
}
