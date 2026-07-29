#pragma once
#include "Animation/AnimStateMachine.h"
#include "Asset/AnimClip.h"
#include "Asset/RootMotion.h"
#include <array>
#include <functional>

namespace MiniEngine 
{
	class BlendClip;
	class ActionClip;
	class SkeletalMeshComponent;

	struct AnimLayer
	{
		bool m_bIsPlaying{ false };
		uint8_t m_curPriority = 0; // 높을수록 바로 재생

		// 레이어간 페이드 인아웃
		float m_fadeElapsed = 0.0f;
		float m_fadeDuration = 0.0f;	// 0 = 페이드 없음
		float m_actionDuration = 0.0f;
		float m_actionElapsed = 0.0f;
		float m_actionEndTime = 0.0f; // 종료 페이드가 시작되는 시점
	
		std::shared_ptr<ActionClip> m_pClip; // 재생할 클립 타입 // 여러 개 사용될 것 - 테스트용 1개
		LocalPoseTRS m_layerPose;				// 재생한 클립의 좌표가 이 값에 저장될 것

		std::function<void()> m_onClipStarted;
		std::function<void()> m_onClipEnded;

		bool IsEnd() const { return m_actionElapsed >= m_actionDuration; }
		bool IsEndArea() const { return m_actionElapsed >= m_actionEndTime; }
		float GetProgress() const { return m_fadeElapsed / m_fadeDuration; }
	};

	// IK
	struct TwoBoneIKBinding
	{
		HumanoidBone upper;
		HumanoidBone lower;
		HumanoidBone end;
	};

	struct IKGoal 
	{
		Vector3 position;		// end effector 본 원점이 도달할 지점
		Quaternion rotation = Quaternion(0.0f, 0.0f, 0.0f, 1.0f); // 회전 (델타) x
		Vector3 poleTarget;	// 중간 관절이 향할 지점 bUsePoleTarget으로 토글
		float positionAlpha = 0.0f;
		float rotationAlpha = 0.0f;
		bool bUsePoleTarget = false;
	};


	// Animator : 애니메이션 총괄 관리
	class Animator
	{

	public:
		Animator(std::shared_ptr<SkeletalMeshComponent> _meshComp);
		virtual ~Animator() {};

		void Init(int _baseTrackStart);
		void Update(float _dt);

		void SampleBaseTrack(float _dt);
		void SampleOverrideTrack(float _dt);
		void FinalizePose();

		// 루트모션 api
		void SetEnableRootMotion(bool _bEnable) { m_bEnableRootMotion = _bEnable; }
		void SetRootBoneIdx(int _idx) { m_rootBoneIdx = _idx; }
		void SetRootMotionConfig(RootMotionConfig& _config) { m_rootMotionCfg = _config; }
		const bool GetIsEnableRootMotion() const { return m_bEnableRootMotion; }
		const RootMotionDelta& GetRootMotionDelta() const { return m_rootMotionDt; }
		const RootMotionConfig& GetRootMotionConfig() const { return m_rootMotionCfg; }
		RootMotionDelta ConsumeRootMotionDelta(); // 델타를 읽고 0 으로 

		// base track api
		AnimStateMachine& GetBaseTrack() { return m_baseTrack; }
		void SetBaseTrackInputAxis(const Vector2& _axis);
		void TranstionBaseTrack(int _nextIdx, float _duration = 0.5f);
		void ReserveBaseLocomotion(uint8_t _cnt) { m_baseTrack.Reserve(_cnt); }
		void AddBaseLocomotion(std::shared_ptr<BlendClip>& _loco) { m_baseTrack.AddState(_loco); }

		// override track api
		void PlayActionClip(std::shared_ptr<ActionClip>& _action, float _fadeDuration = 0.5f, uint8_t _priority = 0);
		void StopActionClip();

		// 단순 재생 여부 확인
		bool IsActionClipPlaying() const { return m_overrideTrack.m_bIsPlaying; }
		// 특정 우선순위 이상이 재생중인지 확인
		bool IsActionClipPlaying(uint8_t _priority) const 
		{ 
			return m_overrideTrack.m_bIsPlaying && 
				m_overrideTrack.m_curPriority >= _priority;
		}

		void SetOverrideTrackStartEvent(std::function<void()>&& _event) { m_overrideTrack.m_onClipStarted = _event; }
		void SetOverrideTrackEndEvent(std::function<void()>&& _event) { m_overrideTrack.m_onClipEnded = _event; }

		// IK
		void SetIKGoal(const TwoBoneIKBinding& _binding, const IKGoal& _goal);
		void ClearIKGoal(HumanoidBone _end);
		void ClearAllIKGoals();
		const IKGoal* GetIKGoal(HumanoidBone _end) const;

		// 골반 글로벌 위치에 더할 오프셋
		void SetIKPelvisOffset(const Vector3& _offsetMeshLocal);
		const Vector3& GetIKPelvisOffset() const { return m_ikPelvisOffset; }
		const std::vector<Matrix>& GetGlobalPose() const { return m_globalPose; }  // 발 IK 레이캐스트 원점용

		// IK 소비 후 팔레트 갱신
		// FinalizePose에서 호출하진 않을 것 -> 캐릭터의 이동 등등의 과정이 모두 끝난 다음 별도로 호출 예정
		void SolveIKAndRefresh(const SkinnedMesh* _mesh); 
		void ResetForMesh();

	private:
		const Skeleton& GetSkeleton() const;

		bool m_bIsInitialized{ false };
		std::weak_ptr<SkeletalMeshComponent> m_meshComp; // 메시 컴포넌트 약참조

		LocalPoseTRS m_poseTarget;		// 블렌드, 트랜지션 등 연산이 반영되는 본 위계구조
		std::vector<Matrix> m_localPose;	// 합성 로컬 행렬 스크래치
		std::vector<Matrix> m_globalPose;	// 메시 로컬 글로벌 포즈(inverseBind 전) IK 작업 공간 
		std::vector<Matrix>* m_pBoneMatrices;	// 스키닝 최종 행렬

		bool m_bEnableRootMotion{ false };
		int m_rootBoneIdx{ 0 };
		RootMotionDelta m_rootMotionDt;
		RootMotionConfig m_rootMotionCfg;

		AnimStateMachine m_baseTrack; // 로코모션 루프용
		AnimLayer m_overrideTrack;	// 단발 액션 오버라이드용

		// IK
		// m_globalPose의 값을 in-place로 변경. SolveIKAndRefresh에서 사용
		void ApplyIKGoals(const Skeleton& _skeleton, const HumanoidBoneMap& _map);
		void RefreshIKAny();
		void BindGlobal(int _boneIdx, const Skeleton& _inSkeleton, Matrix& _out) const;
		bool BindPole(const Skeleton& _skeleton, int _upperBone, int _lowerBone, int _endBone, Vector3& _outPole) const;
		
		// 본 원점을 피벗으로 한 회전
		void RotateGlobalInPlace(Matrix& _global, const Quaternion& _delta);

		struct IKData
		{
			TwoBoneIKBinding binding;
			IKGoal goal;

			void Reset()
			{
				binding = TwoBoneIKBinding();
				goal = IKGoal();
			}
		};
		// ApplyIKGoals 의 처리 순서 정렬용 (upper 본 인덱스 오름차순)
		struct IKOrder
		{
			int upperIdx;
			const IKData* pData;
		};

		std::unordered_map<HumanoidBone, IKData> m_mapIKBinding; // ik의 경우 런타임에 매번 동적이기 보단 이미 초기화 시 정해져 있을 것

		Vector3 m_ikPelvisOffset{ 0.0f, 0.0f, 0.0f };
		bool m_bUseIK{ false };
		bool m_bWarnedIKChainOrder{ false }; // 사지 체인 인덱스 겹침 경고 1회용
	};
}