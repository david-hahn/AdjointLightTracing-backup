#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/forward.h>
#include <tamashii/core/common/math.hpp>

#include <list>
#include <deque>

T_BEGIN_NAMESPACE
struct Ref {
	enum class Type {
		Mesh, Model, Camera, Light
	};
											
								Ref(const Type type) : type{ type }, uuid{uuid::getUUID()},
									model_matrix { glm::mat4{1.0f} }, extra{ nullptr } {}
	virtual						~Ref() = default;
	const Type					type;
	const UUID					uuid;
	glm::mat4					model_matrix;

	std::deque<TRS*>			transforms;			
	void*						extra;				
    const Ref&					operator=(const Ref& other) const { return other; }

	void						updateSceneGraphNodesFromModelMatrix(bool aFlipY = false) const;
};
struct RefMesh : Ref {
								RefMesh() : Ref{ Type::Mesh }, mesh{ nullptr } {}

	std::shared_ptr<Mesh>		mesh;
};
struct RefModel : Ref {
								RefModel() : Ref{ Type::Model }, model{ nullptr }, ref_model_index{ -1 }, mask{ 0xffffffff }, animated{ false } {}
	std::shared_ptr<Model>		model;
	std::list<std::shared_ptr<RefMesh>>	refMeshes;
	int							ref_model_index;	
	uint32_t					mask;
	bool						animated;
};

struct RefCamera : Ref {
	enum class Mode {
		STATIC, FPS, EDITOR
	};
	std::shared_ptr<Camera>		camera;

	Mode						mode;
	glm::mat4					view_matrix;
	bool						y_flipped;			
	glm::vec3					getPosition();
	glm::vec3					getDirection();

	int							ref_camera_index;	
	bool						animated;
protected:
								RefCamera() : Ref{ Type::Camera }, camera{ nullptr }, mode{ Mode::STATIC }, view_matrix{ 1 }, y_flipped{ false },
									ref_camera_index{ -1 }, animated{ false } {}
								RefCamera(std::shared_ptr<Camera> camera, const int ref_camera_index) : Ref{ Type::Camera }, camera{ std::move(camera) },
									mode{ Mode::STATIC }, view_matrix{ 1 }, y_flipped{ false }, ref_camera_index{ ref_camera_index }, animated{ false } {}
};
struct RefCameraPrivate : RefCamera {
								RefCameraPrivate();
	struct Angles {
		float					pitch;
		float					yaw;
		float					roll;
	};
	struct FpsData
	{
								
		const float				mouse_sensitivity = 5.0f;
		const float				yaw_sensitivity = 0.022f;
		const float				pitch_sensitivity = 0.022f;
								
		const float				mouse_wheel_sensitivity = 10.0f;
								
		const float				movement_speed = 650.0f;
		const float				acceleration = 10.0f;

								
		const size_t			smooth_history_usage = 1.0f;
		size_t					smooth_history_count = 0;
		float					smooth_history[16][2] = {};

								
		float					current_movement_speed = 100;
		glm::vec3				current_velocity = {0,0,0};
	};
	struct EditorData
	{							
		float					zoom_speed = 0.2f;

								
								
		glm::vec3				pivot = { 0, 0, 0 };
								
		float					zoom = 40.0f;
	};
	bool						default_camera;
	FpsData						fps_data;
	EditorData					editor_data;

	glm::vec3					axis[3];	
	Angles						angles;		
	glm::vec3					position;

								
								
	bool						updateAngles(float aNewMx, float aNewMy);
								
	bool						updatePosition(bool aFront, bool aBack, bool aLeft, bool aRight, bool aUp, bool aDown);
								
	bool						updateSpeed(float aNewMz);

								
	bool						drag(glm::vec2 aDelta);
	bool						updateCenter(glm::vec2 aDelta);
	bool						updateZoom(float aNewMz);

								
	void						setModelMatrix(glm::mat4& aMatrix, bool aFlipY = false);
	void						updateMatrix(bool aFlipY = false);
};

struct RefLight : Ref {
								RefLight() : Ref{ Type::Light }, light{ nullptr }, ref_light_index{ -1 }, mask{ 0xffffffff },
									animated{ false }, direction{ glm::vec3{1.0f} }, position{ glm::vec3{1.0f} } {}
	std::shared_ptr<Light>		light;
	int							ref_light_index;	
	uint32_t					mask;
	bool						animated;

	glm::vec3					direction;			
	glm::vec3					position;			
};


struct Frustum
{
	enum class Plane : uint8_t { N = 0, F = 1, T = 2, B = 3, L = 4, R = 5, COUNT = 6 };
	struct Plane_s { glm::vec3 n; glm::vec3 p; };
	Plane_s plane[static_cast<uint8_t>(Plane::COUNT)]{};
	glm::vec3 origin{};
	glm::vec3 near_corners[4]{};
	glm::vec3 far_corners[4]{};
	Frustum(const RefCamera *c);
	bool checkPointInside(glm::vec3 p) const;
	bool checkAABBInside(glm::vec3 min, glm::vec3 max) const;
};
T_END_NAMESPACE
