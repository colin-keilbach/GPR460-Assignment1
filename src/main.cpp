#include "RoboCatPCH.h"

// THIS IS WINDOWS-ONLY
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <iostream>
#include "SDL.h"
#include "SDL_ttf.h"
#include <sstream>
#include "NetworkManager.h"

#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
#define DBG_NEW new
#endif

const SDL_Color MESSAGE_COLOR = SDL_Color{ 255, 255, 255, 255 };

namespace gpr
{
#ifdef _WIN32
    typedef std::wstring string;
#else
    typedef std::string string;
#endif

    class System
    {
    public:
        void Init();
        void FatalError(std::string message);
        void Shutdown();
    };

    void System::Init()
    {
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
        if (!AllocConsole()) FatalError("Failed to alloc!");

        FILE* ret = freopen("CONOUT$", "w", stdout);
        if (ret == NULL) FatalError("Failed to reopen stdout");

        std::cout << "Hi console!\n";
    }

    void System::Shutdown()
    {

    }

    void System::FatalError(std::string message)
    {
        //std::wstringstream wss;
        //wss << message.c_str();
        MessageBox(
            NULL,
            message.c_str(),
            "Error",
            MB_ICONERROR | MB_OK
        );

        ExitProcess(1);
    }
}

void RenderText(SDL_Renderer* renderer, TTF_Font* font, SDL_Color color, const std::string& text, int x, int y)
{
    // Draw to surface and create a texture
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    // Setup the rect for the texture
    SDL_Rect dstRect;
    dstRect.x = x;
    dstRect.y = y;
    SDL_QueryTexture(texture, nullptr, nullptr, &dstRect.w, &dstRect.h);

    // Draw the texture
    SDL_RenderCopy(renderer, texture, nullptr, &dstRect);

    // Destroy the surface/texture
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

int main(int argc, char* argv[])
{
    const int WIDTH = 640;
    const int HEIGHT = 480;
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;

    gpr::System system;
    system.Init();

    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("SDL2 Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    TTF_Init();
    TTF_Font* font = TTF_OpenFont("Assets/Carlito-Regular.TTF", 18);
    if (font == nullptr)
    {
        system.FatalError("Failed to load font");
    }

    std::string currentText;
    currentText.reserve(128);

    MessageLog messageLog;
    NetworkManager networkManager(messageLog);
    networkManager.Init();

    SDL_Event event;
    bool quit = false;
    while (!quit)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quit = true;
            }

            if (event.type == SDL_KEYDOWN)
            {
                char key = event.key.keysym.sym;
                if (key == SDLK_ESCAPE)
                {
                    quit = true;
                }

                // Printable characters
                if (key >= 32 && key <= 126)
                {
                    if (event.key.keysym.mod & KMOD_SHIFT)
                    {
                        // capitalization
                        if (key >= 97 && key <= 122) key -= 0x20;
                        // symbols don't seem to have a nice mapping -- this only
                        // works with QWERTY keyboards, though :/
                        if (key == '1') key = '!';
                        if (key == '2') key = '@';
                        if (key == '3') key = '#';
                        if (key == '4') key = '$';
                        if (key == '5') key = '%';
                        if (key == '6') key = '^';
                        if (key == '7') key = '&';
                        if (key == '8') key = '*';
                        if (key == '9') key = '(';
                        if (key == '0') key = ')';
                        if (key == '`') key = '~';
                        if (key == '-') key = '_';
                        if (key == '=') key = '+';
                        if (key == '/') key = '?';
                        if (key == ';') key = ':';
                    }
                    currentText.push_back(key);
                }

                if (key == SDLK_BACKSPACE)
                {
                    currentText.pop_back();
                }

                if (key == SDLK_RETURN)
                {
                    // Fire event: End of Line
                    std::cout << "Line end detected. Message is: " << currentText << std::endl;
                    std::string connectString("!connect ");
                    auto pos = currentText.rfind(connectString, 0);
                    if (pos == std::string::npos)
                    {
                        // Normal message -- just send it
                        networkManager.SendMessageToPeers(currentText);
                        messageLog.AddMessage(currentText);
                    }
                    else
                    {
                        pos += connectString.length() - 1;
                        std::cout << "Found attempt to connect: '" << currentText.substr(pos + 1) << "'" << std::endl;
                        SocketAddressPtr toConnect = SocketAddressFactory::CreateIPv4FromString(currentText.substr(pos + 1));
                        if (toConnect == nullptr)
                        {
                            std::cout << "Given address isn't valid" << std::endl;
                        }
                        else
                        {
                            networkManager.AttemptToConnect(toConnect);
                        }
                    }
                    currentText.clear();
                }
            }
        }

        // Update scene here
        networkManager.CheckForNewConnections();
        networkManager.PostMessagesFromPeers();

        SDL_SetRenderDrawColor(renderer, 127, 127, 216, 255);
        SDL_RenderClear(renderer);

        // Draw scene here
        RenderText(renderer, font, MESSAGE_COLOR, currentText.c_str(), 0, HEIGHT - 18);

        int penY = 0;
        for (const std::string& message : messageLog)
        {
            RenderText(renderer, font, MESSAGE_COLOR, message.c_str(), 0, penY);
            penY += 18;
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}