#include "Core/Time.h"

Time* Time::m_instance;

Time::Time() {
	this->deltaTime = 0.f;
	this->m_currentTime = 0.f;
	this->m_lastTime = 0.f;
}

void 
Time::Start() {

}

void
Time::PreUpdate() {
	this->m_currentTime = static_cast<float>(clock()) / CLOCKS_PER_SEC;
	if (this->m_lastTime != 0.f) {
		this->deltaTime = this->m_currentTime - this->m_lastTime;
	}
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