#include "Server.h"

void discard_line() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int main()
{
    Server *server = nullptr;
    int input;
    bool Exit = true;
    while (Exit)
    {
        
        system("cls");
        cout << "[1] Run [2] Shutdown [3] Exit" << endl;

        cin >> input;
        switch (input)
        {
        case 1:
            if (server == nullptr)
            {
                server = new Server{};
                server->Initialize();
                cout << "Run Success. " << endl;
            }
            else
            {
                cout << "Server is already running." << endl;
            }
            cout << "Press any key to return to the menu" << endl;
            _getch();
            break;

        case 2:
            if (server == nullptr)
            {
                cout << "Server is not running" << endl;
            }
            else
            {
                delete server;
                server = nullptr;
                cout << "Shut down Success." << endl;
            }
            cout << "Press any key to return to the menu" << endl;
            _getch();
            break;

        case 3:
            if (server != nullptr)
            {
                delete server;
                server = nullptr;
                cout << "Shut down Success." << endl;
            }
            Exit = false;
            break;

        default:
            cout << "Invalid input. Press Any Key to Retry" << endl;
            _getch();
            break;
        }
        discard_line();
    }
    cout << "Press Any Key to Exit" << endl;
    _getch();
}
