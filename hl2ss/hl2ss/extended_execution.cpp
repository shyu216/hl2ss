
#include <Windows.h>
#include <atomic>
#include "extended_execution.h"
#include "queue.h"
#include "lock.h"
#include "log.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.ExtendedExecution.Foreground.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Popups.h>
#include <winrt/Windows.Storage.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::ApplicationModel;
using namespace winrt::Windows::ApplicationModel::Core;
using namespace winrt::Windows::ApplicationModel::ExtendedExecution::Foreground;
using namespace winrt::Windows::UI::Core;
using namespace winrt::Windows::UI::Popups;
using namespace winrt::Windows::Storage;

struct InterfaceConfiguration
{
    char host[16];
    std::atomic<int32_t> priority;
    std::atomic<uint32_t> wait_client;
    std::atomic<bool> disable_server;
    std::atomic<bool> enable_client;
    uint8_t _reserved[2];
};

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------

static wchar_t const* const g_name_flat = L"flat_mode.cfg";
static wchar_t const* const g_name_quiet = L"quiet_mode.cfg";
static wchar_t const* const g_name_interface_configuration = L"hl2ss.txt";

static CRITICAL_SECTION g_lock; // DeleteCriticalSection
static HANDLE g_event; // CloseHandle
static std::queue<winrt::hstring> g_mailbox;
static HANDLE g_thread; // CloseHandle
static ExtendedExecutionForegroundSession g_eefs = nullptr;
static bool g_status = false;
static SRWLOCK g_interface_lock[INTERFACE_SLOTS];
static InterfaceConfiguration g_interface_configuration[INTERFACE_SLOTS];
static long g_log_error = 0;
static std::atomic<bool> g_encoder_buffering = false;
static std::atomic<bool> g_reader_buffering = false;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

// OK
static DWORD WINAPI ExtendedExecution_MailboxService(void* param)
{
    (void)param;

    do
    {
    WaitForSingleObject(g_event, INFINITE);
    winrt::hstring message = L"";
    {
    CriticalSection cs(&g_lock);
    if (g_mailbox.size() <= 0) { continue; }	
    while (!g_mailbox.empty()) { message = message + pull(g_mailbox) + L"\n"; }
    }
    IAsyncOperation<IUICommand> ao;
    ExtendedExecution_RunOnMainThread([&]() { ao = MessageDialog(message, L"HL2SS").ShowAsync(); });
    ao.get();
    }
    while (true);
}

// OK
static void ExtendedExecution_OnRevoked(IInspectable const& sender, ExtendedExecutionForegroundRevokedEventArgs const& args)
{
    (void)sender;
    ShowMessage("EEFS Revoked: %d", static_cast<int>(args.Reason()));
    g_status = false;
}

// OK
static void ExtendedExecution_SetFileRegister(winrt::hstring option, bool value)
{
    StorageFolder folder = ApplicationData::Current().LocalFolder();
    winrt::hstring name{ option };

    try
    {
    if (value)
    {
    folder.CreateFileAsync(name).get();
    }
    else
    {
    StorageFile file = folder.GetFileAsync(name).get();
    file.DeleteAsync().get();
    }
    }
    catch (...)
    {
    }
}

// OK
static bool ExtendedExecution_GetFileRegister(winrt::hstring option)
{
    StorageFolder folder = ApplicationData::Current().LocalFolder();
    winrt::hstring name{ option };

    try
    {
    folder.GetFileAsync(name).get();
    return true;
    }
    catch (...)
    {
    }

    return false;
}

// OK
static void ExtendedExecution_ResetInterfaceConfiguration(int id)
{
    InitializeSRWLock(&g_interface_lock[id]);
    memset(g_interface_configuration[id].host, 0, sizeof(InterfaceConfiguration::host));
    g_interface_configuration[id].priority = THREAD_PRIORITY_NORMAL;
    g_interface_configuration[id].wait_client = 1000;
    g_interface_configuration[id].disable_server = false;
    g_interface_configuration[id].enable_client = false;
    memset(g_interface_configuration[id]._reserved, 0, sizeof(InterfaceConfiguration::_reserved));
}

// OK
void ExtendedExecution_Initialize()
{
    for (int id = 0; id < INTERFACE_SLOTS; ++id) { ExtendedExecution_ResetInterfaceConfiguration(id); }

    InitializeCriticalSection(&g_lock);
    g_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    g_thread = CreateThread(NULL, 0, ExtendedExecution_MailboxService, NULL, 0, NULL);
}

// OK
void ExtendedExecution_Request()
{
    g_eefs = ExtendedExecutionForegroundSession();
    g_eefs.Reason(ExtendedExecutionForegroundReason::Unconstrained);
    g_eefs.Description(L"Background Capture");
    g_eefs.Revoked(ExtendedExecution_OnRevoked);
    g_status = g_eefs.RequestExtensionAsync().get() == ExtendedExecutionForegroundResult::Allowed;
    ShowMessage("EEFS Result: %d", g_status);
}

// OK
void ExtendedExecution_GetApplicationVersion(uint16_t data[4])
{
    PackageVersion version = Package::Current().Id().Version();

    data[0] = version.Major;
    data[1] = version.Minor;
    data[2] = version.Build;
    data[3] = version.Revision;
}

// OK
void ExtendedExecution_RunOnMainThread(std::function<void()> f)
{
    CoreApplication::MainView().Dispatcher().RunAsync(CoreDispatcherPriority::High, f).get();
}

// OK
void ExtendedExecution_EnterException(Exception e)
{
    InterlockedOr(&g_log_error, static_cast<long>(e));
}

// OK
Exception ExtendedExecution_GetExceptions()
{
    return static_cast<Exception>(g_log_error);
}

// OK
void ExtendedExecution_MessageBox(winrt::hstring message)
{
    {
    CriticalSection cs(&g_lock);
    g_mailbox.push(message);
    }
    SetEvent(g_event);
}

// OK
void ExtendedExecution_SetFlatMode(bool flat)
{
    ExtendedExecution_SetFileRegister(g_name_flat, flat);
}

// OK
bool ExtendedExecution_GetFlatMode()
{
    return ExtendedExecution_GetFileRegister(g_name_flat);
}

// OK
void ExtendedExecution_SetInterfacePriority(uint32_t id, int32_t priority)
{
    if (id >= INTERFACE_SLOTS) { return; }
    if ((priority < THREAD_PRIORITY_LOWEST) || (priority > THREAD_PRIORITY_HIGHEST)) { return; }
    g_interface_configuration[id].priority = priority;
}

// OK
int32_t ExtendedExecution_GetInterfacePriority(uint32_t id)
{
    if (id >= INTERFACE_SLOTS) { return THREAD_PRIORITY_NORMAL; }
    return g_interface_configuration[id].priority;
}

// OK
void ExtendedExecution_SetEncoderBuffering(bool enable)
{
    g_encoder_buffering = enable;
}

// OK
bool ExtendedExecution_GetEncoderBuffering()
{
    return g_encoder_buffering;
}

// OK
void ExtendedExecution_SetReaderBuffering(bool enable)
{
    g_reader_buffering = enable;
}

// OK
bool ExtendedExecution_GetReaderBuffering()
{
    return g_reader_buffering;
}

// OK
void ExtendedExecution_SetQuietMode(bool quiet)
{
    ExtendedExecution_SetFileRegister(g_name_quiet, quiet);
}

// OK
bool ExtendedExecution_GetQuietMode()
{
    return ExtendedExecution_GetFileRegister(g_name_quiet);
}

// OK
void ExtendedExecution_SetInterfaceHost(uint32_t id, std::string const& host)
{
    if (id >= INTERFACE_SLOTS) { return; }
    int const max_chars = sizeof(InterfaceConfiguration::host) - 1;
    int chars = host.length() < max_chars ? static_cast<int>(host.length()) : max_chars;
    SRWLock srw(&g_interface_lock[id], true);    
    memcpy(g_interface_configuration[id].host, host.c_str(), chars);
    g_interface_configuration[id].host[chars] = '\0';
}

// OK
std::string ExtendedExecution_GetInterfaceHost(uint32_t id)
{
    if (id >= INTERFACE_SLOTS) { return ""; }
    SRWLock srw(&g_interface_lock[id], false);
    return g_interface_configuration[id].host;
}

// OK
void ExtendedExecution_SetInterfaceServerDisable(uint32_t id, bool disable)
{
    if (id >= INTERFACE_SLOTS) { return; }
    g_interface_configuration[id].disable_server = disable;
}

// OK
bool ExtendedExecution_GetInterfaceServerDisable(uint32_t id)
{
    if (id >= INTERFACE_SLOTS) { return false; }
    return g_interface_configuration[id].disable_server;
}

// OK
void ExtendedExecution_SetInterfaceClientEnable(uint32_t id, bool enable)
{
    if (id >= INTERFACE_SLOTS) { return; }
    g_interface_configuration[id].enable_client = enable;
}

// OK
bool ExtendedExecution_GetInterfaceClientEnable(uint32_t id)
{
    if (id >= INTERFACE_SLOTS) { return false; }
    return g_interface_configuration[id].enable_client;
}

// OK
void ExtendedExecution_SetInterfaceWaitClient(uint32_t id, uint32_t wait_client)
{
    if (id >= INTERFACE_SLOTS) { return; }
    g_interface_configuration[id].wait_client = wait_client;
}

// OK
uint32_t ExtendedExecution_GetInterfaceWaitClient(uint32_t id)
{
    if (id >= INTERFACE_SLOTS) { return 0; }
    return g_interface_configuration[id].wait_client;
}

// OK
void ExtendedExecution_LoadInterfaceConfiguration()
{
    std::string host;
    bool disable_server;
    bool enable_client;
    uint32_t wait_client;

    try
    {
    StorageFile file = winrt::Windows::Storage::ApplicationData::Current().LocalFolder().GetFileAsync(g_name_interface_configuration).get();
    auto lines = FileIO::ReadLinesAsync(file).get();
    int size = lines.Size();
    if (size < 4) { throw std::runtime_error("Incomplete CFG file"); }

    host           =      winrt::to_string(lines.GetAt(0));
    disable_server = stoi(winrt::to_string(lines.GetAt(1))) != 0;
    enable_client  = stoi(winrt::to_string(lines.GetAt(2))) != 0;
    wait_client    = stoi(winrt::to_string(lines.GetAt(3)));
    
    for (int id = 0; id < INTERFACE_SLOTS; ++id)
    {
    ExtendedExecution_SetInterfaceHost(id, host);
    ExtendedExecution_SetInterfaceServerDisable(id, disable_server);
    ExtendedExecution_SetInterfaceClientEnable(id, enable_client);
    ExtendedExecution_SetInterfaceWaitClient(id, wait_client);
    }

    ShowMessage("Loaded %S : host=%s disable_server=%d enable_client=%d wait_client=%d", g_name_interface_configuration, host.c_str(), disable_server, enable_client, wait_client);
    }
    catch (...)
    {
    ShowMessage("Failed to load %S", g_name_interface_configuration);
    }
}
