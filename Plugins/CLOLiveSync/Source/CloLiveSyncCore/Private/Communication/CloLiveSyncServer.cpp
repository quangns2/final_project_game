// Copyright 2024-2025 CLO Virtual Fashion. All rights reserved.

#include "Communication/CloLiveSyncServer.h"

CloLiveSyncServer::CloLiveSyncServer() : m_FilePathReceivedCallback(nullptr)
{
}

CloLiveSyncServer::~CloLiveSyncServer()
{
}

void CloLiveSyncServer::SendUpdateSceneRequest(const CloLiveSync::USDOptions& _option)
{
    std::vector<unsigned char> optionBuffer;
    _option.Serialize(optionBuffer);

    CloLiveSync::PacketHeader header;
    header.Message = CloLiveSync::LiveSyncMessage::UPDATE_SCENE;
    header.TotalLength = sizeof(CloLiveSync::PacketHeader) + optionBuffer.size();

    TArray<unsigned char> sendBuffer;
    sendBuffer.SetNumUninitialized(header.TotalLength);

    FMemory::Memcpy(sendBuffer.GetData(), &header, sizeof(CloLiveSync::PacketHeader));
    FMemory::Memcpy(sendBuffer.GetData() + sizeof(CloLiveSync::PacketHeader), optionBuffer.data(), optionBuffer.size());

    SendDataToClientsAsync(sendBuffer);
}

void CloLiveSyncServer::SendFilePath(const FString& _filePath)
{
    const auto utf8String = StringCast<UTF8CHAR>(*_filePath);
    const std::string path(utf8String.Get(), utf8String.Get() + utf8String.Length());

    const size_t strPathSize = path.size() * sizeof(char);

    CloLiveSync::PacketHeader header;
    header.Message = CloLiveSync::LiveSyncMessage::IMPORT_EXTERN_AVATAR;
    header.TotalLength = sizeof(CloLiveSync::PacketHeader) + strPathSize;

    TArray<unsigned char> sendBuffer;
    sendBuffer.SetNumUninitialized(header.TotalLength);

    FMemory::Memcpy(sendBuffer.GetData(), &header, sizeof(CloLiveSync::PacketHeader));
    FMemory::Memcpy(sendBuffer.GetData() + sizeof(CloLiveSync::PacketHeader), path.data(), strPathSize);

    SendDataToClientsAsync(sendBuffer);
}


bool CloLiveSyncServer::SetFilePathReceivedCallback(const CloLiveSync::OnRecvUsdFilePath& _callback)
{
    if(IsRunning() == true)
    {
        return false;
    }

    m_FilePathReceivedCallback = _callback;

    return true;
}

bool CloLiveSyncServer::SetClientConnectionCountChangedCallback(const OnClientConnectionCountChanged& _callback)
{
    if (IsRunning() == true)
    {
        return false;
    }

    m_clientConnectionCountChangedCallback = _callback;

    return true;
}

void CloLiveSyncServer::OnClientCountChanged(int _clientCount)
{
    if (m_clientConnectionCountChangedCallback != nullptr)
    {
        m_clientConnectionCountChangedCallback(_clientCount);
    }
}

void CloLiveSyncServer::OnReceiveData(const TArray<unsigned char>& _data)
{
    if ((_data.Num() == 0) || (_data.Num() < sizeof(CloLiveSync::PacketHeader)))
    {
        return;
    }

    CloLiveSync::PacketHeader header;
    FMemory::Memcpy(&header, _data.GetData(), sizeof(CloLiveSync::PacketHeader));

    if ((strcmp(header.Signature, "LSCOM") != 0)
        || (static_cast<int>(header.TotalLength) > _data.Num())
        || (_data.Num() == sizeof(CloLiveSync::PacketHeader)))
    {
        return;
    }

    size_t remainingSize = header.TotalLength - sizeof(CloLiveSync::PacketHeader);
    switch (header.Message)
    {
        case CloLiveSync::LiveSyncMessage::UPDATE_SCENE:
        {
            std::string utf8FilePath;
            utf8FilePath.resize(remainingSize);
            FMemory::Memcpy(utf8FilePath.data(), _data.GetData() + sizeof(CloLiveSync::PacketHeader), remainingSize);
            utf8FilePath.push_back('\0');
            if (m_FilePathReceivedCallback != nullptr)
            {
                m_FilePathReceivedCallback(utf8FilePath.c_str());
            }

            break;
        }
    }
}
