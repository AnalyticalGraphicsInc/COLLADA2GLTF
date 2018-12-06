#include "GLTFAsset.h"

#include <algorithm>
#include <functional>
#include <map>
#include <set>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

std::map <std::shared_ptr<GLTF::Image>, std::shared_ptr<GLTF::Texture>> _pbrTextureCache;

GLTF::Asset::Asset() {
	metadata = new GLTF::Asset::Metadata();
	globalSampler = std::make_shared<GLTF::Sampler>();
}

void GLTF::Asset::Metadata::writeJSON(void* writer, GLTF::Options* options) {
	rapidjson::Writer<rapidjson::StringBuffer>* jsonWriter = (rapidjson::Writer<rapidjson::StringBuffer>*)writer;
	if (options->version != "") {
		version = options->version;
	}
	if (options->version == "1.0") {
		jsonWriter->Key("premultipliedAlpha");
		jsonWriter->Bool(true);
		jsonWriter->Key("profile");
		jsonWriter->StartObject();
		jsonWriter->Key("api");
		jsonWriter->String("WebGL");
		jsonWriter->Key("version");
		jsonWriter->String("1.0.2");
		jsonWriter->EndObject();
	}
	if (copyright.length() > 0) {
		jsonWriter->Key("copyright");
		jsonWriter->String(copyright.c_str());
	}
	if (generator.length() > 0) {
		jsonWriter->Key("generator");
		jsonWriter->String(generator.c_str());
	}
	if (version.length() > 0) {
		jsonWriter->Key("version");
		jsonWriter->String(version.c_str());
	}
	GLTF::Object::writeJSON(writer, options);
}

std::shared_ptr<GLTF::Scene> GLTF::Asset::getDefaultScene() {
	std::shared_ptr<GLTF::Scene> scene;
	if (this->scene < 0) {
		scene = std::make_shared<GLTF::Scene>();
		this->scenes.push_back(scene);
		this->scene = 0;
	}
	else {
		scene = this->scenes[this->scene];
	}
	return scene;
}

std::vector< std::shared_ptr<GLTF::Accessor>> GLTF::Asset::getAllAccessors() {
	std::set<std::shared_ptr<GLTF::Accessor>> uniqueAccessors;
	std::vector<std::shared_ptr<GLTF::Accessor>> accessors;
	for (auto& skin : getAllSkins()) {
		std::shared_ptr<GLTF::Accessor> inverseBindMatrices = skin->inverseBindMatrices;
		if (inverseBindMatrices != NULL) {
			if (uniqueAccessors.find(inverseBindMatrices) == uniqueAccessors.end()) {
				accessors.push_back(inverseBindMatrices);
				uniqueAccessors.insert(inverseBindMatrices);
			}
		}
	}

	for (auto& primitive : getAllPrimitives()) {
		for (auto& accessor: getAllPrimitiveAccessors(primitive.get())) {
			if (uniqueAccessors.find(accessor) == uniqueAccessors.end()) {
				accessors.push_back(accessor);
				uniqueAccessors.insert(accessor);
			}
		}
		std::shared_ptr<GLTF::Accessor> indicesAccessor = primitive->indices;
		if (indicesAccessor != NULL) {
			if (uniqueAccessors.find(indicesAccessor) == uniqueAccessors.end()) {
				accessors.push_back(indicesAccessor);
				uniqueAccessors.insert(indicesAccessor);
			}
		}
	}

	for (auto& animation : animations) {
		for (auto& channel : animation->channels) {
			auto sampler = channel->sampler;
			if (uniqueAccessors.find(sampler->input) == uniqueAccessors.end()) {
				accessors.push_back(sampler->input);
				uniqueAccessors.insert(sampler->input);
			}
			if (uniqueAccessors.find(sampler->output) == uniqueAccessors.end()) {
				accessors.push_back(sampler->output);
				uniqueAccessors.insert(sampler->input);
			}
		}
	}
	return accessors;
}

std::vector<std::shared_ptr<GLTF::Node>> GLTF::Asset::getAllNodes() {
	std::vector<std::shared_ptr<GLTF::Node>> nodeStack;
	std::vector<std::shared_ptr<GLTF::Node>> nodes;
	std::set<std::shared_ptr<GLTF::Node>> uniqueNodes;
	for (auto& node : getDefaultScene()->nodes) {
		nodeStack.push_back(node);
	}
	while (nodeStack.size() > 0) {
		auto node = nodeStack.back();
		if (uniqueNodes.find(node) == uniqueNodes.end()) {
			nodes.push_back(node);
			uniqueNodes.insert(node);
		}
		nodeStack.pop_back();
		for (auto& child : node->children) {
			nodeStack.push_back(child);
		}
		auto skin = node->skin;
		if (skin != NULL) {
            auto skeleton = skin->skeleton;
			if (skeleton != NULL) {
				nodeStack.push_back(skeleton);
			}
			for (auto jointNode : skin->joints) {
				nodeStack.push_back(jointNode);
			}
		}
	}
	return nodes;
}

std::vector<std::shared_ptr<GLTF::Mesh>> GLTF::Asset::getAllMeshes() {
	std::vector<std::shared_ptr<GLTF::Mesh>> meshes;
	std::set<std::shared_ptr<GLTF::Mesh>> uniqueMeshes;
	for (auto& node : getAllNodes()) {
		if (node->mesh != NULL) {
			if (uniqueMeshes.find(node->mesh) == uniqueMeshes.end()) {
				meshes.push_back(node->mesh);
				uniqueMeshes.insert(node->mesh);
			}
		}
	}
	return meshes;
}

std::vector<std::shared_ptr<GLTF::Primitive>> GLTF::Asset::getAllPrimitives() {
	std::vector<std::shared_ptr<GLTF::Primitive>> primitives;
	std::set<std::shared_ptr<GLTF::Primitive>> uniquePrimitives;
	for (auto& mesh : getAllMeshes()) {
		for (auto& primitive : mesh->primitives) {
			if (uniquePrimitives.find(primitive) == uniquePrimitives.end()) {
				primitives.push_back(primitive);
				uniquePrimitives.insert(primitive);
			}
		}
	}
	return primitives;
}

std::vector<std::shared_ptr<GLTF::Skin>> GLTF::Asset::getAllSkins() {
	std::vector<std::shared_ptr<GLTF::Skin>> skins;
	std::set<std::shared_ptr<GLTF::Skin>> uniqueSkins;
	for (auto& node : getAllNodes()) {
		auto skin = node->skin;
		if (skin != NULL) {
			if (uniqueSkins.find(skin) == uniqueSkins.end()) {
				skins.push_back(skin);
				uniqueSkins.insert(skin);
			}
		}
	}
	return skins;
}

std::vector<std::shared_ptr<GLTF::Material>> GLTF::Asset::getAllMaterials() {
	std::vector<std::shared_ptr<GLTF::Material>> materials;
	std::set<std::shared_ptr<GLTF::Material>> uniqueMaterials;
	for (auto& primitive : getAllPrimitives()) {
		auto material = primitive->material;
		if (material != NULL) {
			if (uniqueMaterials.find(material) == uniqueMaterials.end()) {
				materials.push_back(material);
				uniqueMaterials.insert(material);
			}
		}
	}
	return materials;
}

std::vector<std::shared_ptr<GLTF::Technique>> GLTF::Asset::getAllTechniques() {
	std::vector<std::shared_ptr<GLTF::Technique>> techniques;
	std::set<std::shared_ptr<GLTF::Technique>> uniqueTechniques;
	for (auto& material : getAllMaterials()) {
		std::shared_ptr<GLTF::Technique> technique = material->technique;
		if (technique != NULL) {
			if (uniqueTechniques.find(technique) == uniqueTechniques.end()) {
				techniques.push_back(technique);
				uniqueTechniques.insert(technique);
			}
		}
	}
	return techniques;
}

std::vector<std::shared_ptr<GLTF::Program>> GLTF::Asset::getAllPrograms() {
	std::vector<std::shared_ptr<GLTF::Program>> programs;
	std::set<std::shared_ptr<GLTF::Program>> uniquePrograms;
	for (auto& technique : getAllTechniques()) {
		auto program = technique->program;
		if (program != NULL) {
			if (uniquePrograms.find(program) == uniquePrograms.end()) {
				programs.push_back(program);
				uniquePrograms.insert(program);
			}
		}
	}
	return programs;
}
std::vector<std::shared_ptr<GLTF::Shader>> GLTF::Asset::getAllShaders() {
	std::vector<std::shared_ptr<GLTF::Shader>> shaders;
	std::set<std::shared_ptr<GLTF::Shader>> uniqueShaders;
	for (auto& program : getAllPrograms()) {
		auto vertexShader = program->vertexShader;
		if (vertexShader != NULL) {
			if (uniqueShaders.find(vertexShader) == uniqueShaders.end()) {
				shaders.push_back(vertexShader);
				uniqueShaders.insert(vertexShader);
			}
		}
		auto fragmentShader = program->fragmentShader;
		if (fragmentShader != NULL) {
			if (uniqueShaders.find(fragmentShader) == uniqueShaders.end()) {
				shaders.push_back(fragmentShader);
				uniqueShaders.insert(fragmentShader);
			}
		}
	}
	return shaders;
}

std::vector<std::shared_ptr<GLTF::Texture>> GLTF::Asset::getAllTextures() {
	std::vector<std::shared_ptr<GLTF::Texture>> textures;
	std::set<std::shared_ptr<GLTF::Texture>> uniqueTextures;
	for (auto& material : getAllMaterials()) {
		if (material->type == GLTF::Material::MATERIAL || material->type == GLTF::Material::MATERIAL_COMMON) {
			auto values = material->values;
			if (values->ambientTexture != NULL) {
				if (uniqueTextures.find(values->ambientTexture) == uniqueTextures.end()) {
					textures.push_back(values->ambientTexture);
					uniqueTextures.insert(values->ambientTexture);
				}
			}
			if (values->diffuseTexture != NULL) {
				if (uniqueTextures.find(values->diffuseTexture) == uniqueTextures.end()) {
					textures.push_back(values->diffuseTexture);
					uniqueTextures.insert(values->diffuseTexture);
				}
			}
			if (values->emissionTexture != NULL) {
				if (uniqueTextures.find(values->emissionTexture) == uniqueTextures.end()) {
					textures.push_back(values->emissionTexture);
					uniqueTextures.insert(values->emissionTexture);
				}
			}
			if (values->specularTexture != NULL) {
				if (uniqueTextures.find(values->specularTexture) == uniqueTextures.end()) {
					textures.push_back(values->specularTexture);
					uniqueTextures.insert(values->specularTexture);
				}
			}
			if (values->bumpTexture != NULL) {
				if (uniqueTextures.find(values->bumpTexture) == uniqueTextures.end()) {
					textures.push_back(values->bumpTexture);
					uniqueTextures.insert(values->bumpTexture);
				}
			}
		}
		else if (material->type == GLTF::Material::PBR_METALLIC_ROUGHNESS) {
			auto materialPBR = std::static_pointer_cast<GLTF::MaterialPBR>(material);
			if (materialPBR->metallicRoughness->baseColorTexture != NULL) {
				if (uniqueTextures.find(materialPBR->metallicRoughness->baseColorTexture->texture) == uniqueTextures.end()) {
					textures.push_back(materialPBR->metallicRoughness->baseColorTexture->texture);
					uniqueTextures.insert(materialPBR->metallicRoughness->baseColorTexture->texture);
				}
			}
			if (materialPBR->metallicRoughness->metallicRoughnessTexture != NULL) {
				if (uniqueTextures.find(materialPBR->metallicRoughness->metallicRoughnessTexture->texture) == uniqueTextures.end()) {
					textures.push_back(materialPBR->metallicRoughness->metallicRoughnessTexture->texture);
					uniqueTextures.insert(materialPBR->metallicRoughness->metallicRoughnessTexture->texture);
				}
			}
			if (materialPBR->emissiveTexture != NULL) {
				if (uniqueTextures.find(materialPBR->emissiveTexture->texture) == uniqueTextures.end()) {
					textures.push_back(materialPBR->emissiveTexture->texture);
					uniqueTextures.insert(materialPBR->emissiveTexture->texture);
				}
			}
			if (materialPBR->normalTexture != NULL) {
				if (uniqueTextures.find(materialPBR->normalTexture->texture) == uniqueTextures.end()) {
					textures.push_back(materialPBR->normalTexture->texture);
					uniqueTextures.insert(materialPBR->normalTexture->texture);
				}
			}
			if (materialPBR->occlusionTexture != NULL) {
				if (uniqueTextures.find(materialPBR->occlusionTexture->texture) == uniqueTextures.end()) {
					textures.push_back(materialPBR->occlusionTexture->texture);
					uniqueTextures.insert(materialPBR->occlusionTexture->texture);
				}
			}
			if (materialPBR->specularGlossiness->diffuseTexture != NULL) {
				if (uniqueTextures.find(materialPBR->specularGlossiness->diffuseTexture->texture) == uniqueTextures.end()) {
					textures.push_back(materialPBR->specularGlossiness->diffuseTexture->texture);
					uniqueTextures.insert(materialPBR->specularGlossiness->diffuseTexture->texture);
				}
			}
			if (materialPBR->specularGlossiness->specularGlossinessTexture != NULL) {
				if (uniqueTextures.find(materialPBR->specularGlossiness->specularGlossinessTexture->texture) == uniqueTextures.end()) {
					textures.push_back(materialPBR->specularGlossiness->specularGlossinessTexture->texture);
					uniqueTextures.insert(materialPBR->specularGlossiness->specularGlossinessTexture->texture);
				}
			}
		}
	}
	return textures;
}

std::vector<std::shared_ptr<GLTF::Image>> GLTF::Asset::getAllImages() {
	std::vector<std::shared_ptr<GLTF::Image>> images;
	std::set<std::shared_ptr<GLTF::Image>> uniqueImages;
	for (auto& texture : getAllTextures()) {
		auto image = texture->source;
		if (image != NULL) {
			if (uniqueImages.find(image) == uniqueImages.end()) {
				images.push_back(image);
				uniqueImages.insert(image);
			}
		}
	}
	return images;
}

std::vector<std::shared_ptr<GLTF::Accessor>> GLTF::Asset::getAllPrimitiveAccessors(GLTF::Primitive* primitive) const
{
	std::vector<std::shared_ptr<GLTF::Accessor>> accessors;

	for (const auto& attribute : primitive->attributes) {
		accessors.emplace_back(attribute.second);
	}
	for (const auto& target: primitive->targets) {
		for (const auto& attribute : target->attributes) {
			accessors.emplace_back(attribute.second);
		}
	}

	return move(accessors);
}

std::vector<std::shared_ptr<GLTF::BufferView>> GLTF::Asset::getAllCompressedBufferView() {
	std::vector<std::shared_ptr<GLTF::BufferView>> compressedBufferViews;
	std::set<std::shared_ptr<GLTF::BufferView>> uniqueCompressedBufferViews;
	for (auto& primitive : getAllPrimitives()) {
		auto dracoExtensionPtr = primitive->extensions.find("KHR_draco_mesh_compression");
		if (dracoExtensionPtr != primitive->extensions.end()) {
			auto bufferView = std::static_pointer_cast<GLTF::DracoExtension>(dracoExtensionPtr->second)->bufferView;
			if (uniqueCompressedBufferViews.find(bufferView) == uniqueCompressedBufferViews.end()) {
				compressedBufferViews.push_back(bufferView);
				uniqueCompressedBufferViews.insert(bufferView);
			}
		}
	}
	return compressedBufferViews;
}

void GLTF::Asset::mergeAnimations() {
	if (animations.size() == 0) { return; }

	auto mergedAnimation = std::make_shared<GLTF::Animation>();

	// Merge all animations. In the future, animations should be grouped
	// according to COLLADA <animation_clip/> nodes.
	for (auto& animation : animations) {
		for (auto& channel : animation->channels) {
			mergedAnimation->channels.push_back(channel);
		}
	}

	animations.clear();
	animations.push_back(mergedAnimation);
}

void GLTF::Asset::removeUncompressedBufferViews() {
	for (auto& primitive : getAllPrimitives()) {
		auto dracoExtensionPtr = primitive->extensions.find("KHR_draco_mesh_compression");
		if (dracoExtensionPtr != primitive->extensions.end()) {
			// Currently assume all attributes are compressed in Draco extension.
			for (const auto accessor: getAllPrimitiveAccessors(primitive.get())) {
				if (accessor->bufferView) {
					delete accessor->bufferView;
					accessor->bufferView = NULL;
				}
			}
			auto indicesAccessor = primitive->indices;
			if (indicesAccessor != NULL && indicesAccessor->bufferView) {
				delete indicesAccessor->bufferView;
				indicesAccessor->bufferView = NULL;
			}
		}
	}
}

void GLTF::Asset::removeUnusedSemantics() {
	// Remove unused TEXCOORD semantics
	std::map<std::shared_ptr<GLTF::Material>, std::vector<std::shared_ptr<GLTF::Primitive>>> materialMappings;
	auto primitives = getAllPrimitives();
	for (auto& primitive : primitives) {
		auto material = primitive->material;
		if (material != NULL) {
			auto values = material->values;
			auto attributes = primitive->attributes;
			std::vector<std::string> semantics;
			for (const auto attribute : attributes) {
				std::string semantic = attribute.first;
				if (semantic.find("TEXCOORD") != std::string::npos) {
					semantics.push_back(semantic);
				}
			}
			for (const std::string semantic : semantics) {
				auto removeTexcoord = primitive->attributes.find(semantic);
				size_t index = std::stoi(semantic.substr(semantic.find_last_of('_') + 1));
				if ((values->ambientTexture == NULL || values->ambientTexCoord != index) &&
						(values->diffuseTexture == NULL || values->diffuseTexCoord != index) &&
						(values->emissionTexture == NULL || values->emissionTexCoord != index) &&
						(values->specularTexture == NULL || values->specularTexCoord != index) &&
						(values->bumpTexture == NULL)) {
					auto findMaterialMapping = materialMappings.find(material);
					if (findMaterialMapping == materialMappings.end()) {
						materialMappings[material] = std::vector<std::shared_ptr<GLTF::Primitive>>();
					}
					materialMappings[material].push_back(primitive);

					auto removeTexcoord = primitive->attributes.find(semantic);
					primitive->attributes.erase(removeTexcoord);
					// TODO: This will need to be adjusted for multiple maps
					removeAttributeFromDracoExtension(primitive.get(), semantic);
				}
			}
		}
	}
	// Remove holes, i.e. if we removed TEXCOORD_0, change TEXCOORD_1 -> TEXCOORD_0
	for (const auto materialMapping : materialMappings) {
		std::map<size_t, size_t> indexMapping;
		for (auto& primitive : materialMapping.second) {
			std::map<std::string, std::shared_ptr<GLTF::Accessor>> rebuildAttributes;
			std::vector<std::shared_ptr<GLTF::Accessor>> texcoordAccessors;
			size_t index;
			for (const auto attribute : primitive->attributes) {
				std::string semantic = attribute.first;
				if (semantic.find("TEXCOORD") != std::string::npos) {
					index = std::stoi(semantic.substr(semantic.find_last_of('_') + 1));
					while (index >= texcoordAccessors.size()) {
						texcoordAccessors.push_back(NULL);
					}
					texcoordAccessors[index] = attribute.second;
				} else {
					rebuildAttributes[semantic] = attribute.second;
				}
			}
			index = 0;
			for (size_t i = 0; i < texcoordAccessors.size(); i++) {
				auto texcoordAccessor = texcoordAccessors[i];
				if (texcoordAccessor != NULL) {
					indexMapping[i] = index;
					rebuildAttributes["TEXCOORD_" + std::to_string(index)] = texcoordAccessor;
					index++;
				}
			}
			primitive->attributes = rebuildAttributes;
		}
		// Fix material texcoord references
		auto material = materialMapping.first;
		auto values = material->values;
		for (const auto indexMap : indexMapping) {
			size_t from = indexMap.first;
			size_t to = indexMap.second;
			if (values->ambientTexCoord == from) {
				values->ambientTexCoord = to;
			}
			if (values->diffuseTexCoord == from) {
				values->diffuseTexCoord = to;
			}
			if (values->emissionTexCoord == from) {
				values->emissionTexCoord = to;
			}
			if (values->specularTexCoord == from) {
				values->specularTexCoord = to;
			}
		}
	}
}

void GLTF::Asset::removeAttributeFromDracoExtension(GLTF::Primitive* primitive, const std::string &semantic) {
	auto extensionPtr = primitive->extensions.find("KHR_draco_mesh_compression");
	if (extensionPtr != primitive->extensions.end()) {
		auto dracoExtension = std::static_pointer_cast<GLTF::DracoExtension>(extensionPtr->second);
		auto attPtr = dracoExtension->attributeToId.find(semantic);
		if (attPtr != dracoExtension->attributeToId.end()) {
			const int att_id = attPtr->second;
			// Remove from the extension.
			dracoExtension->attributeToId.erase(attPtr);
			// Remove from draco mesh.
			dracoExtension->dracoMesh->DeleteAttribute(att_id);
		}
	}
}

bool isUnusedNode(std::shared_ptr<GLTF::Node>& node, const std::set<std::shared_ptr<GLTF::Node>>& skinNodes, bool isPbr) {
	if (node->children.size() == 0 && node->mesh == NULL && node->camera == NULL && node->skin == NULL) {
		if (isPbr || node->light == NULL || node->light->type == GLTF::MaterialCommon::Light::AMBIENT) {
			if (std::find(skinNodes.begin(), skinNodes.end(), node) == skinNodes.end()) {
				return true;
			}
		}
	}
	return false;
}

void GLTF::Asset::removeUnusedNodes(GLTF::Options* options) {
	std::vector<std::shared_ptr<GLTF::Node>> nodeStack;
	std::set<std::shared_ptr<GLTF::Node>> skinNodes;
	bool isPbr = !options->glsl && !options->materialsCommon;
	for (auto& skin : getAllSkins()) {
		if (skin->skeleton != NULL) {
			skinNodes.insert(skin->skeleton);
		}
		for (auto& jointNode : skin->joints) {
			skinNodes.insert(jointNode);
		}
	}

	auto defaultScene = getDefaultScene();
	bool needsPass = true;
	while (needsPass) {
		needsPass = false;
		for (size_t i = 0; i < defaultScene->nodes.size(); i++) {
			auto node = defaultScene->nodes[i];
			if (isUnusedNode(node, skinNodes, isPbr)) {
				// Nodes associated with ambient lights may be optimized out,
				// but we should hang on to the lights so that they are 
				// still written into the shader or common materials object.
				if (node->light != NULL) {
					_ambientLights.push_back(node->light);
				}
				defaultScene->nodes.erase(defaultScene->nodes.begin() + i);
				i--;
			}
			else {
				nodeStack.push_back(node);
			}
		}
		while (nodeStack.size() > 0) {
			auto node = nodeStack.back();
			nodeStack.pop_back();
			for (size_t i = 0; i < node->children.size(); i++) {
				auto child = node->children[i];
				if (isUnusedNode(child, skinNodes, isPbr)) {
					if (child->light != NULL) {
						_ambientLights.push_back(child->light);
					}
					// this node is extraneous, remove it
					node->children.erase(node->children.begin() + i);
					i--;
					if (node->children.size() == 0) {
						// another pass may be required to clean up the parent
						needsPass = true;
					}
				}
				else {
					nodeStack.push_back(child);
				}
			}
		}
	}
}

std::shared_ptr<GLTF::BufferView> packAccessorsForTargetByteStride(std::vector<std::shared_ptr<GLTF::Accessor>> accessors, GLTF::Constants::WebGL target, size_t byteStride) {
	std::map<GLTF::Accessor*, size_t> byteOffsets;
	size_t byteLength = 0;
	for (auto& accessor : accessors) {
		int componentByteLength = accessor->getComponentByteLength();
		int padding = byteLength % componentByteLength;
		if (padding != 0) {
			byteLength += (componentByteLength - padding);
		}
		byteOffsets[accessor.get()] = byteLength;
		byteLength += componentByteLength * accessor->getNumberOfComponents() * accessor->count;
	}
	unsigned char* bufferData = new unsigned char[byteLength];
	std::shared_ptr<GLTF::BufferView> bufferView = std::make_shared<GLTF::BufferView>(bufferData, byteLength, target);
	for (auto& accessor : accessors) {
		size_t byteOffset = byteOffsets[accessor.get()];
		auto packedAccessor = std::make_shared<GLTF::Accessor>(accessor->type, accessor->componentType, byteOffset, accessor->count, bufferView);
		int numberOfComponents = accessor->getNumberOfComponents();
		float* component = new float[numberOfComponents];
		for (int i = 0; i < accessor->count; i++) {
			accessor->getComponentAtIndex(i, component);
			packedAccessor->writeComponentAtIndex(i, component);
		}
		accessor->byteOffset = packedAccessor->byteOffset;
		accessor->bufferView = packedAccessor->bufferView;
	}
	return bufferView;
}

bool GLTF::Asset::compressPrimitives(GLTF::Options* options) {
	int totalPrimitives = 0;
	for (auto& primitive : getAllPrimitives()) {
		totalPrimitives++;
		auto dracoExtensionPtr = primitive->extensions.find("KHR_draco_mesh_compression");
		if (dracoExtensionPtr == primitive->extensions.end()) {
			// No extension exists.
			continue;
		}
		auto dracoExtension = std::static_pointer_cast<GLTF::DracoExtension>(dracoExtensionPtr->second);

		// Compress the mesh
		// Setup encoder options.
		draco::Encoder encoder;
		const int posQuantizationBits = options->positionQuantizationBits;
		const int texcoordsQuantizationBits = options->texcoordQuantizationBits;
		const int normalsQuantizationBits = options->normalQuantizationBits;
		const int colorQuantizationBits = options->colorQuantizationBits;
		// Used for compressing joint indices and joint weights.
		const int genericQuantizationBits = options->jointQuantizationBits;

		encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, posQuantizationBits);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, texcoordsQuantizationBits);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, normalsQuantizationBits);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::COLOR, colorQuantizationBits);
		encoder.SetAttributeQuantization(draco::GeometryAttribute::GENERIC, genericQuantizationBits);

		draco::EncoderBuffer buffer;
		const draco::Status status = encoder.EncodeMeshToBuffer(*dracoExtension->dracoMesh, &buffer);
		if (!status.ok()) {
			std::cerr << "Error: Encode mesh.\n";
			return false;
		}

		// Add compressed data to bufferview
		unsigned char* allocatedData = (unsigned char*)malloc(buffer.size());
		std::memcpy(allocatedData, buffer.data(), buffer.size());
		dracoExtension->bufferView = std::make_shared<GLTF::BufferView>(allocatedData, buffer.size());
		// Remove the mesh so duplicated primitives don't need to compress again.
		dracoExtension->dracoMesh.reset();
	}
	return true;
}

std::shared_ptr<GLTF::Buffer> GLTF::Asset::packAccessors() {
	std::map<GLTF::Constants::WebGL, std::map<int, std::vector<std::shared_ptr<GLTF::Accessor>>>> accessorGroups;
	accessorGroups[GLTF::Constants::WebGL::ARRAY_BUFFER] = std::map<int, std::vector<std::shared_ptr<GLTF::Accessor>>>();
	accessorGroups[GLTF::Constants::WebGL::ELEMENT_ARRAY_BUFFER] = std::map<int, std::vector<std::shared_ptr<GLTF::Accessor>>>();
	accessorGroups[(GLTF::Constants::WebGL)-1] = std::map<int, std::vector<std::shared_ptr<GLTF::Accessor>>>();

	size_t byteLength = 0;
	for (auto& accessor : getAllAccessors()) {
		// In glTF 2.0, bufferView is not required in accessor.
		if (accessor->bufferView == NULL) {
			continue;
		}
		GLTF::Constants::WebGL target = accessor->bufferView->target;
		auto targetGroup = accessorGroups[target];
		int byteStride = accessor->getByteStride();
		auto findByteStrideGroup = targetGroup.find(byteStride);
		std::vector<std::shared_ptr<GLTF::Accessor>> byteStrideGroup;
		if (findByteStrideGroup == targetGroup.end()) {
			byteStrideGroup = std::vector<std::shared_ptr<GLTF::Accessor>>();
		}
		else {
			byteStrideGroup = findByteStrideGroup->second;
		}
		byteStrideGroup.push_back(accessor);
		targetGroup[byteStride] = byteStrideGroup;
		accessorGroups[target] = targetGroup;
		byteLength += accessor->bufferView->byteLength;
	}

	// Go through primitives and look for primitives that use Draco extension.
	// If extension is not enabled, the vector will be empty.
	auto compressedBufferViews = getAllCompressedBufferView();
	// Reserve data for compressed data.
	for (auto& compressedBufferView : compressedBufferViews) {
		byteLength += compressedBufferView->byteLength;
	}

	std::vector<int> byteStrides;
	std::map<int, std::vector<std::shared_ptr<GLTF::BufferView>>> bufferViews;
	for (auto targetGroup : accessorGroups) {
		for (auto byteStrideGroup : targetGroup.second) {
			GLTF::Constants::WebGL target = targetGroup.first;
			int byteStride = byteStrideGroup.first;
			std::shared_ptr<GLTF::BufferView> bufferView = packAccessorsForTargetByteStride(byteStrideGroup.second, target, byteStride);
			if (target == GLTF::Constants::WebGL::ARRAY_BUFFER) {
				bufferView->byteStride = byteStride;
			}
			auto findBufferViews = bufferViews.find(byteStride);
			std::vector<std::shared_ptr<GLTF::BufferView>> bufferViewGroup;
			if (findBufferViews == bufferViews.end()) {
				byteStrides.push_back(byteStride);
				bufferViewGroup = std::vector<std::shared_ptr<GLTF::BufferView>>();
			}
			else {
				bufferViewGroup = findBufferViews->second;
			}
			bufferViewGroup.push_back(bufferView);
			bufferViews[byteStride] = bufferViewGroup;
		}
	}
	std::sort(byteStrides.begin(), byteStrides.end(), std::greater<int>());

	// Pack these into a buffer sorted from largest byteStride to smallest
	unsigned char* bufferData = new unsigned char[byteLength];
	std::shared_ptr<GLTF::Buffer> buffer = std::make_shared<GLTF::Buffer>(bufferData, byteLength);
	size_t byteOffset = 0;
	for (int byteStride : byteStrides) {
		for (auto& bufferView : bufferViews[byteStride]) {
			std::memcpy(bufferData + byteOffset, bufferView->buffer->data, bufferView->byteLength);
			bufferView->byteOffset = byteOffset;
			bufferView->buffer = buffer;
			byteOffset += bufferView->byteLength;
		}
	}

	// Append compressed data to buffer.
	for (auto& compressedBufferView : compressedBufferViews) {
		std::memcpy(bufferData + byteOffset, compressedBufferView->buffer->data, compressedBufferView->byteLength);
		compressedBufferView->byteOffset = byteOffset;
		compressedBufferView->buffer = buffer;
		byteOffset += compressedBufferView->byteLength;
	}
	return buffer;
}

void GLTF::Asset::requireExtension(std::string extension) {
	useExtension(extension);
	extensionsRequired.insert(extension);
}

void GLTF::Asset::useExtension(std::string extension) {
	extensionsUsed.insert(extension);
}

void GLTF::Asset::writeJSON(void* writer, GLTF::Options* options) {
	rapidjson::Writer<rapidjson::StringBuffer>* jsonWriter = (rapidjson::Writer<rapidjson::StringBuffer>*)writer;

	if (options->binary && options->version == "1.0") {
		useExtension("KHR_binary_glTF");
	}

	// Write asset metadata
	if (this->metadata) {
		jsonWriter->Key("asset");
		jsonWriter->StartObject();
		this->metadata->writeJSON(writer, options);
		jsonWriter->EndObject();
	}

	// Write scenes and build node array
	std::vector<std::shared_ptr<GLTF::Node>> nodes;
	if (this->scenes.size() > 0) {
		jsonWriter->Key("scenes");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		int sceneId = 0;
		for (auto& scene : this->scenes) {
			scene->id = sceneId;
			sceneId++;
			std::vector<std::shared_ptr<GLTF::Node>> nodeStack;
			for (auto& node : scene->nodes) {
				nodeStack.push_back(node);
			}
			while (nodeStack.size() > 0) {
				auto& node = nodeStack.back();
				nodeStack.pop_back();
				if (node->id < 0) {
					node->id = nodes.size();
					nodes.push_back(node);
				}
				for (auto& child : node->children) {
					nodeStack.push_back(child);
				}
				if (node->skin != NULL) {
					auto skin = node->skin;
					if (skin->skeleton != NULL) {
						auto skeletonNode = skin->skeleton;
						nodeStack.push_back(skin->skeleton);
					}
				}
			}
			if (options->version == "1.0") {
				jsonWriter->Key(scene->getStringId().c_str());
			}
			jsonWriter->StartObject();
			scene->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}

	// Write scene
	if (this->scene >= 0) {
		jsonWriter->Key("scene");
		if (options->version == "1.0") {
			jsonWriter->String(this->scenes[0]->getStringId().c_str());
		}
		else {
			jsonWriter->Int(this->scene);
		}
	}

	// Write nodes and build mesh, skin, camera, and light arrays
	std::vector<std::shared_ptr<GLTF::Mesh>> meshes;
	std::vector<std::shared_ptr<GLTF::Skin>> skins;
	std::vector<std::shared_ptr<GLTF::Camera>> cameras;
	auto lights = _ambientLights;
	if (nodes.size() > 0) {
		jsonWriter->Key("nodes");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& node : nodes) {
			auto& mesh = node->mesh;
			if (mesh != NULL) {
				if (mesh->id < 0) {
					mesh->id = meshes.size();
					meshes.push_back(mesh);
				}
			}
			if (node->skin != NULL) {
				if (node->skin->id < 0) {
					node->skin->id = skins.size();
					skins.push_back(node->skin);
				}
			}
			if (node->camera != NULL) {
				if (node->camera->id < 0) {
					node->camera->id = cameras.size();
					cameras.push_back(node->camera);
				}
			}
			auto light = node->light;
			if (light != NULL && light->id < 0) {
				light->id = lights.size();
				lights.push_back(light);
			}
			if (options->version == "1.0") {
				jsonWriter->Key(node->getStringId().c_str());
			}
			jsonWriter->StartObject();
			node->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	nodes.clear();

	// Write cameras
	if (cameras.size() > 0) {
		jsonWriter->Key("cameras");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& camera : cameras) {
			if (options->version == "1.0") {
				jsonWriter->Key(camera->getStringId().c_str());
			}
			jsonWriter->StartObject();
			camera->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}

	// Write meshes and build accessor and material arrays
	std::vector<std::shared_ptr<GLTF::Accessor>> accessors;
	std::vector<std::shared_ptr<GLTF::BufferView>> bufferViews;
	std::vector<std::shared_ptr<GLTF::Material>> materials;
	std::map<std::string, std::shared_ptr<GLTF::Technique>> generatedTechniques;
	if (meshes.size() > 0) {
		jsonWriter->Key("meshes");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& mesh : meshes) {
			for (auto& primitive : mesh->primitives) {
				if (primitive->material && primitive->material->id < 0) {
					auto material = primitive->material;
					if (!options->materialsCommon) {
						if (material->type == GLTF::Material::Type::MATERIAL_COMMON) {
							auto materialCommon = std::static_pointer_cast<GLTF::MaterialCommon>(material);
							if (options->glsl) {
								std::string techniqueKey = materialCommon->getTechniqueKey(options);
								auto findTechnique = generatedTechniques.find(techniqueKey);
								if (findTechnique != generatedTechniques.end()) {
									material = std::make_shared<GLTF::Material>();
									material->name = materialCommon->name;
									material->values = materialCommon->values;
									material->technique = findTechnique->second;
								}
								else {
									bool hasColor = primitive->attributes.find("COLOR_0") != primitive->attributes.end();
									material = materialCommon->getMaterial(lights, hasColor, options);
									generatedTechniques[techniqueKey] = material->technique;
								}
							}
							else {
								auto materialPbr = materialCommon->getMaterialPBR(options);
								if (options->lockOcclusionMetallicRoughness && materialPbr->occlusionTexture != NULL) {
									auto metallicRoughnessTexture = std::make_shared<GLTF::MaterialPBR::Texture>();
									metallicRoughnessTexture->texture = materialPbr->occlusionTexture->texture;
									materialPbr->metallicRoughness->metallicRoughnessTexture = metallicRoughnessTexture;
								}
								else if (options->metallicRoughnessTexturePaths.size() > 0) {
									std::string metallicRoughnessTexturePath = options->metallicRoughnessTexturePaths[0];
									if (options->metallicRoughnessTexturePaths.size() > 1) {
										size_t index = materials.size();
										if (index < options->metallicRoughnessTexturePaths.size()) {
											metallicRoughnessTexturePath = options->metallicRoughnessTexturePaths[index];
										}
									}
									if (options->metallicRoughnessTexturePaths.size() == 1) {
										metallicRoughnessTexturePath = options->metallicRoughnessTexturePaths[0];
									}
									auto metallicRoughnessTexture = std::make_shared<GLTF::MaterialPBR::Texture>();
									auto image = GLTF::Image::load(metallicRoughnessTexturePath);
									auto textureCacheIt = _pbrTextureCache.find(image);
									std::shared_ptr<GLTF::Texture> texture;
									if (textureCacheIt == _pbrTextureCache.end()) {
										texture = std::make_shared<GLTF::Texture>();
										texture->sampler = globalSampler;
										texture->source = image;
										_pbrTextureCache[image] = texture;
									}
									else {
										texture = textureCacheIt->second;
									}
									metallicRoughnessTexture->texture = texture;
									materialPbr->metallicRoughness->metallicRoughnessTexture = metallicRoughnessTexture;
								}
								material = materialPbr;
							}
						}
					}
					primitive->material = material;
					material->id = materials.size();
					materials.push_back(material);
				}

				// Find bufferViews of compressed data. These bufferViews does not belong to Accessors.
				auto dracoExtensionPtr = primitive->extensions.find("KHR_draco_mesh_compression");
					if (dracoExtensionPtr != primitive->extensions.end()) {
						auto bufferView = std::static_pointer_cast<GLTF::DracoExtension>(dracoExtensionPtr->second)->bufferView;
						if (bufferView->id < 0) {
							bufferView->id = bufferViews.size();
							bufferViews.push_back(bufferView);
						}
					}

				if (primitive->indices) {
					auto indices = primitive->indices;
					if (indices->id < 0) {
						indices->id = accessors.size();
						accessors.push_back(indices);
					}
				}
				for (auto& accessor: getAllPrimitiveAccessors(primitive.get())) {
					if (accessor->id < 0) {
						accessor->id = accessors.size();
						accessors.push_back(accessor);
					}
				}
			}
			if (options->version == "1.0") {
				jsonWriter->Key(mesh->getStringId().c_str());
			}
			jsonWriter->StartObject();
			mesh->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}

	// Write animations and add accessors to the accessor array
	if (animations.size() > 0) {
		jsonWriter->Key("animations");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (size_t i = 0; i < animations.size(); i++) {
			auto& animation = animations[i];
			animation->id = i;
			int numChannels = 0;
			for (auto& channel : animation->channels) {
				if (channel->target->node->id >= 0) {
					numChannels++;
					auto& sampler = channel->sampler;
					if (sampler->input->id < 0) {
						sampler->input->id = accessors.size();
						accessors.push_back(sampler->input);
					}
					if (sampler->output->id < 0) {
						sampler->output->id = accessors.size();
						accessors.push_back(sampler->output);
					}
				}
			}
			if (numChannels > 0) {
				if (options->version == "1.0") {
					jsonWriter->Key(animation->getStringId().c_str());
				}
				jsonWriter->StartObject();
				animation->writeJSON(writer, options);
				jsonWriter->EndObject();
			}
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}

	// Write skins and add accessors to the accessor array
	if (skins.size() > 0) {
		jsonWriter->Key("skins");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& skin : skins) {
			if (skin->inverseBindMatrices != NULL && skin->inverseBindMatrices->id < 0) {
				skin->inverseBindMatrices->id = accessors.size();
				accessors.push_back(skin->inverseBindMatrices);
			}
			if (options->version == "1.0") {
				jsonWriter->Key(skin->getStringId().c_str());
			}
			jsonWriter->StartObject();
			skin->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	skins.clear();

	// Write accessors and add bufferViews to the bufferView array
	if (accessors.size() > 0) {
		jsonWriter->Key("accessors");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& accessor : accessors) {
			if (accessor->bufferView) {
				auto bufferView = accessor->bufferView;
				if (bufferView->id < 0) {
					bufferView->id = bufferViews.size();
					bufferViews.push_back(bufferView);
				}
			}
			if (options->version == "1.0") {
				jsonWriter->Key(accessor->getStringId().c_str());
			}
			jsonWriter->StartObject();
			accessor->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}

	if (options->dracoCompression) {
		this->requireExtension("KHR_draco_mesh_compression");
	}

	meshes.clear();
	accessors.clear();

	// Write materials and build technique and texture arrays
	std::vector<std::shared_ptr<GLTF::Technique>> techniques;
	std::vector<std::shared_ptr<GLTF::Texture>> textures;
	bool usesTechniqueWebGL = false;
	bool usesMaterialsCommon = false;
	bool usesSpecularGlossiness = false;
	if (materials.size() > 0) {
		jsonWriter->Key("materials");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& material : materials) {
			if (material->type == GLTF::Material::Type::MATERIAL || material->type == GLTF::Material::Type::MATERIAL_COMMON) {
				if (material->type == GLTF::Material::Type::MATERIAL) {
					auto technique = material->technique;
					if (technique && technique->id < 0) {
						technique->id = techniques.size();
						techniques.push_back(technique);
					}
					if (!usesTechniqueWebGL) {
						if (options->version != "1.0") {
							this->requireExtension("KHR_technique_webgl");
						}
						usesTechniqueWebGL = true;
					}
				}
				else if (material->type == GLTF::Material::Type::MATERIAL_COMMON && !usesMaterialsCommon) {
					this->requireExtension("KHR_materials_common");
					usesMaterialsCommon = true;
				}
				auto ambientTexture = material->values->ambientTexture;
				if (ambientTexture != NULL && ambientTexture->id < 0) {
					ambientTexture->id = textures.size();
					textures.push_back(ambientTexture);
				}
                auto diffuseTexture = material->values->diffuseTexture;
				if (diffuseTexture != NULL && diffuseTexture->id < 0) {
					diffuseTexture->id = textures.size();
					textures.push_back(diffuseTexture);
				}
                auto emissionTexture = material->values->emissionTexture;
				if (emissionTexture != NULL && emissionTexture->id < 0) {
					emissionTexture->id = textures.size();
					textures.push_back(emissionTexture);
				}
                auto specularTexture = material->values->specularTexture;
				if (specularTexture != NULL && specularTexture->id < 0) {
					specularTexture->id = textures.size();
					textures.push_back(specularTexture);
				}
			}
			else if (material->type == GLTF::Material::Type::PBR_METALLIC_ROUGHNESS) {
				auto materialPBR = std::static_pointer_cast<GLTF::MaterialPBR>(material);
				auto baseColorTexture = materialPBR->metallicRoughness->baseColorTexture;
				if (baseColorTexture != NULL && baseColorTexture->texture->id < 0) {
					baseColorTexture->texture->id = textures.size();
					textures.push_back(baseColorTexture->texture);
				}
				auto metallicRoughnessTexture = materialPBR->metallicRoughness->metallicRoughnessTexture;
				if (metallicRoughnessTexture != NULL && metallicRoughnessTexture->texture->id < 0) {
					metallicRoughnessTexture->texture->id = textures.size();
					textures.push_back(metallicRoughnessTexture->texture);
				}
                auto normalTexture = materialPBR->normalTexture;
				if (normalTexture != NULL && normalTexture->texture->id < 0) {
					normalTexture->texture->id = textures.size();
					textures.push_back(normalTexture->texture);
				}
                auto occlusionTexture = materialPBR->occlusionTexture;
				if (occlusionTexture != NULL && occlusionTexture->texture->id < 0) {
					occlusionTexture->texture->id = textures.size();
					textures.push_back(occlusionTexture->texture);
				}
                auto emissiveTexture = materialPBR->emissiveTexture;
				if (emissiveTexture != NULL && emissiveTexture->texture->id < 0) {
					emissiveTexture->texture->id = textures.size();
					textures.push_back(emissiveTexture->texture);
				}
				if (options->specularGlossiness) {
					if (!usesSpecularGlossiness) {
						this->useExtension("KHR_materials_pbrSpecularGlossiness");
						usesSpecularGlossiness = true;
					}
                    auto diffuseTexture = materialPBR->specularGlossiness->diffuseTexture;
					if (diffuseTexture != NULL && diffuseTexture->texture->id < 0) {
						diffuseTexture->texture->id = textures.size();
						textures.push_back(diffuseTexture->texture);
					}
                    auto specularGlossinessTexture = materialPBR->specularGlossiness->specularGlossinessTexture;
					if (specularGlossinessTexture != NULL && specularGlossinessTexture->texture->id < 0) {
						specularGlossinessTexture->texture->id = textures.size();
						textures.push_back(specularGlossinessTexture->texture);
					}
				}
			}
			if (options->version == "1.0") {
				jsonWriter->Key(material->getStringId().c_str());
			}
			jsonWriter->StartObject();
			material->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	materials.clear();

	// Write lights
	if (usesMaterialsCommon && lights.size() > 0) {
		jsonWriter->Key("extensions");
		jsonWriter->StartObject();
		jsonWriter->Key("KHR_materials_common");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& light : lights) {
			if (options->version == "1.0") {
				jsonWriter->Key(light->getStringId().c_str());
			}
			jsonWriter->StartObject();
			light->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		lights.clear();
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
		jsonWriter->EndObject();
	}

	// Write textures and build sampler and image arrays
	std::vector<std::shared_ptr<GLTF::Sampler>> samplers;
	std::vector<std::shared_ptr<GLTF::Image>> images;
	if (textures.size() > 0) {
		jsonWriter->Key("textures");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& texture : textures) {
			auto sampler = texture->sampler;
			if (sampler && sampler->id < 0) {
				sampler->id = samplers.size();
				samplers.push_back(sampler);
			}
			auto source = texture->source;
			if (source && source->id < 0) {
				source->id = images.size();
				images.push_back(source);
			}
			if (options->version == "1.0") {
				jsonWriter->Key(texture->getStringId().c_str());
			}
			jsonWriter->StartObject();
			texture->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	textures.clear();

	// Write images and add bufferViews if we have them
	if (images.size() > 0) {
		jsonWriter->Key("images");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& image : images) {
			auto bufferView = image->bufferView;
			if (bufferView != NULL && bufferView->id < 0) {
				bufferView->id = bufferViews.size();
				bufferViews.push_back(bufferView);
			}
			if (options->version == "1.0") {
				jsonWriter->Key(image->getStringId().c_str());
			}
			jsonWriter->StartObject();
			image->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	images.clear();

	// Write samplers
	if (samplers.size() > 0) {
		jsonWriter->Key("samplers");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& sampler : samplers) {
			if (options->version == "1.0") {
				jsonWriter->Key(sampler->getStringId().c_str());
			}
			jsonWriter->StartObject();
			sampler->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	samplers.clear();

	// Write techniques and build program array
	std::vector<std::shared_ptr<GLTF::Program>> programs;
	if (techniques.size() > 0) {
		jsonWriter->Key("techniques");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& technique : techniques) {
			if (technique->program) {
				auto program = technique->program;
				if (program->id < 0) {
					program->id = programs.size();
					programs.push_back(program);
				}
			}
			if (options->version == "1.0") {
				jsonWriter->Key(technique->getStringId().c_str());
			}
			jsonWriter->StartObject();
			technique->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	techniques.clear();

	// Write programs and build shader array
	std::vector<std::shared_ptr<GLTF::Shader>> shaders;
	if (programs.size() > 0) {
		jsonWriter->Key("programs");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& program : programs) {
			auto vertexShader = program->vertexShader;
			if (vertexShader != NULL && vertexShader->id < 0) {
				vertexShader->id = shaders.size();
				shaders.push_back(vertexShader);
			}
			auto fragmentShader = program->fragmentShader;
			if (fragmentShader != NULL && fragmentShader->id < 0) {
				fragmentShader->id = shaders.size();
				shaders.push_back(fragmentShader);
			}
			if (options->version == "1.0") {
				jsonWriter->Key(program->getStringId().c_str());
			}
			jsonWriter->StartObject();
			program->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	programs.clear();

	// Write shaders
	if (shaders.size() > 0) {
		jsonWriter->Key("shaders");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& shader : shaders) {
			if (options->version == "1.0") {
				jsonWriter->Key(shader->getStringId().c_str());
			}
			jsonWriter->StartObject();
			shader->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	shaders.clear();

	// Write bufferViews and add buffers to the buffer array
	std::vector<std::shared_ptr<GLTF::Buffer>> buffers;
	if (bufferViews.size() > 0) {
		jsonWriter->Key("bufferViews");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& bufferView : bufferViews) {
			if (bufferView->buffer) {
				auto buffer = bufferView->buffer;
				if (buffer->id < 0) {
					buffer->id = buffers.size();
					buffers.push_back(buffer);
				}
			}
			if (options->version == "1.0") {
				jsonWriter->Key(bufferView->getStringId().c_str());
			}
			jsonWriter->StartObject();
			bufferView->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	bufferViews.clear();

	// Write buffers
	if (buffers.size() > 0) {
		jsonWriter->Key("buffers");
		if (options->version == "1.0") {
			jsonWriter->StartObject();
		}
		else {
			jsonWriter->StartArray();
		}
		for (auto& buffer : buffers) {
			if (options->version == "1.0") {
				jsonWriter->Key(buffer->getStringId().c_str());
			}
			jsonWriter->StartObject();
			buffer->writeJSON(writer, options);
			jsonWriter->EndObject();
		}
		if (options->version == "1.0") {
			jsonWriter->EndObject();
		}
		else {
			jsonWriter->EndArray();
		}
	}
	buffers.clear();

	// Write extensionsUsed and extensionsRequired
	if (this->extensionsRequired.size() > 0 && options->version != "1.0") {
		jsonWriter->Key("extensionsRequired");
		jsonWriter->StartArray();
		for (const std::string extension : this->extensionsRequired) {
			jsonWriter->String(extension.c_str());
		}
		jsonWriter->EndArray();
	}
	if (this->extensionsUsed.size() > 0) {
		jsonWriter->Key("extensionsUsed");
		jsonWriter->StartArray();
		for (const std::string extension : this->extensionsUsed) {
			jsonWriter->String(extension.c_str());
		}
		jsonWriter->EndArray();
	}

	GLTF::Object::writeJSON(writer, options);
}
