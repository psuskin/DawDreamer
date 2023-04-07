#pragma once

#include "../../../Source/RenderEngine.h"

class MultiThread {
public:
	MultiThread() {}

	py::list render(float duration, py::list engines);
};