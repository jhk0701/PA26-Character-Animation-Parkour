#include "pch.h"
#include "Content/Data/PerceptionQueryTree.h"
#include "Scene/PerceptionComponent.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"

#include "Content/Perception/ReturnNode.h"
#include "Content/Perception/SelectCharacterStateNode.h"
#include "Content/Perception/SelectObstacleTagNode.h"
#include "Content/Perception/SelectUsingHeightNode.h"
#include "Content/Perception/SelectUsingInputDirNode.h"
#include "Content/Perception/CheckObstacleNode.h"
#include "Content/Perception/MeasureDepthNode.h"
#include "Content/Perception/BeamCompareHeightNode.h"
#include "Content/Perception/ProtrudeExtractHeightNode.h"
#include "Content/Perception/CheckObstacleOnHangingNode.h"
#include "Content/Perception/TransitionCharacterFSMNode.h"

using namespace MiniEngine;

void PerceptionQueryData::Load(const json& _data)
{

}

// 콘텐츠에서 사용할 지형 인식 로직
// 데이터 로드
std::shared_ptr<PerceptionNode> PerceptionQueryTree::ConstructTree()
{
	// 루트 쿼리
	std::shared_ptr<SelectCharacterStateNode> pRootQuery = std::make_shared<SelectCharacterStateNode>();

	// Landing
	std::shared_ptr<CheckObstacleNode> pStateLanding = std::make_shared<CheckObstacleNode>(); // 평지상태 : 장애물 찾기
	pStateLanding->SetHeightMultiplier(1.0f);

	std::shared_ptr<SelectObstacleTagNode> pCheckObstacleTag = std::make_shared<SelectObstacleTagNode>(); // 장애물 태그 확인
	
	// Obstacle Default
	std::shared_ptr<SelectUsingHeightNode> pProcessDefault = std::make_shared<SelectUsingHeightNode>(); // 장애물 높이 측정 -> 무시 / 깊이측정 / 벽
	std::shared_ptr<SequenceNode> pMeasureDepthSeq = std::make_shared<SequenceNode>(); // 깊이 측정 시퀀스
	std::shared_ptr<MeasureDepthNode> pMeasureDepth = std::make_shared<MeasureDepthNode>(); // 딛을 면의 깊이 측정

	// Obstacle Beam
	std::shared_ptr<SequenceNode> pProcessBeam = std::make_shared<SequenceNode>();	// beam 지형의 높이 측정 시퀀스
	std::shared_ptr<BeamCompareHeightNode> pBeamCompareHeight = std::make_shared<BeamCompareHeightNode>(); // 캐릭터와 장애물의 y 위치 비교

	// Obstacle Protrude
	std::shared_ptr<SequenceNode> pProcessProtrude = std::make_shared<SequenceNode>(); // protrude 지형 정보 추출 후 리턴 시퀀스
	std::shared_ptr<ProtrudeExtractHeightNode> pProtrudeExtractHeight = std::make_shared<ProtrudeExtractHeightNode>();

	// InAir
	std::shared_ptr<CheckObstacleNode> pStateInAir = std::make_shared<CheckObstacleNode>();
	pStateInAir->SetHeightMultiplier(2.0f);

	std::shared_ptr<SequenceNode> pInAirObstacleFoundSeq = std::make_shared<SequenceNode>();
	std::shared_ptr<TransitionCharacterFSMNode> pTransitionToLanding = std::make_shared<TransitionCharacterFSMNode>();
	pTransitionToLanding->SetTargetState((uint8_t)Character::EState::Landing);

	// Hanging — 입력 방향 검사
	std::shared_ptr<SelectUsingInputDirNode> pStateHanging = std::make_shared<SelectUsingInputDirNode>();
	std::shared_ptr<CheckOnHangingMoveUpNode> pOnHangingUp = std::make_shared<CheckOnHangingMoveUpNode>();			// 올라설 벽 ledge
	std::shared_ptr<CheckOnHangingMoveDownNode> pOnHangingDown = std::make_shared<CheckOnHangingMoveDownNode>();	// 내려설 지면
	std::shared_ptr<CheckOnHangingMoveSideNode> pOnHangingSide = std::make_shared<CheckOnHangingMoveSideNode>();	// 새 장애물

	// Beam
	std::shared_ptr<CheckObstacleNode> pStateBeam = std::make_shared<CheckObstacleNode>();
	pStateBeam->SetHeightMultiplier(2.0f);
	pStateBeam->SetStartOffset({ 0.0f, 0.0f, 1.0f });

	// Protrude
	std::shared_ptr<SelectUsingInputDirNode> pStateProtrude = std::make_shared<SelectUsingInputDirNode>();

	// Task (Leaf)
	std::shared_ptr<TaskNode> pEmpty = std::make_shared<ReturnEmptyNode>(); // 빈 결과 리턴, 탐색 계속 신호
	std::shared_ptr<TaskNode> pReturn = std::make_shared<ReturnResultNode>(); // 결과 리턴 : 이 경우 Success 결과도 리턴

	// 루트 확인 : 평지에 있는 상황인지
	pRootQuery->SetChildren(
		{
			// 배치 순서는 Character EState 순서대로
			pStateLanding,
			pStateInAir,
			pStateHanging,
			pStateBeam,
			pStateBeam,
			pStateProtrude
		}
	);
		// 평지에 있는데, 장애물을 발견했는지
		pStateLanding->SetChildren(
			pCheckObstacleTag,	// 찾은 경우 태그 확인
			pEmpty			// 찾지 못한 경우 empty return
		);

			pCheckObstacleTag->SetChildren(
				{
					// ETagEnvDetail 순서
					pProcessDefault,				// Default // 일반 장애물 높이 확인
					pProcessBeam,		// Beam
					pProcessProtrude			// Protrude 돌출부
				}
			);

			// 높이를 재고 세 갈래로 나눈다. 
			pProcessDefault->SetChildren(
				{
					pEmpty,			// 행동할 필요 없는 높이
					pMeasureDepthSeq,	// vault, mantle 중 깊에 따라 선택될 것 -> 그러므로 깊이 확인
					pReturn			// 벽을 넘어야하는 상황 -> 결과 리턴
				}
			);
				pMeasureDepthSeq->SetChildren(
					{
						pMeasureDepth,	// 0 깊이 측정 후
						pReturn			// 1 결과 리턴
					}
				);

			pProcessBeam->SetChildren(
				{
					pBeamCompareHeight,
					pReturn
				}
			);

			pProcessProtrude->SetChildren(
				{
					pProtrudeExtractHeight,
					pReturn
				}
			);

	// Hanging State 처리
	pStateHanging->SetChildren(
		{
			pOnHangingUp,
			pOnHangingDown,
			pOnHangingSide,
			pEmpty
		}
	);
		// 위 방향 탐색 -> 올라설 벽 ledge 를 찾기
		pOnHangingUp->SetChildren(
			pReturn,
			pEmpty
		);

		// 아래 : 내려설 지면. 좁게 볼 필요가 없어 단순 레이로 충분하다
		// 지면(Ground)도 Obstacle 액터라 IObstacle 포인터가 유효하다
		pOnHangingDown->SetChildren(
			pReturn,
			pEmpty
		);

		// 좌우 : 옮겨갈 새 장애물 탐색. 종류 판단과 상태 전환은 HangingState 담당
		pOnHangingSide->SetChildren(
			pReturn,
			pEmpty
		);

		pStateInAir->SetChildren(
			pInAirObstacleFoundSeq,
			pEmpty
		);

		pInAirObstacleFoundSeq->SetChildren(
			{
				pTransitionToLanding,
				pCheckObstacleTag
			}
		);

	pStateBeam->SetChildren(
		pCheckObstacleTag,
		pEmpty
	);

	// 키 입력 기준 상하좌우 방향 탐색
	pStateProtrude->SetChildren(
		{
			pEmpty,
			pEmpty,
			pEmpty,
			pEmpty
		}
	);

	return pRootQuery;
}
