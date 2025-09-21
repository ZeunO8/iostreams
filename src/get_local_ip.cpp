#include <iostreams/get_local_ip.hpp>
#include <iostream>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
#endif

std::string iostreams::get_local_ip()
{
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return {};
#endif

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return {};
    }

    // any public IP and port; no packets will actually be sent for UDP connect
    struct sockaddr_in remote;
    remote.sin_family = AF_INET;
    remote.sin_port = htons(53); // DNS port
    inet_pton(AF_INET, "8.8.8.8", &remote.sin_addr);

    int res = connect(sock, (struct sockaddr*)&remote, sizeof(remote));
    if (res < 0) {
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return {};
    }

    struct sockaddr_in local;
    socklen_t addr_len = sizeof(local);
    if (getsockname(sock, (struct sockaddr*)&local, &addr_len) != 0) {
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        return {};
    }

    char buf[64] = {0};
    inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf));
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return std::string(buf);
}