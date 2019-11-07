#include "Networks.h"


void ScreenLoading::enable()
{
	for (int i = 0; i < BAR_COUNT; ++i)
	{
		float progressRatio = (float)i / (float)BAR_COUNT;
		float radians = 2.0f * PI * progressRatio;

		loadingBars[i] = Instantiate();
		loadingBars[i]->color = vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
		loadingBars[i]->pivot = vec2{ 0.5f, 0.5f };
		loadingBars[i]->size = vec2{ 4, 30 };
		loadingBars[i]->position = 30.0f * vec2{ sinf(radians), cosf(radians) };
		loadingBars[i]->angle = - 360.0f * progressRatio;
	}
}

void ScreenLoading::update()
{
	const float ROUND_TIME = 3.0f;
	for (int i = 0; i < BAR_COUNT; ++i)
	{
		float progressRatio = (float)i / (float)BAR_COUNT;
		auto gameObject = loadingBars[i];
		gameObject->color.a = 1.0f - fractionalPart(((float)Time.time + progressRatio * ROUND_TIME)/ ROUND_TIME);
	}

	if (App->modResources->finishedLoading)
	{
		App->modScreen->swapScreensWithTransition(this, App->modScreen->screenMainMenu);

		// NOTE(jesus): The following is equivalent to the previous line but without transition.
		//this->enabled = false;
		//App->modScene->scenePingPong->enabled = true;
	}
}

void ScreenLoading::disable()
{
	for (int i = 0; i < BAR_COUNT; ++i)
	{
		Destroy(loadingBars[i]);
	}
}
