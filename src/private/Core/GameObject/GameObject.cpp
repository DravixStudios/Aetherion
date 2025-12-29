#include "Core/GameObject/GameObject.h"

GameObject::GameObject(String name) {
	this->m_name = name;

	this->transform = { };
	this->transform.location = { 0.f, 0.f, 0.f };
	this->transform.rotation = { 0.f, 0.f, 0.f };
	this->transform.scale = { 1.f, 1.f, 1.f };
}

String GameObject::GetName() {
	return this->m_name;
}

void GameObject::Start() {
	for (std::pair<String, Component*> component : this->m_components) {
		component.second->Start();
	}
}

void GameObject::Update() {
	for (std::pair<String, Component*> component : this->m_components) {
		component.second->Update();
	}
}

std::map<String, Component*> GameObject::GetComponents() {
	return this->m_components;
}

void GameObject::AddComponent(String name, Component* component) {
	this->m_components[name] = component;
}