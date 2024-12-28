#include "Networking/SocketUI.hpp"

using namespace udp;

SocketUI::SocketUI(tgui::Gui& gui, PORT serverPort) : 
    m_client(sf::IpAddress::LocalHost, serverPort), m_server(serverPort)
{
    setParent(gui);
    m_initEventCallbacks();
}

SocketUI::SocketUI(tgui::Container::Ptr parent, PORT serverPort) :
    m_client(sf::IpAddress::LocalHost, serverPort), m_server(serverPort)
{
    setParent(parent);
    m_initEventCallbacks();
}

void SocketUI::m_initEventCallbacks()
{
    // Setting up the server and client event callbacks here as we dont want to add one every time we create the UI
    m_server.onConnectionOpen(&SocketUI::m_updateConnectionDisplay, this);
    m_server.onConnectionClose(&SocketUI::m_updateConnectionDisplay, this);
    m_client.onConnectionOpen(&SocketUI::m_updateConnectionDisplay, this);
    m_client.onConnectionClose(&SocketUI::m_updateConnectionDisplay, this);
    m_client.onPasswordRequest(&SocketUI::m_updateConnectionDisplay, this);

    m_server.onConnectionOpen(&SocketUI::m_updateInfo, this);
    m_server.onConnectionClose(&SocketUI::m_updateInfo, this);
    m_client.onConnectionOpen(&SocketUI::m_updateInfo, this);
    m_client.onConnectionClose(&SocketUI::m_updateInfo, this);

    TFunction updateFunc({[this](TData* data)
    {
        if (!m_infoParent)
            return;
        m_updateInfo();
        data->setRunning();
    }});
    m_tFuncID = updateFunc.getTypeid();

    m_server.onConnectionOpen(TFunc::Add, updateFunc, std::numeric_limits<float>().infinity());
    m_client.onConnectionOpen(TFunc::Add, updateFunc, std::numeric_limits<float>().infinity());
    m_server.onConnectionClose(TFunc::remove, m_tFuncID);
    m_client.onConnectionClose(TFunc::remove, m_tFuncID);

    m_server.onClientConnected([this](ID id)
    {
        if (!m_infoParent) return;

        std::string s_id = std::to_string(id);

        auto data = m_server.getClientData(id);
        m_clientData->addItem({"Client Data", s_id, "IP: " + sf::IpAddress(data->id).toString()});
        m_clientData->addItem({"Client Data", s_id, "Port: " + std::to_string(data->port)});
        m_clientData->addItem({"Client Data", s_id, "Packets/s: NA"});
        m_clientData->addItem({"Client Data", s_id, "Last packet (s): NA"});
        m_clientData->addItem({"Client Data", s_id, "Connection Time (s): NA"});
        m_clientData->collapse({"Client Data", s_id});
    });
    m_server.onClientDisconnected([this](ID id)
    {
        if (!m_infoParent) return;

        m_clientData->removeItem({"Client Data", std::to_string(id)}, false);
    });
}

SocketUI::~SocketUI()
{
    m_server.closeConnection();
    m_client.closeConnection();

    // if (m_infoParent != nullptr) m_infoParent->destroy();
    // if (m_connectionParent != nullptr) m_connectionParent->close();
    setConnectionVisible(false); // also removes from gui and cleans up memory for connection display
    setInfoVisible(false);
}

void SocketUI::setParent(tgui::Gui& gui)
{
    SocketUI::setParent(gui.getContainer());
}

void SocketUI::setParent(tgui::Container::Ptr parent)
{
    // Close the display first in case we are changing parents
    setConnectionVisible(false);
    // Close the display first in case we are changing parents
    setInfoVisible(false);
    m_UIParent = parent;
}

tgui::Container::Ptr SocketUI::getParent() const
{
    return m_UIParent;
}

void SocketUI::setConnectionVisible(bool visible)
{
    if (m_connectionParent == nullptr)
    {
        if (!visible) return;

        TGUI_ASSERT(m_UIParent, "Parent must be set before setting visiblity of Connection Display");

        m_connectionParent = tgui::ChildWindow::create("Connection Manager", tgui::ChildWindow::Close);
        m_UIParent->add(m_connectionParent);
        m_connectionParent->onClose(setConnectionVisible, this, false);
        m_connectionParent->setResizable(true);

        //* creating the widgets
        m_serverCheck = tgui::CheckBox::create("Server");
        m_clientCheck = tgui::CheckBox::create("Client");
        m_passCheck = tgui::CheckBox::create("Password");
        m_passEdit = tgui::EditBox::create();
        m_IPEdit = tgui::EditBox::create();
        m_tryOpenConnectionBtn = tgui::Button::create("Try Open Connection");
        m_sendPassword = tgui::Button::create("Send Password");
        m_IPState = tgui::Label::create("Checking IP...");
        m_panel = tgui::ScrollablePanel::create();

        //* setting up widgets
        m_passEdit->setDefaultText("Password");
        m_IPEdit->setDefaultText("Server IP");
        m_connectionParent->add(m_panel);
        m_panel->setSize({"100%", "100%"});

        //* setting up events
        m_serverCheck->onCheck([this](){ m_clientCheck->setChecked(false); m_updateConnectionDisplay(); });
        m_serverCheck->onUncheck([this](){ m_closeConnection(); this->m_resetUIConnectionStates(); m_updateConnectionDisplay(); });
        m_clientCheck->onCheck([this](){ m_serverCheck->setChecked(false); m_updateConnectionDisplay(); });
        m_clientCheck->onUncheck([this](){ m_closeConnection(); this->m_resetUIConnectionStates(); m_updateConnectionDisplay(); });
        m_passCheck->onChange(m_updateConnectionDisplay, this);
        m_passEdit->onTextChange([this](){ 
            if (this->m_socket != nullptr)
            {
                this->m_socket->setPassword(m_passEdit->getText().toStdString());
            }
        });
        m_IPEdit->onReturnOrUnfocus([this](){
            if (this->m_socket != nullptr && !this->isServer())
            {
                if (m_IPEdit->getText() == "")
                {
                    m_client.setServerData(sf::IpAddress::LocalHost);
                    return;
                }

                this->m_validIPState = validIP::checking;

                std::thread* tempThread = new std::thread([this](){
                    std::string pass = m_IPEdit->getText().toStdString();
                    if (Socket::isValidIpAddress(pass))
                    {
                        getClient().setServerData(sf::IpAddress::resolve(pass)); 
                        this->m_validIPState = validIP::valid;
                        return;
                    }
                    this->m_validIPState = validIP::invalid;
                });

                TFunc::Add([this, tempThread](TData* data){
                    if (data->isForceStop() || data->isStopRequested())
                    {
                        tempThread->join(); // wait for the thread to finish (fix this so that the program will not halt)
                    }
                    if (this->m_validIPState == validIP::checking)
                    {
                        data->setRunning();
                        std::string temp = "Checking IP";
                        for (int i = 0; i < int(data->getTotalTime()*2)%3 + 1; i++)
                            temp += '.';

                        this->m_IPState->setText(temp);
                        return;
                    }
                    else if (this->m_validIPState == validIP::invalid)
                    {
                        this->m_IPEdit->setDefaultText("Invalid IP Entered");
                        this->m_IPEdit->setText("");
                    }
                    else
                    {
                        this->m_IPEdit->setDefaultText("Server IP");
                        this->m_IPEdit->setText(this->getClient().getServerIP().value().toString()); // guaranteed to be valid by this point
                    }
                    tempThread->detach();
                    delete(tempThread);
                    this->m_updateConnectionDisplay();
                });
                this->m_updateConnectionDisplay();
            } 
        });
        m_tryOpenConnectionBtn->onClick([this](){
            if (this->isConnectionOpen())
            {
                this->m_closeConnection();
            }
            else
            {
                this->m_tryOpenConnection();
            }
            this->m_updateConnectionDisplay();
        });
        m_sendPassword->onClick([this](){
            this->m_tryOpenConnection();
            this->m_updateConnectionDisplay();
        });

        m_updateConnectionDisplay();
    }

    if (visible)
    {
        m_connectionParent->setVisible(true);
        m_connectionParent->setEnabled(true);
        m_connectionParent->moveToFront();
    }
    else
    {
        m_connectionParent->onClose.setEnabled(false);
        m_connectionParent->close();
        m_connectionParent = nullptr;
        m_serverCheck = nullptr;
        m_clientCheck = nullptr;
        m_passCheck = nullptr;
        m_passEdit = nullptr;
        m_IPEdit = nullptr;
        m_IPState = nullptr;
        m_tryOpenConnectionBtn = nullptr;
        m_sendPassword = nullptr;
        m_panel = nullptr;
    }
}

bool SocketUI::isConnectionVisible() const
{
    return m_connectionParent != nullptr;
}

tgui::ChildWindow::Ptr SocketUI::getConnectionWindow()
{
    return m_connectionParent;
}

void SocketUI::setInfoVisible(bool visible)
{
    if (m_infoParent == nullptr)
    {
        if (!visible)
            return;

        TGUI_ASSERT(m_UIParent, "Parent must be set before setting visiblity of Info Display");

        m_infoParent = tgui::ChildWindow::create("Connection Info", tgui::ChildWindow::Close);
        m_UIParent->add(m_infoParent);

        m_infoParent->setResizable(true);
        m_infoParent->onClose(setInfoVisible, this, false);

        m_list = tgui::ListView::create();
        m_infoParent->add(m_list);
        m_list->setResizableColumns(true);
        m_list->addColumn("Name");
        m_list->setColumnAutoResize(0,true);
        m_list->addColumn("Data");
        m_list->setColumnExpanded(1,true);
        m_list->setMultiSelect(false);
        m_list->setAutoLayout(tgui::AutoLayout::Top);
        m_list->setAutoScroll(false);
        m_list->addItem({"ID", "NA"});
        m_list->addItem({"Public IP", "NA"});
        m_list->addItem({"Local IP", "NA"});
        m_list->addItem({"Port", "NA"});
        m_list->addItem({"Connection Open", "NA"});
        m_list->addItem({"Connection Open Time", "NA"});
        m_list->setSize({"100%", tgui::bindMin(tgui::bindHeight(m_infoParent), m_list->getSizeLayout().y)});
        m_list->onItemSelect(tgui::ListView::deselectItems, m_list);

        m_clientData = tgui::TreeView::create();
        m_infoParent->add(m_clientData);
        m_clientData->setAutoLayout(tgui::AutoLayout::Fill);
        m_clientData->onItemSelect(tgui::TreeView::deselectItem, m_clientData);

        if (isConnectionOpen() && isServer())
        {
            for (auto client: m_server.getClients())
            {
                std::string id = std::to_string(client->id);
                m_clientData->addItem({"Client Data", id, "IP: " + sf::IpAddress(client->id).toString()});
                m_clientData->addItem({"Client Data", id, "Port: " + std::to_string(client->port)});
                m_clientData->addItem({"Client Data", id, "Packets/s: " + std::to_string(client->getPacketsPerSecond())});
                m_clientData->addItem({"Client Data", id, "Last packet (s): " + std::to_string(client->getTimeSinceLastPacket())});
                m_clientData->addItem({"Client Data", id, "Connection Time (s): " + std::to_string(client->getConnectionTime())});
            }
            m_clientData->collapseAll();
        }

        m_updateInfo();
    }

    if (visible)
    {
        m_infoParent->setVisible(visible);
        m_infoParent->setEnabled(visible);
        m_infoParent->moveToFront();
    }
    else
    {
        m_infoParent->onClose.setEnabled(false);
        m_infoParent->close();
        m_infoParent = nullptr;
        m_list = nullptr;
        m_clientData = nullptr;
        TFunc::remove(m_tFuncID);
    }
}

bool SocketUI::isInfoVisible() const
{
    return m_infoParent != nullptr;
}

tgui::ChildWindow::Ptr SocketUI::getInfoWindow()
{
    return m_infoParent;
}

void SocketUI::m_updateInfo()
{
    if (!isInfoVisible())
        return;

    if (m_socket == nullptr) 
    {
        m_list->changeItem(0, {"ID", "NA"});
        m_list->changeItem(1, {"Public IP", "NA"});
        m_list->changeItem(2, {"Local IP", "NA"});
        m_list->changeItem(3, {"Port", "NA"});
        m_list->changeItem(4, {"Connection Open", "NA"});
        m_list->changeItem(5, {"Connection Open Time", "NA"});
    }
    else
    {
        m_list->changeItem(0, {"ID", std::to_string(m_socket->getID())});
        if (m_socket->getIP().has_value())
            m_list->changeItem(1, {"Public IP", m_socket->getIP().value().toString()});
        else
            m_list->changeItem(1, {"Public IP", "Unable to resolve"});

        if (m_socket->getLocalIP().has_value())
            m_list->changeItem(2, {"Local IP", m_socket->getLocalIP().value().toString()});
        else 
            m_list->changeItem(2, {"Local IP", "Unable to resolve"});

        m_list->changeItem(3, {"Port", std::to_string(m_socket->getPort())});
        m_list->changeItem(4, {"Connection Open", (m_socket->isConnectionOpen() ? "True" : "False")});
        m_list->changeItem(5, {"Connection Open Time", std::to_string(m_socket->getConnectionTime())});

        if (m_isServer)
        {
            m_clientData->setVisible(true);
            m_clientData->setEnabled(true);
            if (m_clientData->getNode({"Client Data"}).text == "")
                m_clientData->addItem({"Client Data"});

            for (auto data: m_server.getClients())
            {
                tgui::String id(data->id);

                for (auto leaf: m_clientData->getNode({"Client Data", id}).nodes)
                {
                    if (leaf.text.starts_with("Pa"))
                        m_clientData->changeItem({"Client Data", id, leaf.text}, {"Packets/s: " + std::to_string(data->getPacketsPerSecond())});
                    else if (leaf.text.starts_with("L"))
                        m_clientData->changeItem({"Client Data", id, leaf.text}, {"Last packet (s): " + std::to_string(data->getTimeSinceLastPacket())});
                    else if (leaf.text.starts_with("C"))
                        m_clientData->changeItem({"Client Data", id, leaf.text}, {"Connection Time (s): " + std::to_string(data->getConnectionTime())});
                }
            }
        }
        else
        {
            m_clientData->setVisible(false);
            m_clientData->setEnabled(false);
        }
    }
}

bool SocketUI::isConnectionOpen()
{
    return (m_client.isConnectionOpen() || m_server.isConnectionOpen());
}

bool SocketUI::isServer()
{
    return m_isServer;
}

bool SocketUI::isEmpty()
{
    return (m_socket == nullptr);
}

Server& SocketUI::getServer()
{
    return m_server;
}

Client& SocketUI::getClient()
{
    return m_client;
}

Socket* SocketUI::getSocket()
{
    if (!isConnectionOpen())
        return nullptr;

    if (isServer())
        return &m_server;
    return &m_client;    
}

void SocketUI::m_setServer()
{
    m_isServer = true;
    m_socket = &m_server;
}

void SocketUI::m_setClient()
{
    m_isServer = false;
    m_socket = &m_client;
}

void SocketUI::m_setEmpty()
{
    if (this->getSocket() == nullptr) return;
    m_socket->closeConnection();
    m_socket = nullptr;
}

void SocketUI::m_updateConnectionDisplay()
{
    if (!m_connectionParent)
        return;
    m_panel->removeAllWidgets();

    m_addWidgetToConnection(m_serverCheck);

    // Is a Server
    if (m_serverCheck->isChecked())
    {
        m_setServer();

        m_passCheck->setText("Password");
        m_addWidgetToConnection(m_passCheck, 1);

        if (m_passCheck->isChecked())
        {
            m_addWidgetToConnection(m_passEdit, 2);
        }

        if (getServer().isConnectionOpen())
            m_tryOpenConnectionBtn->setText("Close Server");
        else    
            m_tryOpenConnectionBtn->setText("Open Server");

        m_addWidgetToConnection(m_tryOpenConnectionBtn, 1);

        // adding the client check to the UI
        m_addWidgetToConnection(m_clientCheck);
    }
    // Is a Client
    else if (m_clientCheck->isChecked())
    {
        m_addWidgetToConnection(m_clientCheck);
        this->m_setClient();

        if (isConnectionOpen())
            m_IPEdit->setText(m_client.getServerIP().value().toString());
        m_addWidgetToConnection(m_IPEdit, 1);
        if (m_validIPState == validIP::checking) 
            m_addWidgetToConnection(m_IPState, 1, 5);

        if (m_client.NeedsPassword())
        {
            m_addWidgetToConnection(m_passEdit, 2);
            m_addWidgetToConnection(m_sendPassword, 2);
        }

        if (m_client.isConnectionOpen()) // need to check here as the connection confirmation can take time
            m_tryOpenConnectionBtn->setText("Disconnect");
        else
            m_tryOpenConnectionBtn->setText("Try Connect");
        m_addWidgetToConnection(m_tryOpenConnectionBtn, 1);
    }
    else
    {
        m_addWidgetToConnection(m_clientCheck);
        this->m_setEmpty();
    }
}

void SocketUI::m_addWidgetToConnection(tgui::Widget::Ptr widgetPtr, float indent, float spacing)
{
    float currentHeight = 0;
    auto& widgets = m_panel->getWidgets();
    if (widgets.size() != 0) 
    {
        currentHeight = widgets.back()->getPosition().y + widgets.back()->getSize().y;
    }
     
    if (widgetPtr != nullptr)
    {
        m_panel->add(widgetPtr);
        widgetPtr->setPosition((1 + indent) * 10, currentHeight + spacing);
        if (m_socket && widgetPtr != m_tryOpenConnectionBtn)
            widgetPtr->setEnabled(!m_socket->isConnectionOpen());
    }
}

void SocketUI::m_resetUIConnectionStates()
{
    if (!m_connectionParent || m_socket)
        return;
    m_passCheck->setChecked(false);
    m_passEdit->setText("");
    m_IPEdit->setText("");
}

void SocketUI::m_tryOpenConnection()
{
    if (isEmpty())
        return;
    if (isServer())
    {
        if (m_passCheck->isChecked())
            m_server.setPasswordRequired(true, m_passEdit->getText().toStdString());
        else
            m_server.setPasswordRequired(false);
        m_server.tryOpenConnection();
    }
    else
    {
        if (!m_client.NeedsPassword())
        {
            m_client.tryOpenConnection(); // TODO add some user feed back if this fails
            if (m_client.getServerIP().has_value())
                m_IPEdit->setText(m_client.getServerIP().value().toString());
        }
        else
        {
            m_client.setAndSendPassword(m_passEdit->getText().toStdString());
        }
    }
}

void SocketUI::m_closeConnection()
{
    m_server.closeConnection();
    m_client.closeConnection();
    m_socket = nullptr;
}
