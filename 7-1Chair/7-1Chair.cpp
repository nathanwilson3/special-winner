/*
 * 7-1Chair.cpp
 *
 *  Created on: Aug 20, 2020
 *      Author: Nathan Wilson
 */

/* Header inclusions*/
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

//GLM header inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// SOIL image loader inclusion
#include "SOIL2/SOIL2.h"

using namespace std;	// Standard namespace

#define WINDOW_TITLE "7-1 Chair" // Window title Macro

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif


/* Variable declarations for shader, window size initialization, buffer and array objects*/
GLint chairShaderProgram, lampShaderProgram,WindowWidth = 800, WindowHeight = 600;
GLuint VBO, ChairVAO, LightVAO,texture;
GLfloat degrees = glm::radians(-45.0f); // Converts float to degrees

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

glm::vec3 chairPosition(0.0f, 0.0f, 0.0f);

//light color
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

//Light position and scale
glm::vec3 lightPosition(0.5f, 0.5f, 6.0f);
glm::vec3 lightScale(0.3f);


float cameraRotation = glm::radians(-45.0f);

/*Function prototypes*/
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UGenerateTexture(void);
void UMouseMove(int x, int y);
void UMouseClick(int button, int state, int x, int y);
void UMouseMotion(int x, int y);


/*Chair Vertex Shader Source Code*/
const GLchar * chairVertexShaderSource = GLSL(330,
		layout (location = 0) in vec3 position; // Vertex data from Vertex Attrib Pointer 0
		layout (location = 1) in vec3 normal; //VAP position 1 for normals
		layout (location = 2) in vec2 textureCoordinate;

		out vec3 FragmentPos; //For outgoing color / pixels to fragment shader
		out vec3 Normal; //For outgoing normals to fragment shader
		out vec2 mobileTextureCoordinate;


		//Global Variables for the transform matrices
		uniform mat4 model;
		uniform mat4 view;
		uniform mat4 projection;

void main(){
		gl_Position = projection * view * model * vec4(position, 1.0f);	// transforms vertices to clip coordinates

		FragmentPos = vec3(model * vec4(position, 1.0f)); //Gets fragment / pixel position in world space only (exclude view and projection)

		Normal = mat3(transpose(inverse(model))) *  normal; //get normal vectors in world space only and exclude normal translation properties

		mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y); // flipls the horizontal
		}
);

/*Chair Fragment Shader Source Code*/
const GLchar * chairFragmentShaderSource = GLSL(330,

		in vec3 FragmentPos; //For incoming fragment position
		in vec3 Normal; //For incoming normals
		in vec2 mobileTextureCoordinate;

		out vec4 chairColor;	// For outgoing chair Color to GPU

		//Uniform / Global variables for object color, light color, light position, and camera/view position
		uniform vec3 lightColor;
		uniform vec3 lightPos;
		uniform vec3 viewPosition;

		uniform sampler2D uTexture;	// Useful when working with multiple textures

	void main(){

		/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

		//Calculate Ambient Lighting
		float ambientStrength = 0.3f; //Set ambient or global lighting strength
		vec3 ambient = ambientStrength * lightColor; //Generate ambient light color


		//Calculate Diffuse Lighting
		vec3 norm = normalize(Normal); //Normalize vectors to 1 unit
		vec3 lightDirection = normalize(lightPos - FragmentPos); //Calculate distance (light direction) between light source and fragments/pixels on
		float impact = max(dot(norm, lightDirection), 0.8); //Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse = impact * lightColor; //Generate diffuse light color


		//Calculate Specular lighting
		float specularIntensity = 0.3f; //Set specular light strength
		float highlightSize = 32.0f; //Set specular highlight size
		vec3 viewDir = normalize(viewPosition - FragmentPos); //Calculate view direction
		vec3 reflectDir = reflect(-lightDirection, norm); //Calculate reflection vector

		//Calculate specular component
		float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
		vec3 specular = specularIntensity * specularComponent * lightColor;

		//Calculate phong result
		vec3 objectColor = texture(uTexture, mobileTextureCoordinate).xyz;
		vec3 phong = (ambient + diffuse) * objectColor + specular;
		chairColor = vec4(phong, 1.0f); //Send lighting results to GPU
	}
);


/*Lamp Shader Source Code*/
const GLchar * lampVertexShaderSource = GLSL(330,

        layout (location = 0) in vec3 position; //VAP position 0 for vertex position data

        //Uniform / Global variables for the transform matrices
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main()
        {
            gl_Position = projection * view *model * vec4(position, 1.0f); //Transforms vertices into clip coordinates
        }
);


/*Fragment Shader Source Code*/
const GLchar * lampFragmentShaderSource = GLSL(330,

        out vec4 color; //For outgoing lamp color (smaller pyramid) to the GPU

        void main()
        {
            color = vec4(1.0f); //Set color to white (1.0f, 1.0f, 1.0f) with alpha 1.0

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
				std::cout<<"Failed to initialize GLEW" << std::endl;
				return -1;
			}

	UCreateShader();

	UCreateBuffers();

	UGenerateTexture();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glutDisplayFunc(URenderGraphics);

	glutPassiveMotionFunc(UMouseMove);	// Detects mouse movement

	glutMotionFunc(UMouseMotion);

	glutMouseFunc(UMouseClick); // Detects mouse clicks

	glutMainLoop();

	// Destroys buffer objects once used
	glDeleteVertexArrays(1, &ChairVAO);
	glDeleteVertexArrays(1, &LightVAO);
	glDeleteBuffers(1, &VBO);

	return 0;
}

/*Resize the window*/
void UResizeWindow(int w, int h)
{
	WindowWidth = w;
	WindowHeight = h;
	glViewport(0, 0, WindowWidth, WindowHeight);
}


/* Renders graphics*/
void URenderGraphics(void)
{

	glEnable(GL_DEPTH_TEST); // Enables z-depth

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clears the screen

	GLint modelLoc, viewLoc, projLoc, uTextureLoc, lightColorLoc, lightPositionLoc, viewPositionLoc;

	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 projection;

	/*********Use the chair Shader to activate the chair Vertex Array Object for rendering and transforming*********/
	glUseProgram(chairShaderProgram);
	glBindVertexArray(ChairVAO);	// Activate the Vertex Array Object before rendering and transforming them

	CameraForwardZ = front;	// Replaces camera forward vector with Radians normalized as a unit vector

	// Transforms the object
	model = glm::translate(model, chairPosition);
	model = glm::rotate(model, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));	// Rotate the object 45 degrees on the X axis
	//model = glm::rotate(model, glutGet(GLUT_ELAPSED_TIME) * -0.0005f, glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));	// Increase the object size by a scale of 2

	// Transforms the camera
	view = glm::lookAt(CameraForwardZ, cameraPosition, CameraUpY);
	view = glm::translate(view, cameraPosition);
	view = glm::rotate(view, cameraRotation, glm::vec3(0.0f, 1.0f, 0.0f));

	//Create a perspective projection
	projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);

	//Reference matrix uniforms from the Chair Shader program
	modelLoc = glGetUniformLocation(chairShaderProgram, "model");
	viewLoc = glGetUniformLocation(chairShaderProgram, "view");
	projLoc = glGetUniformLocation(chairShaderProgram, "projection");

	//Pass matrix data to the Chair Shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));


	//Reference matrix uniforms from the Chair Shader program for the Chair color, light color, light position, and camera position
	uTextureLoc = glGetUniformLocation(chairShaderProgram, "uTexture");
	lightColorLoc = glGetUniformLocation(chairShaderProgram, "lightColor");
	lightPositionLoc = glGetUniformLocation(chairShaderProgram, "lightPos");
	viewPositionLoc = glGetUniformLocation(chairShaderProgram, "viewPosition");

	//Pass color, light, and camera data to the Chair Shader programs corresponding uniforms
	glUniform1i(uTextureLoc, 0);
	glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
	glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	glDrawArrays(GL_TRIANGLES, 0, 192); //Draw the primitives / Chair

	glBindVertexArray(0); //Deactivate the Chair Vertex Array Object

	/***************Use the Lamp Shader and activate the Lamp Vertex Array Object for rendering and transforming ************/
	glUseProgram(lampShaderProgram);
	glBindVertexArray(LightVAO);

	//Transform the smaller Chair used as a visual cue for the light source
	model = glm::translate(model, lightPosition);
	model = glm::scale(model, lightScale);

    //Reference matrix uniforms from the Lamp Shader program
	modelLoc = glGetUniformLocation(lampShaderProgram, "model");
    viewLoc = glGetUniformLocation(lampShaderProgram, "view");
    projLoc = glGetUniformLocation(lampShaderProgram, "projection");

    //Pass matrix uniforms from the Lamp Shader Program
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));



    //Draws the triangles
    glDrawArrays(GL_TRIANGLES, 0, 192);

    glBindVertexArray(0); //Deactivate the Lamp Vertex Array Object

    glutPostRedisplay();
    glutSwapBuffers(); //Flips the back buffer with the front buffer every frame. Similar to GL Flush

}
/*Creates the Shader program*/
void UCreateShader()
{

	//Chair Vertex Shader
	GLint chairVertexShader = glCreateShader(GL_VERTEX_SHADER); //Creates the Vertex shader
	glShaderSource(chairVertexShader, 1, &chairVertexShaderSource, NULL); // Attaches the Vertex Shader to the source code
	glCompileShader(chairVertexShader); // Compiles the Vertex shader

	//Chair Fragment Shader
	GLint chairFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);	// Creates the Fragment shader
	glShaderSource(chairFragmentShader, 1, &chairFragmentShaderSource, NULL); // Attaches the Fragment Shader to the source code
	glCompileShader(chairFragmentShader); // Compiles the Fragment shader

	//Chair Shader program
	chairShaderProgram = glCreateProgram(); // Creates the Shader program and returns an id
	glAttachShader(chairShaderProgram, chairVertexShader); // Attach Vertex shader to the Shader program
	glAttachShader(chairShaderProgram, chairFragmentShader); // Attach Fragment shader to the Shader program
	glLinkProgram(chairShaderProgram); // Link Vertex and Fragment shaders to the Shader program

	// Delete the Vertex and Fragment shaders once linked
	glDeleteShader(chairVertexShader);
	glDeleteShader(chairFragmentShader);

	//Lamp Vertex shader
    GLint lampVertexShader = glCreateShader(GL_VERTEX_SHADER); //Creates the Vertex shader
    glShaderSource(lampVertexShader, 1, &lampVertexShaderSource, NULL); //Attaches the Vertex shader to the source code
    glCompileShader(lampVertexShader); //Compiles the Vertex shader

    //Lamp Fragment shader
    GLint lampFragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //Creates the Fragment shader
    glShaderSource(lampFragmentShader, 1, &lampFragmentShaderSource, NULL); //Attaches the Fragment shader to the source code
    glCompileShader(lampFragmentShader); //Compiles the Fragment shader

    //Lamp Shader Program
    lampShaderProgram = glCreateProgram(); //Creates the Shader program and returns an id
    glAttachShader(lampShaderProgram, lampVertexShader); //Attach Vertex shader to the Shader program
    glAttachShader(lampShaderProgram, lampFragmentShader); //Attach Fragment shader to the Shader program
    glLinkProgram(lampShaderProgram); //Link Vertex and Fragment shaders to the Shader program

    //Delete the lamp shaders once linked
    glDeleteShader(lampVertexShader);
    glDeleteShader(lampFragmentShader);

}


/*Creates the Buffer and Array Objects*/
void UCreateBuffers()
{

	//Position and Texture coordinate data for 192 triangles
	GLfloat vertices[] = {

								//Positions					//Normals						//Texture Coordinates

							//Front left leg, front			//Positive Z Normals
								-0.5f, -0.5f,  0.5f,		 0.0f, 0.0f, 1.0f, 				0.0f,   0.0f,
								-0.3f, -0.5f,  0.5f,		 0.0f, 0.0f, 1.0f, 				0.25f,  0.0f,
								-0.5f,  0.1f,  0.5f,		 0.0f, 0.0f, 1.0f, 				0.0f,   0.5f,
								-0.3f, -0.5f,  0.5f,		 0.0f, 0.0f, 1.0f, 				0.25f,  0.0f,
								-0.3f,  0.1f,  0.5f,		 0.0f, 0.0f, 1.0f, 				0.25f,  0.5f,
								-0.5f,  0.1f,  0.5f,		 0.0f, 0.0f, 1.0f, 				0.0f,   0.5f,

							 //Front left leg, right		//Positive X Normals
								-0.3f, -0.5f,  0.5f,		 1.0, 0.0f, 0.0f,				0.0f,   0.0f,
								-0.3f, -0.5f,  0.3f,		 1.0, 0.0f, 0.0f,				0.25f,  0.0f,
								-0.3f,  0.1f,  0.5f,	 	 1.0, 0.0f, 0.0f,				0.0f,   0.5f,
								-0.3f, -0.5f,  0.3f,		 1.0, 0.0f, 0.0f,				0.25f,  0.0f,
								-0.3f,  0.1f,  0.3f,		 1.0, 0.0f, 0.0f,				0.25f,  0.5f,
								-0.3f,  0.1f,  0.5f,		 1.0, 0.0f, 0.0f,				0.0f,   0.5f,

							  //Front left leg back			//Negative Z Normals
								-0.5f, -0.5f,  0.3f, 		 0.0f, 0.0f, -1.0f, 			0.0f,   0.0f,
								-0.3f, -0.5f,  0.3f, 		 0.0f, 0.0f, -1.0f, 			0.25f,  0.0f,
								-0.5f,  0.1f,  0.3f, 		 0.0f, 0.0f, -1.0f,				0.0f,   0.5f,
								-0.3f, -0.5f,  0.3f, 		 0.0f, 0.0f, -1.0f, 			0.25f,  0.0f,
								-0.3f,  0.1f,  0.3f, 		 0.0f, 0.0f, -1.0f, 			0.25f,  0.5f,
								-0.5f,  0.1f,  0.3f, 		 0.0f, 0.0f, -1.0f, 			0.0f,   0.5f,

							   //Front left leg left		//Negative X Normals
								-0.5f, -0.5f,  0.5f,	 	-1.0f, 0.0f, 0.0f, 				0.25f,  0.0f,
								-0.5f, -0.5f,  0.3f,		-1.0f, 0.0f, 0.0f, 				0.0f,   0.0f,
								-0.5f,  0.1f,  0.5f,		-1.0f, 0.0f, 0.0f,				0.25f,  0.5f,
								-0.5f, -0.5f,  0.3f,		-1.0f, 0.0f, 0.0f, 				0.0f,   0.0f,
								-0.5f,  0.1f,  0.3f, 		-1.0f, 0.0f, 0.0f, 				0.0f,   0.5f,
								-0.5f,  0.1f,  0.5f, 		-1.0f, 0.0f, 0.0f, 				0.25f,  0.5f,

							//Front left leg, bottom		//Negative Y Normals
								-0.5f, -0.5f,  0.5f,		 0.0f, -1.0f, 0.0f,   			0.0f,    0.2f,
								-0.3f, -0.5f,  0.5f,		 0.0f, -1.0f, 0.0f,				0.2f,    0.2f,
								-0.5f, -0.5f,  0.3f,		 0.0f, -1.0f, 0.0f,				0.0f,    0.0f,
								-0.3f, -0.5f,  0.5f,		 0.0f, -1.0f, 0.0f,				0.2f,    0.2f,
							    -0.3f, -0.5f,  0.3f,		 0.0f, -1.0f, 0.0f,				0.2f,    0.0f,
								-0.5f, -0.5f,  0.3f,		 0.0f, -1.0f, 0.0f,				0.0f,    0.0f,

								//Seat, front				//Positive Z Normals
								-0.5f,  0.1f,  0.5f,		 0.0f, 0.0f, 1.0f, 				0.0f,    0.0f,
								 0.5f,  0.1f,  0.5f, 		 0.0f, 0.0f, 1.0f,				1.0f,    0.0f,
								-0.5f, 0.25f,  0.5f, 		 0.0f, 0.0f, 1.0f,				0.0f,    0.25f,
								 0.5f,  0.1f,  0.5f, 		 0.0f, 0.0f, 1.0f,				1.0f,    0.0f,
								 0.5f, 0.25f,  0.5f, 		 0.0f, 0.0f, 1.0f,				1.0f,    0.25f,
								-0.5f, 0.25f,  0.5f, 		 0.0f, 0.0f, 1.0f,				0.0f,    0.25f,

								 //Seat, top				//Positive Y Normals
								-0.5f, 0.25f,  0.5f, 		 0.0f, 1.0f, 0.0f,				0.0f,    0.0f,
								 0.5f, 0.25f,  0.5f, 		 0.0f, 1.0f, 0.0f,				1.0f,    0.0f,
								-0.5f, 0.25f, -0.5f,		 0.0f, 1.0f, 0.0f,				0.0f,    1.0f,
								 0.5f, 0.25f,  0.5f, 		 0.0f, 1.0f, 0.0f,				1.0f,    0.0f,
								 0.5f, 0.25f, -0.5f, 		 0.0f, 1.0f, 0.0f,				1.0f,    1.0f,
								-0.5f, 0.25f, -0.5f,		 0.0f, 1.0f, 0.0f,				0.0f,    1.0f,

								 //Seat, back				//Negative Z Normals
								-0.5f,  0.1f, -0.5f,		 0.0f, 0.0f, -1.0f,				0.0f,    0.0f,
								 0.5f,  0.1f, -0.5f,		 0.0f, 0.0f, -1.0f,				1.0f,    0.0f,
								-0.5f, 0.25f, -0.5f,		 0.0f, 0.0f, -1.0f,				0.0f,    0.25f,
								 0.5f,  0.1f, -0.5f, 		 0.0f, 0.0f, -1.0f,				1.0f,    0.0f,
								 0.5f, 0.25f, -0.5f,		 0.0f, 0.0f, -1.0f,				1.0f,    0.25f,
								-0.5f, 0.25f, -0.5f, 		 0.0f, 0.0f, -1.0f,				0.0f,    0.25f,

								 //Seat, bottom				//Negative Y Normals
								-0.5f,  0.1f,  0.5f, 		 0.0f, -1.0f, 0.0f,				0.0f, 	1.0f,
								 0.5f,  0.1f,  0.5f,		 0.0f, -1.0f, 0.0f,				1.0f, 	1.0f,
								-0.5f,  0.1f, -0.5f,		 0.0f, -1.0f, 0.0f,				0.0f, 	0.0f,
								 0.5f,  0.1f,  0.5f,		 0.0f, -1.0f, 0.0f,				1.0f, 	1.0f,
								 0.5f,  0.1f, -0.5f, 		 0.0f, -1.0f, 0.0f,				1.0f, 	0.0f,
								-0.5f,  0.1f, -0.5f,		 0.0f, -1.0f, 0.0f,				0.0f, 	0.0f,

								 //Seat, left				//Negative X Normals
								-0.5f,  0.1f,  0.5f,		-1.0f, 0.0f, 0.0f,				1.0f, 	0.0f,
								-0.5f,  0.1f, -0.5f,		-1.0f, 0.0f, 0.0f,				0.0f, 	0.0f,
								-0.5f, 0.25f,  0.5f,		-1.0f, 0.0f, 0.0f, 				1.0f, 	0.25f,
								-0.5f, 0.25f,  0.5f,		-1.0f, 0.0f, 0.0f,				1.0f, 	0.25f,
								-0.5f, 0.25f, -0.5f,		-1.0f, 0.0f, 0.0f,				0.0f, 	0.25f,
								-0.5f,  0.1f, -0.5f,		-1.0f, 0.0f, 0.0f,				0.0f, 	0.0f,

								 //Seat, right				//Positive X Normals
								 0.5f,  0.1f,  0.5f,		 1.0, 0.0f, 0.0f,				0.0f, 	0.0f,
								 0.5f,  0.1f, -0.5f,		 1.0, 0.0f, 0.0f,				1.0f, 	0.0f,
								 0.5f, 0.25f,  0.5f,		 1.0, 0.0f, 0.0f,				0.0f, 	0.25f,
								 0.5f,  0.1f, -0.5f,		 1.0, 0.0f, 0.0f,				1.0f, 	0.0f,
								 0.5f, 0.25f, -0.5f,		 1.0, 0.0f, 0.0f,				1.0f, 	0.25f,
								 0.5f, 0.25f,  0.5f,		 1.0, 0.0f, 0.0f,				0.0f, 	0.25f,

							//Front right leg, front		//Positive Z Normals
								 0.5f, -0.5f,  0.5f,		 0.0f, 0.0f, 1.0f, 				0.25f, 	0.0f,
								 0.3f, -0.5f,  0.5f,		 0.0f, 0.0f, 1.0f,				0.0f, 	0.0f,
								 0.5f,  0.1f,  0.5f,		 0.0f, 0.0f, 1.0f,				0.25f, 	0.5f,
								 0.3f, -0.5f,  0.5f,		 0.0f, 0.0f, 1.0f,				0.0f, 	0.0f,
								 0.3f,  0.1f,  0.5f,		 0.0f, 0.0f, 1.0f,				0.0f, 	0.5f,
								 0.5f,  0.1f,  0.5f, 		 0.0f, 0.0f, 1.0f,				0.25f, 	0.5f,

							 //Front right leg, back		//Negative Z Normals
								 0.5f, -0.5f,  0.3f,		 0.0f, 0.0f, -1.0f,				0.0f, 	0.0f,
								 0.3f, -0.5f,  0.3f,		 0.0f, 0.0f, -1.0f,				0.25f, 	0.0f,
								 0.5f,  0.1f,  0.3f,		 0.0f, 0.0f, -1.0f,				0.0f, 	0.5f,
								 0.3f, -0.5f,  0.3f,		 0.0f, 0.0f, -1.0f,				0.25f, 	0.0f,
								 0.3f,  0.1f,  0.3f, 		 0.0f, 0.0f, -1.0f,				0.25f, 	0.5f,
								 0.5f,  0.1f,  0.3f,		 0.0f, 0.0f, -1.0f,				0.0f, 	0.5f,

							  //Front right leg, left		//Negative X Normals
								 0.3f, -0.5f,  0.5f,		-1.0f, 0.0f, 0.0f, 				0.25f, 	0.0f,
								 0.3f, -0.5f,  0.3f, 		-1.0f, 0.0f, 0.0f,				0.0f, 	0.0f,
								 0.3f,  0.1f,  0.5f,		-1.0f, 0.0f, 0.0f,				0.25f, 	0.5f,
								 0.3f, -0.5f,  0.3f,		-1.0f, 0.0f, 0.0f,				0.0f, 	0.0f,
								 0.3f,  0.1f,  0.3f, 		-1.0f, 0.0f, 0.0f,				0.0f, 	0.5f,
								 0.3f,  0.1f,  0.5f,		-1.0f, 0.0f, 0.0f,				0.25f, 	0.5f,

							  //Front right leg, right		//Positive X Normals
								 0.5f, -0.5f,  0.5f, 		 1.0, 0.0f, 0.0f,				0.0f, 	0.0f,
								 0.5f, -0.5f,  0.3f,		 1.0, 0.0f, 0.0f,				0.25f, 	0.0f,
								 0.5f,  0.1f,  0.5f,		 1.0, 0.0f, 0.0f,				0.0f, 	0.5f,
								 0.5f, -0.5f,  0.3f, 		 1.0, 0.0f, 0.0f,				0.25f, 	0.0f,
								 0.5f,  0.1f,  0.3f, 		 1.0, 0.0f, 0.0f,				0.25f, 	0.5f,
								 0.5f,  0.1f,  0.5f,		 1.0, 0.0f, 0.0f,				0.0f, 	0.5f,

							//Front right leg, bottom		//Negative Y Normals
								 0.5f, -0.5f,  0.5f,		 0.0f, -1.0f, 0.0f,				0.2f, 	0.2f,
								 0.3f, -0.5f,  0.5f,		 0.0f, -1.0f, 0.0f,				0.0f, 	0.2f,
								 0.5f, -0.5f,  0.3f,		 0.0f, -1.0f, 0.0f,				0.2f, 	0.0f,
								 0.3f, -0.5f,  0.5f,		 0.0f, -1.0f, 0.0f,				0.0f, 	0.2f,
								 0.3f, -0.5f,  0.3f,		 0.0f, -1.0f, 0.0f, 			0.0f, 	0.0f,
								 0.5f, -0.5f,  0.3f,		 0.0f, -1.0f, 0.0f,				0.2f, 	0.0f,

								//Back left leg, front		//Positive Z Normals
								-0.5f, -0.5f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.0f, 	0.0f,
								-0.3f, -0.5f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.25f, 	0.0f,
								-0.5f,  1.0f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.0f, 	1.0f,
								-0.3f, -0.5f, -0.4f, 		 0.0f, 0.0f, 1.0f,				0.25f, 	0.0f,
								-0.3f,  1.0f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.25f, 	1.0f,
								-0.5f,  1.0f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.0f, 	1.0f,

								//Back left leg, back		//Negative Z Normals
								-0.5f, -0.5f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.0f, 	0.0f,
								-0.3f, -0.5f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.25f, 	0.0f,
								-0.5f,  1.0f, -0.6f, 		 0.0f, 0.0f, -1.0f,				0.0f, 	1.0f,
								-0.3f, -0.5f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.25f, 	0.0f,
								-0.3f,  1.0f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.25f, 	1.0f,
								-0.5f,  1.0f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.0f, 	1.0f,

								//Back left leg, right		//Positive X Normals
								-0.3f, -0.5f, -0.6f,		 1.0, 0.0f, 0.0f, 				0.25f, 	0.0f,
								-0.3f, -0.5f, -0.4f,		 1.0, 0.0f, 0.0f,				0.0f, 	0.0f,
								-0.3f,  1.0f, -0.6f,		 1.0, 0.0f, 0.0f,				0.25f, 	1.0f,
								-0.3f, -0.5f, -0.4f,		 1.0, 0.0f, 0.0f,				0.0f, 	0.0f,
								-0.3f,  1.0f, -0.4f,		 1.0, 0.0f, 0.0f,				0.0f, 	1.0f,
								-0.3f,  1.0f, -0.6f, 		 1.0, 0.0f, 0.0f,				0.25f, 	1.0f,

								//Back left leg, left		//Negative X Normals
								-0.5f, -0.5f, -0.6f,		-1.0f, 0.0f, 0.0f,				0.0f, 	0.0f,
								-0.5f, -0.5f, -0.4f,		-1.0f, 0.0f, 0.0f,				0.25f, 	0.0f,
								-0.5f,  1.0f, -0.6f,		-1.0f, 0.0f, 0.0f,				0.0f, 	1.0f,
								-0.5f, -0.5f, -0.4f, 		-1.0f, 0.0f, 0.0f,				0.25f, 	0.0f,
								-0.5f,  1.0f, -0.4f,		-1.0f, 0.0f, 0.0f,				0.25f, 	1.0f,
								-0.5f,  1.0f, -0.6f,		-1.0f, 0.0f, 0.0f,				0.0f, 	1.0f,

							  //Back left leg, bottom		//Negative Y Normals
								-0.5f, -0.5f, -0.6f,		 0.0f, -1.0f, 0.0f,				0.0f, 	0.0f,
								-0.3f, -0.5f, -0.6f,		 0.0f, -1.0f, 0.0f,				0.2f, 	0.0f,
								-0.5f, -0.5f, -0.4f,		 0.0f, -1.0f, 0.0f,				0.0f, 	0.2f,
								-0.3f, -0.5f, -0.6f,		 0.0f, -1.0f, 0.0f,				0.2f, 	0.0f,
								-0.3f, -0.5f, -0.4f,		 0.0f, -1.0f, 0.0f,				0.2f, 	0.2f,
								-0.5f, -0.5f, -0.4f,		 0.0f, -1.0f, 0.0f,				0.0f, 	0.2f,

								//Back left leg, top		//Positive Y Normals
								-0.5f,  1.0f, -0.6f,		 0.0f, 1.0f, 0.0f, 				0.0f, 	0.2f,
								-0.3f,  1.0f, -0.6f,		 0.0f, 1.0f, 0.0f, 				0.2f, 	0.2f,
								-0.5f,  1.0f, -0.4f,		 0.0f, 1.0f, 0.0f,				0.0f, 	0.0f,
								-0.3f,  1.0f, -0.6f,		 0.0f, 1.0f, 0.0f, 				0.2f, 	0.2f,
								-0.3f,  1.0f, -0.4f,		 0.0f, 1.0f, 0.0f, 				0.2f, 	0.0f,
								-0.5f,  1.0f, -0.4f,		 0.0f, 1.0f, 0.0f,				0.0f, 	0.0f,

								//Back right leg, left		//Negative X Normals
								 0.3f, -0.5f, -0.6f,		-1.0f, 0.0f, 0.0f,				0.0f, 	0.0f,
								 0.3f, -0.5f, -0.4f,		-1.0f, 0.0f, 0.0f,				0.25f, 	0.0f,
								 0.3f,  1.0f, -0.6f,		-1.0f, 0.0f, 0.0f,				0.0f, 	1.0f,
								 0.3f, -0.5f, -0.4f,		-1.0f, 0.0f, 0.0f,				0.25f, 	0.0f,
								 0.3f,  1.0f, -0.4f,		-1.0f, 0.0f, 0.0f,				0.25f, 	1.0f,
								 0.3f,  1.0f, -0.6f,		-1.0f, 0.0f, 0.0f,				0.0f, 	1.0f,

							   //back right leg, front		//Positive Z Normals
								 0.3f, -0.5f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.0f, 	0.0f,
								 0.5f, -0.5f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.25f, 	0.0f,
								 0.3f,  1.0f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.0f, 	1.0f,
								 0.5f, -0.5f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.25f, 	0.0f,
								 0.5f,  1.0f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.25f, 	1.0f,
								 0.3f,  1.0f, -0.4f,		 0.0f, 0.0f, 1.0f,				0.0f, 	1.0f,

							   //Back right leg, right		//Positive X Normals
								 0.5f, -0.5f, -0.6f,		 1.0, 0.0f, 0.0f,				0.25f, 	0.0f,
								 0.5f, -0.5f, -0.4f,		 1.0, 0.0f, 0.0f,				0.0f, 	0.0f,
								 0.5f,  1.0f, -0.6f,		 1.0, 0.0f, 0.0f,				0.25f, 	1.0f,
								 0.5f, -0.5f, -0.4f,		 1.0, 0.0f, 0.0f,				0.0f, 	0.0f,
								 0.5f,  1.0f, -0.4f,		 1.0, 0.0f, 0.0f,				0.0f, 	1.0f,
								 0.5f,  1.0f, -0.6f,		 1.0, 0.0f, 0.0f,				0.25f, 	1.0f,

							    //Back right leg, back		//Negative Z Normals
								 0.5f, -0.5f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.0f, 	0.0f,
								 0.3f, -0.5f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.25f,  0.0f,
								 0.5f,  1.0f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.0f, 	1.0f,
								 0.3f, -0.5f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.25f, 	0.0f,
								 0.3f,  1.0f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.25f, 	1.0f,
								 0.5f,  1.0f, -0.6f,		 0.0f, 0.0f, -1.0f,				0.0f, 	1.0f,

							  //Back right leg, bottom		//Negative Y Normals
								 0.5f, -0.5f, -0.6f,		 0.0f, -1.0f, 0.0f,				0.2f, 	0.0f,
								 0.3f, -0.5f, -0.6f,		 0.0f, -1.0f, 0.0f,				0.0f, 	0.0f,
								 0.5f, -0.5f, -0.4f,		 0.0f, -1.0f, 0.0f,				0.2f, 	0.2f,
								 0.3f, -0.5f, -0.6f,		 0.0f, -1.0f, 0.0f,				0.0f, 	0.0f,
								 0.3f, -0.5f, -0.4f,		 0.0f, -1.0f, 0.0f,				0.0f, 	0.2f,
								 0.5f, -0.5f, -0.4f,		 0.0f, -1.0f, 0.0f,				0.2f, 	0.2f,

							   //Back right leg, top		//Positive Y Normals
								 0.5f,  1.0f, -0.6f,		  0.0f, 1.0f, 0.0f,				0.2f, 	0.2f,
								 0.3f,  1.0f, -0.6f,		  0.0f, 1.0f, 0.0f,				0.0f, 	0.2f,
								 0.5f,  1.0f, -0.4f,		  0.0f, 1.0f, 0.0f, 			0.2f, 	0.0f,
								 0.3f,  1.0f, -0.6f,	  	  0.0f, 1.0f, 0.0f,				0.0f, 	0.2f,
								 0.3f,  1.0f, -0.4f,		  0.0f, 1.0f, 0.0f,				0.0f, 	0.0f,
								 0.5f,  1.0f, -0.4f,		  0.0f, 1.0f, 0.0f,				0.2f, 	0.0f,

								//Back rest, front			//Positive Z Normals
								-0.3f,  1.0f, -0.45f,	 	 0.0f, 0.0f, 1.0f,				0.0f, 	1.0f,
								 0.3f,  1.0f, -0.45f,	 	 0.0f, 0.0f, 1.0f, 				1.0f, 	1.0f,
								-0.3f,  0.3f, -0.45f,	 	 0.0f, 0.0f, 1.0f, 				0.0f, 	0.0f,
								-0.3f,  0.3f, -0.45f,	 	 0.0f, 0.0f, 1.0f,				0.0f, 	0.0f,
								 0.3f,  0.3f, -0.45f,	 	 0.0f, 0.0f, 1.0f,				1.0f, 	0.0f,
								 0.3f,  1.0f, -0.45f,	 	 0.0f, 0.0f, 1.0f,				1.0f, 	1.0f,

								//Back rest, Back			//Negative Z Normals
								-0.3f,  1.0f, -0.55f,	 	 0.0f, 0.0f, -1.0f, 			1.0f, 	1.0f,
								 0.3f,  1.0f, -0.55f,	 	 0.0f, 0.0f, -1.0f, 			0.0f, 	1.0f,
								-0.3f,  0.3f, -0.55f,	 	 0.0f, 0.0f, -1.0f, 			1.0f, 	0.0f,
								-0.3f,  0.3f, -0.55f,		 0.0f, 0.0f, -1.0f, 			1.0f, 	0.0f,
								 0.3f,  0.3f, -0.55f,	 	 0.0f, 0.0f, -1.0f, 			0.0f, 	0.0f,
								 0.3f,  1.0f, -0.55f,	 	 0.0f, 0.0f, -1.0f, 			0.0f, 	1.0f,

								//Back rest, bottom			//Negative Y Normals
								-0.3f,  0.3f, -0.45f,	 	 0.0f, -1.0f, 0.0f, 			0.0f, 	0.2f,
								-0.3f,  0.3f, -0.55f,	  	 0.0f, -1.0f, 0.0f,				0.0f, 	0.0f,
								 0.3f,  0.3f, -0.45f,	 	 0.0f, -1.0f, 0.0f, 			1.0f, 	0.2f,
								 0.3f,  0.3f, -0.45f,	  	 0.0f, -1.0f, 0.0f, 			1.0f, 	0.2f,
								 0.3f,  0.3f, -0.55f,	 	 0.0f, -1.0f, 0.0f,				1.0f, 	0.0f,
								-0.3f,  0.3f, -0.55f,	 	 0.0f, -1.0f, 0.0f, 			0.0f, 	0.0f,

								 //Back rest, Top			//Positive Y Normals
								-0.3f,  1.0f, -0.45f,	 	 0.0f, 1.0f, 0.0f, 				0.0f, 	0.0f,
								-0.3f,  1.0f, -0.55f,	 	 0.0f, 1.0f, 0.0f,				0.0f, 	0.2f,
								 0.3f,  1.0f, -0.45f,	 	 0.0f, 1.0f, 0.0f,				1.0f, 	0.0f,
								 0.3f,  1.0f, -0.45f,	 	 0.0f, 1.0f, 0.0f,				1.0f, 	0.0f,
								 0.3f,  1.0f, -0.55f,		 0.0f, 1.0f, 0.0f,				1.0f, 	0.2f,
								-0.3f,  1.0f, -0.55f,		 0.0f, 1.0f, 0.0f,				0.0f, 	0.2f,
							};

	//Generate buffer ids
		glGenVertexArrays(1, &ChairVAO);
		glGenBuffers(1, &VBO);

		// Activate the VAO before binding and setting VBOs and VAPs
		glBindVertexArray(ChairVAO);

		//Activate the VBO
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // Copy Vertices to VBO

		//Set attribute pointer 0 to hold position data
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	    glEnableVertexAttribArray(0); //Enables vertex attribute

	    //Set attribute pointer 1 to hold Normal data
	    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	    glEnableVertexAttribArray(1);

	    //Set attribute pointer 2 to hold Texture coordinate data
	    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	    glEnableVertexAttribArray(2);

	    glBindVertexArray(0); //Unbind the chair VAO

	    //Generate buffer ids for lamp (smaller chair)
	    glGenVertexArrays(1, &LightVAO); //Vertex Array for chair vertex copies to serve as light source

	    //Activate the Vertex Array Object before binding and setting any VBOs and Vertex Attribute Pointers
	    glBindVertexArray(LightVAO);

	    //Referencing the same VBO for its vertices
	    glBindBuffer(GL_ARRAY_BUFFER, VBO);

	    //Set attribute pointer to 0 to hold Position data (used for the lamp)
	    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	    glEnableVertexAttribArray(0);
	    glBindVertexArray(0);

	}

	/*Generate and load the texture*/
	void UGenerateTexture(){

				glGenTextures(1, &texture);
				glBindTexture(GL_TEXTURE_2D, texture);

				int width, height;

				unsigned char* image = SOIL_load_image("window-iced-over.jpg", &width, &height, 0, SOIL_LOAD_RGB); // Loads texture file

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
				glGenerateMipmap(GL_TEXTURE_2D);
				SOIL_free_image_data(image);
				glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
}


	/* Implements the UMouseMove function*/
	void UMouseMove(int x, int y)
	{

		//Orbits around the center
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

			//Maintains a 90 degree pitch for gimbal lock
				if(pitch > 89.0f)
						pitch = 89.0f;

				if(pitch < -89.0f)
						pitch = -89.0f;

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
