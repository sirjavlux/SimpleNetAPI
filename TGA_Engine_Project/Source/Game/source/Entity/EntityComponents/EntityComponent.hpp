﻿#pragma once

class Entity;

class EntityComponent
{
public:
  void SetOwner(Entity& InOwner) { owner_ = &InOwner; }
  Entity* GetOwner() { return owner_; }
  
  virtual ~EntityComponent() = default;

  virtual void Init() = 0;
  
  virtual void Update(float InDeltaTime) {}
  virtual void FixedUpdate() {}
  
protected:
  Entity* owner_ = nullptr;
};
