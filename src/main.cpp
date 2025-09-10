#include "Server.h"

int main()
{
    Server *server = new Server{};
    server->Initialize();
    cout << "Enter EXIT to turn off the server" << endl;
    string a;
    while (1)
    {
        cin >> a;
        if (a == "EXIT")
        {
            break;
        }else{
            cout << "Shutdown Fail. Please Retry" << endl; 
        }
    }
    cout << "Shutdown Success. Press any Key" << endl; 
    _getch();
    delete server;
}
