#pragma once

#include "Networks.h"

#define MAX_ANIMATION_FRAMES 256
struct AnimRect {
	float x, y, w, h;
};

class Animation {
	AnimRect frames[MAX_ANIMATION_FRAMES];
	float animSpeed = 0.2;
	int frameCount = 0;
	int currentFrame = 0;

	float timeStarted = 0;

public:
	int pushFrame(AnimRect rect) {
		frames[frameCount++] = rect;
	}
	void setSpeed(float speed) {
		animSpeed = speed;
	}

	void start() {
		timeStarted = Time.time;
	}

	AnimRect getCurrentFrame() {

		ASSERT(frameCount != 0); // No frames on the animation
		float timeElapsed = Time.time - timeStarted;

		if (timeElapsed > 0.2) {
			currentFrame++;
			timeStarted = Time.time;
			if (currentFrame > frameCount)
				currentFrame = 0;
		}

		return frames[currentFrame];
	}

	AnimRect getFrame(int frame) {

		ASSERT(frame > frameCount); // Asking for a non existing frame
		return frames[frame];
	}
};