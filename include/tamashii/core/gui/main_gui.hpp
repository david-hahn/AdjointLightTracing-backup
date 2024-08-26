#pragma once
#include <tamashii/public.hpp>
#include <tamashii/core/forward.h>
#include <deque>

T_BEGIN_NAMESPACE
class MainGUI {
public:
												MainGUI();
	void										draw(UiConf_s* aUiConf);

private:
	void										markLights();
	void										markCamera();
	void										showDraw();
	void										drawImGuizmo();
	void										drawGizmo() const;

	void										drawMenuBar();
	void										drawConsole();
	void										drawMenu();
	void										drawEdit();
	void										drawDraw() const;

	void										drawRightClickMenu() const;

	void										drawInfo() const;
	void										showSaveScene();
	void										showHelp();
	void										showAbout();

	ImGuiIO										*mIo;
	UiConf_s									*mUc;
	const float									mVerticalOffsetMenuBar;

	
	std::string									mOp;	
	std::string									mMode;	
	std::deque<std::pair<std::shared_ptr<Ref>, glm::mat4>>	mHistory;

	bool										mShowHelp;
	bool										mShowAbout;
	bool										mShowSaveScene;
};
T_END_NAMESPACE