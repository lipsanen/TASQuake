#include "ipc2.hpp"
#include "libtasquake/boost_ipc.hpp"
#include "libtasquake/io.hpp"
#include "cpp_quakedef.hpp"

static ipc::server server;
static ipc::client client;
static double ping_interval = 0.5;

enum class IPCMessages { Print };

void TASQuake::Cmd_IPC2_Init() {
    server.start(1996);
}

void TASQuake::Cmd_IPC2_Stop() {
    server.stop();
}

void TASQuake::Cmd_IPC2_Cl_Connect() {
    client.connect("1996");
}

void TASQuake::Cmd_IPC2_Cl_Disconnect() {
    client.disconnect();
}

static void Send_Ping(size_t connection_id) {
    auto iface = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)IPCMessages::Print;
    iface.WriteBytes(&type, 1);
    iface.WriteBytes("PING!", 6);
    server.send_message(connection_id, iface.m_pBuffer->ptr, iface.m_uFileOffset);
}

static void Handle_Server() {
    if(!server.m_bConnected) {
        return;
    }

    std::vector<ipc::Message> messages;
    std::vector<size_t> connections;
    server.get_messages(messages);

    for(auto& msg : messages) {
        free(msg.address);
    }

    static double last_ping = 0;

    double diff = Sys_DoubleTime() - last_ping;
    if(diff > ping_interval) {
        server.get_sessions(connections);

        for(auto& connection : connections) {
            Send_Ping(connection);
        }
        last_ping = Sys_DoubleTime();
    }
}

static void Handle_Client_Message(ipc::Message& msg) {
    uint8_t type;
    if(msg.length < 1) {
        Con_Printf("IPC message was empty!\n");
        return;
    }
    memcpy(&type, msg.address, sizeof(uint8_t));

    switch((IPCMessages)type) {
        case IPCMessages::Print:
            Con_Printf("IPC message: %s\n", (char*)msg.address + 1);
            break;
        default:
            Con_Printf("IPC message with unknown type %d", (int)type);
            break;
    }
}

static void Handle_Client() {
    if(!client.m_bConnected) {
        return;
    }

    std::vector<ipc::Message> messages;
    client.get_messages(messages, 0);

    for(auto& msg : messages) {
        Handle_Client_Message(msg);
        free(msg.address);
    }
}

void TASQuake::IPC2_Frame_Hook() {
    Handle_Server();
    Handle_Client();
}
