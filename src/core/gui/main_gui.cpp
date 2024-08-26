#include <tamashii/core/gui/main_gui.hpp>
#include <tamashii/core/common/input.hpp>
#include <tamashii/core/common/common.hpp>
#include <tamashii/core/common/math.hpp>
#include <tamashii/core/io/io.hpp>
#include <tamashii/core/scene/light.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/camera.hpp>
#include <tamashii/core/scene/material.hpp>
#include <tamashii/core/scene/scene_graph.hpp>
#include <tamashii/core/scene/render_cmd_system.hpp>
#include <tamashii/core/render/render_system.hpp>
#include <tamashii/core/render/render_backend_implementation.hpp>
#include <tamashii/core/common/vars.hpp>
#include <tamashii/core/forward.h>
#include <imgui.h>
#include <ImGuizmo.h>
#define IMOGUIZMO_RIGHT_HANDED
#include <imoguizmo.hpp>
#include <glm/gtc/color_space.hpp>

T_USE_NAMESPACE

namespace {
    
	void updateSceneGraphFromModelMatrix(const Ref& aRef)
    {
		if(aRef.transforms.empty()) return;
		glm::mat4 sg_model_matrix = glm::mat4(1.0f);
		for (TRS* trs : aRef.transforms) {
			sg_model_matrix *= trs->getMatrix(0);
		}

		glm::vec3 sg_scale;
		glm::quat sg_rotation;
		glm::vec3 sg_translation;
		ASSERT(math::decomposeTransform(sg_model_matrix, sg_translation, sg_rotation, sg_scale), "decompose error");

		glm::vec3 c_scale;
		glm::quat c_rotation;
		glm::vec3 c_translation;
		ASSERT(math::decomposeTransform(aRef.model_matrix, c_translation, c_rotation, c_scale), "decompose error");

		const bool translationNeedsUpdate = glm::any(glm::notEqual(sg_translation, c_translation));
		const bool scaleNeedsUpdate = glm::any(glm::notEqual(sg_scale, c_scale));
		const bool rotationNeedsUpdate = glm::any(glm::notEqual(sg_rotation, c_rotation));
		const bool trsNeedsUpdate = (translationNeedsUpdate || scaleNeedsUpdate || rotationNeedsUpdate);
		if (!trsNeedsUpdate) return;

		TRS* trs = aRef.transforms.front();
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
}

MainGUI::MainGUI(): mIo(nullptr), mUc(nullptr), mVerticalOffsetMenuBar(20),
                    mShowHelp(false), mShowAbout(false), mShowSaveScene(false)
{
}

void MainGUI::draw(UiConf_s* aUiConf)
{
	mIo = &ImGui::GetIO();
	mUc = aUiConf;
	
	markLights();
	
	
	showDraw();
	
	drawImGuizmo();
	if (mUc->system->getConfig().show_gui) {
		
		drawGizmo();
		
		drawMenuBar();
		
		
		drawMenu();
		
		drawEdit();
		
		drawDraw();
		
		drawRightClickMenu();
		
		drawInfo();
		
		if (mShowSaveScene) showSaveScene();
		
		if (mShowHelp) showHelp();
		
		if (mShowAbout) showAbout();
	}
}

void MainGUI::markLights()
{
	
	if (((RenderSystem*)mUc->system)->getConfig().mark_lights) {
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(mIo->DisplaySize);
		const auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
		if (ImGui::Begin("Lights", NULL, flags))
		{
			for (auto& ref : mUc->scene->getLightList()) {
				if (glm::any(glm::isnan(ref->position))) continue;
				auto& cam = static_cast<RefCameraPrivate&>(mUc->scene->getCurrentCamera());
				glm::mat4 projection_matrix = cam.camera->getProjectionMatrix();
				glm::mat4 view_matrix = cam.view_matrix;


				glm::uvec3 c = var::varToVec(var::light_overlay_color);
				ImU32 color = IM_COL32(c.x, c.y, c.z, 255);
				auto selection = mUc->scene->getSelection().reference;
				if (selection && selection->type == Ref::Type::Light && dynamic_cast<RefLight&>(*selection).ref_light_index == ref->ref_light_index) {
					color = IM_COL32(225, 156, 99, 255);
				}

				if (ref->light->getType() != Light::Type::SURFACE) {

					glm::vec4 center = projection_matrix * (view_matrix * glm::vec4(ref->position, 1.0f));
					glm::vec3 ndcSpaceCenter = glm::vec3(center) / center.w;
					glm::vec2 windowSpaceCenter = ((glm::vec2(ndcSpaceCenter) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);

					if (center.w < 0 || windowSpaceCenter.x < 0 || windowSpaceCenter.y < 0 || windowSpaceCenter.x > mIo->DisplaySize.x || windowSpaceCenter.y > mIo->DisplaySize.y) continue;

					glm::vec4 radius = projection_matrix * (view_matrix * glm::vec4(ref->position + (glm::vec3(LIGHT_OVERLAY_RADIUS) * glm::vec3(view_matrix[0][0], view_matrix[1][0], view_matrix[2][0])), 1.0f));
					glm::vec3 ndcSpaceRadius = glm::vec3(radius) / radius.w;
					glm::vec2 windowSpacePosRadius = ((glm::vec2(ndcSpaceRadius) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
					float r = glm::abs(glm::length(windowSpaceCenter - windowSpacePosRadius));


					
					ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(windowSpaceCenter.x, windowSpaceCenter.y), r, color);

					if (ref->light->getType() != Light::Type::POINT) {
						glm::vec4 dir = projection_matrix * (view_matrix * glm::vec4(ref->position + ref->direction * LIGHT_OVERLAY_RADIUS * 10.0f, 1.0f));
						glm::vec3 ndcSpaceDir = glm::vec3(dir) / dir.w;
						glm::vec2 windowSpaceDir = ((glm::vec2(ndcSpaceDir) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
						ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceCenter.x, windowSpaceCenter.y), ImVec2(windowSpaceDir.x, windowSpaceDir.y), color);
					}
					else {
						ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(windowSpaceCenter.x, windowSpaceCenter.y), 1, color);
					}
				}
				else {
					auto& sl = dynamic_cast<SurfaceLight&>(*ref->light);
					const glm::mat4 mvp_mat = projection_matrix * view_matrix * ref->model_matrix;
					if (sl.getShape() == SurfaceLight::Shape::SQUARE || sl.getShape() == SurfaceLight::Shape::RECTANGLE) {
						glm::vec4 center = mvp_mat * sl.getCenter();
						glm::vec4 direction = mvp_mat * (sl.getCenter() + sl.getDefaultDirection() * 0.5f);
						glm::vec4 p0 = mvp_mat * (sl.getCenter() + glm::vec4(-0.5, -0.5, 0, 0));
						glm::vec4 p1 = mvp_mat * (sl.getCenter() + glm::vec4(-0.5, 0.5, 0, 0));
						glm::vec4 p2 = mvp_mat * (sl.getCenter() + glm::vec4(0.5, -0.5, 0, 0));
						glm::vec4 p3 = mvp_mat * (sl.getCenter() + glm::vec4(0.5, 0.5, 0, 0));

						glm::vec3 ndcSpaceCenter = glm::vec3(center) / center.w;
						glm::vec2 windowSpaceCenter = ((glm::vec2(ndcSpaceCenter) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
						glm::vec3 ndcSpaceDirection = glm::vec3(direction) / direction.w;
						glm::vec2 windowSpaceDirection = ((glm::vec2(ndcSpaceDirection) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
						glm::vec3 ndcSpaceP0 = glm::vec3(p0) / p0.w;
						glm::vec2 windowSpaceP0 = ((glm::vec2(ndcSpaceP0) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
						glm::vec3 ndcSpaceP1 = glm::vec3(p1) / p1.w;
						glm::vec2 windowSpaceP1 = ((glm::vec2(ndcSpaceP1) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
						glm::vec3 ndcSpaceP2 = glm::vec3(p2) / p2.w;
						glm::vec2 windowSpaceP2 = ((glm::vec2(ndcSpaceP2) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
						glm::vec3 ndcSpaceP3 = glm::vec3(p3) / p3.w;
						glm::vec2 windowSpaceP3 = ((glm::vec2(ndcSpaceP3) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);

						if (p0.w > 0 && p1.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP0.x, windowSpaceP0.y), ImVec2(windowSpaceP1.x, windowSpaceP1.y), color);
						if (p0.w > 0 && p2.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP0.x, windowSpaceP0.y), ImVec2(windowSpaceP2.x, windowSpaceP2.y), color);
						if (p3.w > 0 && p1.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP3.x, windowSpaceP3.y), ImVec2(windowSpaceP1.x, windowSpaceP1.y), color);
						if (p3.w > 0 && p2.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP3.x, windowSpaceP3.y), ImVec2(windowSpaceP2.x, windowSpaceP2.y), color);
						if (center.w > 0 && direction.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceCenter.x, windowSpaceCenter.y), ImVec2(windowSpaceDirection.x, windowSpaceDirection.y), color);
					}
					else if (sl.getShape() == SurfaceLight::Shape::DISK || sl.getShape() == SurfaceLight::Shape::ELLIPSE) {
						glm::vec4 center = mvp_mat * sl.getCenter();
						glm::vec4 direction = mvp_mat * (sl.getCenter() + sl.getDefaultDirection() * 0.5f);

						glm::vec3 ndcSpaceCenter = glm::vec3(center) / center.w;
						glm::vec2 windowSpaceCenter = ((glm::vec2(ndcSpaceCenter) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
						glm::vec3 ndcSpaceDirection = glm::vec3(direction) / direction.w;
						glm::vec2 windowSpaceDirection = ((glm::vec2(ndcSpaceDirection) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
						if (center.w > 0 && direction.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceCenter.x, windowSpaceCenter.y), ImVec2(windowSpaceDirection.x, windowSpaceDirection.y), color);
	
						glm::vec4 t = glm::vec4(0.5f, 0, 0, 0);
						glm::vec4 b = glm::vec4(0, 0.5f, 0, 0);

						int sides = 360 / 360;
						std::vector<ImVec2> points;
						points.reserve(sides);
						int count = 0;
						for (float a = 0; a < 361; a += sides) {
							float rad = glm::radians(a);
							glm::vec3 p0 = (t) * std::sin(rad) + (b) * std::cos(rad);
							glm::vec4 p = mvp_mat * (sl.getCenter() + glm::vec4(p0,0));
							glm::vec3 ndcSpaceP = glm::vec3(p) / p.w;
							glm::vec2 windowSpaceP = ((glm::vec2(ndcSpaceP) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
							
							points.emplace_back(windowSpaceP.x, windowSpaceP.y);
							count++;
						}
                        ImGui::GetBackgroundDrawList()->AddPolyline(points.data(), count, color, 0, 1);
					}
				}
			}
			ImGui::End();
		}
	}
	
}

void MainGUI::markCamera()
{
	auto& cc = mUc->scene->getCurrentCamera();
	auto& cams = mUc->scene->getAvailableCameras();
	for (auto& rc : cams) {
		if (rc.get() == &cc) continue;

		
		Frustum frust(rc.get());
		const glm::mat4 mvp_mat = cc.camera->getProjectionMatrix() * cc.view_matrix;
		ImU32 color = (ImU32)IM_COL32(220, 220, 220, 255);

		glm::vec4 center = mvp_mat * glm::vec4(frust.origin, 1);
		glm::vec4 p0 = mvp_mat * glm::vec4(frust.near_corners[0], 1);
		glm::vec4 p1 = mvp_mat * glm::vec4(frust.far_corners[0], 1);

		glm::vec4 p2 = mvp_mat * glm::vec4(frust.near_corners[1], 1);
		glm::vec4 p3 = mvp_mat * glm::vec4(frust.far_corners[1], 1);

		glm::vec4 p4 = mvp_mat * glm::vec4(frust.near_corners[2], 1);
		glm::vec4 p5 = mvp_mat * glm::vec4(frust.far_corners[2], 1);

		glm::vec4 p6 = mvp_mat * glm::vec4(frust.near_corners[3], 1);
		glm::vec4 p7 = mvp_mat * glm::vec4(frust.far_corners[3], 1);

		glm::vec3 ndcSpaceCenter = glm::vec3(center) / center.w;
		glm::vec2 windowSpaceCenter = ((glm::vec2(ndcSpaceCenter) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
		glm::vec3 ndcSpaceP0 = glm::vec3(p0) / p0.w;
		glm::vec2 windowSpaceP0 = ((glm::vec2(ndcSpaceP0) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
		glm::vec3 ndcSpaceP1 = glm::vec3(p1) / p1.w;
		glm::vec2 windowSpaceP1 = ((glm::vec2(ndcSpaceP1) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);

		glm::vec3 ndcSpaceP2 = glm::vec3(p2) / p2.w;
		glm::vec2 windowSpaceP2 = ((glm::vec2(ndcSpaceP2) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
		glm::vec3 ndcSpaceP3 = glm::vec3(p3) / p3.w;
		glm::vec2 windowSpaceP3 = ((glm::vec2(ndcSpaceP3) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);

		glm::vec3 ndcSpaceP4 = glm::vec3(p4) / p4.w;
		glm::vec2 windowSpaceP4 = ((glm::vec2(ndcSpaceP4) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
		glm::vec3 ndcSpaceP5 = glm::vec3(p5) / p5.w;
		glm::vec2 windowSpaceP5 = ((glm::vec2(ndcSpaceP5) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);

		glm::vec3 ndcSpaceP6 = glm::vec3(p6) / p6.w;
		glm::vec2 windowSpaceP6 = ((glm::vec2(ndcSpaceP6) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
		glm::vec3 ndcSpaceP7 = glm::vec3(p7) / p7.w;
		glm::vec2 windowSpaceP7 = ((glm::vec2(ndcSpaceP7) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);

		if (p0.w > 0 && p1.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP0.x, windowSpaceP0.y), ImVec2(windowSpaceP1.x, windowSpaceP1.y), color);
		if (p2.w > 0 && p3.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP2.x, windowSpaceP2.y), ImVec2(windowSpaceP3.x, windowSpaceP3.y), color);
		if (p4.w > 0 && p5.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP4.x, windowSpaceP4.y), ImVec2(windowSpaceP5.x, windowSpaceP5.y), color);
		if (p6.w > 0 && p7.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP6.x, windowSpaceP6.y), ImVec2(windowSpaceP7.x, windowSpaceP7.y), color);

		if (p0.w > 0 && p2.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP0.x, windowSpaceP0.y), ImVec2(windowSpaceP2.x, windowSpaceP2.y), color);
		if (p2.w > 0 && p4.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP2.x, windowSpaceP2.y), ImVec2(windowSpaceP4.x, windowSpaceP4.y), color);
		if (p4.w > 0 && p6.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP4.x, windowSpaceP4.y), ImVec2(windowSpaceP6.x, windowSpaceP6.y), color);
		if (p6.w > 0 && p0.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP6.x, windowSpaceP6.y), ImVec2(windowSpaceP0.x, windowSpaceP0.y), color);

		if (p0.w > 0 && center.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP0.x, windowSpaceP0.y), ImVec2(windowSpaceCenter.x, windowSpaceCenter.y), color);
		if (p2.w > 0 && center.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP2.x, windowSpaceP2.y), ImVec2(windowSpaceCenter.x, windowSpaceCenter.y), color);
		if (p4.w > 0 && center.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP4.x, windowSpaceP4.y), ImVec2(windowSpaceCenter.x, windowSpaceCenter.y), color);
		if (p6.w > 0 && center.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP6.x, windowSpaceP6.y), ImVec2(windowSpaceCenter.x, windowSpaceCenter.y), color);

		if (p1.w > 0 && p3.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP1.x, windowSpaceP1.y), ImVec2(windowSpaceP3.x, windowSpaceP3.y), color);
		if (p3.w > 0 && p5.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP3.x, windowSpaceP3.y), ImVec2(windowSpaceP5.x, windowSpaceP5.y), color);
		if (p5.w > 0 && p7.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP5.x, windowSpaceP5.y), ImVec2(windowSpaceP7.x, windowSpaceP7.y), color);
		if (p7.w > 0 && p1.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceP7.x, windowSpaceP7.y), ImVec2(windowSpaceP1.x, windowSpaceP1.y), color);
	}
}

void MainGUI::showDraw()
{
	if (mUc->draw_info->mDrawMode && mUc->draw_info->mHoverOver && mUc->scene->getCurrentCamera().mode == RefCamera::Mode::EDITOR) {
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(mIo->DisplaySize);
		constexpr auto flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus;
		if (ImGui::Begin("DrawOverlay", nullptr, flags))
		{
			ImColor color(mUc->draw_info->mCursorColor.x, mUc->draw_info->mCursorColor.y, mUc->draw_info->mCursorColor.z);
			auto& cam = static_cast<RefCameraPrivate&>(mUc->scene->getCurrentCamera());
			glm::mat4 projection_matrix = cam.camera->getProjectionMatrix();
			glm::mat4 view_matrix = cam.view_matrix;
			DrawInfo di = *mUc->draw_info;
			const glm::mat4 vp_mat = projection_matrix * view_matrix;
			glm::vec4 center = vp_mat * glm::vec4(di.mPositionWs, 1);
			

			glm::vec4 direction = vp_mat * (glm::vec4(di.mPositionWs, 1) + (glm::vec4(di.mNormalWsNorm,0) * mUc->draw_info->mRadius));

			glm::vec3 ndcSpaceCenter = glm::vec3(center) / center.w;
			glm::vec2 windowSpaceCenter = ((glm::vec2(ndcSpaceCenter) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
			glm::vec3 ndcSpaceDirection = glm::vec3(direction) / direction.w;
			glm::vec2 windowSpaceDirection = ((glm::vec2(ndcSpaceDirection) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
			if (center.w > 0 && direction.w > 0) ImGui::GetBackgroundDrawList()->AddLine(ImVec2(windowSpaceCenter.x, windowSpaceCenter.y), ImVec2(windowSpaceDirection.x, windowSpaceDirection.y), color);
	
            glm::vec3 b_ws_norm = glm::cross(di.mNormalWsNorm, di.mTangentWsNorm);

			int sides = 360 / 360;
			std::vector<ImVec2> points;
			points.reserve(sides);
			int count = 0;
			for (float a = 0; a < 361; a += sides) {
				float rad = glm::radians(a);
                glm::vec3 p0 = di.mTangentWsNorm * std::sin(rad) + b_ws_norm * std::cos(rad);
				glm::vec4 p = vp_mat * (glm::vec4(di.mPositionWs, 1) + (glm::vec4(normalize(p0),0) * mUc->draw_info->mRadius));
				glm::vec3 ndcSpaceP = glm::vec3(p) / p.w;
				glm::vec2 windowSpaceP = ((glm::vec2(ndcSpaceP) + glm::vec2(1.0)) / glm::vec2(2.0)) * glm::vec2(mIo->DisplaySize.x, mIo->DisplaySize.y);
				
				points.emplace_back(windowSpaceP.x, windowSpaceP.y);
				count++;
			}
            ImGui::GetBackgroundDrawList()->AddPolyline(points.data(), count, color, 0, 1);

			ImGui::End();
		}
	}
}

void MainGUI::drawImGuizmo()
{
	static ImGuizmo::OPERATION	op = ImGuizmo::TRANSLATE;
	static ImGuizmo::MODE mode = ImGuizmo::WORLD;

	if (auto selection = mUc->scene->getSelection().reference) {
		ImGuizmo::BeginFrame();
		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetRect(0, 0, mIo->DisplaySize.x, mIo->DisplaySize.y);
		ImGuizmo::AllowAxisFlip(false);

		
		if (InputSystem::getInstance().isDown(Input::MOUSE_WHEEL)) ImGuizmo::Enable(false);
		else ImGuizmo::Enable(true);

		
		auto& cam = static_cast<RefCameraPrivate&>(mUc->scene->getCurrentCamera());
		glm::mat4 projection_matrix = cam.camera->getProjectionMatrix();
		glm::mat4 view_matrix = cam.view_matrix;
		if(cam.y_flipped) { view_matrix[0][1] *= -1; view_matrix[1][1] *= -1; view_matrix[2][1] *= -1; view_matrix[3][1] *= -1; }
 
		
		if (InputSystem::getInstance().wasReleased(Input::KEY_T)) op = ImGuizmo::TRANSLATE;
		else if (InputSystem::getInstance().wasReleased(Input::KEY_R)) op = ImGuizmo::ROTATE;
		else if (InputSystem::getInstance().wasReleased(Input::KEY_S)) op = ImGuizmo::SCALE;
		if (InputSystem::getInstance().wasReleased(Input::KEY_W)) mode = ImGuizmo::WORLD;
		else if (InputSystem::getInstance().wasReleased(Input::KEY_L)) mode = ImGuizmo::LOCAL;

		if(op == ImGuizmo::TRANSLATE) mOp = "Translate";
		else if (op == ImGuizmo::ROTATE) mOp = "Rotate";
		else if (op == ImGuizmo::SCALE) mOp = "Scale";
		if (mode == ImGuizmo::LOCAL) mMode = "Local";
		else if (mode == ImGuizmo::WORLD) mMode = "World";

		
		static bool drag = false;
		if (!drag && ImGuizmo::IsUsing()) {
			mHistory.emplace_back(selection, selection->model_matrix);
		};
		drag = ImGuizmo::IsUsing();

		
		glm::mat4 delta(1.0f);
		static float snap[3] = { 1,1,1 };
		const float* pSnap = InputSystem::getInstance().isDown(Input::KEY_LCTRL) ? &snap[0] : nullptr;
		if(ImGuizmo::Manipulate(glm::value_ptr(view_matrix), glm::value_ptr(projection_matrix), op, mode,
			glm::value_ptr(selection->model_matrix), glm::value_ptr(delta), pSnap)) {

			TRS* trs = nullptr;
			if (!selection->transforms.empty()) trs = selection->transforms.front();
			if(trs)
			{
				glm::vec3 scale;
				glm::quat rotation;
				glm::vec3 translation;
                ASSERT(math::decomposeTransform(delta, translation, rotation, scale), "decompose error");
				/* trans  */
				if (op == ImGuizmo::OPERATION::TRANSLATE) {
					trs->translation += translation;
					if (trs->hasTranslationAnimation()) for (glm::vec3& v : trs->translationSteps) v += translation;
				}
				/* scale  */
				if (op == ImGuizmo::OPERATION::SCALE) {
					if (trs->hasScale()) trs->scale *= scale;
					else trs->scale = scale;
					if (trs->hasScaleAnimation()) for (glm::vec3& v : trs->scaleSteps) v *= scale;
				}
				/* rotate */
				if (op == ImGuizmo::OPERATION::ROTATE) {
					if (trs->hasRotation()) {
						
						const glm::quat oldRotation{ trs->rotation[3], trs->rotation[0], trs->rotation[1] , trs->rotation[2] };
						glm::quat newRotation = rotation * oldRotation;
						
						trs->rotation = { newRotation[1], newRotation[2], newRotation[3] , newRotation[0] };
						
						
					}
					else trs->rotation = { rotation[1], rotation[2], rotation[3] , rotation[0] };
					if (trs->hasRotationAnimation()) for (glm::vec4& v : trs->rotationSteps) {
						glm::quat newRotation = rotation * glm::quat{ v[3], v[0], v[1] , v[2] };
						v = { newRotation[1], newRotation[2], newRotation[3] , newRotation[0] };
					}
				}
			}
			selection->model_matrix = glm::mat4(1.0f);
			for (const TRS* t : selection->transforms) {
				selection->model_matrix *= t->getMatrix(mUc->scene->getCurrentTime());
			}

			if (selection->type == Ref::Type::Light) {
				auto& lightRef = dynamic_cast<RefLight&>(*selection);
				lightRef.position = lightRef.model_matrix * glm::vec4(0, 0, 0, 1);
				lightRef.direction = glm::normalize(lightRef.model_matrix * lightRef.light->getDefaultDirection());
				if (lightRef.light->getType() == Light::Type::SURFACE) {
					const float scale_x = glm::length(glm::vec3(lightRef.model_matrix[0]));
					const float scale_y = glm::length(glm::vec3(lightRef.model_matrix[1]));
					const float scale_z = glm::length(glm::vec3(lightRef.model_matrix[2]));
					auto& surfaceLight = dynamic_cast<SurfaceLight&>(*lightRef.light);
					surfaceLight.setDimensions(glm::vec3(scale_x, scale_y, scale_z));
				}
				mUc->scene->requestLightUpdate();
				if (!lightRef.transforms.empty()) trs = lightRef.transforms.front();
			}
			else if (selection->type == Ref::Type::Model) {
				mUc->scene->requestModelInstanceUpdate();
				bool light = false;
				const auto& modelRef = dynamic_cast<RefModel&>(*selection);
				for (const auto& refMesh : modelRef.refMeshes) light |= refMesh->mesh->getMaterial()->isLight();
				if (light) mUc->scene->requestLightUpdate();
				if (!modelRef.transforms.empty()) trs = modelRef.transforms.front();
			}
		}
	}
	if (mUc->scene->getCurrentCamera().mode == RefCamera::Mode::EDITOR && InputSystem::getInstance().isDown(Input::KEY_LCTRL) && InputSystem::getInstance().wasPressed(Input::KEY_Z) && mHistory.size()) {
		while (!mHistory.empty() && mHistory.back().first == nullptr) mHistory.pop_back();
		mHistory.back().first->model_matrix = mHistory.back().second;
		if (mHistory.back().first->type == Ref::Type::Light) {
			auto& r = dynamic_cast<RefLight&>(*mHistory.back().first);
			r.position = r.model_matrix * glm::vec4(0, 0, 0, 1);
			r.direction = glm::normalize(r.model_matrix * r.light->getDefaultDirection());
			mUc->scene->requestLightUpdate();
		}
		if (mHistory.back().first->type == Ref::Type::Model) {
			mUc->scene->requestModelInstanceUpdate();
		}
		mHistory.pop_back();
	}
}

void MainGUI::drawGizmo() const
{
	float pivotDistance = -1.0f;
	auto& pcam = reinterpret_cast<RefCameraPrivate&>(mUc->scene->getCurrentCamera());
	if (pcam.mode == RefCamera::Mode::EDITOR)
	{
		pivotDistance = pcam.editor_data.zoom;
	}

	auto viewMatrix = pcam.view_matrix;
	if (pcam.y_flipped) { viewMatrix[0][1] *= -1; viewMatrix[1][1] *= -1; viewMatrix[2][1] *= -1; viewMatrix[3][1] *= -1; }
	const glm::mat4 projMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1000.0f);

	ImOGuizmo::SetRect(0.0f, mIo->DisplaySize.y - 120.0f, 120.0f);
	ImOGuizmo::BeginFrame();
	if (ImOGuizmo::DrawGizmo(value_ptr(viewMatrix), value_ptr(projMatrix), pivotDistance))
	{
		glm::mat4 mm = glm::inverse(viewMatrix);
		pcam.setModelMatrix(mm, true);
		mUc->scene->requestCameraUpdate();
	}
}

void MainGUI::drawMenuBar()
{
	if (InputSystem::getInstance().isDown(Input::KEY_LCTRL) && InputSystem::getInstance().wasPressed(Input::KEY_S)) {
		Common::getInstance().getMainWindow()->ungrabMouse();
		mShowSaveScene = true;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 1);
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(41.0f / 255.0f, 45.0f / 255.0f, 55.0f / 255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(41.0f / 255.0f, 45.0f / 255.0f, 55.0f / 255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(82.0f / 255.0f, 86.0f / 255.0f, 97.0f / 255.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(82.0f / 255.0f, 86.0f / 255.0f, 97.0f / 255.0f, 1.0f));
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Scene")) EventSystem::queueEvent(EventType::ACTION, Input::A_NEW_SCENE);
			else if (ImGui::MenuItem("Open Scene", "CTRL+O")) Common::openFileDialogOpenScene();
			ImGui::Separator();
			if (ImGui::MenuItem("Add Scene", "CTRL+A")) Common::openFileDialogAddScene();
			else if (ImGui::MenuItem("Add Model", "CTRL+M")) Common::openFileDialogOpenModel();
			else if (ImGui::MenuItem("Add Light", "CTRL+L")) Common::openFileDialogOpenLight();

			ImGui::Separator();
			if (ImGui::MenuItem("Export Scene", "CTRL+S")) mShowSaveScene = true;
			ImGui::Separator();
			if (ImGui::MenuItem("Exit", "ALT+F4"))
			{
				EventSystem::queueEvent(EventType::ACTION, Input::A_EXIT, 0, 0, 0, "");
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("View"))
		{
			RenderConfig_s &rc = ((RenderSystem*)mUc->system)->getConfig();
			if (ImGui::MenuItem("Show UI", "F1", rc.show_gui)) {
				rc.show_gui = !(rc.show_gui);
			}
			if (ImGui::MenuItem("Show Lights", "F2", rc.mark_lights)) {
				rc.mark_lights = !(rc.mark_lights);
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("Usage")) {
				mShowHelp = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("About")) {
				mShowAbout = true;
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Clear Cache")) {
				EventSystem::queueEvent(EventType::ACTION, Input::A_CLEAR_CACHE);
			}
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleVar(1);
	ImGui::PopStyleColor(4);
}

void MainGUI::drawConsole()
{
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(mIo->DisplaySize.x, 0.0f));

	constexpr auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoSavedSettings;

	const ImGuiInputTextCallback cb = [](ImGuiInputTextCallbackData* data)
	{
		return 0;
	};

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	if (ImGui::Begin("Console", nullptr, flags))
	{
		char a[256] = {};
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::SetKeyboardFocusHere(0);
		ImGui::InputText("##asd", &a[0], 30, ImGuiInputTextFlags_CallbackEdit, cb);
		ImGui::PopItemWidth();
		ImGui::End();
	}
	ImGui::PopStyleVar(2);
}

void MainGUI::drawMenu()
{
	constexpr float distance = 10.0f;
	const ImVec2 pos = ImVec2(distance, mVerticalOffsetMenuBar + distance);
	constexpr ImVec2 posPivot = ImVec2(0.0f, 0.0f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);

	constexpr ImVec2 minSize = ImVec2(0.0f, 0.0f);
	const ImVec2 maxSize = ImVec2(-1.0f, static_cast<float>(Common::getInstance().getMainWindow()->window()->getSize().y - 150));
	ImGui::SetNextWindowSizeConstraints(minSize, maxSize);

	constexpr auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoSavedSettings;

	if (ImGui::Begin("Settings", nullptr, flags))
	{
		/*ImGui::Text("Info");
		ImGui::Separator();
		ImGui::BulletText("F5: take Screenshot");
		ImGui::Text("FPS Camera Controls");
		ImGui::Separator();
		ImGui::BulletText("Enter Camera: Left Click");
		ImGui::BulletText("Move: WASDQE");
		ImGui::BulletText("Change Speed: Scroll Wheel");
		ImGui::BulletText("Exit Camera: ESC");
		ImGui::NewLine();*/

		
		ImGui::Text("Cameras:");
		if (!mUc->scene->getAvailableCameras().empty()) {
			if (ImGui::BeginCombo("##Camera Combo", mUc->scene->getCurrentCamera().camera->getName().data(), ImGuiComboFlags_NoArrowButton)) {
				for (auto& cam : mUc->scene->getAvailableCameras()) {
					const bool is_selected = (cam->ref_camera_index == mUc->scene->getCurrentCamera().ref_camera_index);
					if (ImGui::Selectable((std::string(cam->camera->getName()) + "##" + std::to_string(cam->ref_camera_index)).c_str(), is_selected)) mUc->scene->setCurrentCamera(cam);
					if (is_selected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			auto& pcam = dynamic_cast<RefCameraPrivate&>(mUc->scene->getCurrentCamera());
			RefCamera::Mode mode = pcam.mode;
			const float width = ImGui::GetContentRegionAvail().x * (pcam.default_camera ? 0.5f : 0.3333f);
			if(pcam.mode == RefCamera::Mode::EDITOR)
			{
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.4f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.3f, 0.6f, 0.4f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.4f, 0.7f, 0.5f, 1.0f });
			}
			if (ImGui::Button("Editor", ImVec2(width, 0))) mode = RefCamera::Mode::EDITOR;
			if (pcam.mode == RefCamera::Mode::EDITOR) ImGui::PopStyleColor(3);
			
			if (!pcam.default_camera)
			{
				ImGui::SameLine();
				if (pcam.mode == RefCamera::Mode::STATIC) {
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.4f, 1.0f });
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.3f, 0.6f, 0.4f, 1.0f });
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.4f, 0.7f, 0.5f, 1.0f });
				}
				if(ImGui::Button("Static", ImVec2(width, 0.0f))) mode = RefCamera::Mode::STATIC;
				if (pcam.mode == RefCamera::Mode::STATIC) ImGui::PopStyleColor(3);
			}
			ImGui::SameLine();
			if (pcam.mode == RefCamera::Mode::FPS)
			{
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.4f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.3f, 0.6f, 0.4f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.4f, 0.7f, 0.5f, 1.0f });
			}
			if (ImGui::Button("FPS", ImVec2(-1, 0))) mode = RefCamera::Mode::FPS;
			if (pcam.mode == RefCamera::Mode::FPS) ImGui::PopStyleColor(3);
			pcam.mode = mode;
			
		}
		ImGui::Separator();

		if (mUc->scene->getCycleTime() != 0.0f) {
			ImGui::Text("Animation:");
			if (mUc->scene->animation()) {
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.8f, 0.3f, 0.4f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.6f, 0.3f, 0.4f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.7f, 0.4f, 0.5f, 1.0f });
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.4f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.3f, 0.6f, 0.4f, 1.0f });
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.4f, 0.7f, 0.5f, 1.0f });
			}

			if ((mUc->scene->animation() && ImGui::Button("Stop", ImVec2(50, 0))) || (!mUc->scene->animation() && ImGui::Button("Start", ImVec2(50, 0))))  mUc->scene->setAnimation(!mUc->scene->animation());
			ImGui::PopStyleColor(3);
			ImGui::SameLine();
			if (ImGui::Button("Reset", ImVec2(50, 0))) mUc->scene->resetAnimation();
			ImGui::SameLine();
			float current_scene_time = std::fmod(mUc->scene->getCurrentTime(), mUc->scene->getCycleTime());
			if (std::isnan(current_scene_time)) current_scene_time = 0;
			ImGui::Text("  %.2f / %.2f", current_scene_time, mUc->scene->getCycleTime());
			ImGui::Separator();
		}

		
		const RenderSystem* system = mUc->system;
		const std::vector<RenderBackendImplementation*>& availableImpl = system->getAvailableBackendImplementations();
		if (availableImpl.size() > 1) {
			ImGui::PushItemWidth(155);
			ImGui::Text("Implementation:");

			RenderBackendImplementation* current_impl = system->getCurrentBackendImplementations();
			int next_impl_index = -1;
			if (ImGui::BeginCombo("##Implementation Combo", current_impl->getName(), ImGuiComboFlags_NoArrowButton)) {
                for (uint32_t i = 0; i < availableImpl.size(); i++)
				{
					const bool isSelected = availableImpl.at(i) == current_impl;
					if (ImGui::Selectable((std::string(availableImpl.at(i)->getName()) + "##" + std::to_string(i)).c_str(), isSelected)) next_impl_index = i;
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			if (next_impl_index != -1 && current_impl != availableImpl.at(next_impl_index)) EventSystem::queueEvent(EventType::ACTION, Input::A_CHANGE_BACKEND_IMPL, next_impl_index);
			ImGui::PopItemWidth();
			ImGui::SameLine();
		}
		if(ImGui::Button("Reload", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f))) EventSystem::queueEvent(EventType::ACTION, Input::A_RELOAD_BACKEND_IMPL);

		
		if (!mUc->scene->getLightList().empty() || !mUc->scene->getModelList().empty()) {
			ImGui::Separator();
			if (ImGui::CollapsingHeader("Scene", 0))
			{
				const auto& ll = mUc->scene->getLightList();
				const auto& ml = mUc->scene->getModelList();
				const int size = static_cast<int>(ll.size() + ml.size());
				std::vector<const char*> items;
				std::vector<std::shared_ptr<Ref>> objects;
				items.reserve(size);
				objects.reserve(size);

				int current = -1;
				const auto& selection = mUc->scene->getSelection().reference;
				for (auto& l : ll) {
					if (selection != nullptr && selection == l) current = static_cast<int>(items.size());
					items.push_back(l->light->getName().data());
					objects.push_back(l);
				}
				for (auto& m : ml) {
					if (selection != nullptr && selection == m) current = static_cast<int>(items.size());
					items.push_back(m->model->getName().data());
					objects.push_back(m);
				}

				if(ImGui::ListBox("##scene_graph", &current, items.data(), size, std::min(4, static_cast<int>(items.size()))))
				{
					if (!objects.empty())mUc->scene->setSelection({ std::move(objects[current]) });
				}
			}
		}

		ImGui::End();
	}
	if (InputSystem::getInstance().wasPressed(Input::KEY_ESCAPE)) mShowHelp = mShowAbout = false;
}

void MainGUI::drawEdit()
{
	Selection selection = mUc->scene->getSelection();
	if (selection.reference) {
		bool lightsRequiereUpdate = false;
		bool materialsRequiereUpdate = false;
		bool modelInstancesRequiereUpdate = false;

		constexpr float distance = 10.0f;
		const ImVec2 pos = ImVec2(mIo->DisplaySize.x - distance, mIo->DisplaySize.y - distance);
		const ImVec2 posPivot = ImVec2(1.0f, 1.0f);
		ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);

		constexpr auto flags =
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoSavedSettings;

		if (ImGui::Begin("Edit", nullptr, flags))
		{
			ImGui::Text("Edit Mode");
			std::string name;
			std::string selectionType;
			if (selection.reference->type == Ref::Type::Model) {
				name = dynamic_cast<RefModel&>(*selection.reference).model->getName();
				selectionType = "Model";
			}
			else if (selection.reference->type == Ref::Type::Light) {
				auto& light= dynamic_cast<RefLight&>(*selection.reference).light;
				name = light->getName();
				Light::Type type = light->getType();
				switch (type) {
				case Light::Type::POINT: selectionType = "Point Light"; break;
				case Light::Type::SPOT: selectionType = "Spot Light"; break;
				case Light::Type::DIRECTIONAL: selectionType = "Directional Light"; break;
				case Light::Type::SURFACE: selectionType = "Surface Light"; break;
				case Light::Type::IES: selectionType = "IES Light"; break;
				}
			}
			ImGui::Text("Selected: %s", name.c_str());
			ImGui::Text("Type: %s", selectionType.c_str());
			ImGui::Text("Operation: %s", mOp.c_str());
			ImGui::Text("System: %s", mMode.c_str());
			bool model_mat_requieres_update = false;
			ImGui::Text("Model Mat:");
			ImGui::SameLine();
			model_mat_requieres_update |= ImGui::DragFloat4("##c0", &selection.reference->model_matrix[0][0], 0.1f, 0, 0, "%.3f", 0);
			ImGui::Text("          ");
			ImGui::SameLine();
			model_mat_requieres_update |= ImGui::DragFloat4("##c1", &selection.reference->model_matrix[1][0], 0.1f, 0, 0, "%.3f", 0);
			ImGui::Text("          ");
			ImGui::SameLine();
			model_mat_requieres_update |= ImGui::DragFloat4("##c2", &selection.reference->model_matrix[2][0], 0.1f, 0, 0, "%.3f", 0);
			ImGui::Text("          ");
			ImGui::SameLine();
			model_mat_requieres_update |= ImGui::DragFloat4("##c3", &selection.reference->model_matrix[3][0], 0.1f, 0, 0, "%.3f", 0);
			
			if (model_mat_requieres_update) updateSceneGraphFromModelMatrix(*selection.reference);
			if (selection.reference->type == Ref::Type::Light) {
				lightsRequiereUpdate |= model_mat_requieres_update;

				auto& refLight = dynamic_cast<RefLight&>(*selection.reference);
				if (model_mat_requieres_update) {
					refLight.position = refLight.model_matrix * glm::vec4(0, 0, 0, 1);
					refLight.direction = glm::normalize(refLight.model_matrix * refLight.light->getDefaultDirection());
					if (refLight.light->getType() == Light::Type::SURFACE) {
						const float scale_x = glm::length(glm::vec3(refLight.model_matrix[0]));
						const float scale_y = glm::length(glm::vec3(refLight.model_matrix[1]));
						const float scale_z = glm::length(glm::vec3(refLight.model_matrix[2]));
						
						dynamic_cast<SurfaceLight&>(*refLight.light).setDimensions(glm::vec3(scale_x, scale_y, scale_z));
					}
				}
				glm::vec3 c = refLight.light->getColor();
				ImGui::Text("Color:    ");
				ImGui::SameLine();
				lightsRequiereUpdate |= ImGui::ColorEdit3("##light_color", &c[0]);
				refLight.light->setColor(c);

				float intensity = refLight.light->getIntensity();
				ImGui::Text("Intensity:");
				ImGui::SameLine();
				lightsRequiereUpdate |= ImGui::DragFloat("##intensity", &intensity, 1.0f, 0.0f, std::numeric_limits<float>::max(), "%f W");
				refLight.light->setIntensity(intensity);

				Light::Type type = refLight.light->getType();
				if (type == Light::Type::SPOT || type == Light::Type::POINT) {
					float range;
					if (type == Light::Type::SPOT) range = dynamic_cast<SpotLight&>(*refLight.light).getRange();
					else if (type == Light::Type::POINT) range = dynamic_cast<PointLight&>(*refLight.light).getRange();
					ImGui::Text("Range:    ");
					ImGui::SameLine();
					lightsRequiereUpdate |= ImGui::DragFloat("##range", &range, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%f m");
					if (type == Light::Type::SPOT) dynamic_cast<SpotLight&>(*refLight.light).setRange(range);
					else if (type == Light::Type::POINT) dynamic_cast<PointLight&>(*refLight.light).setRange(range);
				}

				if (type == Light::Type::SPOT || type == Light::Type::POINT || type == Light::Type::SURFACE || type == Light::Type::IES) {
					if (type == Light::Type::SPOT || type == Light::Type::POINT || type == Light::Type::IES) {
						float radius;
						if (type == Light::Type::SPOT) radius = dynamic_cast<SpotLight&>(*refLight.light).getRadius();
						else if (type == Light::Type::POINT) radius = dynamic_cast<PointLight&>(*refLight.light).getRadius();
						else if (type == Light::Type::IES) radius = dynamic_cast<IESLight&>(*refLight.light).getRadius();
						ImGui::Text("Radius:   ");
						ImGui::SameLine();
						lightsRequiereUpdate |= ImGui::DragFloat("##radius", &radius, 0.01f, 0.0f, std::numeric_limits<float>::max(), "%f m");
						if (type == Light::Type::SPOT) dynamic_cast<SpotLight&>(*refLight.light).setRadius(radius);
						else if (type == Light::Type::POINT) dynamic_cast<PointLight&>(*refLight.light).setRadius(radius);
						else if (type == Light::Type::IES) dynamic_cast<IESLight&>(*refLight.light).setRadius(radius);
					}

					if (type == Light::Type::SPOT) {
						auto& spotLight = dynamic_cast<SpotLight&>(*refLight.light);
						float inner = glm::degrees(spotLight.getInnerConeAngle() * 2.0f);
						float outer = glm::degrees(spotLight.getOuterConeAngle() * 2.0f);
						ImGui::Text("Inner:    ");
						ImGui::SameLine();
						lightsRequiereUpdate |= ImGui::SliderFloat("##inner", &inner, 0.0f, outer/* - 0.00001f*/, "%.3f degree");
						ImGui::Text("Outer:    ");
						ImGui::SameLine();
						lightsRequiereUpdate |= ImGui::SliderFloat("##outer", &outer, 1.0f /*+ 0.00001f*/, 180.0f, "%.3f degree");
						spotLight.setCone(glm::radians(std::min(inner, outer) * 0.5f), glm::radians(outer * 0.5f));
					}
					else if (type == Light::Type::SURFACE) {
						auto& sl = dynamic_cast<SurfaceLight&>(*refLight.light);
						glm::vec3 size = sl.getDimensions();
						SurfaceLight::Shape shape = sl.getShape();
						if (!sl.is3D()) {
							if (shape == SurfaceLight::Shape::RECTANGLE || shape == SurfaceLight::Shape::ELLIPSE) {
								if (shape == SurfaceLight::Shape::RECTANGLE) ImGui::Text("Width:    ");
								else ImGui::Text("Dia Horiz:");
								ImGui::SameLine();
								lightsRequiereUpdate |= ImGui::DragFloat("##width", &size.x, 0.1f, 0.0001f, std::numeric_limits<float>::max(), "%f m");
								if (shape == SurfaceLight::Shape::RECTANGLE) ImGui::Text("Height:   ");
								else ImGui::Text("Dia Vert: ");
								ImGui::SameLine();
								lightsRequiereUpdate |= ImGui::DragFloat("##height", &size.y, 0.1f, 0.0001f, std::numeric_limits<float>::max(), "%f m");
							}
							else {
								if (shape == SurfaceLight::Shape::SQUARE) ImGui::Text("Size:     ");
								else if (shape == SurfaceLight::Shape::DISK) ImGui::Text("Diameter: ");
								ImGui::SameLine();
								lightsRequiereUpdate |= ImGui::DragFloat("##size", &size.x, 0.1f, 0.0001f, std::numeric_limits<float>::max(), "%f m");
								size.y = size.x;
							}
							size.z = (size.y + size.x) * 0.5f;
							if (size.x == 0.0f) size.x = 0.0001f;
							if (size.y == 0.0f) size.y = 0.0001f;
							if (size.z == 0.0f) size.z = 0.0001f;
							refLight.model_matrix[0] = glm::normalize(refLight.model_matrix[0]) * size.x;
							refLight.model_matrix[1] = glm::normalize(refLight.model_matrix[1]) * size.y;
							refLight.model_matrix[2] = glm::normalize(refLight.model_matrix[2]) * size.z;
							sl.setDimensions(size);

							bool double_sided = sl.doubleSided();
							ImGui::Text("2 Sided:  ");
							ImGui::SameLine();
							lightsRequiereUpdate |= ImGui::Checkbox("##sldoublesided", &double_sided);
							sl.doubleSided(double_sided);
						}
					}
				}
				if (type == Light::Type::DIRECTIONAL) {
					auto& directionalLight = dynamic_cast<DirectionalLight&>(*refLight.light);
					float angle = directionalLight.getAngle();
					ImGui::Text("Angle:    ");
					ImGui::SameLine();
					lightsRequiereUpdate |= ImGui::DragFloat("##angle", &angle, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%f Deg");
					directionalLight.setAngle(angle);
				}

			}
			else if (selection.reference->type == Ref::Type::Model) {
				modelInstancesRequiereUpdate |= model_mat_requieres_update;
				
				auto& refModel = dynamic_cast<RefModel&>(*selection.reference);
				auto it = refModel.refMeshes.begin();
				ImGui::Text("Mesh:     "); ImGui::SameLine();
				if (ImGui::BeginCombo("##meshcombo", std::to_string(selection.meshOffset).c_str(), ImGuiComboFlags_NoArrowButton)) {
					for (uint32_t i = 0; i < refModel.refMeshes.size(); i++) {
						const bool isSelected = (i == selection.meshOffset);
						if (ImGui::Selectable(std::to_string(i).c_str(), isSelected)) selection.meshOffset = i;
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
				
				std::advance(it, selection.meshOffset);
				const auto& mesh = (*it)->mesh;

				glm::vec4 base_color = mesh->getMaterial()->getBaseColorFactor();
				glm::vec3 emission_color = glm::convertLinearToSRGB(mesh->getMaterial()->getEmissionFactor());
				float emission_strength = mesh->getMaterial()->getEmissionStrength();
				float metallic = mesh->getMaterial()->getMetallicFactor();
				float roughness = mesh->getMaterial()->getRoughnessFactor();
				float transmission = mesh->getMaterial()->getTransmissionFactor();
				float ior = mesh->getMaterial()->getIOR();
				glm::vec3 attenuation_color = mesh->getMaterial()->getAttenuationColor();
				float attenuation_distance = mesh->getMaterial()->getAttenuationDistance();
				float attenuation_anisotropy = mesh->getMaterial()->getAttenuationAnisotropy();
				bool doubleSided = !mesh->getMaterial()->getCullBackface();

				ImGui::Text("Color:    ");
				ImGui::SameLine();
				materialsRequiereUpdate |= ImGui::ColorEdit3("##base_color", &base_color[0]);
				ImGui::PushItemWidth(64);
				ImGui::Text("Metallic: ");
				ImGui::SameLine();
				materialsRequiereUpdate |= ImGui::DragFloat("##metallic", &metallic, 0.005f, 0.0f, 1, "%.3f");
				ImGui::Text("Roughness:");
				ImGui::SameLine();
				materialsRequiereUpdate |= ImGui::DragFloat("##roughness", &roughness, 0.005f, 0.0f, 1, "%.3f");
				ImGui::PopItemWidth();
				ImGui::Text("Sub Color:");
				ImGui::SameLine();
				materialsRequiereUpdate |= ImGui::ColorEdit3("##attenuation_color", &attenuation_color[0]);
				ImGui::PushItemWidth(64);
				ImGui::Text("Sub Dist: "); ImGui::SameLine();
				materialsRequiereUpdate |= ImGui::DragFloat("##attenuation_distance", &attenuation_distance, 0.005f, 0.0f, std::numeric_limits<float>::max(), "%.3f");
				ImGui::Text("Sub Aniso:"); ImGui::SameLine();
				materialsRequiereUpdate |= ImGui::DragFloat("##attenuation_anisotropy", &attenuation_anisotropy, 0.005f, -1.0f, 1.0f, "%.3f");
				ImGui::PopItemWidth();

				ImGui::Text("Emission: ");
				ImGui::SameLine();
				if (ImGui::ColorEdit3("##emission_color", &emission_color[0]))
				{
					materialsRequiereUpdate |= true;
					lightsRequiereUpdate |= true;
				}
				ImGui::PushItemWidth(64);
				ImGui::Text("Emission*:");
				ImGui::SameLine();
				bool emission_changed = ImGui::DragFloat("##emission_strength", &emission_strength, 0.1f, 0.0f, std::numeric_limits<float>::max(), "%.3f");
				materialsRequiereUpdate |= emission_changed;
				lightsRequiereUpdate |= emission_changed;

				ImGui::Text("Trans:    ");
				ImGui::SameLine();
				materialsRequiereUpdate |= ImGui::DragFloat("##transmission", &transmission, 0.005f, 0.0f, 1, "%.3f");
				ImGui::Text("Ior:      ");
				ImGui::SameLine();
				materialsRequiereUpdate |= ImGui::DragFloat("##ior", &ior, 0.005f, 0.0f, 10.0f, "%.3f");
				ImGui::Text("2 Sided:  ");
				ImGui::SameLine();
				if(ImGui::Checkbox("##doublesided", &doubleSided))
				{
					materialsRequiereUpdate |= true;
					lightsRequiereUpdate |= true;
				}
				ImGui::PopItemWidth();

				mesh->getMaterial()->setBaseColorFactor(base_color);
				mesh->getMaterial()->setEmissionFactor(glm::convertSRGBToLinear(emission_color));
				mesh->getMaterial()->setEmissionStrength(emission_strength);
				mesh->getMaterial()->setMetallicFactor(metallic);
				mesh->getMaterial()->setRoughnessFactor(roughness);
				mesh->getMaterial()->setTransmissionFactor(transmission);
				mesh->getMaterial()->setIOR(ior);
				mesh->getMaterial()->setAttenuationColor(attenuation_color);
				mesh->getMaterial()->setAttenuationDistance(attenuation_distance);
				mesh->getMaterial()->setAttenuationAnisotropy(attenuation_anisotropy);
				mesh->getMaterial()->setCullBackface(!doubleSided);
			}
			ImGui::End();
		}
		if(lightsRequiereUpdate) mUc->scene->requestLightUpdate();
		if(materialsRequiereUpdate) mUc->scene->requestMaterialUpdate();
		if(modelInstancesRequiereUpdate) mUc->scene->requestModelInstanceUpdate();
	}
}

void MainGUI::drawDraw() const
{
	if (mUc->draw_info->mDrawMode) {
		bool materialsRequiereUpdate = false;
		bool modelInstancesRequiereUpdate = false;

		constexpr float distance = 10.0f;
		const ImVec2 pos = ImVec2{ mIo->DisplaySize.x - distance, mIo->DisplaySize.y - distance };
		const ImVec2 posPivot = ImVec2{ 1.0f, 1.0f };
		ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);

		constexpr auto flags =
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoSavedSettings;

		if (ImGui::Begin("Draw", nullptr, flags))
		{
			ImGui::Text("Draw Mode");
			ImGui::Text("Target:"); ImGui::SameLine();
			ImGui::PushItemWidth(228);
			const std::vector modes = { "Vertex Color", "Custom" };
			if (ImGui::BeginCombo("##drawcombocolor", modes[static_cast<uint32_t>(mUc->draw_info->mTarget)], ImGuiComboFlags_NoArrowButton)) {
				for (uint32_t i = 0; i < modes.size(); i++) {
					const bool isSelected = (i == static_cast<uint32_t>(mUc->draw_info->mTarget));
					if (ImGui::Selectable(modes[i], isSelected)) mUc->draw_info->mTarget = static_cast<DrawInfo::Target>(i);
					if (isSelected) ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();

			ImGui::Text("Cursor:");
			ImGui::SameLine();
			ImGui::ColorEdit3("##cursor_color_pick", &mUc->draw_info->mCursorColor[0]);

			ImGui::Text("Color0:");
			ImGui::SameLine();
			ImGui::ColorEdit4("##color_draw0_pick", &mUc->draw_info->mColor0[0]);
			ImGui::Text("Color1:");
			ImGui::SameLine();
			ImGui::ColorEdit4("##color_draw1_pick", &mUc->draw_info->mColor1[0]);

			ImGui::Text("Radius ");
			ImGui::SameLine();
			ImGui::DragFloat("##draw_radius", &mUc->draw_info->mRadius, 0.01f, 0.0f, std::numeric_limits<float>::max(), "%f");
			ImGui::Checkbox("Draw RGB", &mUc->draw_info->mDrawRgb);
			ImGui::Checkbox("Draw ALPHA", &mUc->draw_info->mDrawAlpha);
			const bool softBrushOld = mUc->draw_info->mSoftBrush;
			const bool drawAllOld = mUc->draw_info->mDrawAll;
			ImGui::Checkbox("Soft Brush", &mUc->draw_info->mSoftBrush);
			ImGui::Checkbox("Set Whole Mesh", &mUc->draw_info->mDrawAll);
			if (mUc->draw_info->mDrawAll != drawAllOld && mUc->draw_info->mDrawAll) mUc->draw_info->mSoftBrush = false;
			else if (mUc->draw_info->mSoftBrush != softBrushOld && mUc->draw_info->mSoftBrush) mUc->draw_info->mDrawAll = false;
			ImGui::End();
		}
	}
}

void MainGUI::drawRightClickMenu() const
{
	auto& currentCam = mUc->scene->getCurrentCamera();
	if (currentCam.mode != RefCamera::Mode::EDITOR) return;
	if (!mUc->draw_info->mDrawMode && InputSystem::getInstance().wasReleased(Input::MOUSE_RIGHT)) ImGui::OpenPopup(("Popup"), ImGuiPopupFlags_MouseButtonRight);
	if (ImGui::BeginPopup("Popup"))
	{
		if (ImGui::BeginMenu("Add Light"))
		{
			std::unique_ptr<Light> l = nullptr;
			if (ImGui::MenuItem("Point")) l = std::make_unique<PointLight>();
			else if (ImGui::MenuItem("Spot")) {
				l = std::make_unique<SpotLight>();
			}
			else if (ImGui::MenuItem("Directional")) {
				l = std::make_unique<DirectionalLight>();
			}
			else if (ImGui::BeginMenu("Surface"))
			{
				if (ImGui::MenuItem("Square")) {
					auto surfaceLight = std::make_unique<SurfaceLight>();
					surfaceLight->setShape(SurfaceLight::Shape::SQUARE);
					l = std::move(surfaceLight);
				}
				else if (ImGui::MenuItem("Rectangle")) {
					auto surfaceLight = std::make_unique<SurfaceLight>();
					surfaceLight->setShape(SurfaceLight::Shape::RECTANGLE);
					l = std::move(surfaceLight);
				}
				else if (ImGui::MenuItem("Disk")) {
					auto surfaceLight = std::make_unique<SurfaceLight>();
					surfaceLight->setShape(SurfaceLight::Shape::DISK);
					l = std::move(surfaceLight);
				}
				else if (ImGui::MenuItem("Ellipse")) {
					auto surfaceLight = std::make_unique<SurfaceLight>();
					surfaceLight->setShape(SurfaceLight::Shape::ELLIPSE);
					l = std::move(surfaceLight);
				}
				ImGui::EndMenu();
			}
			
			if (l) {
				auto& cam = dynamic_cast<RefCameraPrivate&>(currentCam);
				glm::quat quat = glm::quat{ glm::radians(glm::vec3{-90, 0, 0}) };
				mUc->scene->addLightRef(std::move(l), cam.editor_data.pivot, { quat[1], quat[2], quat[3] , quat[0] });
			}
			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}
}

void MainGUI::drawInfo() const
{
	constexpr float distance = 10.0f;
	const ImVec2 pos = ImVec2{ mIo->DisplaySize.x - distance, mVerticalOffsetMenuBar + distance };
	const ImVec2 posPivot = ImVec2{ 1.0f, 0.0f };
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);
	ImGui::SetNextWindowBgAlpha(0.3f); 

	constexpr auto flags =
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoSavedSettings;

	if (ImGui::Begin("Statistics", nullptr, flags))
	{
		const glm::vec3 cam_pos = mUc->scene->getCurrentCameraPosition();
		const glm::vec3 cam_view_dir = mUc->scene->getCurrentCameraDirection();
		const RenderConfig_s rc = ((RenderSystem*)mUc->system)->getConfig();
		const Window* w = Common::getInstance().getMainWindow()->window();
		const glm::ivec2 ws = w->getSize();
		ImGui::Text("Frame rate: %.1f fps", static_cast<double>(rc.framerate_smooth));
		ImGui::Text("Frame time: %5.2f ms", static_cast<double>(rc.frametime_smooth));
		
		ImGui::Text("Window:  %d x %d", ws.x, ws.y);
		ImGui::Separator();
		ImGui::Text("View position:");
		ImGui::Text("%.1f %.1f %.1f", static_cast<double>(cam_pos[0]), static_cast<double>(cam_pos[1]), static_cast<double>(cam_pos[2]));
		ImGui::Text("View direction:");
		ImGui::Text("%.1f %.1f %.1f", static_cast<double>(cam_view_dir[0]), static_cast<double>(cam_view_dir[1]), static_cast<double>(cam_view_dir[2]));

		ImGui::End();
	}
}

void MainGUI::showSaveScene()
{
	ImGui::SetNextWindowPos(ImVec2(mIo->DisplaySize.x * 0.5f, mIo->DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

	constexpr auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::Begin("Export Scene", &mShowSaveScene, flags))
	{
		const std::vector<std::string> formats = { "glTF" };
		static io::Export::SceneExportSettings exportSettings;
		ImGui::Text("Export as");
		ImGui::SameLine();

		static uint32_t formatSelection = 0;
		if (ImGui::BeginCombo("##exportcombo", formats[0].c_str(), ImGuiComboFlags_NoArrowButton)) {
			for (uint32_t i = 0; i < formats.size(); i++) {
				const bool isSelected = (i == formatSelection);
				if (ImGui::Selectable(formats[i].c_str(), isSelected)) formatSelection = i;
				if (isSelected) ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		if (formats[formatSelection] == "glTF") exportSettings.mFormat = io::Export::SceneExportSettings::Format::glTF;
		if (formatSelection == 0) {
			ImGui::Checkbox("Embed Images", &exportSettings.mEmbedImages);
			ImGui::Checkbox("Embed Buffer", &exportSettings.mEmbedBuffers);
			ImGui::Checkbox("As Binary", &exportSettings.mWriteBinary);
			ImGui::Checkbox("Exclude Lights", &exportSettings.mExcludeLights);
			ImGui::Checkbox("Exclude Models", &exportSettings.mExcludeModels);
			ImGui::Checkbox("Exclude Cameras", &exportSettings.mExcludeCameras);
		}
		if(ImGui::Button("Save"))
		{
			mShowSaveScene = false;
			Common::openFileDialogExportScene(exportSettings.encode());
		}
		if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) mShowSaveScene = false;
		ImGui::End();
	}
}
void MainGUI::showHelp()
{
	constexpr float distance = 10.0f;
	
	const ImVec2 pos = ImVec2(distance, mVerticalOffsetMenuBar + distance);
	const ImVec2 posPivot = ImVec2(0.0f, 0.0f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always, posPivot);

	const ImVec2 maxSize = ImVec2(mIo->DisplaySize.x - distance - distance, mIo->DisplaySize.y - distance - (mVerticalOffsetMenuBar + distance));
	ImGui::SetNextWindowSize(ImVec2(std::max(200.0f, maxSize.x), std::max(200.0f, maxSize.y)));
	

	constexpr auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::Begin("Usage", &mShowHelp, flags))
	{
		
		ImGui::Columns(3, "locations", false);
		ImGui::SetColumnWidth(-1, 170);

		ImGui::Text("Editor Camera");
		ImGui::BulletText("Rotate Camera:");
		ImGui::BulletText("Move Camera:");
		ImGui::BulletText("Zoom:");
		ImGui::BulletText("Select Object:");
		ImGui::BulletText("Unselect Object:");
		ImGui::NewLine();
		ImGui::BulletText("Local Space:");
		ImGui::BulletText("World Space:");
		ImGui::BulletText("Translate:");
		ImGui::BulletText("Rotate:");
		ImGui::BulletText("Scale:");
		ImGui::NewLine();
		ImGui::BulletText("Delete:");
		ImGui::BulletText("Undo:");
		ImGui::NewLine();
		ImGui::BulletText("Draw Mode:");
		ImGui::NewLine();
		ImGui::Text("- Draw Mode Active -");
		ImGui::BulletText("Draw Color 0:");
		ImGui::BulletText("Draw Color 1:");
		ImGui::BulletText("Change Radius:");
		ImGui::BulletText("Exit Draw Mode:");
		ImGui::NewLine();
		
		ImGui::Text("FPS Camera");
		ImGui::BulletText("Enter Camera:");
		ImGui::BulletText("Move:");
		ImGui::BulletText("Change Speed:");
		ImGui::BulletText("Exit Camera:");
		ImGui::NewLine();
		
		ImGui::Text("UI");
		ImGui::BulletText("Slider Input:");
		ImGui::NewLine();
		
		ImGui::Text("Other");
		ImGui::BulletText("Screenshot:");
		ImGui::BulletText("Impl-Screenshot:");
		ImGui::NewLine();

		
		ImGui::NextColumn();
		ImGui::SetColumnWidth(-1, 250);
		
		ImGui::NewLine();
		ImGui::Text("Middle Mouse Button");
		ImGui::Text("Shift + Middle Mouse Button");
		ImGui::Text("Mouse Wheel");
		ImGui::Text("Left Click");
		ImGui::Text("ESC");
		ImGui::NewLine();
		ImGui::Text("L");
		ImGui::Text("W");
		ImGui::Text("T");
		ImGui::Text("R");
		ImGui::Text("S");
		ImGui::NewLine();
		ImGui::Text("X");
		ImGui::Text("CTRL + Z");
		ImGui::NewLine();
		ImGui::Text("D");
		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::Text("Left Click");
		ImGui::Text("Right Click");
		ImGui::Text("CTRL + Scroll Wheel");
		ImGui::Text("ESC");
		ImGui::NewLine();
		
		
		ImGui::NewLine();
		ImGui::Text("Left Click");
		ImGui::Text("WASDQE");
		ImGui::Text("Scroll Wheel");
		ImGui::Text("ESC");
		ImGui::NewLine();
		
		ImGui::NewLine();
		ImGui::Text("CTRL + Left Click");
		ImGui::NewLine();
		
		ImGui::NewLine();
		ImGui::Text("F5");
		ImGui::Text("F6");

		
		ImGui::NextColumn();
		ImGui::Text("This is Tamashii a rendering framework");
		ImGui::Columns();

		
		

		ImGui::End();
	}
}

void MainGUI::showAbout()
{
    
	const ImVec2 size = ImVec2(150, 150);
	const ImVec2 pos = ImVec2(mIo->DisplaySize.x * 0.5f - size.x * 0.5f, mIo->DisplaySize.y * 0.5f - size.y * 0.5f);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(size);

	const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;
	if (ImGui::Begin("About", &mShowAbout, flags))
	{
		ImGui::Text("Tamashii v0.01");
		ImGui::End();
	}
}
