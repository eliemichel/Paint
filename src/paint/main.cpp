#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <nanovg.h>
#define NANOVG_GLES3_IMPLEMENTATION
#include <nanovg_gl.h>

#include <iostream>

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

const std::string shareDir = "E:\\SourceCode\\Paint\\share\\";

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

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize OpenGL context" << std::endl;
		return -1;
	}

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
	int pasteOffImg = nvgCreateImage(vg, (shareDir + "images\\pasteOff32.png").c_str(), 0);

	int selectImg = nvgCreateImage(vg, (shareDir + "images\\select32.png").c_str(), 0);
	int cropOffImg = nvgCreateImage(vg, (shareDir + "images\\cropOff18.png").c_str(), 0);
	int resizeImg = nvgCreateImage(vg, (shareDir + "images\\resize18.png").c_str(), 0);
	int rotateImg = nvgCreateImage(vg, (shareDir + "images\\rotate18.png").c_str(), 0);

	int pencilImg = nvgCreateImage(vg, (shareDir + "images\\pencil21.png").c_str(), 0);
	int fillImg = nvgCreateImage(vg, (shareDir + "images\\fill21.png").c_str(), 0);
	int textImg = nvgCreateImage(vg, (shareDir + "images\\text21.png").c_str(), 0);
	int eraseImg = nvgCreateImage(vg, (shareDir + "images\\erase21.png").c_str(), 0);
	int pickerImg = nvgCreateImage(vg, (shareDir + "images\\picker21.png").c_str(), 0);
	int zoomImg = nvgCreateImage(vg, (shareDir + "images\\zoom21.png").c_str(), 0);

	int cursorImg = nvgCreateImage(vg, (shareDir + "images\\cursor18.png").c_str(), 0);
	int selectionImg = nvgCreateImage(vg, (shareDir + "images\\selection18.png").c_str(), 0);
	int sizeImg = nvgCreateImage(vg, (shareDir + "images\\size18.png").c_str(), 0);
	int savedImg = nvgCreateImage(vg, (shareDir + "images\\saved18.png").c_str(), 0);
	int zoomOutImg = nvgCreateImage(vg, (shareDir + "images\\zoomOut18.png").c_str(), 0);
	int zoomInImg = nvgCreateImage(vg, (shareDir + "images\\zoomIn18.png").c_str(), 0);

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
			nvgMoveTo(vg, shelf_delim_pos[i] + 0.5, 24 + 2.5);
			nvgLineTo(vg, shelf_delim_pos[i] + 0.5, 24 + 87.5);
			nvgStrokeColor(vg, nvgRGBA(226, 227, 228, 255));
			nvgStroke(vg);
		}

		// // Shelf images
		// Clipboard
		nvgBeginPath(vg);
		nvgRect(vg, 11, 24 + 6, 32, 32);
		nvgFillPaint(vg, nvgImagePattern(vg, 11, 24 + 6, 32, 32, 0, pasteOffImg, 1.0f));
		nvgFill(vg);

		// Image
		nvgBeginPath(vg);
		nvgRect(vg, 141, 24 + 7, 32, 32);
		nvgFillPaint(vg, nvgImagePattern(vg, 141, 24 + 7, 32, 32, 0, selectImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 194, 24 + 5, 21, 21);
		nvgFillPaint(vg, nvgImagePattern(vg, 194, 24 + 5, 21, 21, 0, cropOffImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 194, 24 + 28, 21, 21);
		nvgFillPaint(vg, nvgImagePattern(vg, 194, 24 + 28, 21, 21, 0, resizeImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 194, 24 + 50, 21, 21);
		nvgFillPaint(vg, nvgImagePattern(vg, 194, 24 + 50, 21, 21, 0, rotateImg, 1.0f));
		nvgFill(vg);

		// Tools
		nvgBeginPath(vg);
		nvgRect(vg, 309 + 6, 24 + 13, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, 309 + 6, 24 + 13, 18, 18, 0, pencilImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 309 + 6 + 23, 24 + 13, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, 309 + 6 + 23, 24 + 13, 18, 18, 0, fillImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 309 + 6 + 23 * 2, 24 + 13, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, 309 + 6 + 23 * 2, 24 + 13, 18, 18, 0, textImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 309 + 6, 24 + 13 + 23, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, 309 + 6, 24 + 13 + 23, 18, 18, 0, eraseImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 309 + 6 + 23, 24 + 13 + 23, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, 309 + 6 + 23, 24 + 13 + 23, 18, 18, 0, pickerImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 309 + 6 + 23 * 2, 24 + 13 + 23, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, 309 + 6 + 23 * 2, 24 + 13 + 23, 18, 18, 0, zoomImg, 1.0f));
		nvgFill(vg);

		// // Paint Area
		nvgScissor(vg, 0, 24+92, winWidth, winHeight - (24+92+25));

		nvgBeginPath(vg);
		nvgRect(vg, 0, 24+92, winWidth, winHeight - (24+92+25));
		nvgFillColor(vg, nvgRGBA(200, 209, 225, 255));
		nvgFill(vg);

		float drawingWidth = 254;
		float drawingHeight = 280;

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
		int cursorImg = nvgCreateImage(vg, (shareDir + "images\\cursor18.png").c_str(), 0); // 1 / 3
		int selectionImg = nvgCreateImage(vg, (shareDir + "images\\selection18.png").c_str(), 0); // 159 / 3
		int sizeImg = nvgCreateImage(vg, (shareDir + "images\\size18.png").c_str(), 0); // 315 / 3
		int savedImg = nvgCreateImage(vg, (shareDir + "images\\saved18.png").c_str(), 0); // 471 / 3
		int zoomOutImg = nvgCreateImage(vg, (shareDir + "images\\zoomOut18.png").c_str(), 0); // -143 / 3
		int zoomInImg = nvgCreateImage(vg, (shareDir + "images\\zoomIn18.png").c_str(), 0); // -21 / 3

		nvgBeginPath(vg);
		nvgRect(vg, 1, winHeight - 25 + 3, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, 1, winHeight - 25 + 3, 18, 18, 0, cursorImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 159, winHeight - 25 + 3, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, 159, winHeight - 25 + 3, 18, 18, 0, selectionImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 315, winHeight - 25 + 3, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, 315, winHeight - 25 + 3, 18, 18, 0, sizeImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, 471, winHeight - 25 + 3, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, 471, winHeight - 25 + 3, 18, 18, 0, savedImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, winWidth - 143, winHeight - 25 + 4, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, winWidth - 143, winHeight - 25 + 4, 18, 18, 0, zoomOutImg, 1.0f));
		nvgFill(vg);

		nvgBeginPath(vg);
		nvgRect(vg, winWidth - 21, winHeight - 25 + 4, 18, 18);
		nvgFillPaint(vg, nvgImagePattern(vg, winWidth - 21, winHeight - 25 + 4, 18, 18, 0, zoomInImg, 1.0f));
		nvgFill(vg);


		nvgEndFrame(vg);

		// Swap the screen buffers
		glfwSwapBuffers(window);

		// Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();
	}

	// Free images
	nvgDeleteImage(vg, pasteOffImg);

	nvgDeleteImage(vg, selectImg);
	nvgDeleteImage(vg, cropOffImg);
	nvgDeleteImage(vg, resizeImg);
	nvgDeleteImage(vg, rotateImg);

	nvgDeleteImage(vg, pencilImg);
	nvgDeleteImage(vg, fillImg);
	nvgDeleteImage(vg, textImg);
	nvgDeleteImage(vg, eraseImg);
	nvgDeleteImage(vg, pickerImg);
	nvgDeleteImage(vg, zoomImg);

	nvgDeleteImage(vg, cursorImg);
	nvgDeleteImage(vg, selectionImg);
	nvgDeleteImage(vg, sizeImg);
	nvgDeleteImage(vg, savedImg);
	nvgDeleteImage(vg, zoomOutImg);
	nvgDeleteImage(vg, zoomInImg);

	// Destroy NanoVG ctxw
	nvgDeleteGLES3(vg);

	// Terminates GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	std::cout << key << std::endl;
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}
