#pragma once

#define USE_TASK_MANAGER

struct Texture;

class ModuleResources : public Module
{
public:

	Texture *background = nullptr;
	Texture *space = nullptr;

	Texture* T1_Shooter = nullptr;
	Texture* T1_SoftProjectile = nullptr;
	Texture* T1_Reflector = nullptr;
	Texture* T1_ReflectorBarrier = nullptr;
	Texture* T1_HardProjectile = nullptr;

	Texture* T2_Shooter = nullptr;
	Texture* T2_SoftProjectile = nullptr;
	Texture* T2_Reflector = nullptr;
	Texture* T2_ReflectorBarrier = nullptr;
	Texture* T2_HardProjectile = nullptr;


	bool finishedLoading = false;
private:

	bool init() override;

#if defined(USE_TASK_MANAGER)
	void onTaskFinished(Task *task) override;

	void loadTextureAsync(const char *filename, Texture **texturePtrAddress);
#endif

	struct LoadTextureResult {
		Texture **texturePtr = nullptr;
		Task *task = nullptr;
	};

	LoadTextureResult taskResults[1024] = {};
	int taskCount = 0;
	int finishedTaskCount = 0;
};

