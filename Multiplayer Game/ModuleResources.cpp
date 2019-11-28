#include "Networks.h"


#if defined(USE_TASK_MANAGER)
class TaskLoadTexture : public Task
{
public:

	const char *filename = nullptr;
	Texture *texture = nullptr;

	void execute() override
	{
		texture = App->modTextures->loadTexture(filename);
	}
};
#endif


bool ModuleResources::init()
{
	background = App->modTextures->loadTexture("background.jpg");

#if !defined(USE_TASK_MANAGER)
	space = App->modTextures->loadTexture("space_background.jpg");
	asteroid1 = App->modTextures->loadTexture("asteroid1.png");
	asteroid2 = App->modTextures->loadTexture("asteroid2.png");
	spacecraft1 = App->modTextures->loadTexture("spacecraft1.png");
	spacecraft2 = App->modTextures->loadTexture("spacecraft2.png");
	spacecraft3 = App->modTextures->loadTexture("spacecraft3.png");
	loadingFinished = true;
	completionRatio = 1.0f;
#else
	loadTextureAsync("space_background.jpg", &space);
	// T1 Textures
	loadTextureAsync("T1_ASHE_BODY.png", &T1_Shooter);
	loadTextureAsync("T1_SOFT_PROJECTILE.png", &T1_SoftProjectile);
	loadTextureAsync("T1_BRAUM_BODY.png", &T1_Reflector);
	loadTextureAsync("T1_BRAUM_SHIELD.png", &T1_ReflectorBarrier);
	loadTextureAsync("T1_HARD_PROJECTILE.png", &T1_HardProjectile);
	// T2 Textures
	loadTextureAsync("T2_JAYCE_BODY.png", &T2_Shooter);
	loadTextureAsync("T2_SOFT_PROJECTILE.png", &T2_SoftProjectile); // TODO(Lorien)
	loadTextureAsync("T2_MAFIA_BRAUM_BODY.png", &T2_Reflector);
	loadTextureAsync("T2_MAFIA_BRAUM_SHIELD.png", &T2_ReflectorBarrier);
	loadTextureAsync("T2_HARD_PROJECTILE.png", &T2_HardProjectile); // TODO(Lorien)

#endif

	return true;
}

#if defined(USE_TASK_MANAGER)

void ModuleResources::loadTextureAsync(const char * filename, Texture **texturePtrAddress)
{
	TaskLoadTexture *task = new TaskLoadTexture;
	task->owner = this;
	task->filename = filename;
	App->modTaskManager->scheduleTask(task, this);

	taskResults[taskCount].texturePtr = texturePtrAddress;
	taskResults[taskCount].task = task;
	taskCount++;
}

void ModuleResources::onTaskFinished(Task * task)
{
	ASSERT(task != nullptr);

	TaskLoadTexture *taskLoadTexture = dynamic_cast<TaskLoadTexture*>(task);

	for (int i = 0; i < taskCount; ++i)
	{
		if (task == taskResults[i].task)
		{
			*(taskResults[i].texturePtr) = taskLoadTexture->texture;
			finishedTaskCount++;
			delete task;
			task = nullptr;
			break;
		}
	}

	ASSERT(task == nullptr);

	if (finishedTaskCount == taskCount)
	{
		finishedLoading = true;
	}
}

#endif
