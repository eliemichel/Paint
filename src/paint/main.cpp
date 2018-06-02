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

	void Delete() {
		if (m_img > -1) {
			nvgDeleteImage(m_vg, m_img);
			m_img = -1;
		}
	}

	void Paint(float x, float y) const {
		if (NULL != m_vg && m_img > -1) {
			nvgBeginPath(m_vg);
			nvgRect(m_vg, x, y, m_width, m_height);
			nvgFillPaint(m_vg, nvgImagePattern(m_vg, x, y, m_width, m_height, 0, m_img, 1.0f));
			nvgFill(m_vg);
		}
	}

private:
	int m_img; /// Image ID
	struct NVGcontext* m_vg; /// Parent context
	int m_width, m_height;
};

/**
 * Document properties
 */
struct Document {
	int width, height;
};

/**
 * Global editor state
 */
struct Editor {
	float zoom;
	NVGcolor foregroundColor;

	Editor()
		: zoom(1.0f)
		, foregroundColor(nvgRGB(0, 0, 0))
	{}
};

// TODO: get rid of this global (might require some kind of signals or passing a pointer to this global state to all the widgets)
Editor *ed;

// Custom UI elements

class UiButton : public UiMouseAwareElement {
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
		const ::Rect & r = Rect();
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

class FileButton : public UiButton {
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
		UiButton::OnTick();

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
		UiButton::Paint(vg);

		const ::Rect & r = Rect();
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

class HomeButton : public UiButton {
public:
	HomeButton() {
		SetSizeHint(::Rect(0, 0, 65, 0));
		SetBackgroundColor(245, 246, 247);
		SetTextColor(60, 60, 60);
		SetBorderColor(218, 219, 220);
		SetLabel("Accueil");
	}

public:
	void Paint(NVGcontext *vg) const override {
		UiButton::Paint(vg);

		const ::Rect & r = Rect();
		nvgBeginPath(vg);
		nvgMoveTo(vg, r.x + 0.5, r.y + r.h);
		nvgLineTo(vg, r.x + 0.5, r.y + 0.5);
		nvgLineTo(vg, r.x + r.w - 0.5, r.y + 0.5);
		nvgLineTo(vg, r.x + r.w - 0.5, r.y + r.h);
		nvgStrokeColor(vg, BorderColor());
		nvgStroke(vg);
	}
};

class ViewButton : public UiButton {
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
		UiButton::OnTick();

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
		UiButton::Paint(vg);

		const ::Rect & r = Rect();
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

class ColorButton : public UiMouseAwareElement {
public:
	ColorButton()
		: m_isEnabled(true)
		, m_isMouseOver(false)
	{}

	const NVGcolor & Color() const { return m_color; }
	void SetColor(const NVGcolor & color) { m_color = color; }
	void SetColor(unsigned char r, unsigned char g, unsigned char b) { m_color = nvgRGB(r, g, b); }

	bool IsEnabled() const { return m_isEnabled; }
	void SetEnabled(bool enabled) { m_isEnabled = enabled; }

public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = Rect();
		
		// White border
		if (m_isEnabled) {
			nvgBeginPath(vg);
			nvgRect(vg, r.x, r.y, r.w, r.h);
			nvgFillColor(vg, m_isMouseOver ? nvgRGB(203, 228, 253) : nvgRGB(255, 255, 255));
			nvgFill(vg);
		}

		// Main Border
		nvgBeginPath(vg);
		nvgRect(vg, r.x + 0.5, r.y + 0.5, r.w - 1, r.h - 1);
		nvgStrokeColor(vg, m_isMouseOver && m_isEnabled ? nvgRGB(100, 165, 231) : nvgRGB(160, 160, 160));
		nvgStroke(vg);

		// Color
		if (m_isEnabled) {
			nvgBeginPath(vg);
			nvgRect(vg, r.x + 2, r.y + 2, r.w - 4, r.h - 4);
			nvgFillColor(vg, Color());
			nvgFill(vg);
		}
	}

	void OnMouseEnter() override {
		m_isMouseOver = true;
	}

	void OnMouseLeave() override {
		m_isMouseOver = false;
	}

	void OnMouseClick(int x, int y) override {
		ed->foregroundColor = Color();
	}

private:
	NVGcolor m_color;
	bool m_isEnabled;
	bool m_isMouseOver;
};

class MenuBar : public HBoxLayout {
public:
	MenuBar() {
		SetSizeHint(0, 0, 0, 24);
	}

public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = Rect();

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
		const ::Rect & r = Rect();

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

class PaintArea : public UiMouseAwareElement {
public:
	PaintArea()
		: UiMouseAwareElement()
		, m_doc(NULL)
	{}

	Document * Document() const { return m_doc; }
	void SetDocument(::Document *doc) { m_doc = doc; }

public: // protected
	void Paint(NVGcontext *vg) const override {
		const ::Rect & r = Rect();
		nvgScissor(vg, r.x, r.y, r.w, r.h);

		nvgBeginPath(vg);
		nvgRect(vg, r.x, r.y, r.w, r.h);
		nvgFillColor(vg, nvgRGB(200, 209, 225));
		//nvgFillColor(vg, nvgRGB(225, 159, 150));
		nvgFill(vg);

		float drawingWidth = Document()->width * ed->zoom;
		float drawingHeight = Document()->height * ed->zoom;

		// Shadow
		nvgBeginPath(vg);
		nvgRect(vg, 5 + 10, 24 + 92 + 5 + 10, drawingWidth, drawingHeight);
		nvgFillPaint(vg, nvgBoxGradient(vg, 5, 24 + 92 + 5, drawingWidth + 4.5, drawingHeight + 4.5,
			-5, 9, nvgRGBA(51, 96, 131, 30), nvgRGBA(0, 0, 0, 0)));
		nvgFill(vg);

		// Drawing
		nvgBeginPath(vg);
		nvgRect(vg, 5, 24 + 92 + 5, drawingWidth, drawingHeight);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);

		// Handles
		nvgBeginPath(vg);
		nvgRect(vg, 5 + drawingWidth, 24 + 92 + 5 + drawingHeight, 5, 5);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);
		nvgBeginPath(vg);
		nvgRect(vg, 5 + drawingWidth + 0.5, 24 + 92 + 5 + drawingHeight + 0.5, 4, 4);
		nvgStrokeColor(vg, nvgRGBA(85, 85, 85, 255));
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 5 + drawingWidth, 24 + 92 + 5 + floor((drawingHeight - 5) / 2.), 5, 5);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);
		nvgBeginPath(vg);
		nvgRect(vg, 5 + drawingWidth + 0.5, 24 + 92 + 5 + floor((drawingHeight - 5) / 2.) + 0.5, 4, 4);
		nvgStrokeColor(vg, nvgRGBA(85, 85, 85, 255));
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 5 + floor((drawingWidth - 5) / 2.), 24 + 92 + 5 + drawingHeight, 5, 5);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);
		nvgBeginPath(vg);
		nvgRect(vg, 5 + floor((drawingWidth - 5) / 2.) + 0.5, 24 + 92 + 5 + drawingHeight + 0.5, 4, 4);
		nvgStrokeColor(vg, nvgRGBA(85, 85, 85, 255));
		nvgStroke(vg);

		nvgResetScissor(vg);
	}

private:
	::Document *m_doc;
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

	double MouseX() const { return m_mouseX; }
	void SetMouseX(double x) { m_mouseX = x; }

	double MouseY() const { return m_mouseY; }
	void SetMouseY(double y) { m_mouseY = y; }

private:
	bool m_isValid;
	GLFWwindow* m_window;
	struct NVGcontext* m_vg;
	UiElement *m_content;
	mutable int m_width, m_height;
	double m_mouseX, m_mouseY;
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
	doc->width = 254;
	doc->height = 280;

	VBoxLayout *layout = new VBoxLayout();
	window.SetContent(layout);

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

	HBoxLayout *shelf = new HBoxLayout();
	UiElement *hSpacer = new UiElement();
	hSpacer->SetSizeHint(0, 0, 871, 0);
	shelf->AddItem(hSpacer);

	VBoxLayout *colorShelf = new VBoxLayout();
	UiElement *vSpacerTop = new UiElement();
	vSpacerTop->SetSizeHint(0, 0, 0, 5);
	colorShelf->AddItem(vSpacerTop);

	GridLayout *colorGrid = new GridLayout();
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

	//colorGrid->SetSizeHint(Rect(0, 0, 218, 64));
	colorShelf->AddItem(colorGrid);

	UiElement *vSpacerBottom = new UiElement();
	vSpacerBottom->SetSizeHint(Rect(0, 0, 0, 23));
	colorShelf->AddItem(vSpacerBottom);

	colorShelf->SetSizeHint(0, 0, 218, 0);
	shelf->AddItem(colorShelf);
	shelf->SetSizeHint(0, 0, 0, 92);
	layout->AddItem(shelf);

	PaintArea *paintArea = new PaintArea();
	paintArea->SetDocument(doc);
	layout->AddItem(paintArea);

	StatusBar *statusBar = new StatusBar();
	layout->AddItem(statusBar);
	layout->SetRect(0, 0, WIDTH, HEIGHT);

	statusBar->LoadImages(vg);

	// Load images
	Image pasteOffImg(vg, "images\\pasteOff32.png");

	Image selectImg(vg, "images\\select32.png");
	Image cropOffImg(vg, "images\\cropOff18.png");
	Image resizeImg(vg, "images\\resize18.png");
	Image rotateImg(vg, "images\\rotate18.png");

	Image pencilImg(vg, "images\\pencil21.png");
	Image fillImg(vg, "images\\fill21.png");
	Image textImg(vg, "images\\text21.png");
	Image eraseImg(vg, "images\\erase21.png");
	Image pickerImg(vg, "images\\picker21.png");
	Image zoomImg(vg, "images\\zoom21.png");

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

		nvgBeginPath(vg);
		nvgMoveTo(vg, 0, 23.5);
		nvgLineTo(vg, 56.5, 23.5);
		nvgLineTo(vg, 56.5, 0.5);
		nvgLineTo(vg, 56 + 64.5, 0.5);
		nvgLineTo(vg, 56 + 64.5, 23.5);
		nvgLineTo(vg, winWidth, 23.5);
		nvgStrokeColor(vg, nvgRGBA(218, 219, 220, 255));
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgMoveTo(vg, 0, 24 + 129.5);
		nvgLineTo(vg, winWidth, 24 + 129.5);
		nvgStrokeColor(vg, nvgRGBA(218, 219, 220, 255));
		nvgStroke(vg);

		float shelf_delim_pos[] = { 118, 308, 388, 448, 720, 773, 1162, 1221 };
		for (int i = 0; i < 8; ++i) {
			nvgBeginPath(vg);
			nvgMoveTo(vg, shelf_delim_pos[i] + 0.5, 24 + 2);
			nvgLineTo(vg, shelf_delim_pos[i] + 0.5, 24 + 88);
			nvgStrokeColor(vg, nvgRGBA(226, 227, 228, 255));
			nvgStroke(vg);
		}

		nvgBeginPath(vg);
		nvgMoveTo(vg, 0, 159.5);
		nvgLineTo(vg, winWidth, 159.5);
		nvgStrokeColor(vg, nvgRGB(218, 219, 220));
		nvgStroke(vg);

		// // Shelf images
		// Clipboard
		pasteOffImg.Paint(11, 24 + 6);

		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(141, 141, 141, 255));
		nvgText(vg, 70, 43, "Couper", NULL);
		
		nvgFillColor(vg, nvgRGBA(141, 141, 141, 255));
		nvgText(vg, 70, 65, "Copier", NULL);
		
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgText(vg, 12 + 15, 77, "Coller", NULL);

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(90, 90, 90, 255));
		nvgText(vg, 59, 110, "Presse-papiers", NULL);
		
		// Image
		selectImg.Paint(141, 24 + 7);
		cropOffImg.Paint(194, 24 + 5);
		resizeImg.Paint(194, 24 + 28);
		rotateImg.Paint(194, 24 + 50);

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgText(vg, 140 + 15, 77, u8"Sélectionner", NULL);

		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(141, 141, 141, 255));
		nvgText(vg, 214, 43, "Rogner", NULL);

		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgText(vg, 214, 65, "Redimensionner", NULL);

		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgText(vg, 214, 87, "Faire pivoter", NULL);

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(90, 90, 90, 255));
		nvgText(vg, 119 + 95, 110, "Image", NULL);

		// Tools
		pencilImg.Paint(309 + 6, 24 + 13);
		fillImg.Paint(309 + 6 + 23, 24 + 13);
		textImg.Paint(309 + 6 + 23 * 2, 24 + 13);
		eraseImg.Paint(309 + 6, 24 + 13 + 23);
		pickerImg.Paint(309 + 6 + 23, 24 + 13 + 23);
		zoomImg.Paint(309 + 6 + 23 * 2, 24 + 13 + 23);

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(90, 90, 90, 255));
		nvgText(vg, 309 + 40, 110, "Outils", NULL);

		// Paint
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgText(vg, 394 + 24, 77, "Pinceaux", NULL);

		// Shapes
		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(141, 141, 141, 255));
		nvgText(vg, 637, 43, "Contour", NULL);

		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(141, 141, 141, 255));
		nvgText(vg, 637, 65, "Remplissage", NULL);

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(90, 90, 90, 255));
		nvgText(vg, 449 + 135, 110, "Formes", NULL);

		// Colors
		// // Front Color
		nvgBeginPath(vg);
		nvgRect(vg, 774 + 4, 24 + 4, 46, 66);
		nvgFillColor(vg, nvgRGB(201, 224, 247));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 774 + 4.5, 24 + 4.5, 45, 65);
		nvgStrokeColor(vg, nvgRGB(98, 162, 228));
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 774 + 11.5, 24 + 7.5, 31, 31);
		nvgStrokeColor(vg, nvgRGB(128, 128, 128));
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 774 + 13, 24 + 9, 28, 28);
		nvgFillColor(vg, ed->foregroundColor);
		nvgFill(vg);

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgTextLineHeight(vg, 13.0f / 15.0f);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgTextBox(vg, 774 + 6, 24 + 53, 42, "Couleur 1", NULL);

		// Back Color
		nvgBeginPath(vg);
		nvgRect(vg, 774 + 61.5, 24 + 12.5, 23, 23);
		nvgStrokeColor(vg, nvgRGB(128, 128, 128));
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 774 + 63, 24 + 13, 20, 20);
		nvgFillColor(vg, nvgRGB(255, 255, 255));
		nvgFill(vg);

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgTextLineHeight(vg, 13.0f / 15.0f);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgTextBox(vg, 774 + 52, 24 + 53, 42, "Couleur 2", NULL);

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(90, 90, 90, 255));
		nvgText(vg, 773 + 194, 110, "Couleurs", NULL);

		// Size
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(90, 90, 90, 255));
		nvgText(vg, 722 + 25, 77, "Taille", NULL);

		// // Paint Area
		

		// Status Bar

		window.EndRender();

		// Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();
	}

	// Free images
	pasteOffImg.Delete();

	selectImg.Delete();
	cropOffImg.Delete();
	resizeImg.Delete();
	rotateImg.Delete();

	pencilImg.Delete();
	fillImg.Delete();
	textImg.Delete();
	eraseImg.Delete();
	pickerImg.Delete();
	zoomImg.Delete();

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

	window->SetMouseX(xpos);
	window->SetMouseY(ypos);
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

	// TODO: Handle click, press and release more precisely
	if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
		// TODO: avoid providing the position again since the tree already checked it at the last mouse move
		window->Content()->OnMouseClick(window->MouseX(), window->MouseY());
	}
}

void window_size_callback(GLFWwindow* glfwWindow, int width, int height) {
	UiWindow* window = static_cast<UiWindow*>(glfwGetWindowUserPointer(glfwWindow));
	if (!window) {
		return;
	}

	window->Content()->SetRect(0, 0, width, height);
}
