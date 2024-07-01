#ifndef SOCKETUI_H
#define SOCKETUI_h

#pragma once

#include <numeric>
#include <thread>

#include "TGUI/Backend/SFML-Graphics.hpp"
#include "TGUI/Widgets/ChildWindow.hpp"
#include "TGUI/Widgets/TreeView.hpp"
#include "TGUI/Widgets/ListView.hpp"
#include "TGUI/Widgets/EditBox.hpp"
#include "TGUI/Widgets/Button.hpp"
#include "TGUI/Widgets/CheckBox.hpp"
#include "TGUI/Widgets/ScrollablePanel.hpp"
#include "TGUI/Widgets/Label.hpp"

#include "Networking/Client.hpp"
#include "Networking/Server.hpp"
#include "Utils/TerminatingFunction.hpp"

namespace udp
{

// TODO add a close connection history with the reason for a closed connection to the info display
class SocketUI
{
public:
    /// @param gui the gui to add UI to
    SocketUI(tgui::Gui& gui, PORT serverPort);
    /// @param parent the container to add the UI to
    SocketUI(tgui::Container::Ptr parent, PORT serverPort);
    ~SocketUI();

    /// @brief sets the parent that is used when creating UI
    /// @note to create the display just call the visibility function for the wanted display
    void setParent(tgui::Gui& gui);
    /// @brief sets the parent that is used when UI
    /// @note to create the display just call the visibility function for the wanted display
    void setParent(tgui::Container::Ptr parent);
    /// @returns the pointer to the container where all Socket UI will be added
    tgui::Container::Ptr getParent() const;
    
    //* public connection display functions
    /// @note also moves the window to the front
    void setConnectionVisible(bool visible = true);
    bool isConnectionVisible() const;
    /// @warning dont edit any events
    /// @returns the ptr to the connection window (could be null)
    tgui::ChildWindow::Ptr getConnectionWindow();
    ///////////////////////////////////////

    //* public Info display functions
    /// @brief sets the info window visible or not
    /// @note also moves the window to the front
    void setInfoVisible(bool visible = true);
    /// @returns if the info window is visible
    bool isInfoVisible() const;
    /// @warning dont edit any events
    /// @returns the ptr to the info window (could be null)
    tgui::ChildWindow::Ptr getInfoWindow();
    /////////////////////////////////

    /// @returns true if either the client or server has an open connection
    bool isConnectionOpen();
    /// @returns if the current socket is a server
    /// @note if connection is closed this is invalid
    bool isServer();
    /// @returns true if neither a server or client is selected
    bool isEmpty();
    /// @brief might not be in use if isServer is false
    /// @note do NOT every clear all event connections or else the UI WILL break
    Server& getServer();
    /// @brief might not be in use if isServer is true
    /// @note do NOT every clear all event connections or else the UI WILL break
    Client& getClient();
    /// @note nullptr if no socket is in use
    /// @returns the pointer to the socket that is currently in use
    Socket* getSocket();

protected:
    /// @brief updates the info display
    void m_updateInfo();
    // void m_updateUISize();
    void m_setServer();
    void m_setClient();
    /// @brief this is no longer a client or a server
    void m_setEmpty();

    //* Functions for updating the UI in the connection parent
    void m_updateConnectionDisplay();
    /// @brief adds the given widget to the connection display with proper spacing
    /// @param indent the number of indents to move the widget by
    /// @param spacing the space from the last widget
    /// @note for a space put a nullptr in
    void m_addWidgetToConnection(tgui::Widget::Ptr widgetPtr, float indent = 0, float spacing = 10);
    /// @brief resets the state of the checkboxes, edit boxes, and buttons
    void m_resetUIConnectionStates();

    /// @brief tries to open connection with the current data
    void m_tryOpenConnection();
    /// @brief closes both server and clients connection while setting m_socket = nullptr
    void m_closeConnection();

private:
    /// @brief connects the callbacks for the server and client
    void m_initEventCallbacks();

    enum validIP
    {
        valid,
        invalid,
        checking
    };

    validIP m_validIPState = validIP::valid;

    tgui::ChildWindow::Ptr m_infoParent = nullptr;
    tgui::ListView::Ptr m_list = nullptr;
    tgui::TreeView::Ptr m_clientData = nullptr;
    std::string m_tFuncID;

    tgui::Container::Ptr m_UIParent = nullptr;
    tgui::ChildWindow::Ptr m_connectionParent = nullptr;
    tgui::CheckBox::Ptr m_serverCheck = nullptr;
    tgui::CheckBox::Ptr m_clientCheck = nullptr;
    tgui::CheckBox::Ptr m_passCheck = nullptr;
    tgui::EditBox::Ptr m_passEdit = nullptr;
    tgui::EditBox::Ptr m_IPEdit = nullptr;
    tgui::Label::Ptr m_IPState = nullptr;
    // Open / close connection
    tgui::Button::Ptr m_tryOpenConnectionBtn = nullptr;
    tgui::Button::Ptr m_sendPassword = nullptr;
    tgui::ScrollablePanel::Ptr m_panel = nullptr;

    Socket* m_socket = nullptr;
    Server m_server;
    Client m_client;
    bool m_isServer = false;
};

}

#endif
