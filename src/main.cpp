/* 
Project 4
*/

#include <iostream>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "Shape.h"
#include "GLTextureWriter.h"
#include "Texture.h"


// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
using namespace glm;

GLFWwindow *window; // Main application window
string RESOURCE_DIR = ""; // Where the resources are loaded from
shared_ptr<Program> prog, tex_prog;
shared_ptr<Program> prog2;
shared_ptr<Texture> texture2;
shared_ptr<Shape> sphere;
shared_ptr<Shape> cube;
shared_ptr<Shape> tree;

int g_width = 512;
int g_height = 512;
double randPos[160];
int gMat = 1;
float Lx = 50, Ly = 50, Lz = 30;
double oldx, oldy;
double phi, theta;
vec3 eye, LA, up;


//global reference to texture FBO
GLuint frameBuf[2];
GLuint texBuf[2];
GLuint depthBuf;

//geometry for texture render
GLuint quad_VertexArrayID;
GLuint quad_vertexbuffer;

int g_GiboLen;

//global data for ground plane
GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;

//forward declaring a useful function listed later
void SetMaterial(int i);

/*
Helper function to create the framebuffer object and associated texture to write to
*/
void createFBO(GLuint& fb, GLuint& tex) {
    //initialize FBO (global memory)
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

	//set up framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, fb);
    //set up texture
    glBindTexture(GL_TEXTURE_2D, tex);

   	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
   	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

   	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    	cout << "Error setting up frame buffer - exiting" << endl;
    	exit(0);
	}
}

/* code to define the ground plane */
static void initGeom() {

   	float g_groundSize = 20;
   	float g_groundY = -1.5;

 	// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
    float GrndPos[] = {
	    -g_groundSize, g_groundY, -g_groundSize,
	    -g_groundSize, g_groundY,  g_groundSize,
	    g_groundSize, g_groundY,  g_groundSize,
	    g_groundSize, g_groundY, -g_groundSize
    };

    float GrndNorm[] = {
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0,
		0, 1, 0
    };

  	static GLfloat GrndTex[] = {
   		0, 0, // back
   		0, 10,
    	10, 10,
    	10, 0 
	};

   	unsigned short idx[] = {0, 1, 2, 0, 2, 3};

   	GLuint VertexArrayID;
	//generate the VAO
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

    g_GiboLen = 6;
    glGenBuffers(1, &GrndBuffObj);
    glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

    glGenBuffers(1, &GrndNorBuffObj);
    glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);
    
	glGenBuffers(1, &GrndTexBuffObj);
    glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

    glGenBuffers(1, &GIndxBuffObj);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

}

/**** geometry set up for a quad *****/
void initQuad() {

 	//now set up a simple quad for rendering FBO
  	glGenVertexArrays(1, &quad_VertexArrayID);
  	glBindVertexArray(quad_VertexArrayID);

  	static const GLfloat g_quad_vertex_buffer_data[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f,  1.0f, 0.0f,
  	};

  	glGenBuffers(1, &quad_vertexbuffer);
  	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
  	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

}

/* lots of initialization to set up the opengl state and data */
static void initGL()
{
	GLSL::checkVersion();
  	int width, height;
  	glfwGetFramebufferSize(window, &width, &height);

	oldx = 0;
	oldy = 0;
	phi = 0;
	theta = 0;
	LA = vec3(0, 0, -5);
	eye = vec3(0, 0, 0);
	up = vec3(0, 1, 0);


	// Set background color.
	glClearColor(.12f, .34f, .56f, 1.0f);
	// Enable z-buffer test.
	glEnable(GL_DEPTH_TEST);

	// Initialize the obj mesh VBOs etc
	sphere = make_shared<Shape>();
	sphere->loadMesh(RESOURCE_DIR + "penguinsphere.obj");
	sphere->resize();
	sphere->init();

	cube = make_shared<Shape>();
	cube->loadMesh(RESOURCE_DIR + "cube.obj");
	cube->resize();
	cube->init();

	tree = make_shared<Shape>();
	tree->loadMesh(RESOURCE_DIR + "lowpolytree.obj");
	tree->resize();
	tree->init();

	//Initialize the geometry to render a quad to the screen
	initQuad();
	initGeom();

	// Initialize the GLSL program to render the obj
	prog = make_shared<Program>();
	prog->setVerbose(true);
	prog->setShaderNames(RESOURCE_DIR + "simple_vert.glsl", RESOURCE_DIR + "simple_frag.glsl");
	prog->init();
	prog->addUniform("P");
	prog->addUniform("M");
	prog->addUniform("V");
	prog->addUniform("LPos");
	prog->addUniform("MatAmb");
	prog->addUniform("MatDif");
	prog->addUniform("MatSpec");
	prog->addUniform("shine");
	prog->addAttribute("vertPos");
	prog->addAttribute("vertNor");

	//create two frame buffer objects to toggle between
  	glGenFramebuffers(2, frameBuf);
  	glGenTextures(2, texBuf);
	glGenRenderbuffers(1, &depthBuf);
//	createFBO(frameBuf[0], texBuf[0]);

  	//set up depth necessary since we are rendering a mesh that needs depth test
  	glBindRenderbuffer(GL_RENDERBUFFER, depthBuf);
  	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
  	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuf);
	tex_prog = make_shared<Program>();
	tex_prog->setVerbose(true);
	tex_prog->setShaderNames(RESOURCE_DIR + "pass_vert.glsl", RESOURCE_DIR + "tex_frag.glsl");
	tex_prog->init();
	tex_prog->addUniform("texBuf");
	tex_prog->addAttribute("vertPos");

	prog2 = make_shared<Program>();
	prog2->setVerbose(true);
	prog2->setShaderNames(RESOURCE_DIR + "tex_vert.glsl", RESOURCE_DIR + "tex_frag2.glsl");
	prog2->init();

	texture2 = make_shared<Texture>();
   	texture2->setFilename(RESOURCE_DIR + "snow_1.jpg");
   	texture2->init();
   	texture2->setUnit(2);
   	texture2->setWrapModes(GL_MIRRORED_REPEAT, GL_MIRRORED_REPEAT);

   	prog2->addUniform("P");
	prog2->addUniform("M");
	prog2->addAttribute("vertPos");
   	prog2->addAttribute("vertNor");
	prog2->addAttribute("vertTex");
   	prog2->addUniform("Texture2");

}

/* The render loop - this function is called repeatedly during the OGL program run */
static void render()
{
	// Get current frame buffer size.
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	// Clear framebuffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* Leave this code to just draw the meshes alone */
	//Use the matrix stack for Lab 6
    float aspect = width/(float)height;

    // Create the matrix stacks - please leave these alone for now
    auto P = make_shared<MatrixStack>();
    auto M = make_shared<MatrixStack>();
    // Apply perspective projection.
   	P->pushMatrix();
   		P->perspective(45.0f, aspect, 0.01f, 100.0f);

		// Draw our scene - two meshes
		prog->bind();
		glUniform3f(prog->getUniform("LPos"), Lx, Ly, Lz);
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(P->topMatrix()));

		LA = vec3((cos(glm::radians(phi)) * cos(glm::radians(theta))),
		  (sin(glm::radians(phi))),
		  (cos(glm::radians(phi)) * cos((3.14159265359 / 2) - glm::radians(theta))));

		glm::mat4 V = lookAt(eye, eye + LA, up);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE,value_ptr(V));

		//globl transforms for 'camera'
	  	M->pushMatrix();
	     	M->loadIdentity();

		 	for (int i = 0; i < 40; i++){
			 	M->pushMatrix();
		   			M->translate(vec3(randPos[i * 3 + 0], 0, randPos[i * 3 + 1]));
		   			SetMaterial(randPos[i * 3 + 2]);

			  		/* Draw Penguins */
			  		if (i < 20) {
			  			M->translate(vec3(0, -.5, 0));
			  			M->rotate(randPos[i * 3 + 0], vec3(0, 1, 0));
			  			M->scale(vec3(.75, .75, .75));
						  	/* Body */	
							M->pushMatrix();
					  			/* Left Eye */
								M->pushMatrix();
									M->translate(vec3(-.2, .3, .85));
									M->scale(vec3(.1, .1, .1));
								  	SetMaterial(1);
							  	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
							  	  	sphere->draw(prog);
								M->popMatrix();

								/* Right Eye */
								M->pushMatrix();
									M->translate(vec3(.2, .3, .85));
									M->scale(vec3(.1, .1, .1));
								  	SetMaterial(1);
							  	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
							  	  	sphere->draw(prog);
								M->popMatrix();

								/* Nose */
								M->pushMatrix();
									M->translate(vec3(0, .1, .5));
									M->scale(vec3(.1, .1, .75));
								  	SetMaterial(2);
							  	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
							  	  	sphere->draw(prog);
								M->popMatrix();

								/* Left Wing */
								M->pushMatrix();
									M->translate(vec3(-1, -.45, .2));
									M->rotate(-.1, vec3(0, 0, 1));
									M->scale(vec3(.18, .9, .5));
								  	SetMaterial(1);
							  	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
							  	  	sphere->draw(prog);
								M->popMatrix();

								/* Right Wing */
								M->pushMatrix();
									M->translate(vec3(1, -.45, .2));
									M->rotate(.1, vec3(0, 0, 1));
									M->scale(vec3(.18, .9, .5));
								  	SetMaterial(1);
							  	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
							  	  	sphere->draw(prog);
								M->popMatrix();

								/* Left Foot */
								M->pushMatrix();
									M->translate(vec3(-.35, -1.4, .05));
									M->rotate(-.15, vec3(0, 1, 0));
									M->scale(vec3(.25, .10, .49));
								  	SetMaterial(2);
							  	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
							  	  	cube->draw(prog);
								M->popMatrix();

								/* Right Foot */
								M->pushMatrix();
									M->translate(vec3(.35, -1.4, .05));
									M->rotate(.15, vec3(0, 1, 0));
									M->scale(vec3(.25, .10, .49));
								  	SetMaterial(2);
							  	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
							  	  	cube->draw(prog);
								M->popMatrix();

								/* Body */
							  	M->scale(vec3(1, 1.4, 1));
							  	SetMaterial(0);
						  	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()) );
						  	  	sphere->draw(prog);
					   		M->popMatrix();		 
			  		}
			  		else {
			  			M->scale(vec3(1.5, 1.5, 1.5));
			  			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()));
			  			tree->draw(prog);
			  		}
		   		M->popMatrix();
		 	}
		 	M->translate(vec3(0, -2, 0));
		 	M->scale(vec3(500, .1, 500));
		 	SetMaterial(1);
	  	  	glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE,value_ptr(M->topMatrix()) );
	  	  	cube->draw(prog);
	   	M->popMatrix();

   	P->popMatrix();
	prog->unbind();

}

/* helper function */
static void error_callback(int error, const char *description)
{
	cerr << description << endl;
}

/* key callback */
static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	float speed = .1;

	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
	else if (key == GLFW_KEY_W) {	// forward
		eye += speed * LA;
	}
	else if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) {	// back
		eye -= speed * LA;
	}
	else if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT)) {	// left
		eye -= normalize(cross(LA, up)) * speed;
	}
	else if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT)) {	// right
		eye += normalize(cross(LA, up)) * speed;
	}

}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {

	double deltax = xpos - oldx;
	double deltay = ypos - oldy;

	theta += deltax * 3.14159265359 / g_width;
	phi += deltay * 3.14159265359 / g_height;

	if (phi >= 80) {
		phi = 80;
	}
	if (phi <= -80) {
		phi = -80;
	}

	oldx = xpos;
	oldy = ypos;
}

/* resize window call back */
static void resize_callback(GLFWwindow *window, int width, int height) {
   g_width = width;
   g_height = height;
   glViewport(0, 0, width, height);
}

//helper function to set materials for shadin
void SetMaterial(int i) {
  switch (i) {
    case 0: //shiny blue plastic
 		glUniform3f(prog->getUniform("MatAmb"), 0.02, 0.04, 0.2);
 		glUniform3f(prog->getUniform("MatDif"), 0.0, 0.16, 0.9);
 		glUniform3f(prog->getUniform("MatSpec"), 0.14, 0.2, 0.8); 
 		glUniform1f(prog->getUniform("shine"), 120.0);
      break;
    case 1: // flat grey
 		glUniform3f(prog->getUniform("MatAmb"), 0.13, 0.13, 0.14);
 		glUniform3f(prog->getUniform("MatDif"), 0.3, 0.3, 0.4);
 		glUniform3f(prog->getUniform("MatSpec"), 0.3, 0.3, 0.4); 
 		glUniform1f(prog->getUniform("shine"), 4.0);
      break;
    case 2: //brass
 		glUniform3f(prog->getUniform("MatAmb"), 0.3294, 0.2235, 0.02745);
 		glUniform3f(prog->getUniform("MatDif"), 0.7804, 0.5686, 0.11373);
 		glUniform3f(prog->getUniform("MatSpec"), 0.9922, 0.941176, 0.80784); 
 		glUniform1f(prog->getUniform("shine"), 27.9);
      break;
	 case 3: //copper
 		glUniform3f(prog->getUniform("MatAmb"), 0.1913, 0.0735, 0.0225);
 		glUniform3f(prog->getUniform("MatDif"), 0.7038, 0.27048, 0.0828);
 		glUniform3f(prog->getUniform("MatSpec"), 0.257, 0.1376, 0.08601); 
 		glUniform1f(prog->getUniform("shine"), 12.8);
      break;
	}
}

int main(int argc, char **argv)
{
	if(argc < 2) {
		cout << "Please specify the resource directory." << endl;
		return 0;
	}
	RESOURCE_DIR = argv[1] + string("/");

	// Set error callback.
	glfwSetErrorCallback(error_callback);
	// Initialize the library.
	if(!glfwInit()) {
		return -1;
	}
    //request the highest possible version of OGL - important for mac
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	// Create a windowed mode window and its OpenGL context.
	window = glfwCreateWindow(g_width, g_height, "P4", NULL, NULL);
	if(!window) {
		glfwTerminate();
		return -1;
	}
	// Make the window's context current.
	glfwMakeContextCurrent(window);
	// Initialize GLEW.
	glewExperimental = true;
	if(glewInit() != GLEW_OK) {
		cerr << "Failed to initialize GLEW" << endl;
		return -1;
	}
	//weird bootstrap of glGetError
    glGetError();
	cout << "OpenGL version: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

	// Set vsync.
	glfwSwapInterval(1);
	// Set keyboard callback.
	glfwSetKeyCallback(window, key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(window, g_width/2.0, g_height/2.0);
	glfwGetCursorPos(window, &oldx, &oldy);
	glfwSetCursorPosCallback(window, cursor_position_callback);
    //set the window resize call back
    glfwSetFramebufferSizeCallback(window, resize_callback);

	// Initialize scene. Note geometry initialized in init now
	initGL();

	// Generate positioning for 40 different objects
	for (int i = 0; i < 40; i++) {
		randPos[i * 3 + 0] = rand() % (20 + 20 + 1) - 20;	// randX
	 	randPos[i * 3 + 1] = rand() % (20 + 20 + 1) - 20;	// randY
	 	randPos[i * 3 + 2] = rand() % (3 - 0 + 1) + 0;		// randMaterial
	 	randPos[i * 3 + 3] = rand() % (360 - 0 + 1) + 0;	// Individual rotation
	}

	// Loop until the user closes the window.
	while(!glfwWindowShouldClose(window)) {
		// Render scene.
		render();
		// Swap front and back buffers.
		glfwSwapBuffers(window);
		// Poll for and process events.
		glfwPollEvents();
	}
	// Quit program.
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
