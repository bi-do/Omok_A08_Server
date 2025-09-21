#pragma once

#include <cstdint>
#include "header.h"
#include "define.h"

typedef enum Error_Type
{
    SOCKET_ERR,
    BIND_ERR,
    LISTEN_ERR
} ERROR_TYPE;

enum class Protocol : uint16_t
{
    ROOM_GET,      // 방 목록 요청
    ROOM_CREATE,   // 방 생성 요청
    ROOM_JOIN,     // 방 참가 요청
    ROOM_EXIT,     // 방 나감
    GAME_MOVE_REQ, // 게스트 수 좌표 REQ
    GAME_MOVE_COM, // 호스트가 수 확정 후 Broad Cast
    GAME_RESULT,   // 호스트가 결과 확인 후 Broad Cast
    GAME_START,    // 호스트가 게임 시작

    LOBBY_CHAT, // 로비 채팅
    ROOM_CHAT,  // 방 채팅
    SERVER_BRC  // 서버 브로드캐스팅
};

enum class GameResultState : uint8_t
{
    Win,
    Lose,
    Draw
};

typedef struct Header
{
    uint16_t type = 0;
    uint16_t body_len = 0;
} Header;

typedef struct Buffer
{
    Header header;
    char buffer[PACKET_BODY_BUF]{};

    void BufferSet(Protocol pro, char flag)
    {
        this->header.type = (uint16_t)pro;
        this->header.body_len = 1;

        this->buffer[0] = flag;
    }

    void BufferSet(Protocol pro, int body_length, char *data)
    {
        this->header.type = (uint16_t)pro;
        this->header.body_len = body_length;

        memcpy_s(this->buffer, sizeof(this->buffer), data, body_length);
    }

    void Write(char *data, int length)
    {
        memcpy_s(&this->buffer[header.body_len], sizeof(this->buffer) - this->header.body_len, data, length);
        this->header.body_len += length;
    }
} Buffer;

typedef struct ClientInfo
{
    SOCKET client_sock;      // 클라이언트 소켓
    sockaddr_in client_addr; // 클라이언트 어드레스
    WSAOVERLAPPED *over;     // Overlapped 구조체
    Buffer buf;
    int cur_room_id = -1; // 현재 플레이어 Room ID

    ClientInfo(SOCKET &socket, sockaddr_in &addr, WSAOVERLAPPED &over)
    {
        this->client_sock = socket;
        this->client_addr = addr;
        this->over = &over;
    }

    ~ClientInfo();

} ClientInfo;

typedef struct RoomInfo
{
    int room_id;
    unsigned short title_length;
    char room_title[ROOM_TITLE_LEN];
} RoomInfo;

typedef struct Room
{
    ClientInfo *host = nullptr;
    ClientInfo *guest = nullptr;
    bool HostIsBlack;
    RoomInfo room_info;
    int room_idx;

    void Init(int room_ID, int room_dix)
    {
        this->room_info.room_id = room_ID;
        this->room_idx = room_dix;
    }

    int JoinRoom(ClientInfo *client)
    {
        int result = 0;
        if (host == nullptr)
        {
            host = client;
        }
        else if (guest == nullptr)
        {
            guest = client;
            result = 1;
        }
        else // 방이 꽉찼다면
        {
            return -1;
        }
        client->cur_room_id = this->room_info.room_id;

        return result;
    }
} Room;

typedef struct Cell
{
    short row = -1;
    short col = -1;
} Cell;
