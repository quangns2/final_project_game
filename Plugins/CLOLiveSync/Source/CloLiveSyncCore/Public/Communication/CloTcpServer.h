// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Common/TcpListener.h"
#include "Containers/Queue.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Sockets.h"

#include "LiveSyncProtocol.h"

struct ClientInfo
{
    FSocket* socket;
    double lastCommunicationTime;
};

class CLOLIVESYNCCORE_API CloTcpServer : public FRunnable
{

public:
    CloTcpServer();
    virtual ~CloTcpServer() override;

    bool StartServer(const FIPv4Address _address = FIPv4Address::InternalLoopback, const int32 _port = CloLiveSync::LiveSyncServerPort);
    void StopServer();
    bool IsRunning();
    void SendDataToClientsAsync(const TArray<unsigned char>& _data);

protected:
    virtual void OnClientCountChanged(int _clientCount) = 0;
    virtual void OnReceiveData(const TArray<unsigned char>& _data) = 0;
    virtual void Tick();

private:
    //~ Begin FRunnable interface
    virtual bool Init() override;
    virtual void Stop() override;
    virtual uint32 Run() override;
    //~ End FRunnable interface

    void CreateThread();
    void CheckClientsConnectionState();
    bool HandleClientAccepted(FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint);
    void ReceiveData(ClientInfo& _clientInfo);

private:
    FRunnableThread* m_thread;
    TUniquePtr<FTcpListener> m_tcpListener;
    TArray<ClientInfo> m_clients;
    TQueue<FSocket*, EQueueMode::Spsc> m_pendingClientSockets;
    TQueue<TArray<unsigned char>, EQueueMode::Spsc> m_pendingSocketSendBuffers;
    std::atomic<bool> m_isRunning;
};
