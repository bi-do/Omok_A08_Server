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

    void Send_Room_List(ClientInfo *client, Buffer *buf);

    void Send_Room_CreateResult(ClientInfo *client, Buffer *buf);

    void Send_Room_Join(ClientInfo *client, Buffer *buf);

    void Send_Move_REQ(ClientInfo *client, Buffer *buf);

    void Send_Move_Com(ClientInfo *client, Buffer *buf);

    void Send_Game_Result(ClientInfo *client, Buffer *buf);

    void Send_Game_Start(ClientInfo *client, Buffer *buf);


    void (Server::*func[8])(ClientInfo *client, Buffer *buf) = {
        &Server::Send_Room_List,
        &Server::Send_Room_CreateResult,
        &Server::Send_Room_Join,
        &Server::SendRoomExit,
        &Server::Send_Move_REQ,
        &Server::Send_Move_Com,
        &Server::Send_Game_Result,
        &Server::Send_Game_Start};

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
    Room* GenerateRoom(char* title ,int title_length);

    void DeleteRoom(ClientInfo* room);

    Room* FindRoom(ClientInfo *player);

    /*호스트로 변경*/
    void SetHost(ClientInfo* player);

    /*호스트인지 확인*/
    bool CheckHost(ClientInfo* player);

    /*같은 방에 있는 상대방 반환*/
    ClientInfo * FindOpponent(ClientInfo * player);
};