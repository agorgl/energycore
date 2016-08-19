#include "sock.h"

int sock_init()
{
#ifdef OS_WINDOWS
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2,2), &wsa_data);
#else
    return 0;
#endif
}

int sock_destroy()
{
#ifdef OS_WINDOWS
    return WSACleanup();
#else
    return 0;
#endif
}

int sendall(int s, char* buf, int* len)
{
    /* How many bytes we've sent */
    int total = 0;
    /* How many we have left to send */
    int bytesleft = *len;
    int n = 0;
    while(total < *len) {
        n = send(s, buf + total, bytesleft, 0);
        if (n == -1)
            break;
        total += n;
        bytesleft -= n;
    }
    /* Return number actually sent here */
    *len = total;
    /* Return -1 on failure, 0 on success */
    return n == -1 ? -1 : 0;
}

int sock_close(sock_t sock)
{
    int status = 0;
#ifdef OS_WINDOWS
    status = shutdown(sock, SD_BOTH);
    if (status == 0)
        status = closesocket(sock);
#else
    status = shutdown(sock, SHUT_RDWR);
    if (status == 0)
        status = close(sock);
#endif
    return status;
}
