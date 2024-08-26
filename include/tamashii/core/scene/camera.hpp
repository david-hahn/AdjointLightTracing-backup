#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/scene/asset.hpp>

T_BEGIN_NAMESPACE

class Camera : public Asset {
public:
	enum class Type {
		UNKNOWN, PERSPECTIVE, ORTHOGRAPHIC
	};
								Camera(std::string_view aName = "");
								~Camera() override = default;

	static std::unique_ptr<Camera> alloc(std::string_view aName = "");

	void						initPerspectiveCamera(float aYFov, float aAspectRatio, float aZNear, float aZFar);
	void						initOrthographicCamera(float aXMag, float aYMag, float aZNear, float aZFar);
	void						updateAspectRatio(float aAspectRatio);
	void						updateMag(float aXMag, float aYMag);
	bool						updateAspectRatio(glm::uvec2 aSize);
	bool						updateMag(glm::uvec2 aSize);
	glm::mat4					getProjectionMatrix();
	Type						getType() const;

	float						getZFar() const;
	float						getZNear() const;
	float						getAspectRation() const;
	float						getYFov() const;
	float						getXMag() const;
	float						getYMag() const;

								
								
								
								
								
    static glm::mat4			flipY(glm::mat4 aViewMatrix);
	static void					buildViewMatrix(float* aViewMatrix, glm::vec3 const& aPosition, glm::vec3 const& aRight, glm::vec3 const& aUp, glm::vec3 const& aForward);
	static void					buildModelMatrix(float* aModelMatrix, glm::vec3 const& aPosition, glm::vec3 const& aRight, glm::vec3 const& aUp, glm::vec3 const& aForward);


protected:
	void						calculateProjectionMatrix();

	Type						mType = Type::UNKNOWN;
	float						mZFar = 1000.0f;
	float						mZNear = 0.01f;
	
	float						mAspectRatio = 1.0f;
	float						mYFov = 0.7f;
    glm::uvec2					mSize = {};
	
	float						mXMag = 1.0f;
	float						mYMag = 1.0f;

	glm::mat4					mProjectionMatrix = glm::mat4(1.0f);
};

T_END_NAMESPACE
