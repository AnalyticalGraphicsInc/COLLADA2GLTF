#pragma once

#include <map>
#include <string>

#include "GLTFAccessor.h"
#include "GLTFDracoExtension.h"
#include "GLTFMaterial.h"
#include "GLTFObject.h"

namespace GLTF {
	class Primitive : public GLTF::Object {
	public:
		enum Mode {
			UNKNOWN = -1,
			POINTS = 0,
			LINES = 1,
			LINE_LOOP = 2,
			LINE_STRIP = 3,
			TRIANGLES = 4,
			TRIANGLE_STRIP = 5,
			TRIANGLE_FAN = 6,
		};

		class Target {
		public:
			std::map<std::string, std::shared_ptr<GLTF::Accessor>> attributes;

			Target* clone(GLTF::Object* clone);
			void writeJSON(void* writer, GLTF::Options* options);
		};

		std::map<std::string, std::shared_ptr<GLTF::Accessor>> attributes;
		std::shared_ptr<GLTF::Accessor> indices;
        std::shared_ptr<GLTF::Material> material;
		Mode mode = Mode::UNKNOWN;
		std::vector<std::shared_ptr<Target>> targets;

		virtual GLTF::Object* clone(GLTF::Object* clone);
		virtual void writeJSON(void* writer, GLTF::Options* options);
	};
}
