#pragma once

#include "Networks.h"

#define MAX_ANIMATION_FRAMES 256
struct AnimRect {
	int x, y, w, h;
};

class Animation {
	AnimRect frames[MAX_ANIMATION_FRAMES];
	float animSpeed = 0.2;
	int frameCount = 0;
	int currentFrame = 0;
	bool static_animation = false;
	float timeStarted = 0;

public:
	int pushFrame(AnimRect rect) {
		frames[frameCount++] = rect;
		return frameCount;
	}
	void setSpeed(float speed) {
		animSpeed = speed;
	}

	void setStatic(bool static_anim) {
		static_animation = static_anim;
	}

	void setFrame(int frame) {
		ASSERT(frame < frameCount);
		currentFrame = frame;
	}

	void start() {
		timeStarted = Time.time;
	}

	AnimRect getCurrentFrame() {

		ASSERT(frameCount != 0); // No frames on the animation
		if (!static_animation)
		{
			float timeElapsed = Time.time - timeStarted;

			if (timeElapsed > animSpeed) {
				currentFrame++;
				timeStarted = Time.time;
				if (currentFrame > frameCount)
					currentFrame = 0;
			}
		}
		return frames[currentFrame];
	}

	AnimRect getFrame(int frame) {

		ASSERT(frame > frameCount); // Asking for a non existing frame
		return frames[frame];
	}
};