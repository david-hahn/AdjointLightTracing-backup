#include <tamashii/core/io/io.hpp>

#include <tamashii/public.hpp>
#include <tamashii/core/io/io.hpp>
#include <tamashii/core/topology/topology.hpp>
#include <tamashii/core/scene/image.hpp>
#include <tamashii/core/scene/material.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/scene_graph.hpp>

#include <sstream>
#include <fstream>
#include <string>
#include <array>
#include <filesystem>
#include <glm/glm.hpp>

T_USE_NAMESPACE

constexpr uint32_t CONTENTS_SOLID = 0x1;
constexpr uint32_t CONTENTS_LAVA = 0x8;
constexpr uint32_t CONTENTS_SLIME = 0x10;
constexpr uint32_t CONTENTS_WATER = 0x20;
constexpr uint32_t CONTENTS_FOG = 0x40;
constexpr uint32_t CONTENTS_NOTTEAM1 = 0x80;
constexpr uint32_t CONTENTS_NOTTEAM2 = 0x100;
constexpr uint32_t CONTENTS_NOBOTCLIP = 0x200;
constexpr uint32_t CONTENTS_AREAPORTAL = 0x8000;
constexpr uint32_t CONTENTS_PLAYERCLIP = 0x10000;
constexpr uint32_t CONTENTS_MONSTERCLIP = 0x20000;
constexpr uint32_t CONTENTS_TELEPORTER = 0x40000;
constexpr uint32_t CONTENTS_JUMPPAD = 0x80000;
constexpr uint32_t CONTENTS_CLUSTERPORTAL = 0x100000;
constexpr uint32_t CONTENTS_DONOTENTER = 0x200000;
constexpr uint32_t CONTENTS_BOTCLIP = 0x400000;
constexpr uint32_t CONTENTS_MOVER = 0x800000;
constexpr uint32_t CONTENTS_ORIGIN = 0x1000000;
constexpr uint32_t CONTENTS_BODY = 0x2000000;
constexpr uint32_t CONTENTS_CORPSE = 0x4000000;
constexpr uint32_t CONTENTS_DETAIL = 0x8000000;
constexpr uint32_t CONTENTS_STRUCTURAL = 0x10000000;
constexpr uint32_t CONTENTS_TRANSLUCENT = 0x20000000;
constexpr uint32_t CONTENTS_TRIGGER = 0x40000000;
constexpr uint32_t CONTENTS_NODROP = 0x80000000;

constexpr uint32_t SURF_NODAMAGE = 0x1;
constexpr uint32_t SURF_SLICK = 0x2;
constexpr uint32_t SURF_SKY = 0x4;
constexpr uint32_t SURF_LADDER = 0x8;
constexpr uint32_t SURF_NOIMPACT = 0x10;
constexpr uint32_t SURF_NOMARKS = 0x20;
constexpr uint32_t SURF_FLESH = 0x40;
constexpr uint32_t SURF_NODRAW = 0x80;
constexpr uint32_t SURF_HINT = 0x100;
constexpr uint32_t SURF_SKIP = 0x200;
constexpr uint32_t SURF_NOLIGHTMAP = 0x400;
constexpr uint32_t SURF_POINTLIGHT = 0x800;
constexpr uint32_t SURF_METALSTEPS = 0x1000;
constexpr uint32_t SURF_NOSTEPS = 0x2000;
constexpr uint32_t SURF_NONSOLID = 0x4000;
constexpr uint32_t SURF_LIGHTFILTER = 0x8000;
constexpr uint32_t SURF_ALPHASHADOW = 0x10000;
constexpr uint32_t SURF_NODLIGHT = 0x20000;
constexpr uint32_t SURF_SURFDUST = 0x40000;


namespace BSP {

	struct Lump
	{
		int offset;
		int size;
	};

	struct Header
	{
		char type[4];
		int version;
		Lump lumps[17];
	};

	struct Face
	{
		int shader;
		int effect;
		int type;
		int vertexOffset;
		int vertexCount;
		int meshVertexOffset;
		int meshVertexCount;
		int lightMap;
		int lightMapStart[2];
		int lightMapSize[2];
		glm::vec3 lightMapOrigin;
		glm::vec3 lightMapVecs[2];
		glm::vec3 normal;
		int size[2];
	};

	struct Texture
	{
		char name[64];
		int surface;
		int contents;
	};

	struct MeshVertex {
		unsigned int offset;
	};

	struct Vertex {
		glm::vec3 position;
		glm::vec2 texCoord;
		glm::vec2 lmCoord;
		glm::vec3 normal;
		unsigned char color[4];
	};

	struct LightMap {
		std::array<uint8_t, 128ull * 128ull * 4ull> data;
	};

}

struct BSPMesh {
	std::vector<BSP::Vertex> vertices;
	std::vector<uint32_t> indices;

	int texture;
	int lightMap;
};


static std::vector<BSP::Face> loadFaces(std::ifstream& in, int offset, int size) {
	const int faceCount = size / sizeof(BSP::Face);
	std::vector<BSP::Face> faces(faceCount);

	in.seekg(offset);
	in.read((char*)&faces[0], size);

	return faces;
}

static std::vector<BSP::Vertex> loadVertices(std::ifstream& in, int offset, int size) {
	const int vertexCount = size / sizeof(BSP::Vertex);
	std::vector<BSP::Vertex> vertices(vertexCount);

	in.seekg(offset);
	in.read((char*)&vertices[0], size);

	return vertices;
}

static std::vector<BSP::MeshVertex> loadMeshverts(std::ifstream& in, int offset, int size) {
	const int meshvertCount = size / sizeof(BSP::MeshVertex);
	std::vector<BSP::MeshVertex> meshverts(meshvertCount);

	in.seekg(offset);
	in.read((char*)&meshverts[0], size);

	return meshverts;
}

static std::vector<BSP::Texture> loadTextures(std::ifstream& in, int offset, int size) {
	const int shaderCount = size / sizeof(BSP::Texture);
	std::vector<BSP::Texture> shaders(shaderCount);

	in.seekg(offset);
	in.read((char*)&shaders[0], size);

	return shaders;
}

static std::vector<BSP::LightMap> loadLightMaps(std::ifstream& in, int offset, int size) {
	const int lightMapCount = size / (128 * 128 * 3);
	std::vector<BSP::LightMap> lightMaps(lightMapCount);

	in.seekg(offset);

	for (BSP::LightMap& lightMap : lightMaps) {
		for (int i = 0; i < 128 * 128; i++) {
			in.read((char*)&lightMap.data[i * 4], 3);
			lightMap.data[i * 4 + 3] = 255;
		}
	}

	return lightMaps;
}

static BSP::Vertex operator+(const BSP::Vertex& v1, const BSP::Vertex& v2)
{
	BSP::Vertex temp {};
	temp.position = v1.position + v2.position;
	temp.texCoord = v1.texCoord + v2.texCoord;
	temp.lmCoord = v1.lmCoord + v2.lmCoord;
	temp.normal = v1.normal + v2.normal;
	return temp;
}

static BSP::Vertex operator*(const BSP::Vertex& v1, const float& d)
{
	BSP::Vertex temp {};
	temp.position = v1.position * d;
	temp.texCoord = v1.texCoord * d;
	temp.lmCoord = v1.lmCoord * d;
	temp.normal = v1.normal * d;
	return temp;
}

static bool exists(const std::string& name) {
	std::ifstream f(name.c_str());
	return f.good();
}

static std::string getTexturePath(const std::string& aDirectory, const BSP::Texture& aBspTexture) {
	const std::string name = std::string(aBspTexture.name);

	if (exists(aDirectory + "/" + name + ".jpg")) {
		return aDirectory + "/" + name + ".jpg";
	}

	if (exists(aDirectory + "/" + name + ".png")) {
		return aDirectory + "/" + name + ".png";
	}

	if (exists(aDirectory + "/" + name + ".tga")) {
		spdlog::warn(name);
		return aDirectory + "/" + name + ".tga";
	}

	return {};
}

static std::string getDirectory(const std::string& bspFile) {
	return std::filesystem::path(bspFile).parent_path().parent_path().string();
}


static void tesselate(const BSP::Vertex& v11, const BSP::Vertex& v12, const BSP::Vertex& v13, const BSP::Vertex& v21, const BSP::Vertex& v22, const BSP::Vertex& v23, const BSP::Vertex& v31, const BSP::Vertex& v32, const BSP::Vertex& v33, std::vector<BSP::Vertex>& vertices_out, std::vector<uint32_t>& indices_out, const int bezierLevel)
{
	std::vector<BSP::Vertex> vertices;
	const uint32_t offset = vertices_out.size();

	for (int j = 0; j <= bezierLevel; j++)
	{
		float a = (float)j / bezierLevel;
		float b = 1.f - a;
		vertices.push_back(v11 * b * b + v21 * 2 * b * a + v31 * a * a);
	}

	for (int i = 1; i <= bezierLevel; i++)
	{
		float a = (float)i / bezierLevel;
		float b = 1.f - a;

		BSP::Vertex temp[3];

		temp[0] = v11 * b * b + v12 * 2 * b * a + v13 * a * a;
		temp[1] = v21 * b * b + v22 * 2 * b * a + v23 * a * a;
		temp[2] = v31 * b * b + v32 * 2 * b * a + v33 * a * a;

		for (int j = 0; j <= bezierLevel; j++)
		{
			float a = (float)j / bezierLevel;
			float b = 1.f - a;

			vertices.push_back(temp[0] * b * b + temp[1] * 2 * b * a + temp[2] * a * a);
		}
	}

	vertices_out.insert(vertices_out.end(), vertices.begin(), vertices.end());

	int L1 = bezierLevel + 1;
	for (int i = 0; i < bezierLevel; i++)
	{
		for (int j = 0; j < bezierLevel; j++)
		{
			indices_out.push_back(offset + (i)*L1 + (j));
			indices_out.push_back(offset + (i + 1) * L1 + (j + 1));
			indices_out.push_back(offset + (i)*L1 + (j + 1));

			indices_out.push_back(offset + (i + 1) * L1 + (j + 1));
			indices_out.push_back(offset + (i)*L1 + (j));
			indices_out.push_back(offset + (i + 1) * L1 + (j));
		}
	}
}

static std::tuple<std::vector<std::unique_ptr<Model>>, std::vector<Material*>, std::vector<Texture*>, std::vector<Image*>> createModels(const std::string& aDirectory,
	const std::vector<BSPMesh>& aMeshes, const std::vector<BSP::Texture>& aTextures, const std::vector<BSP::LightMap>& aLightMaps) {
	std::vector<std::unique_ptr<Model>> models;
	std::vector<Material*> materials;
	std::vector<Texture*> textures;
	std::vector<Image*> images;

	
	std::vector<Texture*> baseColorTextures;
	for (const BSP::Texture& bspTexture : aTextures) {
		std::string path = getTexturePath(aDirectory, bspTexture);

		if (!path.empty()) {
			Texture* baseColorTexture = Texture::alloc();

			Image* image = io::Import::load_image_8_bit(path, 4);
			image->needsMipMaps(true);
			baseColorTexture->image = image;
			baseColorTexture->texCoordIndex = 0;

			Sampler sampler {};
			sampler.min = Sampler::Filter::LINEAR;
			sampler.mag = Sampler::Filter::LINEAR;
			sampler.mipmap = Sampler::Filter::LINEAR;
			sampler.wrapU = Sampler::Wrap::REPEAT;
			sampler.wrapV = Sampler::Wrap::REPEAT;
			sampler.wrapW = Sampler::Wrap::REPEAT;
			baseColorTexture->sampler = sampler;

			images.push_back(image);
			textures.push_back(baseColorTexture);
			baseColorTextures.push_back(baseColorTexture);
		}
		else {
			baseColorTextures.push_back(nullptr);
		}

	}

	
	std::vector<Texture*> lightTextures;
	for (const BSP::LightMap& bspLightMap : aLightMaps) {
		Texture* lightmap = Texture::alloc();

		Image* image = Image::alloc("LightMap");

		image->init(128, 128, Image::Format::RGBA8_UNORM, bspLightMap.data.data());
		lightmap->image = image;
		lightmap->texCoordIndex = 1;

		Sampler sampler {};
		sampler.min = Sampler::Filter::LINEAR;
		sampler.mag = Sampler::Filter::LINEAR;
		sampler.mipmap = Sampler::Filter::LINEAR;
		sampler.wrapU = Sampler::Wrap::CLAMP_TO_BORDER;
		sampler.wrapV = Sampler::Wrap::CLAMP_TO_BORDER;
		sampler.wrapW = Sampler::Wrap::CLAMP_TO_BORDER;
		lightmap->sampler = sampler;

		images.push_back(image);
		textures.push_back(lightmap);
		lightTextures.push_back(lightmap);
	}

	
	for (const BSPMesh& bspMesh : aMeshes) {
		auto model = Model::alloc("bsp");
		aabb_s aabb = aabb_s(glm::vec3(std::numeric_limits<float>::max()), glm::vec3(std::numeric_limits<float>::min()));
		std::shared_ptr<Mesh> mesh = Mesh::alloc();
		Material* material = nullptr;
		Texture* baseColorTexture = bspMesh.texture >= 0 ? baseColorTextures[bspMesh.texture] : nullptr;
		Texture* lightTexture = bspMesh.lightMap >= 0 ? lightTextures[bspMesh.lightMap] : nullptr;

		
		for (Material* mat : materials) {
			if (mat->getBaseColorTexture() == baseColorTexture && mat->getLightTexture() == lightTexture) {
				material = mat;
			}
		}

		
		if (material == nullptr) {
			material = Material::alloc(DEFAULT_MATERIAL_NAME);
			material->setRoughnessFactor(1.0f);

			if (lightTexture != nullptr) {
				material->setLightTexture(lightTexture);
				material->setLightFactor({ 1, 1, 1 });
			}
			if (baseColorTexture != nullptr) {
				material->setBaseColorTexture(baseColorTexture);
				material->setBaseColorFactor({ 1, 1, 1, 1 });
			}

			materials.push_back(material);
		}

		mesh->setMaterial(material);

		mesh->setTopology(Mesh::Topology::TRIANGLE_LIST);
		std::vector<vertex_s> vertices;
		for (const BSP::Vertex& bspVertex : bspMesh.vertices) {
			vertex_s vertex {};
			vertex.position = { bspVertex.position.x ,bspVertex.position.y ,bspVertex.position.z, 1 };
			vertex.normal = { bspVertex.normal.x ,bspVertex.normal.y ,bspVertex.normal.z, 1 };
			vertex.tangent = glm::vec4(topology::calcStarkTangent(bspVertex.normal), 1);
			vertex.texture_coordinates_0 = bspVertex.texCoord;
			vertex.texture_coordinates_1 = bspVertex.lmCoord;
			if (lightTexture == nullptr) {
				vertex.color_0 = { bspVertex.color[0] / 255.0, bspVertex.color[1] / 255.0, bspVertex.color[2] / 255.0, bspVertex.color[3] / 255.0 };
			}
			else {
				vertex.color_0 = { 1.0f, 1.0f, 1.0f, 1.0f };
			}
			vertices.push_back(vertex);

			
			if (vertex.position.x < aabb.mMin.x) aabb.mMin.x = vertex.position.x;
			if (vertex.position.y < aabb.mMin.y) aabb.mMin.y = vertex.position.y;
			if (vertex.position.z < aabb.mMin.z) aabb.mMin.z = vertex.position.z;
			
			if (vertex.position.x > aabb.mMax.x) aabb.mMax.x = vertex.position.x;
			if (vertex.position.y > aabb.mMax.y) aabb.mMax.y = vertex.position.y;
			if (vertex.position.z > aabb.mMax.z) aabb.mMax.z = vertex.position.z;
		}

		mesh->setIndices(bspMesh.indices);
		mesh->setVertices(vertices);

		mesh->hasColors0(true);
		mesh->hasPositions(true);
		mesh->hasIndices(true);
		mesh->hasNormals(true);
		mesh->hasTexCoords0(true);
		mesh->hasTexCoords1(true);
		mesh->setAABB(aabb);

		model->addMesh(mesh);
		model->setAABB(aabb);

		models.push_back(std::move(model));
	}

	return { std::move(models), materials, textures, images };
}

static std::unique_ptr<io::SceneData> createScene(const std::string& aDirectory, const std::vector<BSPMesh>& aMeshes,
	const std::vector<BSP::Texture>& aTextures, const std::vector<BSP::LightMap>& aLightMaps) {
	auto si = io::SceneData::alloc();

	
	auto [models, materials, textures, images] = createModels(aDirectory, aMeshes, aTextures, aLightMaps);

	
	spdlog::info("Loading SceneGraph:");
	auto sceneGraph = Node::alloc("SceneGraph");

	Node& rootNode = sceneGraph->addChildNode("RootNode");
	rootNode.setTranslation({ -10, 0, 0 });
	rootNode.setRotation({ -sin(glm::radians(90.0) * 0.5), 0, 0, cos(glm::radians(90.0) * 0.5) });
	rootNode.setScale({ 0.01, 0.01, 0.01 });

	
	for (auto& model : models) {
		si->mModels.emplace_back( std::move(model) );
	}

	
	for (const auto& model : si->mModels) {
		Node& modelNode = rootNode.addChildNode("ModelNode");
		modelNode.setModel(model);
		modelNode.setTranslation({ 0, 0, 0 });
		modelNode.setRotation({ 0, 0, 0, 0 });
		modelNode.setScale({ 1, 1, 1 });
	}

	
	si->mSceneGraphs.push_back(std::move(sceneGraph));

	for (Material* material : materials) {
		si->mMaterials.push_back(material);
	}

	for (Texture* texture : textures) {
		si->mTextures.push_back(texture);
	}

	for (Image* image : images) {
		si->mImages.push_back(image);
	}

	return si;
}

std::unique_ptr<io::SceneData> io::Import::load_bsp(const std::string& aFile) {
	spdlog::info("...load bsp");

	std::vector<BSPMesh> meshes;
	std::vector<BSP::Texture> textures;
	std::vector<BSP::LightMap> lightMaps;

	std::string directory = getDirectory(aFile);


	

	
	std::ifstream in(aFile, std::ios::in | std::ios::binary);
	if (!in) {
		spdlog::critical("...failed");
		return nullptr;
	}

	
	in.seekg(0);
	BSP::Header header {};
	in.read((char*)&header, sizeof(BSP::Header));

	
	if (std::string(header.type, 4) != "IBSP") {
		spdlog::critical("Invalid file '{}'", aFile);
		return nullptr;
	}
	if (header.version != 0x2E) {
		spdlog::critical("File version not supported: version:{}", header.version);
		return nullptr;
	}

	constexpr uint32_t TEXTURES = 1;
	constexpr uint32_t VERTEX = 10;
	constexpr uint32_t MESHVERTEX = 11;
	constexpr uint32_t FACE = 13;
	constexpr uint32_t LIGHTMAP = 14;

	
	std::vector<BSP::Face> faces = loadFaces(in, header.lumps[FACE].offset, header.lumps[FACE].size);

	
	std::vector<BSP::Vertex> vertices = loadVertices(in, header.lumps[VERTEX].offset, header.lumps[VERTEX].size);

	
	std::vector<BSP::MeshVertex> meshVertices = loadMeshverts(in, header.lumps[MESHVERTEX].offset, header.lumps[MESHVERTEX].size);

	
	textures = loadTextures(in, header.lumps[TEXTURES].offset, header.lumps[TEXTURES].size);

	
	lightMaps = loadLightMaps(in, header.lumps[LIGHTMAP].offset, header.lumps[LIGHTMAP].size);


	

	for (const BSP::Face& face : faces) {

		
		if (face.type == 1 || face.type == 3) {
			const int vertexOffset = face.vertexOffset;
			const int vertexCount = face.vertexCount;

			const int meshVertexOffset = face.meshVertexOffset;
			const int meshVertexCount = face.meshVertexCount;

			BSPMesh mesh = BSPMesh();

			mesh.texture = face.shader;
			mesh.lightMap = face.lightMap;

			
			for (int i = 0; i < vertexCount; i++) {
				mesh.vertices.push_back(vertices[vertexOffset + i]);
			}

			
			for (int i = 0; i < meshVertexCount; i += 3) {
				mesh.indices.push_back(meshVertices[meshVertexOffset + i].offset);
				mesh.indices.push_back(meshVertices[meshVertexOffset + i + 2].offset);
				mesh.indices.push_back(meshVertices[meshVertexOffset + i + 1].offset);
			}

			meshes.push_back(mesh);
		}

		
		if (face.type == 2) {

			BSPMesh mesh = BSPMesh();
			mesh.texture = face.shader;
			mesh.lightMap = face.lightMap;

			int dimX = (face.size[0] - 1) / 2;
			int dimY = (face.size[1] - 1) / 2;

			for (int x = 0, n = 0; n < dimX; n++, x = 2 * n)
			{
				for (int y = 0, m = 0; m < dimY; m++, y = 2 * m)
				{
					int controlOffset = face.vertexOffset + x + face.size[0] * y;
					tesselate(
						vertices[controlOffset + face.size[0] * 0 + 0],
						vertices[controlOffset + face.size[0] * 0 + 1],
						vertices[controlOffset + face.size[0] * 0 + 2],

						vertices[controlOffset + face.size[0] * 1 + 0],
						vertices[controlOffset + face.size[0] * 1 + 1],
						vertices[controlOffset + face.size[0] * 1 + 2],

						vertices[controlOffset + face.size[0] * 2 + 0],
						vertices[controlOffset + face.size[0] * 2 + 1],
						vertices[controlOffset + face.size[0] * 2 + 2],
						mesh.vertices,
						mesh.indices,
						3
					);
				}
			}

			meshes.push_back(mesh);
		}

		
		if (face.type == 4) {
		}
	}


	
	auto si = createScene(directory, meshes, textures, lightMaps);

	spdlog::info("...success");

	return si;
}
