
#include <Windows.h>

#include "ipl.h"
#include "server.h"
#include "message_queue.h"
#include "lock.h"
#include "log.h"

#define HL2SS_PLUGIN_EXPORT extern "C" __declspec(dllexport)

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

// OK
HL2SS_PLUGIN_EXPORT
void InitializeStreams(uint32_t enable)
{
    if (enable)
    {
        HL2SS_Load(false);
    }
    else
    {
        HL2SS_Unload();
    }
}

// OK
HL2SS_PLUGIN_EXPORT
void InitializeStreamsOnUI(uint32_t enable)
{
    if (enable)
    {
        HL2SS_Load(false);
    }
    else
    {
        HL2SS_Unload();
    }
}

// OK
HL2SS_PLUGIN_EXPORT
void DebugMessage(char const* str)
{
    ShowMessage("%s", str);
}

// OK
HL2SS_PLUGIN_EXPORT
void GetLocalIPv4Address(wchar_t *buffer, int size)
{
    wcscpy_s(buffer, size / sizeof(wchar_t), L"0.0.0.0");
}

// OK
HL2SS_PLUGIN_EXPORT
int OverrideWorldCoordinateSystem(void* scs_ptr)
{
    return true;
}

// OK
HL2SS_PLUGIN_EXPORT
void CheckExceptions()
{
    HL2SS_Process_EE();
}

//-----------------------------------------------------------------------------
// Message Queue
//-----------------------------------------------------------------------------

// OK
HL2SS_PLUGIN_EXPORT
uint32_t MQ_SI_Peek()
{
    return MessageQueue_Server_RX_Peek();
}

// OK
HL2SS_PLUGIN_EXPORT
void MQ_SI_Pop(uint32_t& command, uint8_t* data)
{
    MessageQueue_Server_RX_Pull(command, data);
}

// OK
HL2SS_PLUGIN_EXPORT
void MQ_SO_Push(uint32_t id)
{
    MessageQueue_Server_TX_Push(id);
}

// OK
HL2SS_PLUGIN_EXPORT
void MQ_Restart()
{
    MessageQueue_Server_Restart();
}

// OK
HL2SS_PLUGIN_EXPORT
uint32_t MQX_CO_Peek()
{
    return MessageQueue_Client_RX_Peek();
}

// OK
HL2SS_PLUGIN_EXPORT 
void MQX_CO_Pop(uint32_t& id)
{
    MessageQueue_Client_RX_Pull(id);
}

// OK
HL2SS_PLUGIN_EXPORT 
void MQX_CI_Push(uint32_t command, uint32_t size, uint8_t const* data)
{
    MessageQueue_Client_TX_Push(command, size, data);
}

// OK
HL2SS_PLUGIN_EXPORT 
void MQX_Restart()
{
    MessageQueue_Client_Restart();
}

//-----------------------------------------------------------------------------
// Lock
//-----------------------------------------------------------------------------

// OK
HL2SS_PLUGIN_EXPORT
void NamedMutex_Destroy(void* p)
{
}

// OK
HL2SS_PLUGIN_EXPORT
void* NamedMutex_Create(wchar_t const* name)
{
    return NULL;
}

// OK
HL2SS_PLUGIN_EXPORT
int NamedMutex_Acquire(void* p, uint32_t timeout)
{
    return true;
}

// OK
HL2SS_PLUGIN_EXPORT
int NamedMutex_Release(void* p)
{
    return true;
}

//-----------------------------------------------------------------------------
// PV
//-----------------------------------------------------------------------------

// OK
HL2SS_PLUGIN_EXPORT
void PersonalVideo_RegisterNamedMutex(wchar_t const* name)
{
}

//-----------------------------------------------------------------------------
// EV
//-----------------------------------------------------------------------------

// OK
HL2SS_PLUGIN_EXPORT
void ExtendedVideo_RegisterNamedMutex(wchar_t const* name)
{
}
