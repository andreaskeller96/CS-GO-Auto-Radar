#include <glad\glad.h>
#include <GLFW\glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <direct.h>

#include <regex>

#include "GLFWUtil.hpp"

#include "vbsp.hpp"
#include "nav.hpp"
#include "radar.hpp"
#include "util.h"

#include "Shader.hpp"
#include "Texture.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "GameObject.hpp"
#include "TextFont.hpp"
#include "Console.hpp"
#include "FrameBuffer.hpp"

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#ifdef WIN32
#include <windows.h>
#include <Commdlg.h>
#endif

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, util_keyHandler keys);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

int window_width = 1024;
int window_height = 1024;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool isClicking = false;

double mousex = 0.0;
double mousey = 0.0;

Camera camera;
Console console;

int SV_WIREFRAME = 0; //0: off 1: overlay 2: only
int SV_RENDERMODE = 0;
int SV_PERSPECTIVE = 0;

int M_HEIGHT_MIN = 0.0f;
int M_HEIGHT_MAX = 10.0f * 100.0f;

int M_CLIP_NEAR = 0.1f * 100.0f;
int M_CLIP_FAR = 100.0f * 100.0f;

int T_RENDER_RESOLUTION_X = 1024;
int T_RENDER_RESOLUTION_Y = 1024;
bool T_ARM_FOR_FB_RENDER = false;

bool SV_DRAW_BSP = true;
bool SV_DRAW_NAV = false;

bool V_DO_QUIT = false;

std::string _filesrc_bsp;
std::string _filesrc_nav;

Radar* _radar;

Mesh* mesh_map_bsp = NULL;
Mesh* mesh_map_nav = NULL;

int M_ORTHO_SIZE = 20;

// Runtime UI stuffz
void update_globals_ui_text(TextFont* target); //Auto update the ui text
TextFont* ui_text_info;
TextFont* ui_text_loaded;

//CCMDS
void _ccmd_renderbsp() { SV_RENDERMODE = 0; }
void _ccmd_rendernav() { SV_RENDERMODE = 1; }
void _ccmd_exit() { V_DO_QUIT = true; }
void _ccmd_perspective() { SV_PERSPECTIVE = 0; }
void _ccmd_orthographic() { SV_PERSPECTIVE = 1; }
void _ccmd_render_image() { T_ARM_FOR_FB_RENDER = true; }
void _ccmd_reset_ang() { camera.pitch = -90; camera.yaw = 0; camera.mouseUpdate(0, 0, true); }

// Some windows stuff
#ifdef WIN32
void _ccmd_open_nav_win() {
	OPENFILENAME ofn;       // common dialog box structure
	char szFile[260];       // buffer for file name
							//HWND hwnd;              // owner window
	HANDLE hf;              // file handle
							// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	//ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "Nav Mesh\0*.NAV\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn) == TRUE) {

		Nav::Mesh bob(ofn.lpstrFile);
		//mesh_map_nav->~Mesh(); //Destroy old mesh
		mesh_map_nav = new Mesh(bob.generateGLMesh());
		
	}
	else {
		console.FeedBack("Couldn't read file. (getopenfilename)");
		mesh_map_nav = NULL;
	}
}

void _ccmd_open_bsp_win() {
	OPENFILENAME ofn;       // common dialog box structure
	char szFile[260];       // buffer for file name
							//HWND hwnd;              // owner window
	HANDLE hf;              // file handle
							// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	//ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "BSP file\0*.BSP\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	// Display the Open dialog box. 

	if (GetOpenFileName(&ofn) == TRUE) {

		vbsp_level bsp_map(ofn.lpstrFile, true);
		mesh_map_bsp = new Mesh(bsp_map.generate_bigmesh());

	}
	else {
		console.FeedBack("Couldn't read file. (getopenfilename)");
		mesh_map_bsp = NULL;
	}
}
#endif

void _ccmd_help() {
	console.FeedBack(R"TERRI00(

Controls:
	WASD                Fly
    Click and drag      Look around
    ` or ~ key          Open console

Commands:
    General:
        QUIT / EXIT                       Closes (wow!)
        NAV                               View nav mesh
        BSP                               View bsp file
        HELP                              Helps you out
    							          
    Camera:						          
        PERSPECTIVE / PERSP               Switches to perspective view
        ORTHOGRAPHIC / ORTHO              Switches to orthographic view
            OSIZE / SIZE <int>            Changes the orthographic scale
        LOOKDOWN                          Sets yaw to 0, pitch to -90
    							          
        RENDER                            Renders the view to render.png
        						          
    Variables:					          
        RENDERMODE <int>                  Same as nav/bsp switch
        PROJMATRIX <int>                  Same as persp/ortho
        
    Variables (the actual height stuff):
        MIN    <int>                      Minimum levels of height (units)
        MAX    <int>                      Maximum levels of height (units)
    
        FARZ <int>                        Far clip plane of the camera
        NEARZ <int>                       Near clip plane of the camera
)TERRI00", MSG_STATUS::SUCCESS);
}

void _ccmd_thanks() {
	console.FeedBack(R"TERRI00(

Thank you:
   JamDoggie
   CrTech
   JimWood)TERRI00", MSG_STATUS::THANKS);
}

int main(int argc, char* argv[]) {
	_filesrc_bsp = "none";
	_filesrc_nav = "none";

	_chdir(argv[0]); //Reset working directory

	std::cout << argv[0] << std::endl;

	for (int i = 1; i < argc; ++i) {
		char* _arg = argv[i];
		std::string arg = _arg;

		if (split(arg, '.').back() == "bsp") {
			_filesrc_bsp = arg;
		}

		if (split(arg, '.').back() == "nav") {
			_filesrc_nav = arg;
		}
	}

#pragma region Initialisation

	std::cout << "CS:GO Heightmap generator" << std::endl;

	//Initialize OpenGL
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); //We are using version 3.3 of openGL
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	//Creating the window
	GLFWwindow* window = glfwCreateWindow(window_width, window_height, "CS:GO Heightmap generator", NULL, NULL);

	//Check if window open
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate(); return -1;
	}
	glfwMakeContextCurrent(window);

	//Settingn up glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl; return -1;
	}

	//Viewport
	glViewport(0, 0, 1024, 1024);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	//Mouse
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

#pragma endregion

	// configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);

	//The unlit shader thing
	Shader shader_unlit("shaders/unlit.vs", "shaders/unlit.fs");
	Shader shader_lit("shaders/depth.vs", "shaders/depth.fs");

	//Mesh handling -----------------------------------------------------------------------------

	if (_filesrc_bsp != "none"){
		vbsp_level bsp_map(_filesrc_bsp, true);
		mesh_map_bsp = new Mesh(bsp_map.generate_bigmesh());
	}

	if (_filesrc_nav != "none") {
		Nav::Mesh bob(_filesrc_nav);
		mesh_map_nav = new Mesh(bob.generateGLMesh());
	}

	//Radar rtest("de_overpass.txt");
	//_radar = &rtest;

	//VertAlphaMesh t300(vbsp_level::genVertAlpha(t200.vertices, t201.vertices));

	util_keyHandler keys(window);
	//Create camera (test)
	camera = Camera(&keys);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);

	TextFont::init(); //Setup textfonts before we use it

					  /*
	ui_text_info = new TextFont("Hello World!");
	ui_text_info->size = glm::vec2(1.0f / window_width, 1.0f / window_height) * 2.0f;
	ui_text_info->alpha = 1.0f;
	ui_text_info->color = glm::vec3(0.75f, 0.75f, 0.75f);
	ui_text_info->screenPosition = glm::vec2(0, (1.0f / window_height) * 15.0f); */
	
	
	ui_text_loaded = new TextFont("Currently Loaded:\n   " + std::string(_filesrc_bsp) + "\n   " + std::string(_filesrc_nav));
	ui_text_loaded->size = glm::vec2(1.0f / window_width, 1.0f / window_height) * 2.0f;
	ui_text_loaded->alpha = 1.0f;
	ui_text_loaded->color = glm::vec3(0.88f, 0.75f, 0.1f);
	ui_text_loaded->screenPosition = glm::vec2(0, 1.0f - ((1.0f / window_height) * 45.0f));

	//update_globals_ui_text(ui_text_info); //Update globals

	//Generate console
	console = Console(&keys, &window_width, &window_height);
	console.RegisterCVar("RENDERMODE", &SV_RENDERMODE);

	//Experimental
#ifdef WIN32
	console.RegisterCmd("OPENNAV", &_ccmd_open_nav_win);
	console.RegisterCmd("OPENBSP", &_ccmd_open_bsp_win);
#endif

	//Help
	console.RegisterCmd("HELP", &_ccmd_help);

	//Quit ccmds
	console.RegisterCmd("QUIT", &_ccmd_exit);
	console.RegisterCmd("EXIT", &_ccmd_exit);

	//Render modes
	console.RegisterCmd("NAV", &_ccmd_rendernav);
	console.RegisterCmd("BSP", &_ccmd_renderbsp);

	console.RegisterCmd("PERSPECTIVE", &_ccmd_perspective);
	console.RegisterCmd("PERSP", &_ccmd_perspective);
	console.RegisterCmd("ORTHOGRAPHIC", &_ccmd_orthographic);
	console.RegisterCmd("ORTHO", &_ccmd_orthographic);
	console.RegisterCmd("LOOKDOWN", &_ccmd_reset_ang);

	//console.RegisterCVar("SX", &T_RENDER_RESOLUTION_X);
	//console.RegisterCVar("SY", &T_RENDER_RESOLUTION_Y);
	console.RegisterCmd("RENDER", &_ccmd_render_image);

	//Register CVARS
	console.RegisterCVar("RENDERMODE", &SV_RENDERMODE);
	console.RegisterCVar("PROJMATRIX", &SV_PERSPECTIVE);
	console.RegisterCVar("MIN", &M_HEIGHT_MIN);
	console.RegisterCVar("MAX", &M_HEIGHT_MAX);
	console.RegisterCVar("OSIZE", &M_ORTHO_SIZE);
	console.RegisterCVar("SIZE", &M_ORTHO_SIZE);

	//Far/near
	console.RegisterCVar("FARZ", &M_CLIP_FAR);
	console.RegisterCVar("NEARZ", &M_CLIP_NEAR);

	//Thanks
	console.RegisterCmd("THANKS", &_ccmd_thanks);

	//FrameBuffer t_frame_buffer = FrameBuffer();

	//The main loop
	while (!glfwWindowShouldClose(window))
	{
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//Input
		processInput(window, keys);
		if(!console.isEnabled)
			camera.handleInput(deltaTime);

		console.handleKeysTick();

		if (T_ARM_FOR_FB_RENDER) {
			//t_frame_buffer.Bind();
			glViewport(0, 0, T_RENDER_RESOLUTION_X, T_RENDER_RESOLUTION_Y);
		}

		//Rendering commands
		glClearColor(0.05f, 0.05f, 0.2f, 1.0f);


		if(T_ARM_FOR_FB_RENDER) glClearColor(0.0f, 1.0f, 0.00f, 1.0f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glPolygonMode(GL_FRONT, GL_FILL);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(1);


		shader_lit.use();
		if(SV_PERSPECTIVE == 0)
			shader_lit.setMatrix("projection", glm::perspective(glm::radians(90.0f / 2), (float)window_width / (float)window_height, (float)M_CLIP_NEAR * 0.01f, (float)M_CLIP_FAR * 0.01f));
		else
			shader_lit.setMatrix("projection", glm::ortho(-(float)M_ORTHO_SIZE, (float)M_ORTHO_SIZE, -(float)M_ORTHO_SIZE, (float)M_ORTHO_SIZE, (float)M_CLIP_NEAR * 0.01f, (float)M_CLIP_FAR * 0.01f));

		shader_lit.setMatrix("view", camera.getViewMatrix());

		glm::mat4 model = glm::mat4();
		shader_lit.setMatrix("model", model);

		shader_lit.setVec3("color", 0.0f, 0.0f, 1.0f);
		shader_lit.setFloat("HEIGHT_MIN", (float)M_HEIGHT_MIN * 0.01f);
		shader_lit.setFloat("HEIGHT_MAX", (float)M_HEIGHT_MAX * 0.01f);

		
		if (SV_RENDERMODE == 0) {
			glEnable(GL_CULL_FACE);
			if (mesh_map_bsp != NULL)
				mesh_map_bsp->Draw();
		}
		
		if (SV_RENDERMODE == 1) {
			glDisable(GL_CULL_FACE);
			if (mesh_map_nav != NULL)
				mesh_map_nav->Draw();
		}




		if (!T_ARM_FOR_FB_RENDER) { // UI
			console.draw();

			//ui_text_info->DrawWithBackground();
			//ui_text_loaded->DrawWithBackground();
		}

		//Sort out render buffer stuff
		if (T_ARM_FOR_FB_RENDER) {
			std::cout << "Done" << std::endl;

			void* data = malloc(3 * T_RENDER_RESOLUTION_X * T_RENDER_RESOLUTION_Y);
			glReadPixels(0, 0, T_RENDER_RESOLUTION_X, T_RENDER_RESOLUTION_Y, GL_RGB, GL_UNSIGNED_BYTE, data);

			if (data != 0) {
				stbi_flip_vertically_on_write(true);
				stbi_write_png("render.png", T_RENDER_RESOLUTION_X, T_RENDER_RESOLUTION_Y, 3, data, T_RENDER_RESOLUTION_X * 3);
				console.FeedBack("Done! Saved to: render.png", MSG_STATUS::SUCCESS);
			}
			else
				console.FeedBack("Something went wrong making render", MSG_STATUS::ERR);

			//t_frame_buffer.Unbind();
			glViewport(0, 0, window_width, window_height);
			T_ARM_FOR_FB_RENDER = false;
		}


		//Check and call events, swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

		if (V_DO_QUIT)
			break;
	}

	//Exit safely
	glfwTerminate();
	return 0;
}

//Automatically readjust to the new size we just received
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	window_width = width;
	window_height = height;
}

bool K_CONTROL_MIN = false;
bool K_CONTROL_MAX = false;
int SV_EDITMODE = 0;

void setWindowTitle() {
	std::string title = "BigmanEngine | ";

}

void processInput(GLFWwindow* window, util_keyHandler keys)
{
	if (keys.getKeyDown(GLFW_KEY_ESCAPE))
		glfwSetWindowShouldClose(window, true);

	if (keys.getKeyDown(GLFW_KEY_GRAVE_ACCENT))
		console.isEnabled = !console.isEnabled;


	return;
	if (keys.getKeyDown(GLFW_KEY_1)) {
		SV_EDITMODE = 0;
		update_globals_ui_text(ui_text_info); //Update globals
	}
	if (keys.getKeyDown(GLFW_KEY_2)) {
		SV_EDITMODE = 1; 
		update_globals_ui_text(ui_text_info); //Update globals
	}
	if (keys.getKeyDown(GLFW_KEY_3)) {
		SV_EDITMODE = 2; //glfwSetWindowTitle(window, "Bigman Engine :: de_overpass.bsp - EDITING NEAR");
		update_globals_ui_text(ui_text_info); //Update globals
	}
	if (keys.getKeyDown(GLFW_KEY_4)) {
		SV_EDITMODE = 3; //glfwSetWindowTitle(window, "Bigman Engine :: de_overpass.bsp - EDITING FAR");
		update_globals_ui_text(ui_text_info); //Update globals
	}

	if (keys.getKeyDown(GLFW_KEY_5)) {
		SV_PERSPECTIVE = 0; 
		update_globals_ui_text(ui_text_info); //Update globals
	}

	if (keys.getKeyDown(GLFW_KEY_6)) {
		SV_PERSPECTIVE = 1; 
		update_globals_ui_text(ui_text_info); //Update globals
	}


	if (keys.getKeyDown(GLFW_KEY_7)) {
		SV_RENDERMODE = 0;
		update_globals_ui_text(ui_text_info); //Update globals
		
	}

	if (keys.getKeyDown(GLFW_KEY_8)) {
		SV_RENDERMODE = 1; 
		update_globals_ui_text(ui_text_info); //Update globals
	}

	if (keys.getKeyDown(GLFW_KEY_9)) {
		SV_EDITMODE = 4;
		//M_ORTHO_SIZE = (_radar->scale / 0.1f) / 2.0f;
		//camera.cameraPos.x = (-_radar->pos_x ) * 0.01f;
		//camera.cameraPos.z = (_radar->pos_y - 1024) * 0.01f;
		update_globals_ui_text(ui_text_info); //Update globals

	}

	if (keys.getKeyDown(GLFW_KEY_0)) {
		camera.yaw = 0;
		camera.pitch = -90;
		camera.mouseUpdate(0, 0, true);
		//camera.cameraFront = glm::vec3(0, 0, -1);
		//camera.cameraUp = glm::vec3(0, 1, 0);
	}
}

std::string _globals_editmode_lookups[] = {
	"Minimum Z",
	"Maximum Z",
	"Far Clip plane",
	"Near Clip plane",
	"Orthographic Size"
};

void update_globals_ui_text(TextFont* target) {

	std::ostringstream ss;

	ss << "Perspective: " << (SV_PERSPECTIVE == 0 ? "Perspective" : "Orthographic") << "\n";
	if (SV_PERSPECTIVE == 1) ss << "Ortho scale: " << M_ORTHO_SIZE << "\n";
	ss << "Viewing: " << (SV_RENDERMODE == 0 ? "BSP" : "NAV") << "\n";
	ss << "Editing: " << _globals_editmode_lookups[SV_EDITMODE] << "\n";


	target->SetText(ss.str());
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	camera.mouseUpdate(xpos, ypos, isClicking);
	mousex = xpos; mousey = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	//camera.fov = glm::clamp(camera.fov + (float)yoffset, 2.0f, 90.0f);

	if(SV_EDITMODE == 0)
		M_HEIGHT_MIN += (float)yoffset * 0.1f;

	if (SV_EDITMODE == 1)
		M_HEIGHT_MAX += (float)yoffset * 0.1f;

	if (SV_EDITMODE == 4)
		M_ORTHO_SIZE += (float)yoffset * 0.1f;

	//if (SV_EDITMODE == 2)	M_CLIP_NEAR += (float)yoffset * 0.1f;

	//if (SV_EDITMODE == 3)	M_CLIP_FAR += (float)yoffset * 0.1f;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		isClicking = true;
	}
	else
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		isClicking = false;
	}
}