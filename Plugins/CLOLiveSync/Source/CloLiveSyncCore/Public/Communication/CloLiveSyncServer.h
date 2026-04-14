// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include "CloTcpServer.h"
#include "LiveSyncProtocol.h"

typedef std::function<void(const int _clientCount)> OnClientConnectionCountChanged;

class CLOLIVESYNCCORE_API CloLiveSyncServer : public CloTcpServer
{
public:
    CloLiveSyncServer();
    virtual ~CloLiveSyncServer() override;

    void SendUpdateSceneRequest(const CloLiveSync::USDOptions& _option);
    void SendFilePath(const FString& _filePath);

    bool SetFilePathReceivedCallback(const CloLiveSync::OnRecvUsdFilePath& _callback);
    bool SetClientConnectionCountChangedCallback(const OnClientConnectionCountChanged& _callback);

private:
    virtual void OnClientCountChanged(int _clientCount) override;
    virtual void OnReceiveData(const TArray<unsigned char>& _data) override;

private:
    CloLiveSync::OnRecvUsdFilePath m_FilePathReceivedCallback;
    OnClientConnectionCountChanged m_clientConnectionCountChangedCallback;
};