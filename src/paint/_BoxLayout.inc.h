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

class _BoxLayout : public UiLayout {
public:
	
	/// Take ownership of the item
	void AddItem(UiElement *item) {
		Items().push_back(item);
	}

public:
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
		for (auto item : Items()) {
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
		int nonNullHints = Items().size() - nullHints;
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
		for (auto item : Items()) {
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

protected:
	bool GetIndexAt(size_t & idx, int x, int y) override {
		/* for regular items
		int itemHeight = Rect().h / m_items.size();
		size_t i = std::min((size_t)(floor((y - Rect().y) / itemHeight)), m_items.size() - 1);
		m_items[i]->QueryAt(x, y);
		*/

		int offset = 0;
		for (size_t i = 0; i < Items().size(); ++i) {
#ifdef BUI_HBOX_IMPLEMENTATION
			offset += Items()[i]->Rect().w;
			if (Rect().x + offset > x) {
#else
			offset += Items()[i]->Rect().h;
			if (Rect().y + offset > y) {
#endif
				idx = i;
				return true;
			}
		}

		return false;
	}
};
