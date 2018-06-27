#pragma once
#include "Stats.hpp"

#include "Utils.hpp"

namespace JumpDistance
{
	float LastDistance;

	bool IsTracing;
	Vector Source;

	void StartTrace(Vector source)
	{
		Source = source;
		IsTracing = true;
	}
	void EndTrace(Vector destination)
	{
		float x = destination.x - Source.x;
		float y = destination.y - Source.y;
		LastDistance = std::sqrt(x * x + y * y);
		IsTracing = false;
	}
	void Reset()
	{
		LastDistance = 0;
		IsTracing = false;
	}
}