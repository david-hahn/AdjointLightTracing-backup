#include <tamashii/core/scene/render_cmd_system.hpp>
#include <tamashii/core/scene/light.hpp>
#include <tamashii/core/scene/model.hpp>
#include <tamashii/core/scene/image.hpp>
#include <tamashii/core/scene/material.hpp>

T_USE_NAMESPACE
RenderCmdSystem tamashii::renderCmdSystem;

void RenderCmdSystem::addBeginFrameCmd()
{
	RCmd_s cmd{};
	cmd.mType = RenderCommand::BEGIN_FRAME;
	addCmd(cmd);
}

void RenderCmdSystem::addEndFrameCmd()
{
	RCmd_s cmd{};
	cmd.mType = RenderCommand::END_FRAME;
	addCmd(cmd);
	mFrames.fetch_add(1);
}

void RenderCmdSystem::addDrawSurfCmd(ViewDef_s* aViewDef)
{
	RCmd_s cmd{};
	cmd.mType = RenderCommand::DRAW_VIEW;
	cmd.mValue.emplace<ViewDef_s*>(aViewDef);
	addCmd(cmd);
}

void RenderCmdSystem::addDrawUICmd(UiConf_s* aUiConf)
{
	RCmd_s cmd{};
	cmd.mType = RenderCommand::DRAW_UI;
	cmd.mValue.emplace<UiConf_s*>(aUiConf);
	addCmd(cmd);
}

void RenderCmdSystem::addEntityAddedCmd(std::shared_ptr<Ref> aRef)
{
	RCmd_s cmd{ RenderCommand::ENTITY_ADDED, std::move(aRef) };
	addCmd(cmd);
}

void RenderCmdSystem::addEntityRemovedCmd(std::shared_ptr<Ref> aRef)
{
	RCmd_s cmd{ RenderCommand::ENTITY_REMOVED, std::move(aRef) };
	addCmd(cmd);
}

void RenderCmdSystem::addAssetAddedCmd(std::shared_ptr<Asset> aAsset)
{
	const RCmd_s cmd{ RenderCommand::ASSET_ADDED, std::move(aAsset) };
	addCmd(cmd);
}

void RenderCmdSystem::addAssetRemovedCmd(std::shared_ptr<Asset> aAsset)
{
	const RCmd_s cmd{ RenderCommand::ASSET_REMOVED, std::move(aAsset) };
	addCmd(cmd);
}

void RenderCmdSystem::addScreenshotCmd(std::string_view aFileName)
{
	RCmd_s cmd{};
	cmd.mType = RenderCommand::SCREENSHOT;
	cmd.mValue.emplace<ScreenshotInfo_s*>(new ScreenshotInfo_s{ std::string(aFileName), 0, 0, 0, {} });
	addCmd(cmd);
}

void RenderCmdSystem::addImplScreenshotCmd()
{
	RCmd_s cmd{};
	cmd.mType = RenderCommand::IMPL_SCREENSHOT;
	addCmd(cmd);
}

void RenderCmdSystem::addDrawOnMeshCmd(const DrawInfo* aDrawInfo, const Intersection* aHitInfo)
{
	RCmd_s cmd{};
	cmd.mType = RenderCommand::IMPL_DRAW_ON_MESH;
	cmd.mValue.emplace<DrawInfo*>(new DrawInfo( *aDrawInfo ));
	addCmd(cmd);
}

bool RenderCmdSystem::nextCmd()
{
	const std::lock_guard lock(mMutex);
	return !mCmdList.empty();
}

RCmd_s RenderCmdSystem::popNextCmd()
{
	const std::lock_guard lock(mMutex);
	if (!mCmdList.empty()) {
		RCmd_s cmd = std::move(mCmdList.front());
		mCmdList.pop_front();
		return cmd;
	}
	return RCmd_s{ RenderCommand::EMPTY, {} };
}

void RenderCmdSystem::deleteCmd(const RCmd_s& aCmd)
{
	switch (aCmd.mType) {
		case RenderCommand::DRAW_VIEW:
			delete std::get<ViewDef_s*>(aCmd.mValue);
			break;
		case RenderCommand::DRAW_UI:
			delete std::get<UiConf_s*>(aCmd.mValue);
			break;
		case RenderCommand::ENTITY_ADDED:
			break;
		case RenderCommand::ENTITY_REMOVED:
			
			break;
		case RenderCommand::ASSET_ADDED:
			break;
		case RenderCommand::ASSET_REMOVED:
			
			break;
		case RenderCommand::SCREENSHOT:
			delete std::get<ScreenshotInfo_s*>(aCmd.mValue);
			break;
		case RenderCommand::IMPL_SCREENSHOT: break;
		case RenderCommand::IMPL_DRAW_ON_MESH:
			delete std::get<DrawInfo*>(aCmd.mValue);
			break;
		case RenderCommand::BEGIN_FRAME: break;
		case RenderCommand::END_FRAME:
			mFrames.fetch_sub(1);
			break;
		case RenderCommand::EMPTY: break;
	}
}

uint32_t RenderCmdSystem::frames() const
{
	return mFrames.load();
}

void RenderCmdSystem::addCmd(const RCmd_s& aCmd)
{
	const std::lock_guard lock(mMutex);
	mCmdList.push_back(aCmd);
}
