// Copyright 2025 CLO Virtual Fashion. All rights reserved.

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace CloLiveSync
{
    constexpr int LiveSyncServerPort = 34448;
    constexpr unsigned char CurrentLiveSyncPacketVersion = 1;

    enum class LiveSyncMessage : int
    {
        UPDATE_SCENE = 0,
        IMPORT_EXTERN_AVATAR,
        NONE                            // You can add new Protocols on top of NONE.
    };

    enum class AvatarAnimationType : unsigned char
    {
        Joint,
        Cache
    };

    enum class AnimationRegion : unsigned char
    {
        PlayRegion,
        EntireRegion
    };

#pragma pack(push, 1)
    struct PacketHeader
    {
        PacketHeader()
            : Signature("LSCOM")
            , Message(LiveSyncMessage::NONE)
            , TotalLength(sizeof(PacketHeader))
            , PacketVersion(CurrentLiveSyncPacketVersion)
            , Reserved("\0")
        {}

        char Signature[6];
        LiveSyncMessage Message;
        unsigned long long TotalLength;
        unsigned char PacketVersion;
        unsigned char Reserved[15];
    };

    struct USDOptions {
        USDOptions()
            : bAssignAvatar(true)
            , avatarPrimPath("/Avatar")
            , bAssignGarment(true)
            , garmentPrimPath("/Garment")
            , bAssignSceneNProp(true)
            , sceneNPropPrimPath("/SceneAndProp")
            , bIncludeGarment(true)
            , bSingleObject(true)
            , bThin(true)
            , bUnifiedUVCoordinates(false)
            , bIncludeSimulationData(false)
            , bIncludeGarmentCacheAnimation(false)
            , bIncludeSeamPuckeringNormalMap(false)
            , bIncludeAvatar(true)
            , bIncludeAvatarAnimation(true)
            , avatarAnimationType(AvatarAnimationType::Joint)
            , bIncludeSceneAndProps(false)
            , animationRegion(AnimationRegion::PlayRegion)
            , bBakeJointAnimFramesOnFPSMismatch(false)
        {}

        // Prim
        bool bAssignAvatar;
        std::string avatarPrimPath;
        bool bAssignGarment;
        std::string garmentPrimPath;
        bool bAssignSceneNProp;
        std::string sceneNPropPrimPath;

        // Garment
        bool bIncludeGarment;
        bool bSingleObject;
        bool bThin;
        bool bUnifiedUVCoordinates;
        bool bIncludeSimulationData;
        bool bIncludeGarmentCacheAnimation;
        bool bIncludeSeamPuckeringNormalMap;

        // Avatar
        bool bIncludeAvatar;
        bool bIncludeAvatarAnimation;
        AvatarAnimationType avatarAnimationType;

        // Scene and Props
        bool bIncludeSceneAndProps;

        // Animation
        AnimationRegion animationRegion;
        bool bBakeJointAnimFramesOnFPSMismatch;


        bool Serialize(std::vector<unsigned char>& _outBuffer) const
        {
            auto appendBool = [&](bool value)
            {
                _outBuffer.push_back(static_cast<unsigned char>(value));
            };
            auto appendString = [&](const std::string& str)
            {
                uint32_t length = static_cast<uint32_t>(str.size());
                _outBuffer.insert(_outBuffer.end(), reinterpret_cast<const unsigned char*>(&length), reinterpret_cast<const unsigned char*>(&length) + sizeof(length));
                _outBuffer.insert(_outBuffer.end(), str.begin(), str.end());
            };
            auto appendEnum = [&](auto type)
            {
                _outBuffer.push_back(static_cast<unsigned char>(type));
            };

            appendBool(bAssignAvatar);
            appendString(avatarPrimPath);
            appendBool(bAssignGarment);
            appendString(garmentPrimPath);
            appendBool(bAssignSceneNProp);
            appendString(sceneNPropPrimPath);
            appendBool(bIncludeGarment);
            appendBool(bSingleObject);
            appendBool(bThin);
            appendBool(bUnifiedUVCoordinates);
            appendBool(bIncludeSimulationData);
            appendBool(bIncludeGarmentCacheAnimation);
            appendBool(bIncludeAvatar);
            appendBool(bIncludeAvatarAnimation);
            appendEnum(avatarAnimationType);
            appendBool(bIncludeSceneAndProps);
            appendBool(bIncludeSeamPuckeringNormalMap);
            appendEnum(animationRegion);
            appendBool(bBakeJointAnimFramesOnFPSMismatch);

            return true;
        }

        bool Deserialize(const std::vector<unsigned char>& data) {
            size_t offset = 0;

            auto extractBool = [&data, &offset]()
            {
                if (offset >= data.size())
                {
                    return false;
                }

                return static_cast<bool>(data[offset++]);
            };

            auto extractString = [&data, &offset]() -> std::string
            {
                if (offset + sizeof(uint32_t) > data.size())
                {
                    return "";
                }

                uint32_t length;
                memcpy(&length, &data[offset], sizeof(length));
                offset += sizeof(length);

                if (offset + length > data.size())
                {
                    return "";
                }

                std::string str(reinterpret_cast<const char*>(&data[offset]), length);
                offset += length;
                return str;
            };

            auto extractAvatarAnimationType = [&data, &offset]() -> AvatarAnimationType
            {
                if (offset >= data.size())
                {
                    return AvatarAnimationType::Joint;
                }
                unsigned char enumValue = data[offset++];
                return static_cast<AvatarAnimationType>(enumValue);
            };

            auto extractAnimationRegion = [&data, &offset]() -> AnimationRegion
            {
                if (offset >= data.size())
                {
                    return AnimationRegion::PlayRegion;
                }
                unsigned char enumValue = data[offset++];
                return static_cast<AnimationRegion>(enumValue);
            };

            if (offset >= data.size())
            {
                return false;
            }

            bAssignAvatar = extractBool();
            avatarPrimPath = extractString();
            bAssignGarment = extractBool();
            garmentPrimPath = extractString();
            bAssignSceneNProp = extractBool();
            sceneNPropPrimPath = extractString();
            bIncludeGarment = extractBool();
            bSingleObject = extractBool();
            bThin = extractBool();
            bUnifiedUVCoordinates = extractBool();
            bIncludeSimulationData = extractBool();
            bIncludeGarmentCacheAnimation = extractBool();
            bIncludeAvatar = extractBool();
            bIncludeAvatarAnimation = extractBool();
            avatarAnimationType = extractAvatarAnimationType();
            bIncludeSceneAndProps = extractBool();
            bIncludeSeamPuckeringNormalMap = extractBool();
            animationRegion = extractAnimationRegion();
            bBakeJointAnimFramesOnFPSMismatch = extractBool();

            return true;
        }
    };
#pragma pack(pop)

    typedef std::function<void(const std::vector<unsigned char>& _data)> OnRecvData;
    typedef std::function<void(const USDOptions& _usdOptions)> OnRecvUpdate;
    typedef std::function<void(const std::string& _usdUtf8FilePath)> OnRecvUsdFilePath;
};