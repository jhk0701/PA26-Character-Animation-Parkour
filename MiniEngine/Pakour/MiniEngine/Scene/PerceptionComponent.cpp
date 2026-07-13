#include "pch.h"
#include "Scene/PerceptionComponent.h"
#include "Scene/Actor.h"
#include "Scene/Scene.h"
#include "Physics/PhysicsWorld.h"
#include "Core/Log.h"
#include "Content/ContentConfig.h"

namespace MiniEngine 
{
	void PerceptionComponent::OnAttach()
	{
		Component::OnAttach();

		m_physics = owner.lock()->GetScene()->GetPhysics();
	}

	void PerceptionComponent::Tick(float _dt)
	{
		Component::Tick(_dt);
	}

	void PerceptionComponent::StartTravel(const Vector3& _moveDir)
	{
		if (m_physics.expired())
			return;

		MG_LOG_INFO("Perception Travel");

		m_ownerDir = _moveDir;
		m_travelResult.clear();
		Travel(0);
	}

	void PerceptionComponent::Travel(int _curDepth)
	{
		if (_curDepth >= MAX_PERCEPTION_STEP)
			return;

		if (m_travelResult.empty())
		{
			FirstTravel(); // 최초 탐색
			return;
		}

		const TravelResult& lastResult = m_travelResult.back();

		if (lastResult.m_envTag
			== static_cast<uint8_t>(Content::Config::ETagEnv::Obstacle)) 
		{
			std::shared_ptr<SceneComponent> pRoot = owner.lock()->GetRoot();
			// 직전 탐색한 지형이 장애물인 경우
			// 넘을 수 있는지 확인 (Up 방향으로 탐색 + 정면 레이캐스트 확인)
			// 고정단위만큼 레이캐스트 조사
			
			// 1,2 단위까지 탐색
			for (int i = 1; i <= 2; ++i)
			{
				Physics::RaycastParam rayParam;
				rayParam.m_origin = lastResult.m_pos;
				rayParam.m_origin.y += m_unit * i ; // 최초 장애물을 발견한 위치에서 단위량 만큼 +y축으로 이동
				rayParam.m_dir = pRoot->localTransform.Forward();
				rayParam.m_maxDistance = m_unit; // 단위량만큼 진행방향을 향해 레이캐스트

				Physics::RaycastResult rayResult;
				if (m_physics.lock()->Raycast(rayParam, rayResult, Physics::ToMask(Physics::Layer::Obstacle)))
				{


				}
			}

			// 3 단위부터는 매달려야 함
		}
		else if (lastResult.m_envTag
			== static_cast<uint8_t>(Content::Config::ETagEnv::Land)) 
		{
			// 직전 탐색한 지형이 땅인 경우
			// 착지
			// 착지 후 다시 정면을 향해 탐색 개시
		}
	}

	void PerceptionComponent::FirstTravel()
	{
		// 기본 이동방향, 바닥 체크
		std::shared_ptr<SceneComponent> pRoot = owner.lock()->GetRoot();

		Physics::RaycastParam rayParam;
		rayParam.m_dir = pRoot->localTransform.Forward();
		rayParam.m_maxDistance = m_maxObsDist;
		rayParam.m_origin = pRoot->localTransform.position + Vector3(0.0f, 1.0f, 0.0f);

		Physics::RaycastResult rayResult;

		bool bIsHit = m_physics.lock()->Raycast(rayParam, rayResult, Physics::ToMask(Physics::Layer::Obstacle));
		if (!bIsHit)
			return; // 탐색된 객체가 없음

		MG_LOG_INFO("Obstacle Hit");

		if (MiniEngine::Actor* pActor = static_cast<MiniEngine::Actor*>(rayResult.GetActor()))
		{
			uint8_t tagEnv;
			pActor->GetTag().GetTagAt(Content::Config::TAG_TYPE_ENV, tagEnv);
			m_travelResult.push_back(
				TravelResult
				{
					EDirection::UP,
					rayResult.m_pos,
					tagEnv,
					rayResult.GetActor()
				}
			);

			Travel(1); // 다음 차례에서 넘을 수 있는지 확인
		}
	}
}