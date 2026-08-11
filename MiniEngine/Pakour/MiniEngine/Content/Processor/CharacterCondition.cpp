#include "pch.h"
#include "CharacterCondition.h"

#include "Scene/PerceptionComponent.h"
#include "Scene/IObstacle.h"

#include "Content/ContentConfig.h"
#include "Content/Character.h"
#include "Content/Data/CharacterPerceptionConfig.h"

#include "Core/Log.h"

using namespace ProcessorConditionUtil;

bool CharacterStateCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
    std::shared_ptr<Character> pChar = ToChar(_context.pOwner);
    return (uint8_t)pChar->GetState() == GetValue();
}

bool CharacterHeightCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
    // 인식한 지형의 모서리 높이가 캐릭터의 현재 높이 + 지정한 값만큼의 높이보다 낮은지 확인
    std::shared_ptr<Character> pChar = ToChar(_context.pOwner);
    return _result.m_obstacleLedge < pChar->GetRoot()->localTransform.position.y + GetValue();
}

bool InputVerticalCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
    std::shared_ptr<Character> pChar = ToChar(_context.pOwner);

    // 기준 값이 음수. 아래를 향하는지 확인
    if (GetValue() < 0)
        return GetValue() > pChar->GetInputDir().y;
    
    // 기준 값이 양수. 위를 향하는지 확인
    return GetValue() < pChar->GetInputDir().y;
}

bool InputHorizontalCondition::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
    std::shared_ptr<Character> pChar = ToChar(_context.pOwner);

    // 기준 값이 음수. 왼쪽을 향하는지 확인
    if(GetValue() < 0) 
        return GetValue() > pChar->GetInputDir().x;

    // 기준 값이 양수. 오른쪽을 향하는지 확인
    return GetValue() < pChar->GetInputDir().x;
}

bool DetectNewObstacle::Evaluate(const TravelResult& _result, const ProcessContext& _context) const
{
    std::shared_ptr<Character> pChar = ToChar(_context.pOwner);
    return pChar->GetCurObstacleInfo().m_bIsNewObstacle;
}
