#include "Constants/struct.h"
#include "Server.h"

ClientInfo::~ClientInfo()
{
    Buffer buf{};
    Server::Instance->SendRoomExit(this, &buf);
    closesocket(client_sock);
    delete over;
}