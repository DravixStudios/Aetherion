#include "Core/GameObject/GameObject.h"

GameObject::GameObject(std::string name) {
	this->m_name = name;
}

std::string GameObject::GetName() {
	return this->m_name;
}

void GameObject::Start() {

}

void GameObject::Update() {

}