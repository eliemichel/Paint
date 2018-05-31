/**
* Paint Portable
* Copyright (c) 2018 - Elie Michel
*/

/*
 * Do not include this if you simply use the BaseUi toolkit.
 * This is a hacky way of defining both VBox and HBox layouts with not too much code duplication.
 * Define BUI_HBOX_IMPLEMENTATION to define the HBoxLayout and undef it for VBoxLayout
 */

#undef _BoxLayout
#ifdef BUI_HBOX_IMPLEMENTATION
#define _BoxLayout HBoxLayout
#else
#define _BoxLayout VBoxLayout
#endif

class _BoxLayout : public UiElement {
public:
	~_BoxLayout() {
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
	bool OnMouseOver(int x, int y) override {
		bool hit = UiElement::OnMouseOver(x, y);
		if (hit) {
			/* for regular items
			int itemHeight = Rect().h / m_items.size();
			size_t i = std::min((size_t)(floor((y - Rect().y) / itemHeight)), m_items.size() - 1);
			m_items[i]->QueryAt(x, y);
			*/

			int offset = 0;
			for (auto item : m_items) {
#ifdef BUI_HBOX_IMPLEMENTATION
				offset += item->Rect().w;
				if (Rect().x + offset > x) {
#else
				offset += item->Rect().h;
				if (Rect().y + offset > y) {
#endif
					item->OnMouseOver(x, y);
					break;
				}
			}
		}
		return hit;
	}

	void ResetDebug() override {
		UiElement::ResetDebug();
		for (auto item : m_items) {
			item->ResetDebug();
		}
	}

	void Update() override {
		/* for regular items
		int itemHeight = floor(Rect().h / m_items.size());
		// Prevent rounding issues
		int lastItemHeight = Rect().h - (m_items.size() - 1) * itemHeight;

		for (size_t i = 0 ; i < m_items.size() ; ++i) {
		m_items[i]->SetRect(Rect().x, Rect().y + i * itemHeight, Rect().w, i == (m_items.size() - 1) ? lastItemHeight : itemHeight);
		}
		//*/

		int sumVHints = 0;
		int nullHints = 0;
		for (auto item : m_items) {
			const ::Rect & rect = item->SizeHint();
#ifdef BUI_HBOX_IMPLEMENTATION
			sumVHints += rect.w;
#else
			sumVHints += rect.h;
#endif
			if (rect.IsNull()) {
				nullHints++;
			}
		}
		int nonNullHints = m_items.size() - nullHints;
#ifdef BUI_HBOX_IMPLEMENTATION
		int remainingHeight = Rect().w - sumVHints;
#else
		int remainingHeight = Rect().h - sumVHints;
#endif
		int itemHeight = nullHints == 0 ? 0 : std::max(0, (int)floor(remainingHeight / nullHints));
		int lastItemHeight = std::max(0, remainingHeight) - (nullHints - 1) * itemHeight;
		int hintedItemHeightDelta = nonNullHints == 0 ? 0 : std::min((int)floor(remainingHeight / nonNullHints), 0);
		// The last height might be a bit different to prevent rounding issues
		int lastHintedItemHeightDelta = -std::max(0, -remainingHeight) + (nonNullHints - 1) * hintedItemHeightDelta;

		int offset = 0;
		int nullCount = 0;
		int nonNullCount = 0;
		for (auto item : m_items) {
			int height = 0;
			const ::Rect & hint = item->SizeHint();
			if (hint.IsNull()) {
				height = nullCount == nullHints - 1 ? lastItemHeight : itemHeight;
				nullCount++;
			}
			else {
#ifdef BUI_HBOX_IMPLEMENTATION
				height = hint.w;
#else
				height = hint.h;
#endif
				height += (nonNullCount == nonNullHints - 1 ? lastHintedItemHeightDelta : hintedItemHeightDelta);
				nonNullCount++;
			}
#ifdef BUI_HBOX_IMPLEMENTATION
			item->SetRect(Rect().x + offset, Rect().y, height, Rect().h);
#else
			item->SetRect(Rect().x, Rect().y + offset, Rect().w, height);
#endif
			item->Update();
			offset += height;
		}
	}

	void OnTick() override {
		UiElement::OnTick();
		for (UiElement *item : m_items) {
			item->OnTick();
		}
	}

	void Paint(NVGcontext *vg) const override {
		UiElement::Paint(vg);
		for (UiElement *item : m_items) {
			item->Paint(vg);
		}
		PaintDebug(vg);
	}

private:
	std::vector<UiElement*> m_items;
};
