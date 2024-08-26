#include <tamashii/core/scene/ref_entities.hpp>
#include <tamashii/core/scene/camera.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/scene_graph.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/math.hpp>

#include <glm/gtx/string_cast.hpp>

T_USE_NAMESPACE

void Ref::updateSceneGraphNodesFromModelMatrix(const bool aFlipY) const
{
	if (transforms.empty()) return;
	glm::mat4 sg_model_matrix(1.0f);
	for (const TRS* trs : transforms) {
		sg_model_matrix *= trs->getMatrix(0);
	}

	glm::vec3 sg_scale;
	glm::quat sg_rotation;
	glm::vec3 sg_translation;
	ASSERT(math::decomposeTransform(sg_model_matrix, sg_translation, sg_rotation, sg_scale), "decompose error");

	glm::vec3 c_scale;
	glm::quat c_rotation;
	glm::vec3 c_translation;
	glm::mat4 c_model_matrix = model_matrix;
	if (aFlipY) c_model_matrix[1] *= -1;
	ASSERT(math::decomposeTransform(c_model_matrix, c_translation, c_rotation, c_scale), "decompose error");

	const bool translationNeedsUpdate = glm::any(glm::notEqual(sg_translation, c_translation));
	const bool scaleNeedsUpdate = glm::any(glm::notEqual(sg_scale, c_scale));
	const bool rotationNeedsUpdate = glm::any(glm::notEqual(sg_rotation, c_rotation));
	const bool trsNeedsUpdate = (translationNeedsUpdate || scaleNeedsUpdate || rotationNeedsUpdate);
	if (!trsNeedsUpdate) return;

	TRS* trs = transforms.front();
	if (translationNeedsUpdate)
	{
		const glm::vec3 diff = c_translation - sg_translation;
		trs->translation += diff;
		if (trs->hasTranslationAnimation()) for (glm::vec3& v : trs->translationSteps) v += diff;
	}
	if (scaleNeedsUpdate)
	{
		
		
		const glm::vec3 diff = c_scale / sg_scale;
		if (trs->hasScale()) trs->scale *= diff;
		else trs->scale = diff;
		if (trs->hasScaleAnimation()) for (glm::vec3& v : trs->scaleSteps) v *= diff;
	}
	if (rotationNeedsUpdate)
	{
		const glm::quat diffQuad = c_rotation * glm::inverse(sg_rotation);

		if (trs->hasRotation())
		{
			const glm::quat newQuad = diffQuad * glm::quat(trs->rotation[3], trs->rotation[0], trs->rotation[1], trs->rotation[2]);
			trs->rotation = { newQuad[1], newQuad[2], newQuad[3] , newQuad[0] };
		}
		else trs->rotation = { diffQuad[1], diffQuad[2], diffQuad[3] , diffQuad[0] };
		if (trs->hasRotationAnimation()) for (glm::vec4& rotVec : trs->rotationSteps) {
			const glm::quat newRotation = diffQuad * glm::quat(rotVec[3], rotVec[0], rotVec[1], rotVec[2]);
			rotVec = { newRotation[1], newRotation[2], newRotation[3] , newRotation[0] };
		}
	}
}

glm::vec3 RefCamera::getPosition()
{
	return model_matrix[3];
}

glm::vec3 RefCamera::getDirection()
{
	return -model_matrix[2];
}

RefCameraPrivate::RefCameraPrivate() : default_camera(false),
axis{ {1,0,0}, {0,1,0}, {0,0,1} },
angles({0,0,0}), position({0, 0, 40})
{}


bool RefCameraPrivate::updateAngles(const float aNewMx, const float aNewMy)
{
	if (aNewMx == 0.0f && aNewMy == 0.0f) return false;
	const Angles oldAngles = this->angles;
	const size_t mouseSmoothHistory = std::size(fps_data.smooth_history);
	fps_data.smooth_history[fps_data.smooth_history_count & (mouseSmoothHistory - 1)][0] = aNewMx;
	fps_data.smooth_history[fps_data.smooth_history_count & (mouseSmoothHistory - 1)][1] = aNewMy;
	
	size_t smooth = fps_data.smooth_history_usage;
	if (smooth < 1) smooth = 1;
	if (smooth > mouseSmoothHistory) smooth = mouseSmoothHistory;

	float mx = 0;
	float my = 0;
	for (size_t i = 0; i < smooth; i++)
	{
		mx += fps_data.smooth_history[(fps_data.smooth_history_count - i + mouseSmoothHistory) & (mouseSmoothHistory - 1)][0];
		my += fps_data.smooth_history[(fps_data.smooth_history_count - i + mouseSmoothHistory) & (mouseSmoothHistory - 1)][1];
	}
	mx /= static_cast<float>(smooth);
	my /= static_cast<float>(smooth);
	fps_data.smooth_history_count++;

	this->angles.yaw -= fps_data.yaw_sensitivity * mx * fps_data.mouse_sensitivity;
	this->angles.pitch -= fps_data.pitch_sensitivity * my * fps_data.mouse_sensitivity;

	
	if (this->angles.pitch < -90) this->angles.pitch = -90;
	if (this->angles.pitch > 90) this->angles.pitch = 90;
	
	if (this->angles.pitch - oldAngles.pitch > 90) this->angles.pitch = oldAngles.pitch + 90;
	else if (oldAngles.pitch - this->angles.pitch > 90) this->angles.pitch = oldAngles.pitch - 90;

	const glm::vec3 eulerAnglesInDegrees(this->angles.pitch, this->angles.yaw, this->angles.roll);
	const glm::mat3 new_axes = glm::toMat3(glm::fquat(glm::radians(eulerAnglesInDegrees)));

	
	this->axis[0] = glm::normalize(new_axes[0]);
	this->axis[1] = glm::normalize(new_axes[1]);
	this->axis[2] = glm::normalize(new_axes[2]);

	
	editor_data.pivot = position - axis[2] * editor_data.zoom;
	return true;
}

bool RefCameraPrivate::updatePosition(const bool aFront, const bool aBack, const bool aLeft, const bool aRight, const bool aUp, const bool aDown) {
	if (!(aFront || aBack || aLeft || aRight || aUp || aDown)) return false;
	const float forward = static_cast<float>(aBack) * fps_data.movement_speed - static_cast<float>(aFront) * fps_data.movement_speed;	
	const float side = static_cast<float>(aRight) * fps_data.movement_speed - static_cast<float>(aLeft) * fps_data.movement_speed;
	const float upward = static_cast<float>(aUp) * fps_data.movement_speed - static_cast<float>(aDown) * fps_data.movement_speed;
	const float frametime = Common::getInstance().getRenderSystem()->getConfig().frametime / 1000.0f;

	
	fps_data.current_velocity = { 0,0,0 };

	float scale;
	float max = fabs(forward);
	if (fabs(side) > max) max = fabs(side);
	if (fabs(upward) > max) max = fabs(upward);
	if (max == 0.0f) scale = 0.0f;
	else {
		const float total = sqrt(forward * forward + side * side + upward * upward);
		scale = fps_data.current_movement_speed * max / (fps_data.movement_speed /*127.0f*/ * total);
	}

	glm::vec3 forceDir = scale * (axis[0] * side + axis[1] * upward + axis[2] * forward);
	float forceSpeed = glm::length(forceDir);
	forceDir = glm::normalize(forceDir);
	if (glm::any(glm::isnan(forceDir))) forceDir = glm::vec3(0, 0, 0);
	forceSpeed *= scale;

	
	
	const float currentSpeed = glm::dot(fps_data.current_velocity, forceDir);
	const float speedDelta = forceSpeed - currentSpeed;
	if (speedDelta <= 0) return false;
	float accelerationSpeed = fps_data.acceleration * forceSpeed;
	if (accelerationSpeed > speedDelta) accelerationSpeed = speedDelta;

	fps_data.current_velocity += accelerationSpeed * forceDir;
	position += frametime * fps_data.current_velocity;

	
	editor_data.pivot = position - axis[2] * editor_data.zoom;
	return true;
}

bool RefCameraPrivate::updateSpeed(const float aNewMz)
{
	if (aNewMz == 0.0f) return false;
	const float newSpeed = fps_data.current_movement_speed + aNewMz * fps_data.mouse_wheel_sensitivity;
	if (newSpeed < 20) fps_data.current_movement_speed = 20;
	else fps_data.current_movement_speed = newSpeed;
	return true;
}


bool RefCameraPrivate::drag(const glm::vec2 aDelta)
{
	if (glm::all(glm::equal(aDelta, glm::vec2(0)))) return false;
	const Angles oldAngles = angles;
	angles.pitch += aDelta.y;
	angles.yaw -= aDelta.x;
	
	if (angles.pitch - oldAngles.pitch > 90) angles.pitch = oldAngles.pitch + 90;
	else if (oldAngles.pitch - angles.pitch > 90) angles.pitch = oldAngles.pitch - 90;
	
	const glm::vec3 eulerAnglesInDegrees(angles.pitch, angles.yaw, angles.roll);
	const glm::mat3 newAxes = glm::toMat3(glm::fquat(glm::radians(eulerAnglesInDegrees)));
	
	axis[0] = glm::normalize(newAxes[0]);
	axis[1] = glm::normalize(newAxes[1]);
	axis[2] = glm::normalize(newAxes[2]);
	position = axis[2] * editor_data.zoom + editor_data.pivot;
	return true;
}

bool RefCameraPrivate::updateCenter(const glm::vec2 aDelta)
{
	if (glm::all(glm::equal(aDelta, glm::vec2(0)))) return false;
	const float sensitivity = editor_data.zoom / 1000.0f;
	editor_data.pivot -= axis[0] * aDelta.x * sensitivity + axis[1] * aDelta.y * sensitivity;
	position = axis[2] * editor_data.zoom + editor_data.pivot;
	return true;
}

bool RefCameraPrivate::updateZoom(const float aNewMz)
{
	if (aNewMz == 0.0f) return false;
	const float l = editor_data.zoom * editor_data.zoom_speed;
	editor_data.zoom = std::max(editor_data.zoom - aNewMz * l, 0.1f);
	position = axis[2] * editor_data.zoom + editor_data.pivot;
	return true;
}

void RefCameraPrivate::setModelMatrix(glm::mat4& aMatrix, const bool aFlipY)
{
	axis[0] = aMatrix[0];
	axis[1] = aMatrix[1];
	axis[2] = aMatrix[2];
	position = aMatrix[3];
	updateMatrix(aFlipY);
	
	const glm::vec3 ea = glm::degrees(eulerAngles(glm::toQuat(aMatrix)));
	angles.pitch = ea.x; angles.yaw = ea.y; angles.roll = 0;
	if(angles.pitch == 180.0f && angles.yaw == 0.0f) std::swap(angles.pitch, angles.yaw);

	
	editor_data.pivot = position - axis[2] * editor_data.zoom;
}

void RefCameraPrivate::updateMatrix(const bool aFlipY)
{
	Camera::buildViewMatrix(glm::value_ptr(view_matrix[0]), position, axis[0], axis[1], axis[2]);
	if (aFlipY) { view_matrix[0][1] *= -1; view_matrix[1][1] *= -1; view_matrix[2][1] *= -1; view_matrix[3][1] *= -1; }
	Camera::buildModelMatrix(glm::value_ptr(model_matrix[0]), position, axis[0], axis[1], axis[2]);
	if (aFlipY) model_matrix[1] *= -1;
	y_flipped = aFlipY;
}


Frustum::Frustum(const RefCamera* c)
{
	const glm::vec3 right = c->model_matrix[0] * glm::vec4(1);
	const glm::vec3 up = c->model_matrix[1] * glm::vec4(-1);
	const glm::vec3 forward = c->model_matrix[2] * (c->y_flipped ? glm::vec4(-1) : glm::vec4(1));
	const glm::vec3 pos = c->model_matrix[3] * glm::vec4(1, 1, 1, 1);
	
	float nh_half;
	float nw_half;
	float fh_half;
	float fw_half;
	if(c->camera->getType() == Camera::Type::PERSPECTIVE)
	{
		const float tangent = tanf(c->camera->getYFov() * 0.5f);
		nh_half = c->camera->getZNear() * tangent;
		nw_half = nh_half * c->camera->getAspectRation();
		fh_half = c->camera->getZFar() * tangent;
		fw_half = fh_half * c->camera->getAspectRation();
	} else 
	{
		nh_half = c->camera->getYMag();
		nw_half = c->camera->getXMag();
		fh_half = nh_half;
		fw_half = nw_half;
	}

	
	const glm::vec3 nc = pos + forward * c->camera->getZNear();
	const glm::vec3 fc = pos + forward * c->camera->getZFar();

	const glm::vec3 near_top = up * nh_half;
	const glm::vec3 near_right = right * nw_half;
	const glm::vec3 far_top = up * fh_half;
	const glm::vec3 far_right = right * fw_half;
	const glm::vec3 ft = fc + far_top;
	const glm::vec3 fb = fc - far_top;
	const glm::vec3 fl = fc - far_right;
	const glm::vec3 fr = fc + far_right;

	origin = pos;
	near_corners[0] = nc + near_top - near_right;
	near_corners[1] = nc + near_top + near_right;
	near_corners[2] = nc - near_top + near_right;
	near_corners[3] = nc - near_top - near_right;
	far_corners[0] = fc + far_top - far_right;
	far_corners[1] = fc + far_top + far_right;
	far_corners[2] = fc - far_top + far_right;
	far_corners[3] = fc - far_top - far_right;

	
	plane[static_cast<uint8_t>(Plane::N)] = Plane_s{ forward, nc };
	plane[static_cast<uint8_t>(Plane::F)] = Plane_s{ -forward, fc };
	plane[static_cast<uint8_t>(Plane::T)] = Plane_s{ glm::cross(ft, right), pos };
	plane[static_cast<uint8_t>(Plane::B)] = Plane_s{ glm::cross(right, fb), pos };
	plane[static_cast<uint8_t>(Plane::L)] = Plane_s{ glm::cross(fl, up), pos };
	plane[static_cast<uint8_t>(Plane::R)] = Plane_s{ glm::cross(up, fr), pos };
}

bool Frustum::checkPointInside(const glm::vec3 p) const
{
	for (size_t i = 0; i < static_cast<size_t>(Plane::COUNT); i++)
	{
		if (glm::dot(p - plane[i].p, plane[i].n) < 0.0f) return false;
	}
	return true;

}

bool Frustum::checkAABBInside(const glm::vec3 min, const glm::vec3 max) const
{
	for (size_t i = 0; i < static_cast<size_t>(Plane::COUNT); i++)
	{
		glm::vec3 p = min;
		if (plane[i].n.x >= 0) p.x = max.x;
		if (plane[i].n.y >= 0) p.y = max.y;
		if (plane[i].n.z >= 0) p.z = max.z;

		
		
		
		

		if (glm::dot(p - plane[i].p, plane[i].n) < 0.0f) return false;
	}
	return true;
}