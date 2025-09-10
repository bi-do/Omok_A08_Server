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

    cout << "Server End" << endl;
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

void Server::SendRoomList(ClientInfo *client, Buffer *buf)
{
    int index = 0;
    int vector_size = this->room_vector.size();

    int count = 0;
    RoomInfo *room_info = nullptr;

    memcpy_s(&index, sizeof(int), buf->buffer, sizeof(int));

    for (int i = index; i < vector_size; i++)
    {
        room_info = &this->room_vector[index]->room_info;
        memcpy_s(buf->buffer + (sizeof(RoomInfo) * count), sizeof(buf->buffer), room_info, sizeof(RoomInfo));
        count++;
    }

    send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
}

void Server::SendRoomCreateResult(ClientInfo *client, Buffer *buf)
{
    Room *room = GenerateRoom();

    room->player_black = client;
    client->cur_room_id = room->room_info.room_id;

    ZeroMemory(buf, sizeof(*buf));

    buf->BufferSet(Protocol::ROOM_CREATE, 1);

    cout << "SendRoomCreateResult" << endl;
    send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
}

void Server::SendRoomJoin(ClientInfo *client, Buffer *buf)
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
        ClientInfo *opponent = room->player_black == client ? room->player_black : room->player_white;

        // Join Send
        buf->BufferSet(Protocol::ROOM_JOIN, 1);
        send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);

        // 상대방 게임 시작 Alert
        buf->BufferSet(Protocol::GAME_START, 1);
        send(opponent->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);

        // 플레이어 게임 시작 Aelrt
        buf->BufferSet(Protocol::GAME_START, 2);
        send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
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
    }
    else
    {
        DeleteRoom(client->cur_room_id);
    }

    // 플레이어에게 Room_Exit의 ACK Send;
    buf->BufferSet(Protocol::ROOM_EXIT, 1);
    send(client->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);

    client->cur_room_id = -1;
}

void Server::SendDoPlay(ClientInfo *client, Buffer *buf)
{
    // 상대방
    ClientInfo *opponent = FindOpponent(client);

    if (opponent == nullptr)
    {
        cout << "oppnent is not exist . invalid call. " << endl;
        return;
    }

    Cell temp_cell{};
    memcpy_s(&temp_cell.row, sizeof(short), buf->buffer, sizeof(short));                 // row                 // row
    memcpy_s(&temp_cell.col, sizeof(short), buf->buffer + sizeof(short), sizeof(short)); // col

    buf->BufferSet(Protocol::GAME_DO, sizeof(Cell), (char *)&temp_cell);

    send(opponent->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
}

void Server::SendGameResult(ClientInfo *client, Buffer *buf)
{
    // 상대방
    ClientInfo *opponent = FindOpponent(client);

    if (opponent == nullptr)
    {
        cout << "oppnent is not exist . invalid call. " << endl;
        return;
    }

    char flag = buf->buffer[0];

    switch ((GameResultState)flag)
    {
    case GameResultState::Win:
        flag = (char)GameResultState::Lose;
        break;
    case GameResultState::Lose:
        flag = (char)GameResultState::Lose;
        break;
    case GameResultState::Draw:
        break;
    }

    buf->BufferSet(Protocol::GAME_RESULT, flag);
    send(opponent->client_sock, (char *)buf, sizeof(buf->header) + buf->header.body_len, 0);
}

HANDLE Server::GetIOCPHandle()
{
    return this->IOCPHandle;
}

Room *Server::GenerateRoom()
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

    return temp_room;
}

void Server::DeleteRoom(int room_id)
{
    Room *room = nullptr;
    Room *end_room = nullptr;

    auto it = this->room_map.find(room_id);
    if (it == this->room_map.end())
    {
        cout << "Room Delete Fail" << endl;
        return;
    }
    else
    {
        room = it->second;
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

ClientInfo *Server::FindOpponent(ClientInfo *player)
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

    if (room->player_black == nullptr || room->player_white == nullptr)
    {

        cout << "Opponent is not exist" << endl;

        return nullptr;
    }

    ClientInfo *opponent = room->player_black == player ? room->player_black : room->player_white;
    return opponent;
};