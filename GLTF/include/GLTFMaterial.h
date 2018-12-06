#pragma once

#include <string>
#include <cmath>

#include "GLTFObject.h"
#include "GLTFTechnique.h"
#include "GLTFTexture.h"

namespace GLTF {
	class Material : public GLTF::Object {
	public:
		enum Type {
			MATERIAL,
			MATERIAL_COMMON,
			PBR_METALLIC_ROUGHNESS,
			UNKNOWN
		};

		class Values {
		public:
            std::shared_ptr<float[]> ambient;
			std::shared_ptr<GLTF::Texture> ambientTexture;
			int ambientTexCoord = 0;
            std::shared_ptr<float[]> diffuse;
			std::shared_ptr<GLTF::Texture> diffuseTexture;
			int diffuseTexCoord = 0;
            std::shared_ptr<float[]> emission;
			std::shared_ptr<GLTF::Texture> emissionTexture;
			int emissionTexCoord = 0;
            std::shared_ptr<float[]> specular;
			std::shared_ptr<GLTF::Texture> specularTexture;
			int specularTexCoord = 0;
            std::shared_ptr<float[]> shininess;
            std::shared_ptr<float[]> transparency;
			std::shared_ptr<GLTF::Texture> bumpTexture;

			void writeJSON(void* writer, GLTF::Options* options);
		};

		std::shared_ptr<GLTF::Technique> technique;
		Type type = Type::UNKNOWN;
		std::shared_ptr<Values> values;
		bool doubleSided = false;

		Material();
		bool hasTexture();
		virtual std::string typeName();
		virtual void writeJSON(void* writer, GLTF::Options* options);
	};

	class MaterialPBR : public GLTF::Material {
	public: 
		class Texture : public GLTF::Object {
		public:
			float scale = 1;
			std::shared_ptr<GLTF::Texture> texture;
			int texCoord = -1;

			void writeJSON(void* writer, GLTF::Options* options);
		};

		class MetallicRoughness : public GLTF::Object {
		public:
            std::shared_ptr<float[]> baseColorFactor;
			std::shared_ptr<Texture> baseColorTexture;
			float metallicFactor = -1.0;
			float roughnessFactor = -1.0;
			std::shared_ptr<Texture> metallicRoughnessTexture;

			void writeJSON(void* writer, GLTF::Options* options);
		};

		class SpecularGlossiness : public GLTF::Object {
		public:
            std::shared_ptr<float[]> diffuseFactor;
			std::shared_ptr<Texture> diffuseTexture;
            std::shared_ptr<float[]> specularFactor;
			std::shared_ptr<Texture> specularGlossinessTexture;
            std::shared_ptr<float[]> glossinessFactor;

			void writeJSON(void* writer, GLTF::Options* options);
		};

		std::shared_ptr<MetallicRoughness> metallicRoughness;
		std::shared_ptr<Texture> normalTexture;
		std::shared_ptr<Texture> occlusionTexture;
        std::shared_ptr<float[]> emissiveFactor;
		std::shared_ptr<Texture> emissiveTexture;
		std::shared_ptr<SpecularGlossiness> specularGlossiness;

		/** Either "OPAQUE", "BLEND" or "MASK". Default = "OPAQUE" */
		std::string alphaMode;

		/** Only when alphaMode == "MASK". Default = 0.5 */
		float alphaCutoff = NAN;

		bool doubleSided = false;

		MaterialPBR();
		void writeJSON(void* writer, GLTF::Options* options);
	};

	class MaterialCommon : public GLTF::Material {
	public:
		enum Technique {
			BLINN,
			PHONG,
			LAMBERT,
			CONSTANT,
			UNKNOWN
		};

		class Light: public GLTF::Object {
		public:
			enum Type {
				AMBIENT,
				DIRECTIONAL,
				POINT,
				SPOT,
				UNKOWN
			};

			Type type = Type::UNKOWN;
			float color[4];
			float constantAttenuation;
			float linearAttenuation;
			float quadraticAttenuation;
			std::weak_ptr<GLTF::Node> node;

			virtual std::string typeName();
			virtual void writeJSON(void* writer, GLTF::Options* options);
		};

		int jointCount = 0;
		bool transparent = false;

		MaterialCommon::Technique technique = MaterialCommon::Technique::UNKNOWN;

		MaterialCommon();
		const char* getTechniqueName();
		std::shared_ptr<GLTF::Material> getMaterial(std::vector<std::shared_ptr<GLTF::MaterialCommon::Light>> lights, GLTF::Options* options);
        std::shared_ptr<GLTF::Material> getMaterial(std::vector<std::shared_ptr<GLTF::MaterialCommon::Light>> lights, bool hasColorAttribute, GLTF::Options* options);
		std::string getTechniqueKey(GLTF::Options* options);
        std::shared_ptr<GLTF::MaterialPBR> getMaterialPBR(GLTF::Options* options);
		virtual void writeJSON(void* writer, GLTF::Options* options);
	};
}
