#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <nanovg.h>
#define NANOVG_GLES3_IMPLEMENTATION
#include <nanovg_gl.h>

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);

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
		m_img = nvgCreateImage(m_vg, (shareDir + filename).c_str(), 0);
		nvgImageSize(m_vg, m_img, &m_width, &m_height);
	}

	~Image() {
		Delete();
	}

	void Delete() {
		if (m_img > -1) {
			nvgDeleteImage(m_vg, m_img);
			m_img = -1;
		}
	}

	void Paint(float x, float y) const {
		nvgBeginPath(m_vg);
		nvgRect(m_vg, x, y, m_width, m_height);
		nvgFillPaint(m_vg, nvgImagePattern(m_vg, x, y, m_width, m_height, 0, m_img, 1.0f));
		nvgFill(m_vg);
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

	Editor() : zoom(1.0f) {}
};

struct Rect {
	int x, y, w, h;

	Rect() : x(0), y(0), w(0), h(0) {}
	Rect(int _x, int _y, int _w, int _h) : x(_x), y(_y), w(_w), h(_h) {}

	bool Contains(int _x, int _y) const {
		return _x >= x && _y >= y && _x < x + w && _y < y + h;
	}
};

class UiElement {
public:
	virtual bool QueryAt(int x, int y) {
		bool hit = Rect().Contains(x, y);
		m_debug = hit;
		return hit;
	}

	virtual void ResetDebug() {
		m_debug = false;
	}

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

	/// MUST NOT call this->SetRect()
	virtual void Update() {
	}

	virtual void Paint(NVGcontext *vg) const {
		if (m_debug) {
			nvgBeginPath(vg);
			nvgRect(vg, Rect().x, Rect().y, Rect().w, Rect().h);
			nvgFillColor(vg, nvgRGBA(255, 0, 0, 64));
			nvgFill(vg);
		}
	}

private:
	::Rect m_rect;
	bool m_debug;
};

class VBoxLayout : public UiElement {
public:
	~VBoxLayout() {
		while (!m_items.empty()) {
			delete m_items.back();
			m_items.pop_back();
		}
	}

	bool QueryAt(int x, int y) override {
		bool hit = UiElement::QueryAt(x, y);
		if (hit) {
			int itemHeight = Rect().h / m_items.size();
			size_t i = std::min((size_t)(floor((y - Rect().y) / itemHeight)), m_items.size() - 1);
			m_items[i]->QueryAt(x, y);
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
		int itemHeight = floor(Rect().h / m_items.size());
		// Prevent rounding issues
		int lastItemHeight = Rect().h - (m_items.size() - 1) * itemHeight;

		for (size_t i = 0 ; i < m_items.size() ; ++i) {
			m_items[i]->SetRect(Rect().x, Rect().y + i * itemHeight, Rect().w, i == (m_items.size() - 1) ? lastItemHeight : itemHeight);
		}
	}

	void Paint(NVGcontext *vg) const override {
		UiElement::Paint(vg);
		for (UiElement *item : m_items) {
			item->Paint(vg);
		}
	}

	/// Take ownership of the item
	void AddItem(UiElement *item) {
		m_items.push_back(item);
	}

private:
	std::vector<UiElement*> m_items;
};

// TODO: get rid of tis global
VBoxLayout *layout;

// The MAIN function, from here we start the application and run the game loop
int main()
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
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Paint", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

	// Document
	Editor *ed = new Editor();
	Document *doc = new Document();
	doc->width = 254;
	doc->height = 280;

	layout = new VBoxLayout();
	layout->AddItem(new UiElement());
	layout->AddItem(new UiElement());
	layout->SetRect(0, 0, WIDTH, HEIGHT);

	// Init NanoVG
	glfwMakeContextCurrent(window);
	std::cout << "Starting NanoVG" << std::endl;
	struct NVGcontext* vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG); // TODO: try w/o NVG_STENCIL_STROKES
	if (vg == NULL) {
		std::cout << "Failed to initialize NanoVG context" << std::endl;
		glfwTerminate();
		return -1;
	}

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

	Image cursorImg(vg, "images\\cursor18.png");
	Image selectionImg(vg, "images\\selection18.png");
	Image sizeImg(vg, "images\\size18.png");
	Image savedImg(vg, "images\\saved18.png");
	Image zoomOutImg(vg, "images\\zoomOut18.png");
	Image zoomInImg(vg, "images\\zoomIn18.png");

	int font = nvgCreateFont(vg, "SegeoUI", (shareDir + "fonts\\segoeui.ttf").c_str());

	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		int winWidth, winHeight;
		int fbWidth, fbHeight;
		float pxRatio;

		glfwGetWindowSize(window, &winWidth, &winHeight);
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
		// Calculate pixel ration for hi-dpi devices.
		pxRatio = (float)fbWidth / (float)winWidth;

		glViewport(0, 0, fbWidth, fbHeight);

		// Render
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

		nvgBeginFrame(vg, winWidth, winHeight, pxRatio);

		// Tabs
		nvgBeginPath(vg);
		nvgRect(vg, 0, 0, 56, 24);
		nvgFillColor(vg, nvgRGBA(25, 121, 202, 255));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 56, 0, 64, 24);
		nvgFillColor(vg, nvgRGBA(245, 246, 247, 255));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 56+64, 0, winWidth - (56+64), 24);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgFill(vg);

		// // Tab Titles
		nvgFontFaceId(vg, font);
		nvgFontSize(vg, 15);

		nvgTextAlign(vg, NVG_ALIGN_LEFT);
		nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
		nvgText(vg, 12, 16, "Fichier", NULL);

		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgText(vg, 69, 16, "Accueil", NULL);

		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgText(vg, 134, 16, "Affichage", NULL);
		
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
		nvgFillColor(vg, nvgRGB(0, 0, 0));
		nvgFill(vg);

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgTextLineHeight(vg, 13.0f / 15.0f);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgTextBox(vg, 774 + 6, 24 + 53, 42, "Couleur 1", NULL);

		// Back Color
		nvgBeginPath(vg);
		nvgRect(vg, 774 + 61.5, 24 + 12.5, 23, 23);
		nvgStrokeColor(vg, nvgRGB(128, 128, 228));
		nvgStroke(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 774 + 63, 24 + 13, 20, 20);
		nvgFillColor(vg, nvgRGB(255, 255, 255));
		nvgFill(vg);

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgTextLineHeight(vg, 13.0f / 15.0f);
		nvgFillColor(vg, nvgRGBA(60, 60, 60, 255));
		nvgTextBox(vg, 774 + 52, 24 + 53, 42, "Couleur 2", NULL);

		float color[][3] = {
			{ 0, 0, 0 },{ 127, 127, 127 },{ 136, 0, 21 }, {237, 28, 36},{ 255, 127, 39 },{ 255, 242, 0 },{ 34, 177, 76 },{ 0, 162, 232 },{ 63, 72, 204 },{ 163, 73, 164 },
			{ 255, 255, 255 },{ 195, 195, 195},{ 185, 122, 87 },{ 255, 174, 201 },{ 255, 201, 14 },{ 239, 228, 176 },{ 181, 230, 29 },{ 153, 217, 234 },{ 112, 146, 190 }, {200, 191, 231},
			{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 },{ -1, -1, -1 }
		};
		for (int i = 0; i < 30; ++i) {
			int x = 22 * (i % 10);
			int y = 22 * (i / 10);

			if (color[i][0] != -1) {
				nvgBeginPath(vg);
				nvgRect(vg, 774 + 97 + x, 24 + 5 + y, 20, 20);
				nvgFillColor(vg, nvgRGB(255, 255, 255));
				nvgFill(vg);
			}

			nvgBeginPath(vg);
			nvgRect(vg, 774 + 97.5 + x, 24 + 5.5 + y, 19, 19);
			nvgStrokeColor(vg, nvgRGB(160, 160, 160));
			nvgStroke(vg);

			if (color[i][0] != -1) {
				nvgBeginPath(vg);
				nvgRect(vg, 774 + 99 + x, 24 + 7 + y, 16, 16);
				nvgFillColor(vg, nvgRGB(color[i][0], color[i][1], color[i][2]));
				nvgFill(vg);
			}
		}

		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(90, 90, 90, 255));
		nvgText(vg, 773 + 194, 110, "Couleurs", NULL);

		// Size
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGBA(90, 90, 90, 255));
		nvgText(vg, 722 + 25, 77, "Taille", NULL);

		// // Paint Area
		nvgScissor(vg, 0, 24+92, winWidth, winHeight - (24+92+25));

		nvgBeginPath(vg);
		nvgRect(vg, 0, 24+92, winWidth, winHeight - (24+92+25));
		nvgFillColor(vg, nvgRGBA(200, 209, 225, 255));
		nvgFill(vg);

		float drawingWidth = doc->width * ed->zoom;
		float drawingHeight = doc->height * ed->zoom;

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

		// Status Bar
		nvgBeginPath(vg);
		nvgRect(vg, 0, winHeight - 25, winWidth, 25);
		nvgFillColor(vg, nvgRGBA(240, 240, 240, 255));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgMoveTo(vg, 0, winHeight - 26 + 0.5);
		nvgLineTo(vg, winWidth, winHeight - 26 + 0.5);
		nvgStrokeColor(vg, nvgRGBA(218, 219, 220, 255));
		nvgStroke(vg);

		float sb_delim_pos[] = { 155, 311, 467, 623, winWidth - 199, winWidth - 1 };
		for (int i = 0; i < 6; ++i) {
			nvgBeginPath(vg);
			nvgMoveTo(vg, sb_delim_pos[i] + 0.5, winHeight - 24);
			nvgLineTo(vg, sb_delim_pos[i] + 0.5, winHeight - 1);
			nvgStrokeColor(vg, nvgRGBA(226, 227, 228, 255));
			nvgStroke(vg);
		}

		// // Status bar images
		cursorImg.Paint(1, winHeight - 25 + 3);
		selectionImg.Paint(159, winHeight - 25 + 3);
		sizeImg.Paint(315, winHeight - 25 + 3);
		savedImg.Paint(471, winHeight - 25 + 3);
		zoomOutImg.Paint(winWidth - 143, winHeight - 25 + 4);
		zoomInImg.Paint(winWidth - 21, winHeight - 25 + 4);

		// UI Objects
		layout->Paint(vg);

		nvgEndFrame(vg);

		// Swap the screen buffers
		glfwSwapBuffers(window);

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

	cursorImg.Delete();
	selectionImg.Delete();
	sizeImg.Delete();
	savedImg.Delete();
	zoomOutImg.Delete();
	zoomInImg.Delete();

	// Destroy NanoVG ctxw
	nvgDeleteGLES3(vg);

	// Delete document
	delete ed;
	delete doc;

	// Terminates GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();

	delete layout;

	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	std::cout << key << std::endl;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	layout->ResetDebug();
	layout->QueryAt(xpos, ypos);
}
