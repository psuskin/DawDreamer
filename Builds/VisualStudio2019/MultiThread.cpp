#include "MultiThread.h"

py::list MultiThread::render(float duration, py::list engineList) {
	auto func = static_cast<bool (RenderEngine::*)(double, bool)>(&RenderEngine::render);

	std::vector<RenderEngine*> engines;
	std::vector<std::thread> threads;
	for (size_t i = 0; i < py::len(engineList); i++) {
		engines.push_back(engineList[i].cast<RenderEngine*>());
		threads.push_back(std::thread(func, engines.back(), duration, false));
	}

	for (auto& thread : threads) {
		thread.join();
	}

	py::list audios;
	for (auto& engine : engines) {
		audios.append(engine->getAudioFrames());
	}

	return audios;
}