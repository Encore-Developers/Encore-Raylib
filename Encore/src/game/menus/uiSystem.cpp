#include "game/menus/uiSystem.h"

void TabBar::Update() {
	tabs->activeIndex = activeTab;
	tabs->Update();
	activeTab = tabs->activeIndex;
	if(tabs->activeIndex!=-1 && tabs->activeIndex<subUIs.size())
		subUIs[tabs->activeIndex]->Update();
}

void TabBar::Draw() {
	tabs->Draw();
	if (tabs->activeIndex != -1 && tabs->activeIndex < subUIs.size())
		subUIs[tabs->activeIndex]->Draw();
}