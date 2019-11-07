#include "Networks.h"


void ScreenBackground::enable()
{
	background = Instantiate();
	background->texture = App->modResources->background;
	background->size = { 1920.f, 1080.f };
	background->order = -2;
	//background->scene = this;
}

void ScreenBackground::update()
{
	background->size = { (float)Window.width, (float)Window.height };
}

void ScreenBackground::disable()
{
	Destroy(background);
}
