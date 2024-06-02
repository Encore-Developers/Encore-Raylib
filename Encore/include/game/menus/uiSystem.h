#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include "raylib.h"

class UI;
class Element {
public:
	bool hovered = false;
	std::string elementID = "";
	bool disabled = false;
	virtual ~Element() = default;
	void SetID(std::string id) {
		elementID = id;
	}
	virtual void Draw() = 0;
	virtual void Update() = 0;
	bool isHovered() const {
		return hovered;
	}
};

class Button : public Element {
private:
	bool clicked = false;
public:
	std::string text = "";
	Rectangle bounds{ 0,0,0,0 };
	Button(std::string id, Rectangle rect, std::string buttonText) {
		SetID(id);
		bounds = rect;
		text = buttonText;
	}
	void Draw() override {
		DrawRectangle(bounds.x, bounds.y, bounds.width, bounds.height, hovered ? BLUE : GRAY);
		DrawRectangle(bounds.x + 5, bounds.y + 5, bounds.width - 10, bounds.height - 10, clicked ? BLACK : ColorAlpha(WHITE, 0));
		DrawText(text.c_str(), bounds.x + ((bounds.width - MeasureText(text.c_str(), 25)) / 2.0), bounds.y + (bounds.height / 2.0) - 12.5, 25, WHITE);
	}
	void Update() override {
		hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
		clicked = hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
	}
	bool isClicked() const {
		return clicked;
	}
	bool isHovered() const {
		return hovered;
	}
};

class SliderBar : public Element {
private:
	Button* addButton = new Button("", { 0,0,0,0 }, ">");
	Button* subButton = new Button("", { 0,0,0,0 }, "<");
	Rectangle sliderRect{ 0,0,0,0 };
	Rectangle labelRect{ 0,0,0,0 };
public:
	std::string text = "";
	float sliderValue = 0;
	float increment = 0.5;
	float minVal = -10;
	float maxVal = 10;
	SliderBar(std::string id, Rectangle labelBounds, Rectangle sliderBounds, std::string labelText, float defaultValue = 0, float incrementBy = 0.5, float min = -10, float max = 10) {
		SetID(id);
		labelRect = labelBounds;
		subButton->bounds = { sliderBounds.x,sliderBounds.y,sliderBounds.height,sliderBounds.height };
		sliderRect = { sliderBounds.x + sliderBounds.height, sliderBounds.y, sliderBounds.width - (sliderBounds.height * 2),sliderBounds.height };
		addButton->bounds = { sliderBounds.x+sliderBounds.width-sliderBounds.height,sliderBounds.y,sliderBounds.height,sliderBounds.height};
		text = labelText;
		sliderValue = 0;
		increment = incrementBy;
		minVal = min;
		maxVal = max;
	}
	void Draw() override {
		addButton->Draw();
		subButton->Draw();
		DrawRectangle(sliderRect.x, sliderRect.y, sliderRect.width, sliderRect.height, GRAY);
		DrawRectangle(sliderRect.x, sliderRect.y, sliderRect.width*((sliderValue-minVal)/(maxVal-minVal)), sliderRect.height, hovered?BLUE:LIGHTGRAY);
		
	}
	void Update() override {
		addButton->Update();
		if (addButton->isClicked() && sliderValue<maxVal) {
			sliderValue += increment;
		}
		subButton->Update();
		if (subButton->isClicked() && sliderValue > minVal) {
			sliderValue -= increment;
		}
	}
};

class Toggle : public Element {
public:
	bool state = false;
	std::string text = "";
	Rectangle bounds{ 0,0,0,0 };
	Toggle(std::string id, Rectangle rect, std::string buttonText) {
		SetID(id);
		bounds = rect;
		text = buttonText;
	}
	void Draw() override {
		DrawRectangle(bounds.x, bounds.y, bounds.width, bounds.height, hovered ? BLUE : GRAY);
		DrawRectangle(bounds.x + 5, bounds.y + 5, bounds.width - 10, bounds.height - 10, state ? BLACK : ColorAlpha(WHITE, 0));
		DrawText(text.c_str(), bounds.x + ((bounds.width - MeasureText(text.c_str(), 25)) / 2.0), bounds.y + (bounds.height / 2.0) - 12.5, 25, WHITE);
	}
	void Update() override {
		hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
		if (hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
			state = !state;
		}
	}
	bool getState() const {
		return state;
	}
};

class ToggleGroup : public Element {
private:
	std::vector<Toggle*> toggles;
public:
	int activeIndex = -1;
	ToggleGroup(const std::string& id) {
		SetID(id);
	}

	void AddToggle(Toggle* toggle) {
		toggles.push_back(toggle);
	}

	void Draw() override {
		for (auto& toggle : toggles) {
			toggle->Draw();
		}
	}

	void Update() override {
		for (int i = 0; i < toggles.size(); ++i) {
			toggles[i]->Update();
			if (toggles[i]->getState()) {
				if (i != activeIndex) {
					if (activeIndex != -1) {
						toggles[activeIndex]->state = false; // Deselect the previously active toggle
					}
					activeIndex = i;
				}
			}
		}
		if (activeIndex != -1) {
			toggles[activeIndex]->state = true;
		}
	}
	~ToggleGroup() override {
		for (auto& t : toggles) {
			delete t;
		}
	}
};

class TabBar : public Element {
private:
	ToggleGroup* tabs = new ToggleGroup("");
	std::vector<UI*> subUIs;
public:
	int activeTab = -1;
	TabBar(const std::string& id) {
		SetID(id);
		tabs->SetID(id + "_tabs");
	}
	~TabBar() override {
		for (auto subUI : subUIs) {
			delete subUI;
		}
		delete tabs;
	}
	void AddTab(std::string tabID, Rectangle rect, std::string buttonText, UI* subUI) {
		tabs->AddToggle(new Toggle(tabID, rect, buttonText));
		subUIs.push_back(subUI);
	}
	void Update() override;
	void Draw() override;
};

class Row {
public:
	std::string defaultElementID;
	std::vector<Element*> elements;
	void Update() {
		for (auto element : elements) {
			element->Update();
		}
	}
	void Draw() {
		for (auto element : elements) {
			element->Draw();
		}
	}
	~Row() {
		for (auto element : elements) {
			delete element;
		}
	}
};


class UI {
private:
	std::map < std::string, std::pair<int, int>> elementMap;
public:
	std::vector<Row*> rows;
	void AddElement(int row, Element* element)
	{
		elementMap[element->elementID] = { row,rows[row]->elements.size() };
		rows[row]->elements.push_back(element);
	}
	Element* GetElement(std::string id) {
		auto it = elementMap.find(id);
		if (it == elementMap.end()) {
			throw std::invalid_argument("Element ID not found: " + id);
		}
		auto [rowIndex, elementIndex] = it->second;
		return rows[rowIndex]->elements[elementIndex];
	}
	Button* GetButton(std::string id) {
		return dynamic_cast<Button*>(GetElement(id));
	}
	Toggle* GetToggle(std::string id) {
		return dynamic_cast<Toggle*>(GetElement(id));
	}
	ToggleGroup* GetToggleGroup(std::string id) {
		return dynamic_cast<ToggleGroup*>(GetElement(id));
	}

	SliderBar* GetSliderBar(std::string id) {
		return dynamic_cast<SliderBar*>(GetElement(id));
	}
	TabBar* GetTabBar(std::string id) {
		return dynamic_cast<TabBar*>(GetElement(id));
	}

	void AddRow() {
		rows.push_back(new Row);
	}
	void Update() {
		for (auto& row : rows) {
			row->Update();
		}
	}void Draw() {
		for (auto& row : rows) {
			row->Draw();
		}
	}
	~UI() {
		for (auto row : rows) {
			delete row;
		}
	}
};