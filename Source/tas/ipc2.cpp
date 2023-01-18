#include "ipc2.hpp"
#include "libtasquake/boost_ipc.hpp"
#include "libtasquake/io.hpp"
#include "cpp_quakedef.hpp"
#include "ipc_prediction.hpp"
#include "optimizer_quake.hpp"
#include "real_prediction.hpp"

static ipc::server server;
static ipc::client client;
static double ping_interval = 5;

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

void TASQuake::SV_BroadCastMessage(void* ptr, uint32_t length) {
    std::vector<size_t> connections;
    server.get_sessions(connections);

    for(auto& connection_id : connections) {
        server.send_message(connection_id, ptr, length);
    }
}

void TASQuake::SV_SendMessage(size_t connection_id, void* ptr, uint32_t length) {
    server.send_message(connection_id, ptr, length);
}

void TASQuake::CL_SendMessage(void* ptr, uint32_t length) {
    client.send_message(ptr, length);
}

void TASQuake::SV_SendRun(const OptimizerRun& run) {
    auto writer = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)IPCMessages::OptimizerRun;
    writer.WriteBytes(&type, 1);
    run.WriteToBuffer(writer);
    SV_BroadCastMessage(writer.m_pBuffer->ptr, writer.m_uFileOffset);
}

std::int64_t TASQuake::Get_First_Session() {
    std::vector<size_t> connections;
    server.get_sessions(connections);

    if(connections.empty()) {
        return -1;
    } else {
        return connections[0];
    }
}

static void Send_Ping(size_t connection_id) {
    auto iface = TASQuakeIO::BufferWriteInterface::Init();
    uint8_t type = (uint8_t)TASQuake::IPCMessages::Print;
    iface.WriteBytes(&type, 1);
    iface.WriteBytes("PING!", 6);
    server.send_message(connection_id, iface.m_pBuffer->ptr, iface.m_uFileOffset);
}

static void Handle_Server_Message(ipc::Message& msg) {
    uint8_t type;
    if(msg.length < 1) {
        Con_Printf("IPC message was empty!\n");
        return;
    }
    memcpy(&type, msg.address, sizeof(uint8_t));

    switch((TASQuake::IPCMessages)type) {
        case TASQuake::IPCMessages::OptimizerProgress:
            TASQuake::MultiGame_ReceiveProgress(msg);
            break;
        case TASQuake::IPCMessages::OptimizerRun:
            TASQuake::MultiGame_ReceiveRun(msg);
            break;
        case TASQuake::IPCMessages::Predict:
            IPC_Prediction_Read_Response(msg);
            break;
        case TASQuake::IPCMessages::OptimizerGoal:
            TASQuake::MultiGame_ReceiveGoal(msg);
            break;
        default:
            Con_Printf("IPC message with unknown type %d", (int)type);
            break;
    }
}


static void Handle_Server() {
    if(!server.m_bConnected) {
        return;
    }

    std::vector<ipc::Message> messages;
    std::vector<size_t> connections;
    server.get_messages(messages);

    for(auto& msg : messages) {
        Handle_Server_Message(msg);
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

    switch((TASQuake::IPCMessages)type) {
        case TASQuake::IPCMessages::OptimizerRun:
            TASQuake::Receive_Optimizer_Run(msg);
            break;
        case TASQuake::IPCMessages::Print:
            Con_Printf("IPC message: %s\n", (char*)msg.address + 1);
            break;
        case TASQuake::IPCMessages::Predict:
            GamePrediction_Receive_IPC(msg);
            break;
        case TASQuake::IPCMessages::OptimizerTask:
            TASQuake::Receive_Optimizer_Task(msg);
            break;
        case TASQuake::IPCMessages::OptimizerStop:
            TASQuake::Receive_Optimizer_Stop();
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
