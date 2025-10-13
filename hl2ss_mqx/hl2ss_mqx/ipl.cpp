
#include "server.h"
#include "message_queue.h"
#include "ipc_mq.h"
#include "ipc_mqx.h"
#include "log.h"

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

static bool g_standalone = false;
static bool g_flat = false;
static bool g_quiet = false;
static bool g_rm = false;
static uint32_t g_exceptions = 0;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

// OK
void HL2SS_Load(bool standalone)
{
    g_standalone = standalone;

    Server_Startup();

    MessageQueue_Server_Startup();
    MessageQueue_Client_Startup();

    MQ_Startup();
    MQX_Startup();
}

// OK
void HL2SS_Unload()
{
    MQX_Cleanup();
    MQ_Cleanup();

    MessageQueue_Client_Cleanup();
    MessageQueue_Server_Cleanup();

    Server_Cleanup();
}

// OK
void HL2SS_Process_HS()
{
}

// OK
void HL2SS_Process_MQ()
{
    uint32_t size = MessageQueue_Server_RX_Peek();
    if (size == MQ_MARKER) { return; }

    uint32_t command;
    char* data = new char[size + 1]; // delete[]
    MessageQueue_Server_RX_Pull(command, data);
    data[size] = '\0';

    switch (command)
    {
    case 0xFFFFFFFE:
        ShowMessage("MQ: %s", data);
        MessageQueue_Server_TX_Push(0);
        break;
    case MQ_MARKER:
        MessageQueue_Server_Restart();
        break;
    default:
        ShowMessage("MQ: Received command %x with size %d", command, size);
        MessageQueue_Server_TX_Push(MQ_MARKER);
        break;
    }

    delete[] data;
}

// OK
void HL2SS_Process_MQX()
{
    char const* const text = "Hello from HoloLens 2";

    static int state = 0;
    uint32_t id;

    switch (state)
    {
    case 0:
        MessageQueue_Client_TX_Push(0xFFFFFFFE, static_cast<uint32_t>(strlen(text)), text);
        state = 1;
        break;
    case 1:
        if (MessageQueue_Client_RX_Peek() == MQ_MARKER) { break; }
        MessageQueue_Client_RX_Pull(id);
        if (id == MQ_MARKER) { MessageQueue_Client_Restart(); } else { ShowMessage("MQX: Received response %x", id); }
        state = 0;
        break;
    }
}

// OK
void HL2SS_Process_EE()
{
}
