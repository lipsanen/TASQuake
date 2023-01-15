#pragma once
#include <cstdint>
#include <vector>
#include "libtasquake/boost_ipc.hpp"
#include "libtasquake/optimizer.hpp"

namespace TASQuake {
    enum class IPCMessages { Print, Predict, OptimizerRun, OptimizerProgress };

    void IPC2_Frame_Hook();
    void Cmd_IPC2_Init();
    void Cmd_IPC2_Stop();
    void Cmd_IPC2_Cl_Connect();
    void Cmd_IPC2_Cl_Disconnect();
    void SV_SendMessage(size_t connection_id, void* ptr, uint32_t length);
    void CL_SendMessage(void* ptr, uint32_t length);
    void SV_SendRun(const OptimizerRun& run); // Send run to all clients
    void CL_SendRun(const OptimizerRun& run); // Send result back to server
    void CL_SendOptimizerProgress(int iterations);
    std::int64_t Get_First_Session();
}
