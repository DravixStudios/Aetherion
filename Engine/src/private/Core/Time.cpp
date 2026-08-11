#include "Core/Time.h"

Time* Time::m_instance;

Time::Time() {
	this->deltaTime = 0.f;
	this->m_startTime = std::chrono::high_resolution_clock::now();
	this->m_currentTime = this->m_startTime;
	this->m_lastTime = this->m_startTime;
}

void 
Time::Start() {

}

void
Time::PreUpdate() {
	this->m_currentTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> duration = this->m_currentTime - this->m_lastTime;
	this->deltaTime = duration.count();
}

void
Time::Update() {

}

void
Time::PostUpdate() {
	this->m_lastTime = this->m_currentTime;
}

Time*
Time::GetInstance() {
	if (Time::m_instance == nullptr)
		Time::m_instance = new Time();
	return Time::m_instance;
}