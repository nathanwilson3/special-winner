/*
 * 5-1.cpp
 *
 *  Created on: Aug 5, 2020
 *      Author: natha
 *
 */

/*Header inclusions*/
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

//GLM Header inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;	// Standard namespace

#define WINDOW_TITLE "Modern OPenGL" // Window title Macros

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

/*Variable declarations for shader, window size initialization, buffer and array objects*/
GLint shaderProgram, WindowWidth = 800, WindowHeight = 600;
GLuint VBO, VAO;

GLfloat cameraSpeed = 0.0005f;	// Movement speed per frame
GLchar currentKey; // will store key pressed
int keymod; //Check that alt key is pressed

GLfloat scaleX = 2.0f;
GLfloat scaleY = 2.0f;
GLfloat scaleZ = 2.0f;


GLfloat lastMouseX = 400, lastMouseY = 300;	// Locks mouse cursor at the center of the screen
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch = 0.0f;	// mouse offset, yaw, and pitch variables
GLfloat sensitivity = 0.005f;	// Used for mouse / camera rotation sensitivity
bool mouseDetected = true;	// Initially true when mouse movement is detected

bool rotate = false;

bool zoom = false;

//Global vector declarations
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);	// Initial camera position.  Placed at center of object
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f);	// Temporary y unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f);	// Temporary z unit vector
glm::vec3 front;	// Temporary z unit vector for mouse

/*Function Prototypes*/
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UMouseMove(int x, int y);
void UMouseClick(int button, int state, int x, int y);
void UMouseMotion(int x, int y);

/*Vertex Shader Source Code*/
const GLchar * vertexShaderSource = GLSL(330,
		layout (location = 0) in vec3 position;	// Vertex data from Vertex Attrib Pointer 0
		layout (location = 1) in vec3 color;	// Color data from Vertex Attrib Pointer 1

		out vec3 mobileColor;	// Variable to transfer color data to the fragment shader

		// Global variables for the transform matrices
		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

void main(){
		gl_Position = projection * view * model * vec4(position, 1.0f);	// Transforms vertices to clip coordineates
		mobileColor = color;	// references incoming color data
	}
);

/*Fragment Shader Source Code*/
const GLchar * fragmentShaderSource = GLSL(330,

		in vec3 mobileColor;	// Variable to hold incoming color data from the Vertex shader

		out vec4 gpuColor;	// Variable to pass color data to the GPU

	void main() {

		gpuColor = vec4(mobileColor, 1.0);	// Sends color data to the GPU for rendering

	}
);

/*Main Program*/
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	glutCreateWindow(WINDOW_TITLE);

	glutReshapeFunc(UResizeWindow);


	glewExperimental = GL_TRUE;
			if (glewInit() != GLEW_OK)
			{
				std::cout<<"Failed to initialize GLEW"<< std::endl;
				return -1;
			}

	UCreateShader();

	UCreateBuffers();

	// Use the Shader program
	glUseProgram(shaderProgram);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);	// Set background color

	glutDisplayFunc(URenderGraphics);

	glutPassiveMotionFunc(UMouseMove);	// Detects mouse movement

	glutMotionFunc(UMouseMotion);

	glutMouseFunc(UMouseClick); // Detects mouse clicks

	glutMainLoop();

	//Destroys Buffer Objects once used
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	return 0;
}

/* Resizes the window*/
void UResizeWindow(int w, int h)
{
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}

/*Renders Graphics*/
void URenderGraphics(void)
{

	glEnable(GL_DEPTH_TEST);	//Enable z-depth

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clears the screen

	glBindVertexArray(VAO);	// Activate the Vertex Array Object before rendering and transforming them

	CameraForwardZ = front;	// Replaces camera forward vector with Radians normalized as a unit vector

	// Transforms the object
	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));	// Place the object at the center of the viewport
	model = glm::rotate(model, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));	// Rotate the object 45 degrees on the X axis
	model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));	// Increase the object size by a scale of 2


	// Transforms the camera
	glm::mat4 view;
	view = glm::lookAt(CameraForwardZ, cameraPosition, CameraUpY);

	//Create a perspective projection
	glm::mat4 projection;
	projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);

	// Retrieves and passes transform matrices to the Shader program
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glutPostRedisplay();

		//Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindVertexArray(0);	// Deactivate the Vertex Array Object

	glutSwapBuffers();	// Flips the back buffer with the front buffer every frame.  Similar to GL FLush
}


/*Creates the Shader program*/
void UCreateShader()
{


	// Vertex Shader
	GLint vertexShader = glCreateShader(GL_VERTEX_SHADER);	// Creates the Vertex Shader
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);	// Attaches the Vertex shader to the source code
	glCompileShader(vertexShader);	// Compiles the Vertex shader

	// Fragment shader
	GLint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);	// Creates the Fragment shader
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);	// Attaches the Fragment shader to the source code
	glCompileShader(fragmentShader);	// Compiles the Fragment shader

	// Shader program
	shaderProgram = glCreateProgram();	// Creates the Shader program and returns an id
	glAttachShader(shaderProgram, vertexShader);	// Attaches the Vertex shader to the Shader program
	glAttachShader(shaderProgram, fragmentShader);	// Attaches the Fragment shader to the Shader program
	glLinkProgram(shaderProgram);	// Link Vertex and Fragment shaders to the Shader program

	// Delete the Vertex and Fragment shaders once linked
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

}

void UCreateBuffers()
{

	GLfloat vertices[] = {

								//Positions				//Color
								-0.5f, -0.5f, -0.5f,	1.0f, 0.0f, 0.0f,
								 0.5f, -0.5f, -0.5f,	1.0f, 0.0f, 0.0f,
								 0.5f,  0.5f, -0.5f, 	1.0f, 0.0f, 0.0f,
								 0.5f,  0.5f, -0.5f, 	1.0f, 0.0f, 0.0f,
								-0.5f,  0.5f, -0.5f, 	1.0f, 0.0f, 0.0f,
								-0.5f, -0.5f, -0.5f, 	1.0f, 0.0f, 0.0f,

								-0.5f, -0.5f,  0.5f, 	0.0f, 1.0f, 0.0f,
								 0.5f, -0.5f,  0.5f, 	0.0f, 1.0f, 0.0f,
								 0.5f,  0.5f,  0.5f, 	0.0f, 1.0f, 0.0f,
								 0.5f,  0.5f,  0.5f, 	0.0f, 1.0f, 0.0f,
								-0.5f,  0.5f,  0.5f, 	0.0f, 1.0f, 0.0f,
								-0.5f, -0.5f,  0.5f, 	0.0f, 1.0f, 0.0f,

								-0.5f,  0.5f,  0.5f, 	0.0f, 0.0f, 1.0f,
								-0.5f,  0.5f, -0.5f, 	0.0f, 0.0f, 1.0f,
								-0.5f, -0.5f, -0.5f, 	0.0f, 0.0f, 1.0f,
								-0.5f, -0.5f, -0.5f, 	0.0f, 0.0f, 1.0f,
								-0.5f, -0.5f,  0.5f, 	0.0f, 0.0f, 1.0f,
								-0.5f,  0.5f,  0.5f, 	0.0f, 0.0f, 1.0f,

								 0.5f,  0.5f,  0.5f,	1.0f, 1.0f, 0.0f,
								 0.5f,  0.5f, -0.5f,	1.0f, 1.0f, 0.0f,
								 0.5f, -0.5f, -0.5f, 	1.0f, 1.0f, 0.0f,
								 0.5f, -0.5f, -0.5f, 	1.0f, 1.0f, 0.0f,
								 0.5f, -0.5f,  0.5f, 	1.0f, 1.0f, 0.0f,
								 0.5f,  0.5f,  0.5f, 	1.0f, 1.0f, 0.0f,

								-0.5f, -0.5f, -0.5f, 	0.0f, 1.0f, 1.0f,
								 0.5f, -0.5f, -0.5f, 	0.0f, 1.0f, 1.0f,
								 0.5f, -0.5f,  0.5f, 	0.0f, 1.0f, 1.0f,
								 0.5f, -0.5f,  0.5f, 	0.0f, 1.0f, 1.0f,
							    -0.5f, -0.5f,  0.5f, 	0.0f, 1.0f, 1.0f,
								-0.5f, -0.5f, -0.5f, 	0.0f, 1.0f, 1.0f,

								-0.5f,  0.5f, -0.5f,	1.0f, 0.0f, 1.0f,
								 0.5f,  0.5f, -0.5f,	1.0f, 0.0f, 1.0f,
								 0.5f,  0.5f,  0.5f, 	1.0f, 0.0f, 1.0f,
								 0.5f,  0.5f,  0.5f, 	1.0f, 0.0f, 1.0f,
								-0.5f,  0.5f,  0.5f, 	1.0f, 0.0f, 1.0f,
								-0.5f,  0.5f, -0.5f, 	1.0f, 0.0f, 1.0f,

							};

	// Generate buffer ids
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	// Activate the Vertex Array Object before binding and setting any VBOs and Vertex Attribute Pointers.
	glBindVertexArray(VAO);

	//Activate the VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);	// Copy vertices to VBO


	// Set attribute pointer 0 to hold Position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);	// Enables vertex attribute

	// Set attribute pointer 1 to hold Color data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);	// Enables vertex attribute

	glBindVertexArray(0);	// Deactivates the VAO which is good practice
}

/* Implements the UMouseMove function*/
void UMouseMove(int x, int y){

        front.x = 10.0f * cos(yaw);
        front.y = 10.0f * sin(pitch);
        front.z = sin(yaw) * cos(pitch) * 10.0f;

}


/*Implements UMouseMotion function*/
void UMouseMotion(int x, int y)
{
	// If alt and left mouse button pressed
	if (rotate){

		// Gets the direction the mouse was moved in x and y
		mouseXOffset = x - lastMouseX;
		mouseYOffset = lastMouseY - y;	// Inverted y

		//Updates with new mouse coordinates
		lastMouseX = x;
		lastMouseY = y;

		//Applies sensitivity to mouse direction
		mouseXOffset *= sensitivity;
		mouseYOffset *= sensitivity;

		//Gets the direction of the mouse, if change is in yaw, then the mouse is moving along the x axis
		if(yaw != yaw+mouseXOffset && pitch == pitch+mouseYOffset)
		{
			//Increment yaw
			yaw += mouseXOffset;

			//Otherwise, move mouse in Y direction
		}else if(pitch != pitch+mouseYOffset && yaw == yaw+mouseXOffset)
		{
			//Increment pitch
			pitch += mouseYOffset;
		}

		front.x = 10.0f * cos(yaw);
		front.y = 10.0f * sin(pitch);
		front.z = sin(yaw) * cos(pitch) * 10.0f;
	}

	//If alt key and right mouse button are pressed, zoom in and out
	if (zoom){

		//If zooming in, incrment XYZ values, else, decrement
		if(lastMouseY > y)
		{
			// Incrment values
			scaleX += 0.1;
			scaleY += 0.1;
			scaleZ += 0.1;

			//repost display
			glutPostRedisplay();

		}else
		{
			// Decrement values
			scaleX -= 0.1;
			scaleY -= 0.1;
			scaleZ -= 0.1;

			glutPostRedisplay();
		}

		//Updates with new mouse coordinates
		lastMouseX = x;
		lastMouseY = y;
	}
}


/*Implements UMouseClick function*/
void UMouseClick(int button, int state, int x, int y)
{
	keymod = glutGetModifiers(); // Checks for modifiers such as alt, shit, etc.

	rotate = false; //Set rotate boolean to false

	//Alt key and left mouse button pressed, should only rotate on x and y axis, (no zooming)
	if(button == GLUT_LEFT_BUTTON && keymod == GLUT_ACTIVE_ALT && state == GLUT_DOWN)
	{

		//If true, then rotate = true
		rotate = true;

		//Zoom = false
		zoom = false;

	}else if(button == GLUT_RIGHT_BUTTON && keymod == GLUT_ACTIVE_ALT && state == GLUT_DOWN)
	{
		//Zoom is true and rotate is false
		zoom = true;

		rotate = false;
	}
}

