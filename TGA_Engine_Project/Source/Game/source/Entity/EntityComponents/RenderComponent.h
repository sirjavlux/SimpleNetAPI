﻿#pragma once

#include "EntityComponent.h"
#include "Utility/NetTag.h"

struct RenderData
{
  uint8_t sortingPriority = 0;
  uint64_t textureHash = 0;
  Tga::SpriteSharedData sharedData = {};
};

inline bool operator<(const RenderData& InLhs, const RenderData& InRhs)
{
  if (InLhs.sortingPriority != InRhs.sortingPriority)
  {
    return InLhs.sortingPriority < InRhs.sortingPriority;
  }
  if (InLhs.textureHash != InRhs.textureHash)
  {
    return InLhs.textureHash < InRhs.textureHash;
  }
  if (InLhs.sharedData.myBlendState != InRhs.sharedData.myBlendState)
  {
    return InLhs.sharedData.myBlendState < InRhs.sharedData.myBlendState;
  }
  if (InLhs.sharedData.mySamplerFilter != InRhs.sharedData.mySamplerFilter)
  {
    return InLhs.sharedData.mySamplerFilter < InRhs.sharedData.mySamplerFilter;
  }
  if (InLhs.sharedData.mySamplerAddressMode != InRhs.sharedData.mySamplerAddressMode)
  {
    return InLhs.sharedData.mySamplerAddressMode < InRhs.sharedData.mySamplerAddressMode;
  }
  
  return false;
}

inline bool operator==(const RenderData& InLhs, const RenderData& InRhs)
{
  return InLhs.textureHash == InRhs.textureHash
    && InLhs.sharedData.myBlendState == InRhs.sharedData.myBlendState
    && InLhs.sharedData.mySamplerFilter == InRhs.sharedData.mySamplerFilter
    && InLhs.sharedData.mySamplerAddressMode == InRhs.sharedData.mySamplerAddressMode;
}

class RenderComponent : public EntityComponent
{
public:
  void Init() override;
  
  void Update(float InDeltaTime) override;

  Tga::Sprite2DInstanceData& GetSpriteInstanceData() { return spriteInstance_; }
  Tga::SpriteSharedData& GetSpriteSharedData() { return sharedData_; }

  void SetSpriteTexture(const char* InTexturePath);

  const NetTag& GetTextureId() const { return textureIdentifier_; }

  RenderData GetRenderData() const;

  void SetRenderSortingPriority(const uint8_t InSortingPriority) { sortingPriority_ = InSortingPriority; }

  void SetSpriteSize(const Tga::Vector2i& InSpriteSize) { spriteSize_ = InSpriteSize; }
  void SetSpriteSizeMultiplier(const Tga::Vector2f InSpriteSizeMultiplier) { spriteSizeMultiplier_ = InSpriteSizeMultiplier; }

  void SetColor(const Tga::Color& InColor);

  const std::string& GetTexturePath() const { return texturePath_; }

  void OnSendReplication(DataReplicationPacketComponent& OutComponent) override;
  void OnReadReplication(const DataReplicationPacketComponent& InComponent) override;

  void SetIsVisible(const bool InShouldBeVisible) { bIsVisible_ = InShouldBeVisible; }
  bool IsVisible() const { return bIsVisible_; }
  
private:
  std::string texturePath_;
  Tga::Sprite2DInstanceData spriteInstance_ = {};
  Tga::SpriteSharedData sharedData_ = {};
  uint8_t sortingPriority_ = 0;
  NetTag textureIdentifier_;
  bool bIsVisible_ = true;
  Tga::Vector2i spriteSize_ = { 16, 16 };
  Tga::Vector2f spriteSizeMultiplier_ = { 1.f, 1.f };
};
