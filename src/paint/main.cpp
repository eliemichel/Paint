/**
 * Paint Portable
 * Copyright (c) 2018 - Elie Michel
 */
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <nanovg.h>
#define NANOVG_GLES3_IMPLEMENTATION
#include <nanovg_gl.h>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include "BaseUi.h"

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* glfwWindow, int button, int action, int mods);
void window_size_callback(GLFWwindow* glfwWindow, int width, int height);

// Window dimensions
const GLuint WIDTH = 1200, HEIGHT = 600;

const std::string shareDir = "E:\\SourceCode\\Paint\\share\\";

/**
 * Image to be painted on GUI
 */
class Image {
public:
	/**
	 * You must take care of Delete()-ing or destroying the Image object *before* the NVG context is deleted.
	 */
	Image(struct NVGcontext* vg, const std::string & filename)
		: m_img(-1)
		, m_vg(vg)
	{
		Load(vg, filename);
	}

	Image()
		: m_img(-1)
		, m_vg(NULL)
	{}

	~Image() {
		Delete();
	}

	void Load(struct NVGcontext* vg, const std::string & filename) {
		if (NULL == m_vg) {
			m_vg = vg;
		}
		m_img = nvgCreateImage(m_vg, (shareDir + filename).c_str(), 0);
		nvgImageSize(m_vg, m_img, &m_width, &m_height);
	}

	void Create(struct NVGcontext* vg, int w, int h) {
		if (NULL == m_vg) {
			m_vg = vg;
		}
		unsigned char* data = new unsigned char[w * h * 4];
		for (size_t i = 0; i < w * h * 4; ++i) {
			data[i] = 255;
		}
		m_img = nvgCreateImageRGBA(vg, w, h, NVG_IMAGE_NEAREST, data);
		delete[] data;
		m_width = w;
		m_height = h;
	}

	void Delete() {
		if (m_img > -1) {
			nvgDeleteImage(m_vg, m_img);
			m_img = -1;
		}
	}

	void Resize(int w, int h) {
		if (m_img == -1) {
			return;
		}

		// Get old image data through a framebuffer
		GLuint tex = nvglImageHandleGLES3(m_vg, m_img);
		unsigned char* oldData = new unsigned char[m_width * m_height * 4];
		
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glBindTexture(GL_TEXTURE_2D, tex);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

		glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, oldData);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteFramebuffers(1, &fbo);

		// Copy from old to new data
		unsigned char* data = new unsigned char[w * h * 4];
		for (size_t j = 0; j < h; ++j) {
			for (size_t i = 0; i < w; ++i) {
				if (i < m_width && j < m_height) {
					for (size_t k = 0; k < 4; ++k) {
						data[(j * w + i) * 4 + k] = oldData[(j * m_width + i) * 4 + k];
					}
				}
				else {
					for (size_t k = 0; k < 4; ++k) {
						data[(j * w + i) * 4 + k] = 255;
					}
				}
			}
		}

		delete[] oldData;
		
		nvgDeleteImage(m_vg, m_img);
		m_img = nvgCreateImageRGBA(m_vg, w, h, NVG_IMAGE_NEAREST, data);
		delete[] data;
		m_width = w;
		m_height = h;
	}

	void Paint(float x, float y, float w = -1, float h = -1) const {
		if (NULL != m_vg && m_img > -1) {
			nvgBeginPath(m_vg);
			nvgRect(m_vg, x, y, w >= 0 ? w : m_width, h >= 0 ? h : m_height);
			nvgFillPaint(m_vg, nvgImagePattern(m_vg, x, y, m_width, m_height, 0, m_img, 1.0f));
			nvgFill(m_vg);
		}
	}

	int Handle() const { return m_img; }

	int Width() const { return m_width; }
	int Height() const { return m_height; }

private:
	int m_img; /// Image ID
	struct NVGcontext* m_vg; /// Parent context
	int m_width, m_height;
};

/**
 * Document properties
 */
class Document {
public:
	void CreateImage(NVGcontext* vg, int w, int h) {
		m_img.Create(vg, w, h);
	}

	const Image & Img() const { return m_img; }

	int Width() const { return m_width; }
	int Height() const { return m_height; }
	void SetSize(int w, int h) {
		m_width = w;
		m_height = h;
		m_img.Resize(w, h);
	}

private:
	int m_width, m_height;
	Image m_img;
};

/**
 * Global editor state
 */
enum Tool {
	PencilTool,
	FillTool,
	TextTool,
	EraseTool,
	PickTool,
	ZoomTool,
	BrushTool,
	SelectTool,
};
enum ColorRole {
	ForegroundColor,
	BackgroundColor,
};
struct Editor {
	float zoom = 1.0f;
	NVGcolor foregroundColor = nvgRGB(0, 0, 0);
	NVGcolor backgroundColor = nvgRGB(255, 255, 255);
	Tool currentTool = BrushTool;
	ColorRole currentColor = ForegroundColor;

	// This is supposed to be a global editing state, not a place for pointers, but as for now this is the less dirty I can do
	UiLayout *popupLayout = NULL;
};

// TODO: get rid of this global (might require some kind of signals or passing a pointer to this global state to all the widgets)
Editor *ed;

// Custom UI elements

/// Add IsMouseOver() to UiMouseAwareElement
class UiTrackMouseElement : public UiMouseAwareElement {
public:
	UiTrackMouseElement()
		: UiMouseAwareElement()
		, m_isMouseOver(false)
	{}

public: // protected
	void OnMouseEnter() override {
		m_isMouseOver = true;
	}

	void OnMouseLeave() override {
		m_isMouseOver = false;
	}

protected:
	bool IsMouseOver() const { return m_isMouseOver; }

private:
	bool m_isMouseOver;
};

/// Blue-bordered buttons
class UiDefaultButton : public UiTrackMouseElement {
protected:
	/// To be overridden
	virtual bool IsCurrent() const { return false; }

public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();
		bool isCurrent = IsCurrent();
		bool isMouseOver = IsMouseOver();

		if (isCurrent || isMouseOver) {
			// Background
			nvgBeginPath(vg);
			nvgRect(vg, r.x, r.y, r.w, r.h);
			nvgFillColor(vg, isCurrent ? (isMouseOver ? nvgRGB(213, 230, 247) : nvgRGB(201, 224, 247)) : nvgRGB(232, 239, 247));
			nvgFill(vg);
			// Main Border
			nvgBeginPath(vg);
			nvgRect(vg, r.x + 0.5, r.y + 0.5, r.w - 1, r.h - 1);
			nvgStrokeColor(vg, isCurrent ? (isMouseOver ? nvgRGB(122, 176, 231) : nvgRGB(98, 162, 228)) : nvgRGB(164, 206, 249));
			nvgStroke(vg);
		}
	}
};

class UiTabButton : public UiMouseAwareElement {
public:
	const NVGcolor & BackgroundColor() const { return m_backgroundColor; }
	void SetBackgroundColor(const NVGcolor & color) { m_backgroundColor = color; }
	void SetBackgroundColor(unsigned char r, unsigned char g, unsigned char b) { m_backgroundColor = nvgRGB(r, g, b); }

	const NVGcolor & TextColor() const { return m_textColor; }
	void SetTextColor(const NVGcolor & color) { m_textColor = color; }
	void SetTextColor(unsigned char r, unsigned char g, unsigned char b) { m_textColor = nvgRGB(r, g, b); }

	const NVGcolor & BorderColor() const { return m_borderColor; }
	void SetBorderColor(const NVGcolor & color) { m_borderColor = color; }
	void SetBorderColor(unsigned char r, unsigned char g, unsigned char b) { m_borderColor = nvgRGB(r, g, b); }

	const std::string & Label() const { return m_label; }
	void SetLabel(const std::string & label) { m_label = label; }

public:
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();
		nvgBeginPath(vg);
		nvgRect(vg, r.x, r.y, r.w, r.h);
		nvgFillColor(vg, BackgroundColor());
		nvgFill(vg);

		nvgFontSize(vg, 15);
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, TextColor());
		nvgText(vg, r.x + r.w / 2, r.y + 16, Label().c_str(), NULL);
	}

private:
	NVGcolor m_backgroundColor;
	NVGcolor m_textColor;
	NVGcolor m_borderColor;
	std::string m_label;
};

class FileButton : public UiTabButton {
public:
	FileButton()
		: m_isFadingOut(false)
		, m_fadingDuration(1.0)
	{
		SetSizeHint(0, 0, 56, 0);
		SetBackgroundColor(25, 121, 202);
		SetTextColor(255, 255, 255);
		SetBorderColor(218, 219, 220);
		SetLabel("Fichier");
	}

public: // protected
	void OnTick() override {
		UiTabButton::OnTick();

		if (m_isFadingOut) {
			float t = (glfwGetTime() - m_fadingStartTime) / m_fadingDuration;
			if (t > 1.0f) {
				m_isFadingOut = false;
				t = 1.0f;
			}
			SetBackgroundColor(nvgLerpRGBA(nvgRGB(41, 140, 225), nvgRGB(25, 121, 202), pow(t, 0.5)));
		}
	}

	void Paint(NVGcontext *vg) const override {
		UiTabButton::Paint(vg);

		const ::Rect & r = InnerRect();
		nvgBeginPath(vg);
		nvgMoveTo(vg, r.x, r.y + r.h - 0.5);
		nvgLineTo(vg, r.x + r.w, r.y + r.h - 0.5);
		nvgStrokeColor(vg, BorderColor());
		nvgStroke(vg);
	}

	void OnMouseEnter() override {
		m_isFadingOut = false;
		SetBackgroundColor(41, 140, 225);
	}

	void OnMouseLeave() override {
		m_isFadingOut = true;
		m_fadingStartTime = glfwGetTime();
	}

private:
	bool m_isFadingOut;
	float m_fadingStartTime; // in seconds
	const float m_fadingDuration; // in seconds
};

class HomeButton : public UiTabButton {
public:
	HomeButton() {
		SetSizeHint(::Rect(0, 0, 64, 0));
		SetBackgroundColor(245, 246, 247);
		SetTextColor(60, 60, 60);
		SetBorderColor(218, 219, 220);
		SetLabel("Accueil");
	}

public:
	void Paint(NVGcontext *vg) const override {
		UiTabButton::Paint(vg);

		const ::Rect & r = InnerRect();
		nvgBeginPath(vg);
		nvgMoveTo(vg, r.x + 0.5, r.y + r.h);
		nvgLineTo(vg, r.x + 0.5, r.y + 0.5);
		nvgLineTo(vg, r.x + r.w - 0.5, r.y + 0.5);
		nvgLineTo(vg, r.x + r.w - 0.5, r.y + r.h);
		nvgStrokeColor(vg, BorderColor());
		nvgStroke(vg);
	}
};

class ViewButton : public UiTabButton {
public:
	ViewButton()
		: m_isFadingOut(false)
		, m_fadingDuration(1.0)
	{
		SetSizeHint(0, 0, 77, 0);
		SetBackgroundColor(253, 253, 255);
		SetTextColor(60, 60, 60);
		SetBorderColor(253, 253, 255);
		SetLabel("Affichage");
	}

public: // protected
	void OnTick() override {
		UiTabButton::OnTick();

		if (m_isFadingOut) {
			float t = (glfwGetTime() - m_fadingStartTime) / m_fadingDuration;
			if (t > 1.0f) {
				m_isFadingOut = false;
				t = 1.0f;
			}
			SetBorderColor(nvgLerpRGBA(nvgRGB(235, 236, 236), nvgRGB(253, 253, 255), pow(t, 0.5)));
		}
	}

	void Paint(NVGcontext *vg) const override {
		UiTabButton::Paint(vg);

		const ::Rect & r = InnerRect();
		nvgBeginPath(vg);
		nvgMoveTo(vg, r.x + 0.5, r.y + r.h + 1);
		nvgLineTo(vg, r.x + 0.5, r.y + 0.5);
		nvgLineTo(vg, r.x + r.w - 0.5, r.y + 0.5);
		nvgLineTo(vg, r.x + r.w - 0.5, r.y + r.h + 1);
		nvgStrokeColor(vg, BorderColor());
		nvgStroke(vg);

		// Bottom border
		nvgBeginPath(vg);
		nvgMoveTo(vg, r.x, r.y + r.h - 0.5);
		nvgLineTo(vg, r.x + r.w, r.y + r.h - 0.5);
		nvgStrokeColor(vg, nvgRGB(218, 219, 220));
		nvgStroke(vg);
	}

	void OnMouseEnter() override {
		m_isFadingOut = false;
		SetBorderColor(235, 236, 236);
	}

	void OnMouseLeave() override {
		m_isFadingOut = true;
		m_fadingStartTime = glfwGetTime();
	}

private:
	bool m_isFadingOut;
	float m_fadingStartTime; // in seconds
	const float m_fadingDuration; // in seconds
};

class ColorButton : public UiTrackMouseElement {
public:
	ColorButton()
		: m_isEnabled(true)
	{}

	const NVGcolor & Color() const { return m_color; }
	void SetColor(const NVGcolor & color) { m_color = color; }
	void SetColor(unsigned char r, unsigned char g, unsigned char b) { m_color = nvgRGB(r, g, b); }

	bool IsEnabled() const { return m_isEnabled; }
	void SetEnabled(bool enabled) { m_isEnabled = enabled; }

public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();
		
		// White border
		if (m_isEnabled) {
			nvgBeginPath(vg);
			nvgRect(vg, r.x, r.y, r.w, r.h);
			nvgFillColor(vg, IsMouseOver() ? nvgRGB(203, 228, 253) : nvgRGB(255, 255, 255));
			nvgFill(vg);
		}

		// Main Border
		nvgBeginPath(vg);
		nvgRect(vg, r.x + 0.5, r.y + 0.5, r.w - 1, r.h - 1);
		nvgStrokeColor(vg, IsMouseOver() && m_isEnabled ? nvgRGB(100, 165, 231) : nvgRGB(160, 160, 160));
		nvgStroke(vg);

		// Color
		if (m_isEnabled) {
			nvgBeginPath(vg);
			nvgRect(vg, r.x + 2, r.y + 2, r.w - 4, r.h - 4);
			nvgFillColor(vg, Color());
			nvgFill(vg);
		}
	}

	void OnMouseClick(int button, int action, int mods) override {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			if (ed->currentColor == ForegroundColor) {
				ed->foregroundColor = Color();
			} else {
				ed->backgroundColor = Color();
			}
		}
	}

private:
	NVGcolor m_color;
	bool m_isEnabled;
};

class ToolButton : public UiDefaultButton {
public:
	~ToolButton() {
		DeleteImage();
	}

	const Tool & TargetTool() const { return m_targetTool; }
	void SetTargetTool(const Tool & tool) { m_targetTool = tool; }

	void LoadImage(NVGcontext *vg, const std::string & path) {
		m_image.Load(vg, path);
	}
	// Call this or destroy object before the NVG context gets freed
	void DeleteImage() {
		m_image.Delete();
	}

protected:
	bool IsCurrent() const override {
		return ed->currentTool == TargetTool();
	}

public: // protected
	void Paint(NVGcontext *vg) const override {
		UiDefaultButton::Paint(vg);
		const ::Rect & r = InnerRect();
		m_image.Paint(r.x + 1, r.y);
	}

	void OnMouseClick(int button, int action, int mods) override {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			ed->currentTool = TargetTool();
		}
	}

private:
	Tool m_targetTool;
	Image m_image;
};

class MenuBar : public HBoxLayout {
public:
	MenuBar() {
		SetSizeHint(0, 0, 0, 24);
	}

public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();

		nvgBeginPath(vg);
		nvgRect(vg, r.x, r.y, r.w, r.h);
		nvgFillColor(vg, nvgRGB(253, 253, 255));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgMoveTo(vg, r.x, r.y + r.h - 0.5);
		nvgLineTo(vg, r.x + r.w, r.y + r.h - 0.5);
		nvgStrokeColor(vg, nvgRGB(218, 219, 220));
		nvgStroke(vg);

		HBoxLayout::Paint(vg);
	}
};

class StatusBar : public HBoxLayout {
public:
	StatusBar() {
		SetSizeHint(0, 0, 0, 25);
	}

	~StatusBar() {
		DeleteImages();
	}

	void LoadImages(NVGcontext *vg) {
		m_cursorImg.Load(vg, "images\\cursor18.png");
		m_selectionImg.Load(vg, "images\\selection18.png");
		m_sizeImg.Load(vg, "images\\size18.png");
		m_savedImg.Load(vg, "images\\saved18.png");
		m_zoomOutImg.Load(vg, "images\\zoomOut18.png");
		m_zoomInImg.Load(vg, "images\\zoomIn18.png");
	}

	// Call this or destroy object before the NVG context gets freed
	void DeleteImages() {
		m_cursorImg.Delete();
		m_selectionImg.Delete();
		m_sizeImg.Delete();
		m_savedImg.Delete();
		m_zoomOutImg.Delete();
		m_zoomInImg.Delete();
	}

public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();

		nvgBeginPath(vg);
		nvgRect(vg, r.x, r.y, r.w, r.h);
		nvgFillColor(vg, nvgRGB(240, 240, 240));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgMoveTo(vg, r.x, r.y - 0.5);
		nvgLineTo(vg, r.x + r.w, r.y - 0.5);
		nvgStrokeColor(vg, nvgRGB(218, 219, 220));
		nvgStroke(vg);

		float sb_delim_pos[] = { 155, 311, 467, 623, r.w - 199, r.w - 1 };
		for (int i = 0; i < 6; ++i) {
			nvgBeginPath(vg);
			nvgMoveTo(vg, r.x + sb_delim_pos[i] + 0.5, r.y + 1);
			nvgLineTo(vg, r.x + sb_delim_pos[i] + 0.5, r.y + r.h - 1);
			nvgStrokeColor(vg, nvgRGB(226, 227, 228));
			nvgStroke(vg);
		}

		m_cursorImg.Paint(r.x + 1, r.y + 3);
		m_selectionImg.Paint(r.x + 159, r.y + 3);
		m_sizeImg.Paint(r.x + 315, r.y + 3);
		m_savedImg.Paint(r.x + 471, r.y + 3);
		m_zoomOutImg.Paint(r.x + r.w - 143, r.y + 4);
		m_zoomInImg.Paint(r.x + r.w - 21, r.y + 4);

		HBoxLayout::Paint(vg);
	}

private:
	Image m_cursorImg, m_selectionImg, m_sizeImg, m_savedImg, m_zoomOutImg, m_zoomInImg;
};

/**
 * May not be a good idea to mix UI and paint stroke engine
 */
class DrawingArea : public UiMouseAwareElement {
public:
	DrawingArea()
		: UiMouseAwareElement()
		, m_doc(NULL)
		, m_init(false)
		, m_lastMouseX(0)
		, m_lastMouseY(0)
		, m_isStroking(false)
	{}

	~DrawingArea() {
		if (m_init) {
			glDeleteFramebuffers(1, &m_frameBuffer);
			glDeleteRenderbuffers(1, &m_stencilBuffer);
		}
	}

	// TODO: change signature
	struct NVGcontext* StrokeEngine() { return m_vg; }
	void SetStrokeEngine(struct NVGcontext* vg) { m_vg = vg; }

	Document * Document() const { return m_doc; }
	void SetDocument(::Document *doc) { m_doc = doc; }

public: // protected
	void OnMouseOver(int x, int y) override {
		if (m_isStroking) {
			Stroke(m_lastMouseX, m_lastMouseY, x, y);
		}

		m_lastMouseX = x;
		m_lastMouseY = y;
	}

	void OnMouseClick(int button, int action, int mods) override {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			m_isStroking = true;
		}
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
			m_isStroking = false;
		}
	}

	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();
		nvgBeginPath(vg);
		nvgRect(vg, r.x, r.y, r.w, r.h);
		nvgFillColor(vg, nvgRGB(255, 255, 255));
		nvgFill(vg);

		if (NULL != Document()) {
			Document()->Img().Paint(r.x, r.y, r.w, r.h);
		}
	}

	void Update() override {
		UpdateStrokeEngine();
	}

private:
	void InitStrokeEngine() {
		if (!Document()) {
			return;
		}
		// Stencil buffer
		UpdateStrokeEngine();
		m_init = true;
	}

	void UpdateStrokeEngine() {
		glGenFramebuffers(1, &m_frameBuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

		GLuint tex = nvglImageHandleGLES3(m_vg, Document()->Img().Handle());
		glBindTexture(GL_TEXTURE_2D, tex);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

		if (m_init) {
			glDeleteRenderbuffers(1, &m_stencilBuffer);
		}
		glGenRenderbuffers(1, &m_stencilBuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, m_stencilBuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, Document()->Width(), Document()->Height());
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_stencilBuffer);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void Stroke(float startX, float startY, float endX, float endY) {
		if (!m_init) {
			InitStrokeEngine();
		}

		const ::Rect & r = InnerRect();
		int fbWidth = 1024, fbHeight = 1024;

		glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

		glViewport(0, 0, fbWidth, fbHeight);

		// Render
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.8f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		nvgBeginFrame(m_vg, fbWidth, fbHeight, 1.0f);
		nvgSave(m_vg);
		nvgTranslate(m_vg, 0, 1024);
		nvgScale(m_vg, 1, -1);

		nvgBeginPath(m_vg);
		nvgMoveTo(m_vg, startX - r.x, startY - r.y);
		nvgLineTo(m_vg, endX - r.x, endY - r.y);
		nvgStrokeColor(m_vg, ed->foregroundColor);
		nvgStroke(m_vg);

		nvgRestore(m_vg);
		nvgEndFrame(m_vg);

		// restore framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

private:
	::Document *m_doc;
	GLuint m_frameBuffer;
	GLuint m_stencilBuffer;
	bool m_init;
	struct NVGcontext *m_vg;
	float m_lastMouseX, m_lastMouseY;
	bool m_isStroking;
};

class DocumentArea : public UiMouseAwareElement {
public:
	DocumentArea()
		: UiMouseAwareElement()
		, m_doc(NULL)
		, m_drawingArea(NULL)
		, m_isResizingWidth(false)
		, m_isResizingHeight(false)
		, m_mouseX(0)
		, m_mouseY(0)
	{
		m_drawingArea = new DrawingArea();
	}

	~DocumentArea() {
		if (NULL != m_drawingArea) {
			delete m_drawingArea;
		}
	}

	Document * Document() const { return m_doc; }
	void SetDocument(::Document *doc) {
		m_doc = doc;
		if (NULL != m_drawingArea) {
			m_drawingArea->SetDocument(doc);
		}
	}

	struct NVGcontext* StrokeEngine() { return m_drawingArea == NULL ? NULL : m_drawingArea->StrokeEngine(); }
	void SetStrokeEngine(struct NVGcontext* vg) {
		if (NULL != m_drawingArea) {
			m_drawingArea->SetStrokeEngine(vg);
		}
	}

public: // protected
	void OnMouseOver(int x, int y) override {
		m_mouseX = x;
		m_mouseY = y;

		if (m_isResizingWidth) {
			m_rubberBand.w = m_startDeltaX + m_mouseX;
		}
		if (m_isResizingHeight) {
			m_rubberBand.h = m_startDeltaY + m_mouseY;
		}
		if (!(m_isResizingWidth || m_isResizingHeight)) {
			if (m_drawingArea->Rect().Contains(x, y)) {
				m_drawingArea->OnMouseOver(x, y);
			}
		}
	}

	void OnMouseClick(int button, int action, int mods) override {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			float drawingWidth = Document()->Width() * ed->zoom;
			float drawingHeight = Document()->Height() * ed->zoom;

			if (m_widthHandle.Contains(m_mouseX, m_mouseY)) {
				m_isResizingWidth = true;
				m_startDeltaX = drawingWidth - m_mouseX;
			}

			if (m_heightHandle.Contains(m_mouseX, m_mouseY)) {
				m_isResizingHeight = true;
				m_startDeltaY = drawingHeight - m_mouseY;
			}

			if (m_bothHandle.Contains(m_mouseX, m_mouseY)) {
				m_isResizingWidth = true;
				m_isResizingHeight = true;
				m_startDeltaX = drawingWidth - m_mouseX;
				m_startDeltaY = drawingHeight - m_mouseY;
			}

			if (m_isResizingWidth || m_isResizingHeight) {
				if (NULL != m_drawingArea) {
					m_rubberBand = m_drawingArea->Rect();
				}
			}
		}

		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
			if (m_isResizingWidth || m_isResizingHeight) {
				Document()->SetSize(m_rubberBand.w / ed->zoom, m_rubberBand.h / ed->zoom);

				Update();
			}

			m_isResizingWidth = false;
			m_isResizingHeight = false;
		}

		if (m_drawingArea->Rect().Contains(m_mouseX, m_mouseY)) {
			m_drawingArea->OnMouseClick(button, action, mods);
		}
	}

	void OnMouseEnter() override {
		if (m_drawingArea->Rect().Contains(m_mouseX, m_mouseY)) {
			m_drawingArea->OnMouseEnter();
		}
	}

	void OnMouseLeave() override {
		if (m_drawingArea->Rect().Contains(m_mouseX, m_mouseY)) {
			m_drawingArea->OnMouseLeave();
		}
	}

	void Update() override {
		const ::Rect & r = Rect();
		float drawingWidth = Document()->Width() * ed->zoom;
		float drawingHeight = Document()->Height() * ed->zoom;

		m_widthHandle = ::Rect(r.x + 5 + drawingWidth, r.y + 5 + floor((drawingHeight - 5) / 2.), 5, 5);
		m_heightHandle = ::Rect(r.x + 5 + floor((drawingWidth - 5) / 2.), r.y + 5 + drawingHeight, 5, 5);
		m_bothHandle = ::Rect(r.x + 5 + drawingWidth, r.y + 5 + drawingHeight, 5, 5);

		m_drawingArea->SetRect(r.x + 5, r.y + 5, drawingWidth, drawingHeight);
	}

	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = Rect();
		float drawingWidth = Document()->Width() * ed->zoom;
		float drawingHeight = Document()->Height() * ed->zoom;

		nvgScissor(vg, r.x, r.y, r.w, r.h);

		nvgBeginPath(vg);
		nvgRect(vg, r.x, r.y, r.w, r.h);
		nvgFillColor(vg, nvgRGB(200, 209, 225));
		//nvgFillColor(vg, nvgRGB(225, 159, 150));
		nvgFill(vg);

		// Shadow
		nvgBeginPath(vg);
		nvgRect(vg, r.x + 5 + 10, r.y + 5 + 10, drawingWidth, drawingHeight);
		nvgFillPaint(vg, nvgBoxGradient(vg, r.x + 5, r.y + 5, drawingWidth + 4.5, drawingHeight + 4.5,
			-5, 9, nvgRGBA(51, 96, 131, 30), nvgRGBA(0, 0, 0, 0)));
		nvgFill(vg);

		if (NULL != m_drawingArea) {
			m_drawingArea->Paint(vg);
		}

		// Handles
		for (const ::Rect & handle : { m_widthHandle , m_heightHandle , m_bothHandle }) {
			nvgBeginPath(vg);
			nvgRect(vg, handle.x, handle.y, handle.w, handle.h);
			nvgFillColor(vg, nvgRGB(255, 255, 255));
			nvgFill(vg);
			nvgBeginPath(vg);
			nvgRect(vg, handle.x + 0.5, handle.y + 0.5, handle.w - 1, handle.h - 1);
			nvgStrokeColor(vg, nvgRGB(85, 85, 85));
			nvgStroke(vg);
		}

		if (m_isResizingWidth || m_isResizingHeight) {
			nvgBeginPath(vg);
			nvgRect(vg, m_rubberBand.x + 0.5, m_rubberBand.y + 0.5, m_rubberBand.w - 1, m_rubberBand.h - 1);
			nvgStrokeColor(vg, nvgRGBA(0, 0, 0, 128));
			//nvgStrokePaint(vg, NVGpaint paint);
			nvgStroke(vg);
		}

		nvgResetScissor(vg);
	}

private:
	::Document *m_doc;
	DrawingArea *m_drawingArea;
	bool m_isResizingWidth, m_isResizingHeight;
	int m_mouseX, m_mouseY;
	float m_startDeltaX, m_startDeltaY;
	::Rect m_widthHandle, m_heightHandle, m_bothHandle;
	::Rect m_rubberBand;
};

class Shelf : public HBoxLayout {
public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();
		HBoxLayout::Paint(vg);
		nvgBeginPath(vg);
		nvgMoveTo(vg, r.x, r.y + r.h - 0.5);
		nvgLineTo(vg, r.x + r.w, r.y + r.h - 0.5);
		nvgStrokeColor(vg, nvgRGB(218, 219, 220));
		nvgStroke(vg);
	}
};

class ShelfSection : public VBoxLayout {
public:
	ShelfSection() : VBoxLayout() {
		UiElement *content = new UiElement();
		AddItem(content);

		m_label = new Label();
		m_label->SetColor(90, 90, 90);
		AddItem(m_label);
	}

	void SetContent(UiElement *content) {
		RemoveItem();
		UiElement *previousContent = RemoveItem();
		if (NULL != previousContent) {
			delete previousContent;
		}
		AddItem(content);
		AddItem(m_label);
	}

	void SetLabelText(const std::string & text) { m_label->SetText(text); }

private:
	Label * m_label;
};

/// Vertical lines between shelf sections
class ShelfSeparator : public UiElement {
public:
	ShelfSeparator() : UiElement() {
		SetSizeHint(0, 0, 1, 0);
	}

public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();
		nvgBeginPath(vg);
		nvgMoveTo(vg, r.x + 0.5, r.y + 2);
		nvgLineTo(vg, r.x + 0.5, r.y + r.h - 4);
		nvgStrokeColor(vg, nvgRGB(226, 227, 228));
		nvgStroke(vg);
	}
};
/// This is a mouse aware vbox layout with frame on hover
class DoubleShelfButtonLayout : public VBoxLayout {
public:
	DoubleShelfButtonLayout() : m_isMouseOver(false) {}

public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();

		// Frame
		if (IsMouseOver()) {
			nvgBeginPath(vg);
			nvgRect(vg, r.x + 0.5, r.y + 0.5, r.w - 1, r.h - 1);
			nvgStrokeColor(vg, nvgRGB(147, 190, 234));
			nvgStroke(vg);
		}

		VBoxLayout::Paint(vg);

		// Separators
		if (IsMouseOver()) {
			const std::vector<UiElement*> & items = Items();
			for (int i = 0; i < items.size() - 1; ++i) {
				nvgBeginPath(vg);
				nvgRect(vg, r.x + 0.5, r.y + items[i]->Rect().h - 0.5, r.w - 1, 0);
				nvgStrokeColor(vg, nvgRGB(147, 190, 234));
				nvgStroke(vg);
			}
		}
	}

	void OnMouseOver(int x, int y) override {
		VBoxLayout::OnMouseOver(x, y);
		if (InnerRect().Contains(x, y)) {
			m_isMouseOver = true;
		}
	}

	void ResetMouse() override {
		VBoxLayout::ResetMouse();
		if (m_isMouseOver && !m_wasMouseOver) {
			OnMouseEnter();
		}
		if (!m_isMouseOver && m_wasMouseOver) {
			OnMouseLeave();
		}

		m_wasMouseOver = m_isMouseOver;
		m_isMouseOver = false;
	}

	virtual void OnMouseEnter() {
	}

	virtual void OnMouseLeave() {
	}

	bool IsMouseOver() const { return m_isMouseOver; }

private:
	bool m_isMouseOver, m_wasMouseOver;
};

class ColorShelfButton : public UiDefaultButton {
public:
	ColorShelfButton()
		: m_colorRole(ForegroundColor)
	{}

	void SetColorRole(ColorRole colorRole) { m_colorRole = colorRole; }
	ColorRole ColorRole() const { return m_colorRole; }

	void SetText(const std::string & text) { m_text = text; }
	const std::string & Text() const { return m_text; }

protected:
	bool IsCurrent() const override {
		return ed->currentColor == ColorRole();
	}

public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();
		bool isForegroundColor = ColorRole() == ForegroundColor;

		UiDefaultButton::Paint(vg);

		// Color Thumb
		float hsize = isForegroundColor ? 16.0 : 12.0; // half square size
		float cx = r.x + r.w / 2;
		float cy = r.y + 19;
		// // Border
		nvgBeginPath(vg);
		nvgRect(vg, cx - hsize + 0.5, cy - hsize + 0.5, hsize * 2 - 1, hsize * 2 - 1);
		nvgStrokeColor(vg, nvgRGB(128, 128, 128));
		nvgStroke(vg);
		// // Color
		nvgBeginPath(vg);
		nvgRect(vg, cx - hsize + 2, cy - hsize + 2, hsize * 2 - 4, hsize * 2 - 4);
		nvgFillColor(vg, isForegroundColor ? ed->foregroundColor : ed->backgroundColor);
		nvgFill(vg);

		// Label
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgTextLineHeight(vg, 13.0f / 15.0f);
		nvgFillColor(vg, nvgRGB(60, 60, 60));
		nvgTextBox(vg, r.x + 2, r.y + r.h - 6 - 11, r.w - 4, Text().c_str(), NULL);
	}

	void OnMouseClick(int button, int action, int mods) override {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			ed->currentColor = ColorRole();
		}
	}

private:
	::ColorRole m_colorRole;
	std::string m_text;
};

// Default button with an image on it
class ImageButton : public UiDefaultButton {
public:
	~ImageButton() {
		DeleteImage();
	}

	void LoadImage(NVGcontext *vg, const std::string & path) {
		m_image.Load(vg, path);
	}
	// Call this or destroy object before the NVG context gets freed
	void DeleteImage() {
		m_image.Delete();
	}

protected:
	virtual void PaintImage(NVGcontext *vg) const {
		const ::Rect & r = InnerRect();
		m_image.Paint(r.x + (r.w - m_image.Width()) / 2, r.y + 3);
	}

	const Image & ImageLabel() const { return m_image; };

public: // protected
	void Paint(NVGcontext *vg) const override {
		UiDefaultButton::Paint(vg);
		PaintImage(vg);
	}

private:
	Image m_image;
};

// Default button with a label on it
class TextButton : public UiDefaultButton {
public:
	void SetText(const std::string & text) { m_text = text; }
	const std::string & Text() const { return m_text; }

protected:
	void PaintLabel(NVGcontext *vg) const {
		const ::Rect & r = InnerRect();
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgTextLineHeight(vg, 13.0f / 15.0f);
		nvgFillColor(vg, nvgRGB(60, 60, 60));
		nvgTextBox(vg, r.x + 2, r.y + r.h - 6 - 11, r.w - 4, Text().c_str(), NULL);
	}

public: // protected
	void Paint(NVGcontext *vg) const override {
		UiDefaultButton::Paint(vg);
		PaintLabel(vg);
	}

private:
	std::string m_text;
};

// Default button with both a label and an image
// (Do not do multiple inheritage, it stinks)
class TextImageButton : public ImageButton {
public:
	void SetText(const std::string & text) { m_text = text; }
	const std::string & Text() const { return m_text; }

protected:
	void PaintLabel(NVGcontext *vg) const {
		const ::Rect & r = InnerRect();
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgTextLineHeight(vg, 13.0f / 15.0f);
		nvgFillColor(vg, nvgRGB(60, 60, 60));
		nvgTextBox(vg, r.x + 2, r.y + r.h - 6 - 11, r.w - 4, Text().c_str(), NULL);
	}

public: // protected
	void Paint(NVGcontext *vg) const override {
		UiDefaultButton::Paint(vg);
		const ::Rect & r = UiDefaultButton::InnerRect();
		PaintImage(vg);
		PaintLabel(vg);
	}

private:
	std::string m_text;
};

// TODO: avoid loading several times the arrow image
class ArrowTextButton : public TextImageButton {
protected:
	virtual void PaintImage(NVGcontext *vg) const {
		const ::Rect & r = InnerRect();
		ImageLabel().Paint(r.x + (r.w - ImageLabel().Width()) / 2, r.y + r.h - 3 - ImageLabel().Height());
	}
};

class Popup : public UiElement {
public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = InnerRect();
		nvgBeginPath(vg);
		nvgRect(vg, r.x, r.y, r.w, r.h);
		nvgFillColor(vg, nvgRGB(251, 252, 253));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, r.x + 1.5, r.y + 1.5, r.w - 3, r.h - 3);
		nvgStrokeColor(vg, nvgRGB(254, 254, 255));
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgRect(vg, r.x + 0.5, r.y + 0.5, r.w - 1, r.h - 1);
		nvgStrokeColor(vg, nvgRGB(220, 221, 222));
		nvgStroke(vg);
	}
};

// TODO: remove
class SizeShelfButton : public TextImageButton {
public:
	~SizeShelfButton() {
		DeleteArrowImage();
	}

	void LoadArrowImage(NVGcontext *vg, const std::string & path) {
		m_arrowImage.Load(vg, path);
	}
	void LoadImages(NVGcontext *vg, const std::string & path, const std::string & arrowPath) {
		LoadImage(vg, path);
		LoadArrowImage(vg, arrowPath);
	}
	// Call this or destroy object before the NVG context gets freed
	void DeleteArrowImage() {
		m_arrowImage.Delete();
	}

public: // protected
	void Paint(NVGcontext *vg) const override {
		TextImageButton::Paint(vg);
		const ::Rect & r = InnerRect();
		m_arrowImage.Paint(r.x + (r.w - m_arrowImage.Width()) / 2, r.y + r.h - 3 - m_arrowImage.Height());
	}

	void OnMouseClick(int button, int action, int mods) override {
		if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
			// Pop up
			// avoid poping it multiple times
			while (ed->popupLayout->ItemCount() > 1) {
				delete ed->popupLayout->RemoveItem();
			}
			const ::Rect & r = InnerRect();
			Popup *popup = new Popup();
			popup->SetRect(r.x, r.y + r.h, 132, 166);
			ed->popupLayout->AddItem(popup);
		}
	}

private:
	Image m_arrowImage;
};

class UiWindow {
public:
	UiWindow()
		: m_isValid(false)
		, m_content(NULL)
	{
		std::cout << "Starting GLFW context, OpenGL ES 3.0" << std::endl;
		// Init GLFW
		glfwInit();
		// Set all the required options for GLFW
		glfwWindowHint(GLFW_OPENGL_API, GLFW_OPENGL_ES_API);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
		//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

		// Create a GLFWwindow object that we can use for GLFW's functions
		m_window = glfwCreateWindow(WIDTH, HEIGHT, "Paint", NULL, NULL);
		glfwMakeContextCurrent(m_window);
		if (m_window == NULL)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			m_isValid = false;
			return;
		}

		// Set the required callback functions
		glfwSetWindowUserPointer(m_window, static_cast<void*>(this));
		glfwSetKeyCallback(m_window, key_callback);
		glfwSetCursorPosCallback(m_window, cursor_pos_callback);
		glfwSetMouseButtonCallback(m_window, mouse_button_callback);
		glfwSetWindowSizeCallback(m_window, window_size_callback);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize OpenGL context" << std::endl;
			m_isValid = false;
			return;
		}

		// Init NanoVG
		glfwMakeContextCurrent(m_window);
		std::cout << "Starting NanoVG" << std::endl;
		m_vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG); // TODO: try w/o NVG_STENCIL_STROKES
		if (NULL == m_vg) {
			std::cout << "Failed to initialize NanoVG context" << std::endl;
			glfwTerminate();
			m_isValid = false;
			return;
		}

		m_isValid = true;
	}

	~UiWindow() {
		glfwSetWindowUserPointer(m_window, NULL);

		// Must be before nvgDelete
		if (NULL != m_content) {
			delete m_content;
		}

		// Destroy NanoVG ctxw
		nvgDeleteGLES3(m_vg);

		// Terminates GLFW, clearing any resources allocated by GLFW.
		glfwTerminate();
	}

	struct NVGcontext* DrawingContext() { return m_vg; }

	bool ShouldClose() {
		return glfwWindowShouldClose(m_window);
	}

	void BeginRender() const {
		int winWidth, winHeight;
		int fbWidth, fbHeight;
		float pxRatio;

		glfwGetWindowSize(m_window, &m_width, &m_height);
		glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
		// Calculate pixel ration for hi-dpi devices.
		pxRatio = (float)fbWidth / (float)m_width;

		glViewport(0, 0, fbWidth, fbHeight);

		// Render
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		nvgBeginFrame(m_vg, m_width, m_height, pxRatio);
	}

	void EndRender() const {
		// UI Objects
		Content()->OnTick();
		Content()->Paint(m_vg);

		nvgEndFrame(m_vg);

		// Swap the screen buffers
		glfwSwapBuffers(m_window);
	}

	void Render() const {
		BeginRender();
		EndRender();
	}

	int Width() const { return m_width; }
	int Height() const { return m_height; }

	UiElement *Content() const { return m_content; }
	void SetContent(UiElement *element) { m_content = element; }

private:
	bool m_isValid;
	GLFWwindow* m_window;
	struct NVGcontext* m_vg;
	UiElement *m_content;
	mutable int m_width, m_height;
};

// The MAIN function, from here we start the application and run the game loop
int main()
{
	UiWindow window;
	struct NVGcontext* vg = window.DrawingContext();

	// Document
	//Editor *ed = new Editor(); // made global
	ed = new Editor();
	Document *doc = new Document();
	doc->CreateImage(vg, 1, 1);
	doc->SetSize(254, 280);
	

	PopupLayout *popupLayout = new PopupLayout();
	window.SetContent(popupLayout);

	VBoxLayout *layout = new VBoxLayout();

	// Menu bar
	MenuBar *top = new MenuBar();
	FileButton *fileButton = new FileButton();
	top->AddItem(fileButton);
	HomeButton *homeButton = new HomeButton();
	top->AddItem(homeButton);
	UiElement *buttonSpacer = new UiElement();
	buttonSpacer->SetSizeHint(0, 0, 1, 0);
	top->AddItem(buttonSpacer);
	ViewButton *viewButton = new ViewButton();
	top->AddItem(viewButton);
	UiElement *rightButtonSpacer = new UiElement();
	top->AddItem(rightButtonSpacer);
	layout->AddItem(top);

	// Shelf
	Shelf *shelf = new Shelf();
	shelf->SetOverflowBehavior(HideOverflow);

	ShelfSection *clipboardShelf = new ShelfSection();
	clipboardShelf->SetLabelText("Presse-papiers");

	DoubleShelfButtonLayout *clipboardButtons = new DoubleShelfButtonLayout();
	clipboardButtons->SetMargin(6, 4, 70, 0);
	// Top
	ImageButton *topClipboardButton = new ImageButton();
	topClipboardButton->SetInnerSizeHint(0, 0, 42, 38);
	topClipboardButton->LoadImage(vg, "images\\pasteOff32.png");
	clipboardButtons->AddItem(topClipboardButton);
	// Bottom
	ArrowTextButton *bottomClipboardButton = new ArrowTextButton();
	bottomClipboardButton->SetMargin(0, -1, 0, 0); // merge border with previous button
	bottomClipboardButton->SetInnerSizeHint(0, 0, 42, 29);
	bottomClipboardButton->LoadImage(vg, "images\\arrow8.png");
	bottomClipboardButton->SetText("Coller");
	clipboardButtons->AddItem(bottomClipboardButton);
	clipboardButtons->AutoSizeHint();

	clipboardShelf->SetContent(clipboardButtons);
	clipboardShelf->AutoSizeHint();
	shelf->AddItem(clipboardShelf);

	shelf->AddItem(new ShelfSeparator());

	ShelfSection *imageShelf = new ShelfSection();
	imageShelf->SetLabelText("Image");

	DoubleShelfButtonLayout *selectButtons = new DoubleShelfButtonLayout();
	selectButtons->SetMargin(4, 4, 6 + 111, 0);
	// Top
	ImageButton *topSelectButton = new ImageButton();
	topSelectButton->SetInnerSizeHint(0, 0, 68, 38);
	topSelectButton->LoadImage(vg, "images\\select32.png");
	selectButtons->AddItem(topSelectButton);
	// Bottom
	ArrowTextButton *bottomSelectButton = new ArrowTextButton();
	bottomSelectButton->SetMargin(0, -1, 0, 0); // merge border with previous button
	bottomSelectButton->SetInnerSizeHint(0, 0, 68, 29);
	bottomSelectButton->LoadImage(vg, "images\\arrow8.png");
	bottomSelectButton->SetText(u8"Sélectionner");
	selectButtons->AddItem(bottomSelectButton);
	selectButtons->AutoSizeHint();
	imageShelf->SetContent(selectButtons);
	imageShelf->AutoSizeHint();

	shelf->AddItem(imageShelf);

	shelf->AddItem(new ShelfSeparator());

	// Tools shelf
	ShelfSection *toolsShelf = new ShelfSection();
	toolsShelf->SetLabelText("Outils");
	toolsShelf->SetMargin(0, 12, 0, 0);

	GridLayout *toolsGrid = new GridLayout();
	toolsGrid->SetMargin(4, 0, 6, 0);
	toolsGrid->SetInnerSizeHint(0, 0, 69, 51);
	toolsGrid->SetRowCount(2);
	toolsGrid->SetColCount(3);
	toolsGrid->SetRowSpacing(7);
	toolsGrid->SetColSpacing(0);

	const Tool tools[] = {PencilTool, FillTool, TextTool, EraseTool, PickTool, ZoomTool};
	const std::string filenames[] = {
		"images\\pencil21.png",
		"images\\fill21.png",
		"images\\text21.png",
		"images\\erase21.png",
		"images\\picker21.png",
		"images\\zoom21.png",
	};
	for (size_t i = 0; i < 6; ++i) {
		ToolButton *toolButton = new ToolButton();
		toolButton->LoadImage(vg, filenames[i]); 
		toolButton->SetTargetTool(tools[i]);
		toolsGrid->AddItem(toolButton);
	}
	
	toolsShelf->SetContent(toolsGrid);
	toolsShelf->AutoSizeHint();

	shelf->AddItem(toolsShelf);

	shelf->AddItem(new ShelfSeparator());

	ShelfSection *brushesShelf = new ShelfSection();
	DoubleShelfButtonLayout *brushesButtons = new DoubleShelfButtonLayout();
	brushesButtons->SetMargin(4, 4, 6, 0);
	// Top
	ImageButton *topBrushesButton = new ImageButton();
	topBrushesButton->SetInnerSizeHint(0, 0, 50, 38);
	topBrushesButton->LoadImage(vg, "images\\brush32.png");
	brushesButtons->AddItem(topBrushesButton);
	// Bottom
	ArrowTextButton *bottomBrushesButton = new ArrowTextButton();
	bottomBrushesButton->SetMargin(0, -1, 0, 0); // merge border with previous button
	bottomBrushesButton->SetInnerSizeHint(0, 0, 50, 29);
	bottomBrushesButton->LoadImage(vg, "images\\arrow8.png");
	bottomBrushesButton->SetText("Pinceaux");
	brushesButtons->AddItem(bottomBrushesButton);
	brushesButtons->AutoSizeHint();
	brushesShelf->SetContent(brushesButtons);
	brushesShelf->AutoSizeHint();

	shelf->AddItem(brushesShelf);

	shelf->AddItem(new ShelfSeparator());

	ShelfSection *shapeShelf = new ShelfSection();
	shapeShelf->SetLabelText("Formes");
	shapeShelf->SetSizeHint(0, 0, 270, 0);
	shelf->AddItem(shapeShelf);

	shelf->AddItem(new ShelfSeparator());

	ShelfSection *sizeShelf = new ShelfSection();
	
	SizeShelfButton *sizeButton = new SizeShelfButton();
	sizeButton->SetMargin(4, 4, 6, 0);
	sizeButton->SetInnerSizeHint(0, 0, 42, 66);
	sizeButton->LoadImages(vg, "images\\stroke32.png", "images\\arrow8.png");
	sizeButton->SetText("Taille");
	sizeShelf->SetContent(sizeButton);
	sizeShelf->AutoSizeHint();

	shelf->AddItem(sizeShelf);

	shelf->AddItem(new ShelfSeparator());

	// Color shelf
	ShelfSection *colorShelf = new ShelfSection();
	colorShelf->SetLabelText("Couleurs");
	colorShelf->SetMargin(0, 4, 0, 0);

	GridLayout *colorGrid = new GridLayout();
	colorGrid->SetMargin(1, 1, 1, 0);
	colorGrid->SetInnerSizeHint(0, 0, 218, 64);
	colorGrid->SetRowCount(3);
	colorGrid->SetColCount(10);
	colorGrid->SetRowSpacing(2);
	colorGrid->SetColSpacing(2);

	float color[][3] = {
		{ 0, 0, 0 },{ 127, 127, 127 },{ 136, 0, 21 },{ 237, 28, 36 },{ 255, 127, 39 },{ 255, 242, 0 },{ 34, 177, 76 },{ 0, 162, 232 },{ 63, 72, 204 },{ 163, 73, 164 },
		{ 255, 255, 255 },{ 195, 195, 195 },{ 185, 122, 87 },{ 255, 174, 201 },{ 255, 201, 14 },{ 239, 228, 176 },{ 181, 230, 29 },{ 153, 217, 234 },{ 112, 146, 190 },{ 200, 191, 231 },
		{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 }
	};
	for (int i = 0; i < 30; ++i) {
		ColorButton *colorButton = new ColorButton();
		colorButton->SetColor(color[i][0], color[i][1], color[i][2]);
		colorButton->SetEnabled(color[i][0] != -1);
		colorGrid->AddItem(colorButton);
	}

	HBoxLayout *colorWidgets = new HBoxLayout();
	
	ColorShelfButton *color1Button = new ColorShelfButton();
	color1Button->SetSizeHint(0, 0, 46, 66);
	color1Button->SetColorRole(ForegroundColor);
	color1Button->SetText("Couleur 1");
	colorWidgets->AddItem(color1Button);

	ColorShelfButton *color2Button = new ColorShelfButton();
	color2Button->SetSizeHint(0, 0, 46, 66);
	color2Button->SetColorRole(BackgroundColor);
	color2Button->SetText("Couleur 2");
	colorWidgets->AddItem(color2Button);

	colorWidgets->AddItem(colorGrid);

	TextImageButton *editColorButton = new TextImageButton();
	editColorButton->SetMargin(1, 0, 0, 0);
	editColorButton->SetInnerSizeHint(0, 0, 66, 66);
	editColorButton->LoadImage(vg, "images\\colors32.png");
	editColorButton->SetText("Modifier les couleurs");
	colorWidgets->AddItem(editColorButton);

	colorWidgets->SetMargin(4, 0, 6, 0);
	colorWidgets->AutoSizeHint();

	colorShelf->SetContent(colorWidgets);
	colorShelf->AutoSizeHint();

	shelf->AddItem(colorShelf);

	shelf->AddItem(new ShelfSeparator());

	ShelfSection *paint3dShelf = new ShelfSection();
	TextImageButton *paint3dButton = new TextImageButton();
	paint3dButton->SetMargin(4, 4, 6, 0);
	paint3dButton->SetInnerSizeHint(0, 0, 48, 66);
	paint3dButton->LoadImage(vg, "images\\paint3d32.png");
	paint3dButton->SetText("Ouvrir Paint 3D");
	paint3dShelf->SetContent(paint3dButton);
	paint3dShelf->AutoSizeHint();
	shelf->AddItem(paint3dShelf);

	shelf->AddItem(new ShelfSeparator());

	shelf->SetSizeHint(0, 0, 0, 92);
	layout->AddItem(shelf);

	DocumentArea *paintArea = new DocumentArea();
	paintArea->SetDocument(doc);
	paintArea->SetStrokeEngine(vg);
	layout->AddItem(paintArea);

	StatusBar *statusBar = new StatusBar();
	statusBar->LoadImages(vg);
	layout->AddItem(statusBar);

	popupLayout->AddItem(layout);

	ed->popupLayout = popupLayout;
	popupLayout->SetRect(0, 0, WIDTH, HEIGHT);

	// Load images
	Image cropOffImg(vg, "images\\cropOff18.png");
	Image resizeImg(vg, "images\\resize18.png");
	Image rotateImg(vg, "images\\rotate18.png");

	int font = nvgCreateFont(vg, "SegeoUI", (shareDir + "fonts\\segoeui.ttf").c_str());

	// Main loop
	while (!window.ShouldClose())
	{
		window.BeginRender();
		int winWidth = window.Width();
		int winHeight = window.Height();

		nvgFontFaceId(vg, font);
		nvgFontSize(vg, 15);

		// Shelf
		nvgBeginPath(vg);
		nvgRect(vg, 0, 24, winWidth, 92);
		nvgFillColor(vg, nvgRGBA(245, 246, 247, 255));
		nvgFill(vg);

		// // Shelf images
		// Clipboard
		
		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(141, 141, 141, 255));
		nvgText(vg, 70, 43, "Couper", NULL);
		
		nvgFillColor(vg, nvgRGBA(141, 141, 141, 255));
		nvgText(vg, 70, 65, "Copier", NULL);

		// Image
		cropOffImg.Paint(194, 24 + 5);
		resizeImg.Paint(194, 24 + 28);
		rotateImg.Paint(194, 24 + 50);

		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(141, 141, 141, 255));
		nvgText(vg, 214, 43, "Rogner", NULL);

		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgText(vg, 214, 65, "Redimensionner", NULL);

		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgText(vg, 214, 87, "Faire pivoter", NULL);

		// Shapes
		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(141, 141, 141, 255));
		nvgText(vg, 637, 43, "Contour", NULL);

		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(141, 141, 141, 255));
		nvgText(vg, 637, 65, "Remplissage", NULL);

		window.EndRender();

		// Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();
	}

	// Free images
	cropOffImg.Delete();
	resizeImg.Delete();
	rotateImg.Delete();

	// Delete document
	delete ed;
	delete doc;

	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mode)
{
	UiWindow* window = static_cast<UiWindow*>(glfwGetWindowUserPointer(glfwWindow));
	if (!window) {
		return;
	}

	std::cout << key << std::endl;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(glfwWindow, GL_TRUE);
}

void cursor_pos_callback(GLFWwindow* glfwWindow, double xpos, double ypos)
{
	UiWindow* window = static_cast<UiWindow*>(glfwGetWindowUserPointer(glfwWindow));
	if (!window) {
		return;
	}

	window->Content()->ResetDebug();
	window->Content()->ResetMouse();
	window->Content()->OnMouseOver(xpos, ypos);
}


void mouse_button_callback(GLFWwindow* glfwWindow, int button, int action, int mods)
{
	UiWindow* window = static_cast<UiWindow*>(glfwGetWindowUserPointer(glfwWindow));
	if (!window) {
		return;
	}

	window->Content()->OnMouseClick(button, action, mods);
}

void window_size_callback(GLFWwindow* glfwWindow, int width, int height) {
	UiWindow* window = static_cast<UiWindow*>(glfwGetWindowUserPointer(glfwWindow));
	if (!window) {
		return;
	}

	window->Content()->SetRect(0, 0, width, height);
}
