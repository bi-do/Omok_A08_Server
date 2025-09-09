#pragma once
#include <winsock.h>
#include <ws2tcpip.h>
#include <cstdint>

typedef enum Error_Type
{
    SOCKET_ERR,
    BIND_ERR,
    LISTEN_ERR
} ERROR_TYPE;

enum class Header_Type : uint8_t{
    SESSION,
    GAME,
    CHAT
};

enum class Session_Type : uint8_t{

};

enum class Game_Type : uint8_t{

};

enum class Chat_Type : uint8_t{

};

typedef struct Protocol
{
    
} Protocol;

typedef struct ClientInfo
{
    SOCKET client_sock;         // 클라이언트 소켓
    sockaddr_in client_addr;    // 클라이언트 어드레스
    WSAOVERLAPPED * over;       // Overlapped 구조체

    ClientInfo(SOCKET &socket, sockaddr_in &addr , WSAOVERLAPPED& over)
    {
        this->client_sock = socket;
        this->client_addr = addr;
        this->over = &over;
    }

    ~ClientInfo() {

    }
} ClientInfo;