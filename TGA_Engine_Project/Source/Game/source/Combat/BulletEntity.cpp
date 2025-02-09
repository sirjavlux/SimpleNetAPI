﻿#include "stdafx.h"

#include "BulletEntity.h"
#include "../Definitions.hpp"
#include "../Entity/EntityManager.h"
#include "../Entity/Collision/CircleCollider.hpp"
#include "../Entity/EntityComponents/ColliderComponent.h"
#include "../Entity/EntityComponents/CombatComponent.h"
#include "../Locator/Locator.h"

void BulletEntity::Init()
{
	bShouldUseCustomReplicationData_ = false;

	std::vector<std::pair<float, uint16_t>> packetLodFrequencies;
	packetLodFrequencies.emplace_back(1.0f, 1);
	//packetLodFrequencies.emplace_back(std::powf(1.5f, 2.f), 4);
	//packetLodFrequencies.emplace_back(std::powf(2.f, 2.f), 8);
	//packetLodFrequencies.emplace_back(std::powf(2.5f, 2.f), 30);
	
	customAssociatedDataMovementReplication_.packetFrequency = 1;
	customAssociatedDataMovementReplication_.packetLodFrequencies = packetLodFrequencies;
}

void BulletEntity::InitComponents()
{
	RenderComponent* renderComponent = EntityManager::Get()->AddComponentToEntityOfType<RenderComponent>(id_, NetTag("RenderComponent").GetHash());
	renderComponent->SetSpriteTexture("Sprites/SpaceShip/Bullet.png");
	renderComponent->SetSpriteSizeMultiplier({ 3.f, 1.f });
	renderComponent->SetRenderSortingPriority(10);
	
	SetShouldReplicatePosition(true);

	ColliderComponent* colliderComponent = EntityManager::Get()->AddComponentToEntityOfType<ColliderComponent>(id_, NetTag("ColliderComponent").GetHash());
	std::shared_ptr<CircleCollider> circleCollider = std::make_shared<CircleCollider>();
	circleCollider->radius = 0.01f;
	colliderComponent->SetCollider(circleCollider);
	colliderComponent->SetCollisionFilter(ECollisionFilter::Player | ECollisionFilter::WorldDestructible);
	colliderComponent->SetColliderCollisionType(ECollisionFilter::Projectile);

	CombatComponent* combatComponent = EntityManager::Get()->AddComponentToEntityOfType<CombatComponent>(id_, NetTag("CombatComponent").GetHash());
	combatComponent->SetMaxHealth(1.f);
	combatComponent->HealToFullHealth();
	combatComponent->SetCollisionDamage(12.f);
	
	combatComponent->entityDeathDelegate.AddDynamic<BulletEntity>(this, &BulletEntity::OnEntityDeath);

	SetParentEntity(shooterId_);
}

void BulletEntity::Update(float InDeltaTime)
{

}

void BulletEntity::FixedUpdate()
{
	if (Net::PacketManager::Get()->IsServer())
	{
		targetPosition_ += direction_ * speed_ * FIXED_UPDATE_DELTA_TIME;

		// Check world bounds, destroy entity if outside world bounds
		if (targetPosition_.x > WORLD_SIZE_X / 2.f
			|| targetPosition_.x < -WORLD_SIZE_X / 2.f
			|| targetPosition_.y > WORLD_SIZE_Y / 2.f
			|| targetPosition_.y < -WORLD_SIZE_Y / 2.f)
		{
			EntityManager::Get()->MarkEntityForDestruction(GetId());
		}
	}
}

void BulletEntity::OnReadReplication(const DataReplicationPacketComponent& InComponent)
{
	const uint16_t oldShooterId = shooterId_;
	InComponent.variableDataObject.DeSerializeMemberVariable(*this, shooterId_);
	if (oldShooterId != shooterId_)
	{
		SetParentEntity(shooterId_);
	}
}

void BulletEntity::OnSendReplication(DataReplicationPacketComponent& OutComponent)
{
	const uint16_t oldShooterId = shooterId_;
	OutComponent.variableDataObject.SerializeMemberVariable(*this, shooterId_);
	if (oldShooterId != shooterId_)
	{
		SetParentEntity(shooterId_);
	}
}

void BulletEntity::OnEntityDeath(uint16_t InEnemy)
{
	if (EntityManager::Get()->IsServer())
	{
		EntityManager::Get()->MarkEntityForDestruction(GetId());
	}
	else
	{
		GetFirstComponent<RenderComponent>().lock()->SetIsVisible(false);
	}
}
