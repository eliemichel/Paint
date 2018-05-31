/**
 * Paint Portable
 * Copyright (c) 2018 - Elie Michel
 */

#ifndef H_BASE_UI
#define H_BASE_UI

struct Rect {
	int x, y, w, h;

	Rect() : x(0), y(0), w(0), h(0) {}
	Rect(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}

	bool Contains(int _x, int _y) const {
		return _x >= x && _y >= y && _x < x + w && _y < y + h;
	}
	bool IsNull() const {
		return x == 0 && y == 0 && w == 0 && h == 0;
	}
};

class UiElement {
public:
	// Getters / Setters

	void SetRect(Rect rect) {
		m_rect = rect;
		Update();
	}
	void SetRect(int x, int y, int w, int h) {
		SetRect(::Rect(x, y, w, h));
	}
	const Rect & Rect() const {
		return m_rect;
	}

	void SetSizeHint(::Rect hint) {
		m_sizeHint = hint;
	}
	virtual ::Rect SizeHint() {
		return m_sizeHint;
	}

public:
	virtual bool OnMouseOver(int x, int y) {
		bool hit = Rect().Contains(x, y);
		m_debug = hit;
		return hit;
	}

	virtual void ResetDebug() {
		m_debug = false;
	}

	/// Call when geometry gets updated
	/// MUST NOT call this->SetRect()
	virtual void Update() {
	}

	virtual void OnTick() {
	}

	virtual void Paint(NVGcontext *vg) const {
		PaintDebug(vg);
	}
	virtual void PaintDebug(NVGcontext *vg) const {
		if (m_debug && false) {
			nvgBeginPath(vg);
			nvgRect(vg, Rect().x, Rect().y, Rect().w, Rect().h);
			nvgFillColor(vg, nvgRGBA(255, 0, 0, 64));
			nvgFill(vg);
		}
	}


private:
	::Rect m_rect, m_sizeHint;
	bool m_debug;
};


class UiLayout : public UiElement {
public:
	~UiLayout() {
		while (!m_items.empty()) {
			delete m_items.back();
			m_items.pop_back();
		}
	}

	/// Take ownership of the item
	void AddItem(UiElement *item) {
		m_items.push_back(item);
	}

public:
	void ResetDebug() override {
		UiElement::ResetDebug();
		for (auto item : Items()) {
			item->ResetDebug();
		}
	}

	void OnTick() override {
		UiElement::OnTick();
		for (UiElement *item : Items()) {
			item->OnTick();
		}
	}

	void Paint(NVGcontext *vg) const override {
		UiElement::Paint(vg);
		for (UiElement *item : Items()) {
			item->Paint(vg);
		}
		PaintDebug(vg);
	}

protected:
	std::vector<UiElement*> & Items() { return m_items; }
	const std::vector<UiElement*> & Items() const { return m_items; }

private:
	std::vector<UiElement*> m_items;
};

class GridLayout : public UiLayout {
public:
	GridLayout()
		: m_rowCount(1)
		, m_colCount(1)
		, m_rowSpacing(0)
		, m_colSpacing(0)
	{}

	void SetRowCount(int count) { m_rowCount = std::max(1, count); }
	int RowCount() const { return m_rowCount; }

	void SetColCount(int count) { m_colCount = std::max(1, count); }
	int ColCount() const { return m_colCount; }

	void SetRowSpacing(int spacing) { m_rowSpacing = spacing; }
	int RowSpacing() const { return m_rowSpacing; }

	void SetColSpacing(int spacing) { m_colSpacing = spacing; }
	int ColSpacing() const { return m_colSpacing; }

public:
	bool OnMouseOver(int x, int y) override {
		bool hit = UiElement::OnMouseOver(x, y);
		if (hit) {
			int relativeX = x - Rect().x;
			int relativeY = y - Rect().y;

			int itemWidth = (Rect().w - ColSpacing() * (ColCount() - 1)) / ColCount() + ColSpacing();
			int itemHeight = (Rect().h - RowSpacing() * (RowCount() - 1)) / RowCount() + RowSpacing();

			size_t colIndex = std::min((int)(floor(relativeX / itemWidth)), ColCount() - 1);
			size_t rowIndex = std::min((int)(floor(relativeY / itemHeight)), RowCount() - 1);

			bool isInColSpacing = (colIndex + 1) * itemWidth - relativeX <= ColSpacing();
			bool isInRowSpacing = (rowIndex + 1) * itemHeight - relativeY <= RowSpacing();

			if (!isInRowSpacing && !isInColSpacing) {
				size_t i = rowIndex * ColCount() + colIndex;
				if (i < Items().size()) {
					Items()[i]->OnMouseOver(x, y);
				}
			}
		}
		return hit;
	}

	void Update() override {
		int itemWidth = (Rect().w - ColSpacing() * (ColCount() - 1)) / ColCount() + ColSpacing();
		int itemHeight = (Rect().h - RowSpacing() * (RowCount() - 1)) / RowCount() + RowSpacing();
		// Prevent rounding issues
		int lastItemWidth = Rect().w - (ColCount() - 1) * itemWidth;
		int lastItemHeight = Rect().h - (RowCount() - 1) * itemHeight;

		for (size_t i = 0 ; i < Items().size() ; ++i) {
			size_t colIndex = i % ColCount();
			size_t rowIndex = i / ColCount();
			Items()[i]->SetRect(
				Rect().x + colIndex * itemWidth,
				Rect().y + rowIndex * itemHeight,
				colIndex == (ColCount() - 1) ? lastItemWidth : (itemWidth - ColSpacing()),
				rowIndex == (RowCount() - 1) ? lastItemHeight : (itemHeight - RowSpacing())
			);
		}
	}

private:
	int m_rowCount, m_colCount;
	int m_rowSpacing, m_colSpacing;
};


#define BUI_HBOX_IMPLEMENTATION
#include "_BoxLayout.inc.h"
#undef BUI_HBOX_IMPLEMENTATION
#include "_BoxLayout.inc.h"

#endif // H_BASE_UI