#pragma once

#include <set>
#include <string>
#include <vector>

#include "GLTFAnimation.h"
#include "GLTFDracoExtension.h"
#include "GLTFObject.h"
#include "GLTFScene.h"

#include "draco/compression/encode.h"

namespace GLTF {
	class Asset : public GLTF::Object {
	private:
		std::vector<std::shared_ptr<GLTF::MaterialCommon::Light>> _ambientLights;
	public:
		class Metadata : public GLTF::Object {
		public:
			std::string copyright;
			std::string generator = "COLLADA2GLTF";
			std::string version = "2.0";
			virtual void writeJSON(void* writer, GLTF::Options* options);
		};

		std::shared_ptr<GLTF::Sampler> globalSampler;

		Metadata* metadata = NULL;
		std::set<std::string> extensionsUsed;
		std::set<std::string> extensionsRequired;

		std::vector<std::shared_ptr<GLTF::Scene>> scenes;
		std::vector<std::shared_ptr<GLTF::Animation>> animations;
		int scene = -1;

		Asset();
		std::shared_ptr<GLTF::Scene> getDefaultScene();
		std::vector<std::shared_ptr<GLTF::Accessor>> getAllAccessors();
		std::vector<std::shared_ptr<GLTF::Node>> getAllNodes();
		std::vector<std::shared_ptr<GLTF::Mesh>> getAllMeshes();
		std::vector<std::shared_ptr<GLTF::Primitive>> getAllPrimitives();
		std::vector<std::shared_ptr<GLTF::Skin>> getAllSkins();
		std::vector<std::shared_ptr<GLTF::Material>> getAllMaterials();
		std::vector<std::shared_ptr<GLTF::Technique>> getAllTechniques();
		std::vector<std::shared_ptr<GLTF::Program>> getAllPrograms();
		std::vector<std::shared_ptr<GLTF::Shader>> getAllShaders();
		std::vector<std::shared_ptr<GLTF::Texture>> getAllTextures();
		std::vector<std::shared_ptr<GLTF::Image>> getAllImages();
		std::vector<std::shared_ptr<GLTF::Accessor>> getAllPrimitiveAccessors(GLTF::Primitive* primitive) const;
		void mergeAnimations();
		void removeUnusedSemantics();
		void removeUnusedNodes(GLTF::Options* options);
		std::shared_ptr<GLTF::Buffer> packAccessors();

		// Functions for Draco compression extension.
		std::vector<std::shared_ptr<GLTF::BufferView>> getAllCompressedBufferView();
		bool compressPrimitives(GLTF::Options* options);
		void removeUncompressedBufferViews();
		void removeAttributeFromDracoExtension(GLTF::Primitive* primitive, const std::string &semantic);

		void requireExtension(std::string extension);
		void useExtension(std::string extension);
		virtual void writeJSON(void* writer, GLTF::Options* options);
	};
}
