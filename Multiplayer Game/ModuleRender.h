#pragma once

struct Texture;

class ModuleRender : public Module
{
public:

	// Virtual functions

	bool init() override;

	bool postUpdate() override;

	bool cleanUp() override;


	// Public methods

	void resizeBuffers(unsigned int width, unsigned int height);

	void present();

	vec2 getWindowsSize();


	// Attributes

	vec2 cameraPosition = {};

	bool mustRenderColliders = false;


private:

	void renderScene();

	bool CreateDeviceD3D(HWND hWnd);
	void CleanupDeviceD3D();
	void CreateRenderTarget();
	void CleanupRenderTarget();
	void ProcessAnimationUVs(Texture* texture, GameObject* go);

	Texture * whitePixel = nullptr;
	Texture * blackPixel = nullptr;

	GameObject* orderedGameObjects[MAX_GAME_OBJECTS] = {};

	uint8 shaderSource[Kilobytes(128)];
};

