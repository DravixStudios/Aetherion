#pragma once
#include <iostream>
#include <time.h>

class Time {
public:
	Time();

	void Start();
	void PreUpdate();
	void Update();
	void PostUpdate();

	float deltaTime;

	static Time* GetInstance();
private:
	float m_currentTime;
	float m_lastTime;

	static Time* m_instance;
};