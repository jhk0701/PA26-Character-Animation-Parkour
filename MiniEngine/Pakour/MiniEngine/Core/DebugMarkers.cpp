#include "pch.h"
#include "DebugMarkers.h"

#include <cfloat>
#include <cmath>

namespace MiniEngine::Debug 
{
	namespace 
	{
		struct Entry 
		{
			DebugLine seg;
			float timeLeft;
		};

		std::vector<Entry> g_entries;

		inline float ToTimeLeft(float _duration) 
		{
			if (_duration < 0.0f) return -1.0f;
			if (_duration == 0.0f) return FLT_MAX;
			return _duration;
		}

		inline void PushSeg(const Vector3& _a, const Vector3& _b, uint32_t _color, float _timeLeft) 
		{
			g_entries.push_back({ { _a, _b, _color }, _timeLeft });
		}

		void AppendCircle(const Vector3& _center, const Vector3& _u, const Vector3& _v,
			float _radius, uint32_t _color, float _timeLeft, int _segments)
		{
			const float step = PI * 2.0f / static_cast<float>(_segments);
			Vector3 prev = _center + _u * _radius; // angle 0
			for (int i = 1; i <= _segments; ++i)
			{
				const float a = step * static_cast<float>(i);
				Vector3 cur = _center + _u * (_radius * std::cos(a)) + _v * (_radius * std::sin(a));
				PushSeg(prev, cur, _color, _timeLeft);
				prev = cur;
			}
		}
	}

	void DrawPoint(const Vector3& _pos, uint32_t _colARGB, float _size, EMarkerShape _shape, float _duration)
	{
		const float t = ToTimeLeft(_duration);

		switch (_shape)
		{
		case EMarkerShape::Cross: __fallthrough;
		default :
			PushSeg({ _pos.x - _size, _pos.y, _pos.z }, { _pos.x + _size, _pos.y, _pos.z }, _colARGB, t);
			PushSeg({ _pos.x, _pos.y - _size, _pos.z }, { _pos.x, _pos.y + _size, _pos.z }, _colARGB, t);
			PushSeg({ _pos.x, _pos.y, _pos.z - _size }, { _pos.x, _pos.y, _pos.z + _size }, _colARGB, t);
			break;

		case EMarkerShape::Sphere:
		{
			const int SEG = 24;
			const Vector3 EX{ 1.0f, 0.0f, 0.0f }, EY{ 0.0f, 1.0f, 0.0f }, EZ{ 0.0f, 0.0f, 1.0f };

			AppendCircle(_pos, EX, EY, _size, _colARGB, t, SEG);
			AppendCircle(_pos, EY, EZ, _size, _colARGB, t, SEG);
			AppendCircle(_pos, EZ, EX, _size, _colARGB, t, SEG);
			break;
		}
		case EMarkerShape::Box:
		{
			const Vector3 c[8] = {
					{ _pos.x - _size, _pos.y - _size, _pos.z - _size }, { _pos.x + _size, _pos.y - _size, _pos.z - _size },
					{ _pos.x + _size, _pos.y + _size, _pos.z - _size }, { _pos.x - _size, _pos.y + _size, _pos.z - _size },
					{ _pos.x - _size, _pos.y - _size, _pos.z + _size }, { _pos.x + _size, _pos.y - _size, _pos.z + _size },
					{ _pos.x + _size, _pos.y + _size, _pos.z + _size }, { _pos.x - _size, _pos.y + _size, _pos.z + _size },
			};
			static const int e[12][2] = {
				{0,1},{1,2},{2,3},{3,0}, // -Z 면
				{4,5},{5,6},{6,7},{7,4}, // +Z 면
				{0,4},{1,5},{2,6},{3,7}, // 연결 에지
			};
			for (const auto& edge : e)
				PushSeg(c[edge[0]], c[edge[1]], _colARGB, t);
			break;
		}
		}
	}

	void DrawLine(const Vector3& _start, const Vector3& _end, uint32_t _colARGB, float _duration)
	{
		g_entries.push_back({ { _start, _end, _colARGB}, ToTimeLeft(_duration) });
	}

	void Clear()
	{
		g_entries.clear();
	}

	void NewFrame(float _dt)
	{
		for (auto it = g_entries.begin(); it != g_entries.end();)
		{
			float& t = it->timeLeft;

			if (t < 0.0f)
			{
				it = g_entries.erase(it);
				continue;
			}

			if (t == FLT_MAX) 
			{
				++it;
				continue;
			}

			t -= _dt;

			if (t <= 0.0f)
				it = g_entries.erase(it);
			else
				++it;
		}
	}

	void Collect(std::vector<DebugLine>& _out)
	{
		_out.reserve(_out.size() + g_entries.size());
		for (const Entry& e : g_entries)
			_out.push_back(e.seg);
	}
}


