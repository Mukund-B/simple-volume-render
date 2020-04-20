// SOLUTION code for Host OpenGL Application for EE590 Win2020 Assignment 4

//#include <GL/GL.h>
#include <glew.h>
#include <glfw3.h>

////#include GLM GL Math header library
#include <glm.hpp>
#include <gtc\matrix_transform.hpp>
#include <gtc\matrix_access.hpp>
#include <gtc\matrix_inverse.hpp>
#include <gtx\string_cast.hpp>
#include <DSLoaders\DDSLoader.h>

//include shader wrapper C++ class
#include "shaderWrapper.h"
#include <algorithm>
#include <vector>
#include "helper.h"

// Define a helpful macro for handling offsets into buffer objects
#define BUFFER_OFFSET( offset )   ((GLvoid*) (offset))
typedef unsigned int uint;
GLuint SCR_SIZE = 1000;

//Global variables
//Transform Variables
glm::mat4 modelMatrix;
glm::mat4 viewMatrix;
glm::mat4 projMatrix;
glm::mat4 MVP, MV;
glm::mat3 N;

float g_scale_factor = 1.0f;
float g_rotate_angle = 45.0f;
float g_cam_z = 10.00f;  // can't have the lookdir vector (lookpt - eyept) = 0.
double pos_x, pos_y, pos_z;

GLuint PID;
GLenum gl_polygon_mode = GL_FILL;

GLuint volTexID, lutTexID;
int g_renderType = 0;

//Framebuffer callback function
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	SCR_SIZE = std::min(width, height);
	glViewport(0, 0, SCR_SIZE, SCR_SIZE);
}

//Key Callback
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS)
		return;
	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		glfwSetWindowShouldClose(window, true);
		break;

	case GLFW_KEY_X:  // Rotate by g_rotate_angle degress about X axis
		modelMatrix = glm::rotate(modelMatrix, (float)glm::radians(g_rotate_angle), glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case GLFW_KEY_Y: // Rotate by g_rotate_angle degress about Y axis
		modelMatrix = glm::rotate(modelMatrix, (float)glm::radians(g_rotate_angle), glm::vec3(0.0f, 1.0f, 0.0f));
		break;
	case GLFW_KEY_Z: // Rotate by g_rotate_angle degress about Z axis
		modelMatrix = glm::rotate(modelMatrix, (float)glm::radians(g_rotate_angle), glm::vec3(0.0f, 0.0f, 1.0f));
		break;

	case GLFW_KEY_P: // "Plus": shift camera position in +z-dir by 0.1 units.
		g_cam_z += 0.1f;
		viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, g_cam_z), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		fprintf(stderr, "<key_callback> P pressed: Cam z_pos = %f\n", g_cam_z);
		break;
	case GLFW_KEY_M: // "Minus": shift camera position in -z-dir by 0.1 units.
		g_cam_z -= 0.1f;
		viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, g_cam_z), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		fprintf(stderr, "<key_callback> M pressed: Cam z_pos = %f\n", g_cam_z);
		break;

	case GLFW_KEY_R: // "Minus": shift camera position in -z-dir by 0.1 units.
		g_renderType += 1;
		if (g_renderType == 3)
			g_renderType = 0;
		fprintf(stderr, "<key_callback> R pressed: Current Render Type %d. 0) Max, 1) Max, 2) Compositing\n", g_renderType);
		break;

	default:
		break;
	}
}

//Adding Mouse Inputs
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	//changes scale based on mouse scroll
	float scale;
	scale = (yoffset > 0) ? 2.0 : 0.5;  //Scale up if yoffset is +ve and down if it's -ve

	g_scale_factor *= scale;
	modelMatrix = glm::scale(modelMatrix, glm::vec3(scale));
	fprintf(stderr, "<scroll_callback>Scale factor = %f\n", g_scale_factor);
}
bool lbutton_down, rbutton_down;
double prev_x, prev_y;

void mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	//Sets flag if right/left click button is pressed and initialises parameter
	//Releases flag once key is released
	//Used in the cursor callback
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (GLFW_PRESS == action) {
			lbutton_down = true;
			glfwGetCursorPos(window, &prev_x, &prev_y);
		}

		else if (GLFW_RELEASE == action)
			lbutton_down = false;
	}

	else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (GLFW_PRESS == action) {
			rbutton_down = true;
			glfwGetCursorPos(window, &prev_x, &prev_y);
		}
		else if (GLFW_RELEASE == action)
			rbutton_down = false;
	}
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	//Based on the button depressed, performs transform based on cursor drag
	//left button press + drag - translate
	//right button press + drag - rotate
	if (lbutton_down) {
		double delY = prev_y - ypos;
		double delX = prev_x - xpos;
		delX *= -0.001;
		delY *= 0.001;
		pos_x += delX;
		pos_y += delY;
		fprintf(stderr, "<mouse_callback>Left Button Drag. New Position: (%f, %f, 0)\n", pos_x, pos_y);
		modelMatrix = glm::translate(modelMatrix, glm::vec3(delX, delY, 0));
		prev_x = xpos;
		prev_y = ypos;
	}
	else if (rbutton_down) {
		double delY = prev_y - ypos;
		double delX = prev_x - xpos;
		fprintf(stderr, "%f, %f\n", delX, delY);
		glm::vec3 vec;
		if (delX == 0 && delY == 0) {
			return;
		}
		if (delX == 0) {
			vec = glm::vec3(0.0f, 1.0f, 0.0f);
			fprintf(stderr, "<mouse_callback>Right Button Drag. Rotating along vector (%f, %f, 0.0)\n", 0.0, 1.0);
		}
		else if (delY == 0) {
			vec = glm::vec3(1.0f, 0.0f, 0.0f);
			fprintf(stderr, "<mouse_callback>Right Button Drag. Rotating along vector (%f, %f, 0.0)\n", 1.0, 0.0);
		}
		else {
			vec = glm::vec3(1.0f, (float)delX / delY, 0.0f);
			fprintf(stderr, "<mouse_callback>Right Button Drag. Rotating along vector (%f, %f, 0.0)\n", 1.0, (float)delX / delY);
		}
		modelMatrix = glm::rotate(modelMatrix, (float)glm::radians(5.0f), vec);
		prev_x = xpos;
		prev_y = ypos;
	}
}

glm::vec4 transferFunc(float scalar) {
	glm::vec3 high = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 low = glm::vec3(0.0, 0.0, 0.0);
	float alpha = (exp(scalar) - 1.0) / (exp(1.0) - 1.0);
	return glm::vec4(scalar * high + (1.0f - scalar) * low, alpha);
}

// Define init( ) function 
// HERE

void init(GLuint PID, GLuint* vao)
{
	parseConfig();
	initAABB(vao);
	lutTexID = initTF(128, g_userDef);

	volTexID = initVol(g_volFile);
	//int bin_size = 5;
	//int n_bins = ceil(256 / bin_size);
	//std::vector<int> hist (n_bins);
	//for (int i = 0; i < volWidth * volHeight * volDepth; i++) {
	//	int t = floor(voldata[i] / bin_size);
	//	hist[floor(voldata[i] / bin_size)]++;
	//}
	//printf("Histogram:\n");
	//for (int i = 0; i < 10; i++) {
	//	int d = std::distance(hist.begin(), std::max_element(hist.begin(), hist.end()));
	//	printf("%d : %d\n", d, hist[d]);
	//	hist[d] = 0;
	//}
}

// Define main( ) entry point function
// - should contain GLFW and GLEW initialization steps
// - call init( ) function
// - enter main render loop and do OpenGL drawing
// HERE
int main(int argc, char** argv)
{
	GLFWwindow* window;  // encapsulates both window and context
	if (!glfwInit())
		exit(EXIT_FAILURE);
	//Command line options
	//INPUT ARG1: Config file location (.ini)
	switch (argc) {
		case 1:
			g_config = "";
			break;
		case 2:
			g_config = argv[1];
			break;
		default:
			printf("USAGE: Final_Project.exe ARG1\n");
			printf("INPUT ARG1 : Config file location (Full Path)\n");
			printf("Exiting...");
			exit(EXIT_SUCCESS);
			break;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(SCR_SIZE, SCR_SIZE, "Ray Casting", NULL, NULL);
	if (NULL == window)
	{
		fprintf(stderr, "Failed to create GLFW window.\n");
		glfwTerminate();
		return EXIT_FAILURE;
	}
	glfwMakeContextCurrent(window);
	// register the GLFW framebuffer & key callback functions
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
	//Mouse Callbacks - Mouse + Scroll + Cursor
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	GLenum glerr = glewInit();
	if (GLEW_OK != glerr)
		fprintf(stderr, "glewInit Error: %s\n", glewGetErrorString(glerr));

	glEnable(GL_DEPTH_TEST);
	//glfwSwapInterval(1);  //Limit rate of render
	
	//Blending
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLuint vao;
	CShader shader("C:\\Users\\Mukund\\Documents\\UW\\GPU\\GPUSciVis_Win20\\shaders\\vshaderFP.glsl", "C:\\Users\\Mukund\\Documents\\UW\\GPU\\GPUSciVis_Win20\\shaders\\fshaderFP.glsl");
	GLuint PID = shader.getProgram();
	init(PID, &vao);
	shader.use();
	//transform matrices
	projMatrix = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 20.0f);  //Aspect ratio fixed to 1
	//projMatrix = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 100.0f);
	viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, g_cam_z), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::mat4(1.0);

	//Textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, lutTexID);
	shader.setUnifInt("xferTex", 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, volTexID);
	shader.setUnifInt("voldataTex", 1);
	
	shader.setUnifFloat("h", 0.01f);					//step size
	float m_focalLength = 1.0f / tan(glm::radians(60.0) / 2.0f);
	shader.setUnifFloat("focal_length", m_focalLength);   //focal length
	glm::vec3 ray_origin;
	// render loop
	while (!glfwWindowShouldClose(window))
	{
		//glClearColor(0.0f, 0.4f, 0.6f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // also clear the depth buffer now!
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		
		//Transforms
		MVP = projMatrix * viewMatrix * modelMatrix;
		MV = viewMatrix * modelMatrix;
		glUniformMatrix4fv(glGetUniformLocation(PID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(PID, "MV"), 1, GL_FALSE, &MV[0][0]);
		ray_origin = glm::vec3(glm::inverse(MV) * glm::vec4(0.0f, 0.0f, g_cam_z, 1.0f));
		glUniform2fv(glGetUniformLocation(PID, "viewport"), 1, &glm::vec2(SCR_SIZE)[0]);
		//shader.setUnifVec2("viewport", glm::vec2(SCR_SIZE, SCR_SIZE));
		shader.setUnifVec3("ray_origin", ray_origin);
		shader.setUnifInt("renderType", g_renderType);

		glBindVertexArray(vao); // Bind first VAO
		glDrawArrays(GL_TRIANGLES, 0, 36); // draw the points
		glBindVertexArray(0); //Unbind

		glFlush();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}
