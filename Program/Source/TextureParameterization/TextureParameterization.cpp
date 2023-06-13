#include <Common.h>
#include <ViewManager.h>
#include <AntTweakBar/AntTweakBar.h>
#include <ResourcePath.h>

#include "OpenMesh.h"
#include "MeshObject.h"
#include "DrawModelShader.h"
#include "PickingShader.h"
#include "PickingTexture.h"
#include "DrawPickingFaceShader.h"
#include "DrawTextureShader.h"
#include "DrawPointShader.h"
#include "../../Include/OpenMesh/Core/IO/MeshIO.hh"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../Include/STB/stb_image_write.h"
#include "../../Include/STB/stb_image.h"
using namespace glm;
using namespace std;


int mainWindow, win1, win2;








glm::vec3 worldPos;
bool updateFlag = false;
bool isRightButtonPress = false;
GLuint currentFaceID = 0;
int currentMouseX = 0;
int currentMouseY = 0;
int windowWidth = 1200;
int windowHeight = 600;
const std::string ProjectName = "TextureParameterization";

GLuint			program;			// shader program
mat4			proj_matrix;		// projection matrix
float			aspect;
ViewManager		meshWindowCam;

MeshObject model;

// shaders
DrawModelShader drawModelShader;
DrawPickingFaceShader drawPickingFaceShader;
PickingShader pickingShader;
PickingTexture pickingTexture;
DrawPointShader drawPointShader;
ShaderObject RightWindowObject;
GLuint texture;
GLuint RightWindowVBO;
GLuint RightWindowEBO;
GLuint RightWindowVAO;
// vbo for drawing point
GLuint vboPoint;
float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
	// positions   // texCoords
	-0.5f,  0.5f,     1.0f, 1.0f,   // 右上
	 0.5f, -0.5f,      1.0f, 0.0f,   // 右下
	-0.5f, -0.5f,     0.0f, 0.0f,   // 左下
	-0.5f,  0.5f,       0.0f, 1.0f    // 左上
};

float RightWindowData[] = {
		0.8f,-0.8f, 1.0f, 0.0f,
	   -0.8f,-0.8f, 0.0f, 0.0f,
	   -0.8f, 0.8f, 0.0f, 1.0f,
		0.8f, 0.8f, 1.0f, 1.0f
};




enum SelectionMode
{
	ADD_FACE,
	DEL_FACE,
	SELECT_POINT,
	OUTPUT,
};
enum TEXTURE
{
	DEFAULT,
	
};
SelectionMode selectionMode = ADD_FACE;
TEXTURE texTure = DEFAULT;
TwBar* bar;

TwEnumVal SelectionModeEV[] = { {ADD_FACE, "Add face"}, {DEL_FACE, "Delete face"}, {SELECT_POINT, "Point"}, { OUTPUT,"outputmesh"}};
TwType SelectionModeType;
TwEnumVal texTureEV[] = { {DEFAULT, "Default"} };
TwType texTureType;
void SetupGUI()
{
#ifdef _MSC_VER
	TwInit(TW_OPENGL, NULL);
#else
	TwInit(TW_OPENGL_CORE, NULL);
#endif
	TwGLUTModifiersFunc(glutGetModifiers);
	bar = TwNewBar("Texture Parameter Setting");
	TwDefine(" 'Texture Parameter Setting' size='220 90' ");
	TwDefine(" 'Texture Parameter Setting' fontsize='3' color='96 216 224'");

	// Defining season enum type
	SelectionModeType = TwDefineEnum("SelectionModeType", SelectionModeEV, 4);
	texTureType = TwDefineEnum("texTureType", texTureEV, 1);
	// Adding season to bar
	TwAddVarRW(bar, "SelectionMode", SelectionModeType, &selectionMode, NULL);
	TwAddVarRW(bar, "TexTure", texTureType, &texTure, NULL);
}

void My_LoadModel()
{
	if (model.Init(ResourcePath::modelPath))
	{
		/*int id = 0;
		while (model.AddSelectedFace(id))
		{
			++id;
		}
		model.Parameterization();
		drawTexture = true;*/

		puts("Load Model");
	}
	else
	{
		puts("Load Model Failed");
	}
}

void InitOpenGL()
{
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_POINT_SMOOTH);
}

void InitData()
{
	ResourcePath::shaderPath = "./Shader/" + ProjectName + "/";
	ResourcePath::imagePath = "./Imgs/" + ProjectName + "/";
	ResourcePath::modelPath = "./Model/armadillo.obj";

	//Initialize shaders
	///////////////////////////	
	drawModelShader.Init();
	pickingShader.Init();
	pickingTexture.Init(windowWidth, windowHeight);
	drawPickingFaceShader.Init();
	drawPointShader.Init();

	
	RightWindowObject.Init();
	RightWindowObject.AddShader(GL_VERTEX_SHADER, "./Shader/TextureParameterization/rightwindow.vs.glsl");
	RightWindowObject.AddShader(GL_FRAGMENT_SHADER, "./Shader/TextureParameterization/rightwindow.fs.glsl");
	RightWindowObject.Finalize();
	RightWindowObject.Enable();
	glGenBuffers(1, &RightWindowVBO);
	glBindBuffer(GL_ARRAY_BUFFER, RightWindowVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(RightWindowData), RightWindowData, GL_STATIC_DRAW);
	glGenVertexArrays(1, &RightWindowVAO);
	glBindVertexArray(RightWindowVAO);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(sizeof(float) * 2));
	glEnableVertexAttribArray(1);
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RightWindowEBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	// 設置參數
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// 生成紋理
	int width, height, nrChannels;
	unsigned char* data = stbi_load("./Imgs/TextureParameterization/checkerboard4.jpg", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	glGenBuffers(1, &vboPoint);

	//Load model to shader program
	My_LoadModel();
}

void Reshape(int width, int height)
{
	windowWidth = width;
	windowHeight = height;

	TwWindowSize(width, height);
	glutReshapeWindow(windowWidth, windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);

	aspect = windowWidth * 1.0f / windowHeight;
	meshWindowCam.SetWindowSize(windowWidth, windowHeight);
	pickingTexture.Init(windowWidth, windowHeight);
}

// GLUT callback. Called to draw the scene.
void RenderMeshWindow()
{
	glutSetWindow(win1);
	//Update shaders' input variable
	///////////////////////////
	glm::mat4 mvMat = meshWindowCam.GetViewMatrix() * meshWindowCam.GetModelMatrix();
	glm::mat4 pMat = meshWindowCam.GetProjectionMatrix(aspect);

	// write faceID+1 to framebuffer
	pickingTexture.Enable();

	glClearColor(0.0f, 1.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	pickingShader.Enable();
	pickingShader.SetMVMat(value_ptr(mvMat));
	pickingShader.SetPMat(value_ptr(pMat));

	model.Render();

	pickingShader.Disable();
	pickingTexture.Disable();

	
	// draw model
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawModelShader.Enable();
	glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));

	drawModelShader.SetWireColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	drawModelShader.SetFaceColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	drawModelShader.UseLighting(true);
	drawModelShader.DrawWireframe(false);
	drawModelShader.SetNormalMat(normalMat);
	drawModelShader.SetMVMat(mvMat);
	drawModelShader.SetPMat(pMat);
	
	model.Render();

	drawModelShader.Disable();
	
	// render selected face
	if (selectionMode == SelectionMode::ADD_FACE || selectionMode == SelectionMode::DEL_FACE)
	{
		drawPickingFaceShader.Enable();
		drawPickingFaceShader.SetMVMat(value_ptr(mvMat));
		drawPickingFaceShader.SetPMat(value_ptr(pMat));
		model.RenderSelectedFace();
		drawPickingFaceShader.Disable();
	}

	glUseProgram(0);

	// render closest point
	if (selectionMode == SelectionMode::SELECT_POINT)
	{
		if (updateFlag)
		{
			float depthValue = 0;
			int windowX = currentMouseX;
			int windowY = windowHeight - currentMouseY;
			glReadPixels(windowX, windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthValue);

			GLint _viewport[4];
			glGetIntegerv(GL_VIEWPORT, _viewport);
			glm::vec4 viewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
			glm::vec3 windowPos(windowX, windowY, depthValue);
			glm::vec3 wp = glm::unProject(windowPos, mvMat, pMat, viewport);
			model.FindClosestPoint(currentFaceID - 1, wp, worldPos);

			updateFlag = false;
		}
		/*
			Using OpenGL 1.1 to draw point
		*/
		/*glPushMatrix();
		glPushAttrib(GL_POINT_BIT);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMultMatrixf(glm::value_ptr(pMat));

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glMultMatrixf(glm::value_ptr(mvMat));

			glPointSize(15.0f);
			glColor3f(1.0, 0.0, 1.0);
			glBegin(GL_POINTS);
			glVertex3fv(glm::value_ptr(worldPos));
			glEnd();
		glPopAttrib();
		glPopMatrix();*/

		
		glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), glm::value_ptr(worldPos), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glm::vec4 pointColor(1.0, 0.0, 1.0, 1.0);
		drawPointShader.Enable();
		drawPointShader.SetMVMat(mvMat);
		drawPointShader.SetPMat(pMat);
		drawPointShader.SetPointColor(pointColor);
		drawPointShader.SetPointSize(15.0);

		glDrawArrays(GL_POINTS, 0, 1);

		drawPointShader.Disable();

		glBindBuffer(GL_ARRAY_BUFFER, 0);

	}

	TwDraw();
	glutSwapBuffers();
}

void RenderAll()
{
	glutSetWindow(win1);
	RenderMeshWindow();
}

void RightWindowInit(){
	


}

//Timer event
void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
}

void SelectionHandler(unsigned int x, unsigned int y)
{
	GLuint faceID = pickingTexture.ReadTexture(x, windowHeight - y - 1);
	if (faceID != 0)
	{
		currentFaceID = faceID;
	}

	if (selectionMode == ADD_FACE)
	{
		if (faceID != 0)
		{
			model.AddSelectedFace(faceID - 1);	
			
		}
	}
	else if (selectionMode == DEL_FACE)
	{
		if (faceID != 0)
		{
			model.DeleteSelectedFace(faceID - 1);
		}
	}
	else if (selectionMode == SELECT_POINT)
	{
		currentMouseX = x;
		currentMouseY = y;
		updateFlag = true;
	}
	else if (selectionMode == OUTPUT)
	{
		MyMesh mesh;
		model.CopySelectFace(mesh);
		std::cout << "# Vertices: " << mesh.n_vertices() << std::endl;
		std::cout << "# Edges   : " << mesh.n_faces() << std::endl;
		std::cout << "# Faces   : " << mesh.n_faces() << std::endl;
		OpenMesh::IO::Options wopt;
		wopt += OpenMesh::IO::Options::VertexNormal;
		
		
		if (!OpenMesh::IO::write_mesh(mesh, "out.obj", wopt))
		{
			std::cerr << "Error" << std::endl;
			std::cerr << "Possible reasons:\n";
			std::cerr << "1. Chosen format cannot handle an option!\n";
			std::cerr << "2. Mesh does not provide necessary information!\n";
			std::cerr << "3. Or simply cannot open file for writing!\n";
			
		}
	}
}

//Mouse event
void MyMouse(int button, int state, int x, int y)
{
	if (!TwEventMouseButtonGLUT(button, state, x, y))
	{
		meshWindowCam.mouseEvents(button, state, x, y);

		if (button == GLUT_RIGHT_BUTTON)
		{
			if (state == GLUT_DOWN)
			{
				isRightButtonPress = true;
				SelectionHandler(x, y);
			}
			else if (state == GLUT_UP)
			{
				isRightButtonPress = false;
			}
		}
	}
}

//Keyboard event
void MyKeyboard(unsigned char key, int x, int y)
{
	if (!TwEventKeyboardGLUT(key, x, y))
	{
		meshWindowCam.keyEvents(key);
	}
}
void MyMouseMoving(int x, int y) {
	if (!TwEventMouseMotionGLUT(x, y))
	{
		meshWindowCam.mouseMoveEvent(x, y);

		if (isRightButtonPress)
		{
			SelectionHandler(x, y);
		}
	}
}
void My_Display()
{
	glutPostRedisplay();
	glutSetWindow(mainWindow);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Update cube shader
	///////////////////////////	
	glClearColor(1, 1, 1, 0);
}

void RightWindowDisplay() {
	glutPostRedisplay();
	glutSetWindow(win2);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Update cube shader
	///////////////////////////	
	RightWindowObject.Enable();
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(RightWindowVAO);
	glBindBuffer(GL_ARRAY_BUFFER, RightWindowVBO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

}

void My_Display2()
{
	glutPostRedisplay();
	glutSetWindow(win2);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//Update cube shader
	///////////////////////////	
	glClearColor(0, 1, 0,1);
	glColor3f(0, 1, 0);
	glBegin(GL_QUADS);
	glVertex2d(-1, -1);
	glVertex2d(1, -1);
	glVertex2d(1, 1);
	glVertex2d(-1, 1);
	glEnd();

	glutSwapBuffers();
}
int main(int argc, char* argv[])
{
#ifdef __APPLE__
	//Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif

	glutInitWindowPosition(100, 100);
	glutInitWindowSize(windowWidth, windowHeight);
	mainWindow = glutCreateWindow(ProjectName.c_str()); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
	
#ifdef _MSC_VER
	glewInit();
#endif
	
	glutDisplayFunc(My_Display);
	
	
	win1 = glutCreateSubWindow(mainWindow, 0, 0, 600, 600);
	glutIdleFunc(RenderAll);
	glutReshapeFunc(Reshape);
	glutSetOption(GLUT_RENDERING_CONTEXT, GLUT_USE_CURRENT_CONTEXT);
	SetupGUI();
	glutMouseFunc(MyMouse);
	glutKeyboardFunc(MyKeyboard);
	glutMotionFunc(MyMouseMoving);
	glutDisplayFunc(RenderMeshWindow);


	win2 = glutCreateSubWindow(mainWindow, 600, 0, 600, 600);
	glutDisplayFunc(RightWindowDisplay);
	glutMouseFunc(MyMouse);
	glutKeyboardFunc(MyKeyboard);
	glutMotionFunc(MyMouseMoving);



	win2= glutCreateSubWindow(mainWindow, 600, 0, 600, 600);
	glutMouseFunc(MyMouse);
	glutKeyboardFunc(MyKeyboard);
	glutMotionFunc(MyMouseMoving);
	glutDisplayFunc(My_Display2);

	InitOpenGL();
	InitData();

	//Print debug information 
	Common::DumpInfo();

	// Enter main event loop.
	glutMainLoop();

	return 0;
}

