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

	virtual Rect SizeHint() {
		return m_sizeHint;
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

	/// DEBUG
	void SetSizeHint(::Rect hint) {
		m_sizeHint = hint;
	}


private:
	::Rect m_rect, m_sizeHint;
	bool m_debug;
};

#define BUI_HBOX_IMPLEMENTATION
#include "_BoxLayout.inc.h"
#undef BUI_HBOX_IMPLEMENTATION
#include "_BoxLayout.inc.h"

#endif // H_BASE_UI