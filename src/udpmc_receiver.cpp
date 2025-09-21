#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <iostreams/udpmc_receiver.hpp>
#include <iostreams/resolve_host_or_ip_to_ip.hpp>
#include <iostreams/populate_addr_from_ip.hpp>
#include <iostreams/socket_init.hpp>
#include <iostreams/tcp_server.hpp>
#include <cstring>
using namespace iostreams;
void receiver_diagnose(bool condition, const char* msg) {
    if (!condition) {
        std::cerr << "DIAGNOSE ERROR: " << msg << " failed.";
#if defined(_WIN32)
        std::cerr << " WSAGetLastError: " << WSAGetLastError();
#endif
		std::cerr << std::endl;
    }
}
udpmc_receiver::udpmc_receiver(const std::string& host, int port) : udp_istream(bind(host, port))
{
	buf.readSize = 1536;
	memset(&buf.addr, 0, sizeof(buf.addr));
}
streams::udp_streambuf::SocketPair udpmc_receiver::bind(const std::string& host, int port)
{
	socket_init::initialize();
	streams::udp_streambuf::SocketIdentifier sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!streams::SetNonBlocking(sock))
    {
        throw std::runtime_error("SetNonBlocking failed");
    }
    bool reuseAddr = true;
    receiver_diagnose(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseAddr, sizeof(reuseAddr)) >= 0, "Setting SO_REUSEADDR");
	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ::bind(sock, (sockaddr*)&server_addr, sizeof(server_addr));

    bool loopback = true;
    receiver_diagnose(setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loopback, sizeof(loopback)) >= 0, "Setting IP_MULTICAST_LOOP");

    ip_mreq mreq = {};
	auto ip = resolve_host_or_ip_to_ip(host);
    mreq.imr_multiaddr.s_addr = inet_addr(ip.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    receiver_diagnose(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mreq, sizeof(mreq)) >= 0, "Setting IP_ADD_MEMBERSHIP");

	return {sock, server_addr};
}
