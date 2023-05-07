#pragma once

#include "../../../Source/RenderEngine.h"

class MultiThread {
public:
	MultiThread() {}

	void init();

	py::list render(float duration, py::list engines);
};