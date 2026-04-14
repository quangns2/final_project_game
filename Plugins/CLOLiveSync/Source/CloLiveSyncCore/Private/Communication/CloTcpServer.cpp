// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "Communication/CloTcpServer.h"

#include "Async/Async.h"

#include "CloLiveSyncCore.h"

CloTcpServer::CloTcpServer() : m_thread(nullptr)
                                , m_tcpListener(nullptr)
                                , m_isRunning(false)
{
}

CloTcpServer::~CloTcpServer()
{
    StopServer();
}

bool CloTcpServer::StartServer(const FIPv4Address _address, const int32 _port)
{
    FIPv4Endpoint Endpoint(_address, _port);
    m_tcpListener = MakeUnique<FTcpListener>(Endpoint);

    if (!m_tcpListener.IsValid() || !m_tcpListener->IsActive())
    {
        return false;
    }

    m_tcpListener->OnConnectionAccepted().BindRaw(this, &CloTcpServer::HandleClientAccepted);
    m_isRunning = true;
    CreateThread();

    return true;
}

void CloTcpServer::StopServer()
{
    m_isRunning = false;

    if (m_thread)
    {
        m_thread->Kill(true);
        delete m_thread;
        m_thread = nullptr;
    }

    if (m_tcpListener.IsValid())
    {
        m_tcpListener.Reset();
    }

    for (ClientInfo& client : m_clients)
    {
        if (client.socket != nullptr)
        {
            client.socket->Close();
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(client.socket);
        }
    }
    m_clients.Reset();
}
bool CloTcpServer::IsRunning()
{
    return m_isRunning;
}

void CloTcpServer::SendDataToClientsAsync(const TArray<unsigned char>& _data)
{
    m_pendingSocketSendBuffers.Enqueue(_data);
}

void CloTcpServer::Tick()
{
    // Client registration
    {
        FSocket* clientSocket;
        while (m_pendingClientSockets.Dequeue(clientSocket))
        {
            TSharedPtr<FInternetAddr> ClientAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
            clientSocket->GetPeerAddress(*ClientAddress);
            FString ClientInfoStr = FString::Printf(TEXT("%s:%d"), *ClientAddress->ToString(false), ClientAddress->GetPort());
            UE_LOG(LogCloLiveSyncCore, Log, TEXT("Connecting socket for client: %s"), *ClientInfoStr);
            
            m_clients.Add(ClientInfo{ clientSocket, FPlatformTime::Seconds() });
            OnClientCountChanged(m_clients.Num());
        }
    }

    CheckClientsConnectionState();

    // Receiving data from each client
    {
        for (auto& client : m_clients)
        {
            ReceiveData(client);
        }
    }

    // Sending data to each client
    {
        TArray<unsigned char> data;
        while (m_pendingSocketSendBuffers.Dequeue(data))
        {
            int BytesSent = 0;
            for (auto& client : m_clients)
            {
                client.socket->Send(data.GetData(), data.Num(), BytesSent);
            }
        }
    }
}

bool CloTcpServer::Init()
{
    return true;
}

void CloTcpServer::Stop()
{
    m_isRunning = false;
}

uint32 CloTcpServer::Run()
{
    double LastTime = FPlatformTime::Seconds();
    constexpr float IdealFrameTime = 1.0f / 30.0f;

    while (m_isRunning)
    {
        const double CurrentTime = FPlatformTime::Seconds();

        Tick();
        FPlatformProcess::Sleep(FMath::Max<float>(0.01f, IdealFrameTime - (FPlatformTime::Seconds() - CurrentTime)));
    }

    return 0;
}

void CloTcpServer::CreateThread()
{
    const TCHAR* ThreadName = TEXT("CloTcpServerThread");
    m_thread = FRunnableThread::Create(this, ThreadName, 0, TPri_Normal);
}

void CloTcpServer::CheckClientsConnectionState()
{
    for (int32 Index = m_clients.Num() - 1; Index >= 0; --Index)
    {
        ClientInfo& client = m_clients[Index];
        if (client.socket == nullptr)
        {
            continue;
        }

        bool bShouldClose = false;
        int32 BytesRead;
        uint8 Dummy;
        if (!client.socket->Recv(&Dummy, sizeof(Dummy), BytesRead, ESocketReceiveFlags::Peek))
        {
            FString ClientInfoStr;
            TSharedPtr<FInternetAddr> ClientAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
            client.socket->GetPeerAddress(*ClientAddress);
            ClientInfoStr = FString::Printf(TEXT("%s:%d"), *ClientAddress->ToString(false), ClientAddress->GetPort());

            client.socket->Close();
            ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(client.socket);
            m_clients.RemoveAt(Index);

            UE_LOG(LogCloLiveSyncCore, Warning, TEXT("Closing socket for client: %s"), *ClientInfoStr);

        	OnClientCountChanged(m_clients.Num());
        }
    }
}


bool CloTcpServer::HandleClientAccepted(FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint)
{
    if (!ClientSocket)
    {
        return false;
    }

    ClientSocket->SetNoDelay(true);
    ClientSocket->SetNonBlocking(true);
    m_pendingClientSockets.Enqueue(ClientSocket);

    return true;
}

void CloTcpServer::ReceiveData(ClientInfo& _clientInfo)
{
    if ((_clientInfo.socket == nullptr) || (_clientInfo.socket->GetConnectionState() != ESocketConnectionState::SCS_Connected))
    {
        return;
    }

    uint32 size = 0;
    if (_clientInfo.socket->HasPendingData(size))
    {
        TArray<unsigned char> receivedData;
        receivedData.SetNumUninitialized(FMath::Min(size, 65507u));

        int32 read = 0;
        _clientInfo.socket->Recv(receivedData.GetData(), receivedData.Num(), read);
        _clientInfo.lastCommunicationTime = FPlatformTime::Seconds();

        if (read > 0)
        {
            if (receivedData.Num() > 0)
            {
                OnReceiveData(receivedData);
            }
        }
    }
}
