#include "Server.h"

Server::Server()
{
    if (Server::Instance == nullptr)
    {
        Server::Instance = this;
    }
    else
    {
        delete this;
    }
}

Server::~Server()
{
    delete this->accept_thread;

    for (IOCPThread *element : this->iocp_arr)
    {
        delete element;
    }

    EnterCriticalSection(&this->client_arr_cs);
    for (ClientInfo *element : this->client_list)
    {
        delete element;
    }
    LeaveCriticalSection(&this->client_arr_cs);

    closesocket(this->listen_sock);
    CloseHandle(this->IOCPHandle);

    DeleteCriticalSection(&this->client_arr_cs);
    WSACleanup();

    cout << "Server Shutdown" << endl;
}

int Server::Initialize()
{
    // 임계 구간 초기화
    InitializeCriticalSection(&this->client_arr_cs);

    WSAStartup(MAKEWORD(2, 2), &wsa);

    // IOCP 모듈 생성
    this->IOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

    // IOCP 소켓 생성
    this->listen_sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (this->listen_sock == INVALID_SOCKET)
    {
        cout << "Listen Socket Generate fail. ERR code : " << GetLastError() << endl;
        return SOCKET_ERR;
    }

    // 서버 주소 설정
    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_port = htons(ACCEPT_PORT);
    inet_pton(AF_INET, ACCEPT_IP, &this->server_addr.sin_addr);

    // 소켓 바인딩
    if (SOCKET_ERROR == bind(this->listen_sock, (sockaddr *)&this->server_addr, sizeof(sockaddr_in)))
    {
        cout << "Socket Bind ERR. ERR code : " << GetLastError() << endl;
        return BIND_ERR;
    }

    // IOCP 스레드 생성
    for (int i = 0; i < IOCPTHREAD_CNT; i++)
    {
        this->iocp_arr[i] = new IOCPThread{};
        this->iocp_arr[i]->Run();
    }

    // 소켓 리슨
    if (SOCKET_ERROR == listen(this->listen_sock, SOMAXCONN))
    {
        cout << "Socket Listen Err. ERR code : " << GetLastError() << endl;
        return BIND_ERR;
    }

    // Accept 스레드 시작
    this->accept_thread = new AcceptThread{this->listen_sock};
    this->accept_thread->Run();

    cout << "Server Start" << endl;

    return 5;
}

void Server::ClientPush(ClientInfo *client)
{
    EnterCriticalSection(&this->client_arr_cs);
    this->client_list.push_back(client);
    LeaveCriticalSection(&this->client_arr_cs);
    char IPtemp[20]{};

    inet_ntop(AF_INET, &client->client_addr.sin_addr, IPtemp, sizeof(IPtemp));

    cout << "Client Connect. IP : " << IPtemp << endl;
}

void Server::ClientRemove(ClientInfo *client)
{
    EnterCriticalSection(&this->client_arr_cs);
    this->client_list.remove(client);
    LeaveCriticalSection(&this->client_arr_cs);
    char IPtemp[20]{};

    inet_ntop(AF_INET, &client->client_addr.sin_addr, IPtemp, sizeof(IPtemp));

    cout << "Client DisConnect  : " << IPtemp << endl;

    delete client;
}

void Server::ParseAndWork(ClientInfo *client, Buffer *buf)
{
    (this->*this->func[buf->header.type])(client, buf);
    delete buf;
}

void Server::Send_Room_List(ClientInfo *client, Buffer *buf)
{
    int index = 0;
    int vector_size = this->room_vector.size();

    int count = 0;
    int offset = 0;
    RoomInfo *room_info = nullptr;

    memcpy_s(&index, sizeof(int), buf->buffer, sizeof(int));
    ZeroMemory(buf, sizeof(*buf));

    for (int i = (index) * 10; i < vector_size && count < 10; i++)
    {
        room_info = &this->room_vector[i]->room_info;
        buf->Write((char *)&room_info->room_id, sizeof(int));
        buf->Write((char *)&room_info->title_length, sizeof(unsigned short));
        buf->Write((char *)room_info->room_title, room_info->title_length);
        count++;
        offset += 6 + room_info->title_length;
    }

    buf->header.body_len = offset;

    send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
}

void Server::Send_Room_CreateResult(ClientInfo *client, Buffer *buf)
{
    Room *room = GenerateRoom(&buf->buffer[1], buf->buffer[0]);

    room->host = client;
    client->cur_room_id = room->room_info.room_id;

    ZeroMemory(buf, sizeof(*buf));

    buf->BufferSet(Protocol::ROOM_CREATE, 1);

    cout << "SendRoomCreateResult" << endl;
    send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
}

void Server::Send_Room_Join(ClientInfo *client, Buffer *buf)
{
    int room_id = 0;

    memcpy_s(&room_id, sizeof(int), buf->buffer, sizeof(int));

    Room *room = nullptr;

    auto it = this->room_map.find(room_id);
    if (it == room_map.end())
    {
        buf->BufferSet(Protocol::ROOM_JOIN, -1);
        send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
        cout << "Room is not exist" << endl;
        return;
    }
    else
    {
        room = it->second;
    }

    if (-1 != room->JoinRoom(client)) // 방에 빈 자리가 있을 때
    {
        client->cur_room_id = room_id;

        // 상대방
        ClientInfo *opponent = FindOpponent(client);

        // Join Send
        buf->BufferSet(Protocol::ROOM_JOIN, 1);
        send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);

        // 상대방 게임 시작 Alert
        buf->BufferSet(Protocol::ROOM_JOIN, 2);
        send(opponent->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);

        // 플레이어 게임 시작 Aelrt
        // buf->BufferSet(Protocol::GAME_START, 2);
        // send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
    }
    else // 방이 꽉찼을 때
    {
        buf->BufferSet(Protocol::ROOM_JOIN, -1);
        send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
    }
}

void Server::SendRoomExit(ClientInfo *client, Buffer *buf)
{
    // 상대방
    ClientInfo *opponent = FindOpponent(client);

    if (opponent != nullptr)
    {
        // 상대방에게 플레이어 Exit Alert
        buf->BufferSet(Protocol::ROOM_EXIT, 2);
        send(opponent->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
        SetHost(opponent);
    }
    else
    {
        DeleteRoom(client);
    }

    // 플레이어에게 Room_Exit의 ACK Send;
    buf->BufferSet(Protocol::ROOM_EXIT, 1);
    send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);

    client->cur_room_id = -1;
}

void Server::Send_Move_REQ(ClientInfo *client, Buffer *buf)
{
    if (CheckHost(client))
    {
        cout << "This is an unexpected request : This API is for Guest-only." << endl;
        return;
    }
    else
    {
        ClientInfo *opponent = FindOpponent(client);

        if (opponent == nullptr)
        {
            cout << "oppnent is not exist . invalid call. " << endl;
            return;
        }

        send(opponent->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
    }
}

void Server::Send_Move_Com(ClientInfo *client, Buffer *buf)
{
    if (!CheckHost(client))
    {
        cout << "This is an unexpected request : This API is for Host-only." << endl;
        return;
    }
    else
    {
        ClientInfo *opponent = FindOpponent(client);

        if (opponent == nullptr)
        {
            cout << "oppnent is not exist . invalid call. " << endl;
            return;
        }

        send(opponent->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
    }
}

void Server::Send_Game_Result(ClientInfo *client, Buffer *buf)
{
    if (!CheckHost(client))
    {
        cout << "This is an unexpected request : This API is for Host-only." << endl;
        return;
    }
    else
    {
        ClientInfo *opponent = FindOpponent(client);

        if (opponent == nullptr)
        {
            cout << "oppnent is not exist . invalid call. " << endl;
            return;
        }
        send(opponent->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
    }
}

void Server::Send_Game_Start(ClientInfo *client, Buffer *buf)
{
    if (!CheckHost(client))
    {
        cout << "This is an unexpected request : This API is for Host-only." << endl;
        return;
    }
    else
    {
        bool host_is_black = buf->buffer[0] == 0 ? true : false;

        Room *room = FindRoom(client);
        room->HostIsBlack = host_is_black;

        ClientInfo *opponent = FindOpponent(client);

        if (opponent == nullptr)
        {
            cout << "oppnent is not exist . invalid call. " << endl;
            return;
        }
        send(opponent->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
    }
}

void Server::Send_Lobby_Chat(ClientInfo *client, Buffer *buf)
{
    for (ClientInfo *client : this->client_list)
    {
        if (client->cur_room_id == -1)
        {
            send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
        }
    }
}

void Server::Send_Game_Chat(ClientInfo *client, Buffer *buf)
{
    ClientInfo *opponent = FindOpponent(client);

    if (opponent != nullptr)
    {
        send(opponent->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
    }

    send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
}

void Server::Send_Server_BRC(ClientInfo *client, Buffer *buf)
{
}

HANDLE Server::GetIOCPHandle()
{
    return this->IOCPHandle;
}

Room *Server::GenerateRoom(char *title, int title_length)
{
    int roomid;

    // 방 ID 생성
    do
    {
        roomid = rand();
    } while (this->room_map.find(roomid) != this->room_map.end());

    Room *temp_room = new Room{};

    EnterCriticalSection(&this->room_arr_cs);
    this->room_map.emplace(roomid, temp_room);
    this->room_vector.push_back(temp_room);
    LeaveCriticalSection(&this->room_arr_cs);

    temp_room->Init(roomid, this->room_vector.size() - 1);

    memcpy_s(temp_room->room_info.room_title, ROOM_TITLE_LEN, title, title_length);
    temp_room->room_info.title_length = title_length;

    return temp_room;
}

void Server::DeleteRoom(ClientInfo *player)
{
    Room *room = FindRoom(player);
    Room *end_room = nullptr;

    if (room == nullptr)
    {
        return;
    }
    else
    {
        end_room = this->room_vector.back();
    }

    if (room != end_room)
    {
        end_room->room_idx = room->room_idx;
        this->room_vector[this->room_vector.size() - 1] = room;
        this->room_vector[room->room_idx] = end_room;
    }

    EnterCriticalSection(&this->room_arr_cs);
    this->room_map.erase(room->room_info.room_id);
    this->room_vector.pop_back();
    LeaveCriticalSection(&this->room_arr_cs);

    delete room;
};

Room *Server::FindRoom(ClientInfo *player)
{
    int room_id = player->cur_room_id;
    Room *room = nullptr;

    auto it = room_map.find(room_id);
    if (it == this->room_map.end())
    {
        cout << "Room is not exist" << endl;
        return nullptr;
    }
    else
    {
        room = it->second;
    }

    return room;
}

void Server::SetHost(ClientInfo *player)
{
    Room *room = FindRoom(player);

    if (room == nullptr)
    {
        return;
    }

    room->host = player;
    room->guest = nullptr;
    return;
}

bool Server::CheckHost(ClientInfo *player)
{
    Room *room = FindRoom(player);

    return room->host == player ? true : false;
}

ClientInfo *Server::FindOpponent(ClientInfo *player)
{
    int room_id = player->cur_room_id;
    Room *room = FindRoom(player);

    auto it = room_map.find(room_id);

    if (room == nullptr)
    {
        return nullptr;
    }

    if (room->host == nullptr || room->guest == nullptr)
    {

        cout << "Opponent is not exist" << endl;
        return nullptr;
    }

    ClientInfo *opponent = room->host == player ? room->guest : room->host;
    return opponent;
};