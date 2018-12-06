#pragma once

#include "GLTFConstants.h"
#include "GLTFImage.h"
#include "GLTFObject.h"
#include "GLTFSampler.h"

namespace GLTF {
	class Texture : public GLTF::Object {
	public:
		std::shared_ptr<GLTF::Sampler> sampler = NULL;
		std::shared_ptr<GLTF::Image> source = NULL;
		
		virtual std::string typeName();
		virtual void writeJSON(void* writer, GLTF::Options* options);
	};
}
