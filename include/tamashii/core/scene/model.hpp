#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/forward.h>
#include <tamashii/core/scene/asset.hpp>
#include <tamashii/core/scene/render_scene.hpp>

#include <string>
#include <list>
#include <vector>

T_BEGIN_NAMESPACE
struct vertex_s {
	glm::vec4	position;
	glm::vec4	normal;						
	glm::vec4	tangent;					
											
	glm::vec2	texture_coordinates_0;
	glm::vec2	texture_coordinates_1;
	glm::vec4	color_0;					
	
	
};

struct aabb_s {
                                            aabb_s(const glm::vec3 aMin = glm::vec3(std::numeric_limits<float>::max()),
                                                   const glm::vec3 aMax = glm::vec3(std::numeric_limits<float>::min())) : mMin(aMin), mMax(aMax) {}
	glm::vec3								mMin;
	glm::vec3								mMax;

	bool									intersect(glm::vec3 aOrigin, glm::vec3 aDirection, float& aT) const;
	bool									inside(glm::vec3 aPoint) const;
	void									set(const aabb_s& aAabb);
	void									set(glm::vec3 aVec);
	aabb_s									transform(const glm::mat4& aMatrix) const;
	aabb_s									merge(const aabb_s& aAabb) const;
											
	glm::vec3								getPoint(uint8_t aIdx) const;
};
struct triangle_s {
	glm::vec3								mVert[3];
	glm::vec3								mN[3];
	glm::vec3								mT[3];
	glm::vec2								mUV0[3];
	glm::vec3								mGeoN;

	bool									intersect(glm::vec3 aOrigin, glm::vec3 aDirection, float& aT, glm::vec2& aBarycentric, CullMode aCullMode = CullMode::None) const;
};
struct sphere_s {
	glm::vec3								mCenter;
	float									mRadius;

	bool									intersect(glm::vec3 aOrigin, glm::vec3 aDirection, float& aT) const;
};
struct disk_s {
	glm::vec3								mCenter;
	float									mRadius;
	glm::vec3								mNormal;

	bool									intersect(glm::vec3 aOrigin, glm::vec3 aDirection, float& aT) const;
};

class Mesh : public Asset {
public:
	enum class Topology {
		UNKNOWN,
		POINT_LIST,
		LINE_LIST,
		LINE_STRIP,
		TRIANGLE_LIST,
		TRIANGLE_STRIP,
		TRIANGLE_FAN
	};
	class CustomData
	{
	public:
		CustomData();
		explicit CustomData(size_t aBytes);
		CustomData(size_t aBytes, const void* aData);
		~CustomData() = default;

		CustomData(const CustomData& other);
		CustomData& operator=(const CustomData& other);
		CustomData(CustomData&& other) noexcept;
		CustomData& operator=(CustomData&& other) noexcept;

		template <typename T>
		T* alloc(const size_t aCount = 1)
		{
			if (mData) free();
			mByteSize = sizeof(T) * aCount;
			mData = std::make_unique<uint8_t[]>( mByteSize );
			return reinterpret_cast<T*>(mData.get());
		}
		template <typename T = void>
		T* data() const
		{
			return reinterpret_cast<T*>(mData.get());
		}

		[[nodiscard]] size_t bytes() const;
		void free();
	private:
		size_t								mByteSize;
		std::unique_ptr<uint8_t[]>			mData;
	};

											Mesh(std::string_view aName = "");

											~Mesh() override;
											Mesh(const Mesh& aMesh);

	static std::unique_ptr<Mesh>			alloc(std::string_view aName = "");

	
	Topology								getTopology() const;
	uint32_t*								getIndicesArray();
	std::vector<uint32_t>*					getIndicesVector();
	std::vector<uint32_t>&					getIndicesVectorRef();
	vertex_s*								getVerticesArray();
	std::vector<vertex_s>*					getVerticesVector();
	std::vector<vertex_s>&					getVerticesVectorRef();
	CustomData*								addCustomData(const std::string& aKey);
	CustomData*								getCustomData(const std::string& aKey);
	void									deleteCustomData(const std::string& aKey);
	std::map<std::string, CustomData>&		getCustomDataMap();
	const std::map<std::string, CustomData>&getCustomDataMap() const;
	size_t									getIndexCount() const;
	size_t									getVertexCount() const;
	size_t									getPrimitiveCount() const;
	Material*								getMaterial() const;
	const aabb_s &							getAABB() const;
	triangle_s								getTriangle(uint32_t aIndex, const glm::mat4* aModelMatrix = nullptr) const;


	bool									hasIndices() const;
	bool									hasPositions() const;
	bool									hasNormals() const;
	bool									hasTangents() const;
	bool									hasTexCoords0() const;
	bool									hasTexCoords1() const;
	bool									hasColors0() const;

	
	void									setTopology(Topology aTopology );
	void									setIndices(const std::vector<uint32_t>& aIndices);
	void									setVertices(const std::vector<vertex_s>& aVertices);
	void									setMaterial(Material* aMaterial);
	void									setAABB(const aabb_s& aAabb);

	
	void									hasIndices(bool aBool);
	void									hasPositions(bool aBool);
	void									hasNormals(bool aBool);
	void									hasTangents(bool aBool);
	void									hasTexCoords0(bool aBool);
	void									hasTexCoords1(bool aBool);
	void									hasColors0(bool aBool);

	void									clear();

private:
	Topology								mTopology;

	bool									mHasIndices;
	bool									mHasPositions;
	bool									mHasNormals;
	bool									mHasTangents;
	bool									mHasTextureCoordinates0;
	bool									mHasTextureCoordinates1;
	bool									mHasColors0;

	std::vector<uint32_t>					mIndices;
	std::vector<vertex_s>					mVertices;
	std::map<std::string, CustomData>		mCustomData;

											
	aabb_s									mAabb;
	
	Material*								mMaterial;
};

class Model : public Asset {
public:
											Model(std::string_view aName = "");
											~Model() override = default;
											Model(const Model& aModel);


	static std::unique_ptr<Model>			alloc(std::string_view aName = "");

	void									addMesh(const std::shared_ptr<Mesh>& aMesh);

	const aabb_s&							getAABB() const;
	void									setAABB(const aabb_s& aAabb);

	std::list<std::shared_ptr<Mesh>>		getMeshList() const;
	uint32_t								size() const;

	void									clear();

	
	std::list<std::shared_ptr<Mesh>>::iterator			begin();
	std::list<std::shared_ptr<Mesh>>::const_iterator	begin() const;
	std::list<std::shared_ptr<Mesh>>::iterator			end();
	std::list<std::shared_ptr<Mesh>>::const_iterator	end() const;
private:
	std::list<std::shared_ptr<Mesh>>		mMeshes;
	aabb_s									mAABB; 
};
T_END_NAMESPACE
