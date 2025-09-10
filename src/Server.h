#pragma once
#include <unordered_map>
#include <list>
#include "Constants/Constants.h"
#include "Threadheader.h"
#include <random>

class AcceptThread;

using namespace std;

class IOCPThread;

class Server
{
private:
    // Winsock 시작
    WSAData wsa;

    // IOCP 핸들
    HANDLE IOCPHandle;

    /*리슨 소켓*/
    SOCKET listen_sock;

    /*서버 주소 구조체*/
    sockaddr_in server_addr;

    AcceptThread *accept_thread;

    IOCPThread *iocp_arr[IOCPTHREAD_CNT]{};

    /*클라이언트 배열*/
    list<ClientInfo *> client_list;

    /*방 배열*/
    unordered_map<int, Room *> room_map;

    /*정렬용 방 배열*/
    vector<Room*> room_vector;

    /*클라이언트 배열 Lock*/
    CRITICAL_SECTION client_arr_cs;

    /*방 배열 Lcok*/
    CRITICAL_SECTION room_arr_cs;

    void SendRoomList(ClientInfo *client, Buffer *buf);

    void SendRoomCreateResult(ClientInfo *client, Buffer *buf);

    void SendRoomJoin(ClientInfo *client, Buffer *buf);

    

    void SendDoPlay(ClientInfo *client, Buffer *buf);

    void SendGameResult(ClientInfo *client, Buffer *buf);

    void (Server::*func[6])(ClientInfo *client, Buffer *buf) = {
        &Server::SendRoomList,
        &Server::SendRoomCreateResult,
        &Server::SendRoomJoin,
        &Server::SendRoomExit,
        &Server::SendDoPlay,
        &Server::SendGameResult};

public:
    inline static Server *Instance = nullptr;

    Server();

    ~Server();

    /*초기화*/
    int Initialize();

    void SendRoomExit(ClientInfo *client, Buffer *buf);

    /*클라이언트 접속 시 호출*/
    void ClientPush(ClientInfo *client);

    /*클라이언트 리스트에서 제거*/
    void ClientRemove(ClientInfo *client);

    HANDLE GetIOCPHandle();

    /*파싱 후 타입에 맞춘 함수 실행*/
    void ParseAndWork(ClientInfo *client, Buffer *buf);

    /*방 생성*/
    Room* GenerateRoom();

    void DeleteRoom(int room_id);

    /*같은 방에 있는 상대방 반환*/
    ClientInfo * FindOpponent(ClientInfo * player);
};