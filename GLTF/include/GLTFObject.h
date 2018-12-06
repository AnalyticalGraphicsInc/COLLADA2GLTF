#pragma once

#include "GLTFOptions.h"

#include <map>
#include <string>
#include <vector>

namespace GLTF {
	class Extension;
	class Object {
	public:
		int id = -1;
		std::string stringId;
		std::string name;
		std::map<std::string, std::shared_ptr<GLTF::Extension>> extensions;
		std::map<std::string, std::shared_ptr<GLTF::Object>> extras;

		std::string getStringId();
		virtual std::string typeName();
		virtual GLTF::Object* clone(GLTF::Object* clone);
		virtual void writeJSON(void* writer, GLTF::Options* options);
	};
}
