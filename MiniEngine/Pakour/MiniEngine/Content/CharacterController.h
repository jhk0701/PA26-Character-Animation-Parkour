#pragma once
#include "Scene/Controller.h"

namespace MiniEngine { class SceneComponent; }

class Character;
class CharacterController : public MiniEngine::Controller
{
public:
	void Construct() override;
	void Tick(float _dt) override;

	// 카메라
	std::weak_ptr<MiniEngine::SceneComponent> GetCamHolder() const { return m_camHolder; }
	void ResetCamRot() { m_camRotate.x = 0.0f; m_camRotate.y = 0.0f; }

protected:
	void OnPossessed(MiniEngine::Input& _input) override;

private:
	std::shared_ptr<Character> GetChar();
	
	std::weak_ptr<MiniEngine::SceneComponent> m_camHolder;
	float m_camRotateSpeed{ 20.0f };
	float m_camPitchMaxDeg{ 70.0f };
	MiniEngine::Vector2 m_camRotate{ 0.0f, 0.0f }; // yaw, pitch

	void RotateCamera(float _dt);

	float m_followLerpWeight{ 0.15f };
	void FollowPawn(float _dt);
};