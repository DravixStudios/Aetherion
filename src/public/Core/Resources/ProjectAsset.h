#pragma once
#include "Core/Containers.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Project {
	struct Asset {
		String name;
		String description;
	
		String editorScene;
		String runtimeScene;
	};

	inline void 
	to_json(json& j, const Project::Asset& project) {
		j = json{
			{ "name", project.name },
			{ "description", project.description },
			{ "editorScene", project.editorScene },
			{ "runtimeScene", project.runtimeScene }
		};
	}

	inline void
	from_json(const json& j, Project::Asset& project) {
		j.at("name").get_to(project.name);
		j.at("description").get_to(project.description);
		j.at("editorScene").get_to(project.editorScene);
		j.at("runtimeScene").get_to(project.runtimeScene);
	}
}
