
#include "server_channel.h"
#include "extended_execution.h"
#include "lock.h"
#include "log.h"

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

// OK
DWORD Channel::Thunk_Entry(void* self)
{
    static_cast<Channel*>(self)->Entry();
    return 0;
}

// OK
Channel::Channel(char const* name, char const* port, uint32_t id) : 
m_name(name), 
m_port(port), 
m_id(id)
{
    m_no_delay   = FALSE;

    m_event_quit = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_thread     = CreateThread(NULL, 0, Thunk_Entry, this, 0, NULL);
}

// OK
Channel::~Channel()
{
    SetEvent(m_event_quit);
    WaitForSingleObject(m_thread, INFINITE);
    CloseHandle(m_thread);
    CloseHandle(m_event_quit);
}

// OK
void Channel::Entry()
{
    m_socket_listen = Server_CreateSocket(m_port);
    if (m_socket_listen == INVALID_SOCKET) { return; }

    bool ok = Startup();
    if (ok) { Loop(); }
    Cleanup();

    closesocket(m_socket_listen);
}

// OK
void Channel::SelectClient()
{
    bool disable_server = ExtendedExecution_GetInterfaceServerDisable(m_id);
    bool enable_client  = ExtendedExecution_GetInterfaceClientEnable(m_id);

    m_socket_client = Server_AcceptClient(m_socket_listen, !enable_client);
    if (m_socket_client != INVALID_SOCKET)
    {
    if (!disable_server) { return; }
    closesocket(m_socket_client);
    m_socket_client = INVALID_SOCKET;
    }
    if (!enable_client) { return; }
    std::string node = ExtendedExecution_GetInterfaceHost(m_id);
    m_socket_client = Server_CreateClient(node.c_str(), m_port);
    if (m_socket_client != INVALID_SOCKET) { return; }
    uint32_t wait_client = ExtendedExecution_GetInterfaceWaitClient(m_id);
    Sleep(wait_client);
}

// OK
void Channel::Loop()
{
    int base_priority;

    ShowMessage("%s: Listening at port %s", m_name, m_port);

    m_event_client = CreateEvent(NULL, TRUE, FALSE, NULL);

    base_priority = GetThreadPriority(GetCurrentThread());

    do
    {
    ShowMessage("%s: Waiting for client", m_name);

    SelectClient();

    if (m_socket_client == INVALID_SOCKET) { continue; }

    Server_SetupClient(m_socket_client, m_no_delay);

    ShowMessage("%s: Client connected", m_name);

    SetThreadPriority(GetCurrentThread(), ExtendedExecution_GetInterfacePriority(m_id));

    Run();

    ResetEvent(m_event_client);

    SetThreadPriority(GetCurrentThread(), base_priority);

    closesocket(m_socket_client);

    ShowMessage("%s: Client disconnected", m_name);
    }
    while (WaitForSingleObject(m_event_quit, 0) == WAIT_TIMEOUT);

    CloseHandle(m_event_client);

    ShowMessage("%s: Closed", m_name);
}

// OK
void Channel::SetNoDelay(bool no_delay)
{
    m_no_delay = no_delay * TRUE;
}
