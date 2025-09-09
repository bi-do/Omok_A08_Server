#include "Server.h"

int main()
{
    Server *server = new Server{};
    server->Initialize();
    cout << "서버를 종료하고 싶다면 아무키나 입력하세요" << endl;
    _getch();
    delete server;
}
