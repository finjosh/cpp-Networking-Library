#include <iostream>

#include "SFML/Graphics.hpp"

#include "TGUI/TGUI.hpp"
#include "TGUI/Backend/SFML-Graphics.hpp"

#include "include/Networking/SocketUI.hpp"
#include "include/Networking/Client.hpp"
#include "include/Networking/Server.hpp"

#include "Utils/CommandPrompt.hpp"
#include "Utils/TerminatingFunction.hpp"
#include "Utils/Debug/TFuncDisplay.hpp"
#include "Utils/Debug/VarDisplay.hpp"

using namespace std;
using namespace sf;

void addThemeCommands();
void tryLoadTheme(std::list<std::string> themes, std::list<std::string> directories);

int main()
{
    // setup for sfml and tgui
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Networking Library");
    window.setFramerateLimit(144);
    window.setPosition(Vector2i(-8, -8));

    tgui::Gui gui{window};
    // gui.setRelativeView({0, 0, 1920/(float)window.getSize().x, 1080/(float)window.getSize().y});
    tryLoadTheme({"Dark.txt", "Black.txt"}, {"", "Assets/", "themes/", "Themes/", "assets/", "Assets/Themes/", "Assets/themes/", "assets/themes/", "assets/Themes/"});
    // -----------------------

    udp::SocketUI sDisplay(gui, 50001);
    sDisplay.setConnectionVisible();
    sDisplay.setInfoVisible();
    sDisplay.getServer().onDataReceived([&sDisplay](sf::Packet packet, udp::ID id){
        if (packet.endOfPacket()) return;

        std::string str;
        packet >> str;
        Command::Prompt::print(std::to_string(id) + ": " + str);

        sf::Packet temp = udp::Socket::DataPacketTemplate();
        temp << str;
        sDisplay.getServer().sendToAll(packet, {id});
    });
    sDisplay.getClient().onDataReceived([](sf::Packet packet, udp::ID id){ 
        if (packet.endOfPacket()) return;

        std::string str;
        packet >> str;
        Command::Prompt::print(std::to_string(id) + ": " + str); 
    });

    //! Required to initialize VarDisplay and CommandPrompt
    // creates the UI for the VarDisplay
    VarDisplay::init(gui);
    // creates the UI for the CommandPrompt
    Command::Prompt::init(gui);
    addThemeCommands();
    // create the UI for the TFuncDisplay
    TFuncDisplay::init(gui);
    
    //! ---------------------------------------------------
    
    Command::Handler::get().addCommand("send", "Sends a message to all other clients connected if connection is open",
        [&sDisplay](Command::Data* data)
        {
            if (!sDisplay.isConnectionOpen()) return;

            sf::Packet temp = udp::Socket::DataPacketTemplate();
            std::string str;
            while (data->getNumTokens())
            {
                str += data->getToken() + " ";
                data->removeToken();
            }
            temp << str;

            if (sDisplay.isServer())
            {   
                sDisplay.getServer().sendToAll(temp);
            }
            else
            {
                sDisplay.getClient().sendToServer(temp);
            }
        });

    float upkeep = 0.f;

    sf::Clock deltaClock;
    while (window.isOpen())
    {
        EventHelper::Event::ThreadSafe::update();
        window.clear();
        // updating the delta time var
        sf::Time deltaTime = deltaClock.restart();
        upkeep += deltaTime.asSeconds();
        while (const std::optional<sf::Event> event = window.pollEvent())
        {
            //! Required for LiveVar and CommandPrompt to work as intended
            LiveVar::UpdateLiveVars(event.value());
            if (!Command::Prompt::UpdateEvent(event.value()))
                gui.handleEvent(event.value());
            //! ----------------------------------------------------------

            if (event->is<sf::Event::Closed>())
                window.close();
        }
        //! Updates all the vars being displayed
        VarDisplay::Update();
        //! ------------------------------=-----
        //! Updates all Terminating Functions
        TerminatingFunction::UpdateFunctions(deltaTime.asSeconds());
        //* Updates for the terminating functions display
        TFuncDisplay::Update();
        //! ------------------------------

        if (sDisplay.isConnectionOpen() && upkeep >= sDisplay.getSocket()->getTimeout()/4)
        {
            sf::Packet packet = udp::Socket::DataPacketTemplate();
            if (sDisplay.isServer())
                sDisplay.getServer().sendToAll(packet);
            else
                sDisplay.getClient().sendToServer(packet);

            upkeep = 0;
        }

        // draw for tgui
        gui.draw();
        // display for sfml window
        window.display();
    }

    window.close();

    return EXIT_SUCCESS;
}

void addThemeCommands()
{
    Command::Handler::get().addCommand("setTheme", "Function used to set the theme of the UI (The previous outputs in the command prompt will not get updated color)", 
        {Command::helpCommand, "Trying calling one of the sub commands"}, {});
    Command::Handler::get().findCommand("setTheme")
    ->addCommand("default", "(Currently does not work, coming soon) Sets the theme back to default", 
        [](){ 
            tgui::Theme::setDefault(""); //! This does not work due to a tgui issue
        })
    // Dark theme can be found here: https://github.com/finjosh/TGUI-DarkTheme
    .addCommand("dark", "Sets the them to the dark theme", 
        [](){ 
            tgui::Theme::getDefault()->load("themes/Dark.txt"); 
        })
    .addCommand("light", "Sets the them to the light theme", 
        [](){ 
            tgui::Theme::getDefault()->load("themes/Light.txt"); 
        })
    .addCommand("black", "Sets the them to the black theme", 
        [](){ 
            tgui::Theme::getDefault()->load("themes/Black.txt"); 
        })
    .addCommand("grey", "Sets the them to the transparent grey theme", 
        [](){ 
            tgui::Theme::getDefault()->load("themes/TransparentGrey.txt"); 
        });
}

void tryLoadTheme(std::list<std::string> themes, std::list<std::string> directories)
{
    for (auto theme: themes)
    {
        for (auto directory: directories)
        {
            if (std::filesystem::exists(directory + theme))
            {
                tgui::Theme::setDefault(directory + theme);
                return;
            }
        }
    }
}