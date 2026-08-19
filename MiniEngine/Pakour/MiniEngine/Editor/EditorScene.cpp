#include "pch.h"
#include "Editor/EditorScene.h"

#include "Scene/Actor.h"
#include "Scene/CameraComponent.h"

namespace MiniEngine
{
	void EditorScene::Construct(ID3D11Device* _device, ID3D11DeviceContext* _context)
	{
		// 기반 Construct 가 PhysX 월드 + 기본 카메라 + 디렉셔널 라이트 + 디버그 드로어를 세운다.
		Scene::Construct(_device, _context);

		// 액터는 카메라 하나뿐. 장애물/캐릭터는 스폰하지 않는다.
		std::shared_ptr<Actor> pCamActor = SpawnActor<Actor>();
		pCamActor->SetName("EditorCamera");

		// 첫 SceneComponent 라 Actor 의 루트로 자동 지정된다(Actor::AddComponent).
		std::shared_ptr<CameraComponent> pCam = pCamActor->AddComponent<CameraComponent>();
		pCam->localTransform.position = Vector3(0.0f, 1.5f, -5.0f);
		pCam->RegisterMainCamera();
	}
}
