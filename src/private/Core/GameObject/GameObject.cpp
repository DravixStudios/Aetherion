#include "Core/GameObject/GameObject.h"

GameObject::GameObject(std::string name) {
	this->m_name = name;
}

std::string GameObject::GetName() {
	return this->m_name;
}

void GameObject::Start() {
	for (std::pair<std::string, Component*> component : this->m_components) {
		component.second->Start();
	}
}

void GameObject::Update() {
	for (std::pair<std::string, Component*> component : this->m_components) {
		component.second->Update();
	}
}

std::map<std::string, Component*> GameObject::GetComponents() {
	return this->m_components;
}

void GameObject::AddComponent(std::string name, Component* component) {
	this->m_components[name] = component;
}