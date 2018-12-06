#pragma once

#include <set>
#include <string>
#include <vector>

#include "GLTFObject.h"
#include "GLTFShader.h"

namespace GLTF {
	class Program : public GLTF::Object {
	public:
		std::set<std::string> attributes;
		std::shared_ptr<GLTF::Shader> fragmentShader;
		std::shared_ptr<GLTF::Shader> vertexShader;

		virtual std::string typeName();
		virtual void writeJSON(void* writer, GLTF::Options* options);
	};
}
