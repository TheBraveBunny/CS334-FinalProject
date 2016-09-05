/**
*	CS 334 - Fundamentals of Computer Graphics
*	Framework for assignment 2
*
*	Based on:
*		http://antongerdelan.net/opengl/cubemaps.html
*		https://github.com/capnramses/antons_opengl_tutorials_book/tree/master/21_cube_mapping
*
*	Instructions:
*	- Use Arrow Keys, q, w, e, a, s, d, z, x and c
*	- Press ESC to exit
*/

#include <iostream>
#include <fstream>
#include <string>
#include <assert.h>
#include <ctime>

#include "GL/glew.h"
#include "GL/glut.h"

#include "stb_image.h"
#include "math_funcs.h"
#include "obj_parser.h"
#include "gl_utils.h"

#define MESH_FILE_SUZANNE "suzanne.obj"
#define MESH_FILE "sphere.obj"
#define MONKEY_VERT_FILE "reflect_vs.glsl"
#define MONKEY_FRAG_FILE "reflect_fs.glsl"
#define MONKEY_REFRACT_VERT_FILE "refract_vs.glsl"
#define MONKEY_REFRACT_FRAG_FILE "refract_fs.glsl"

/* Window information */
float windowWidth = 1200;
float windowHeight = 800;

/* Constant values */
const float PI = 3.14159265359; 

char mode = 'b';
int displayRoute = -1;

struct hold {
	bool isHole;
	char *holdType;
	int xIndex;
	int yIndex;
	int *routesUsing;
	int numRoutesUsing;
};

struct route {
	route *next;
	int rating;
	hold **routeHolds;
	char *theme;
	int **importantPositions;
};

struct wall {
	int height;
	int width;

	route **routeList;
	int numberOfRoutes;

	hold **holes;
};

float routeColors[10][3] {
	{1.0, 0.0, 0.0},
	{ 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 1.0 },
	{ 1.0, 1.0, 0.0 },
	{ 1.0, 0.0, 1.0 },
	{ 0.0, 1.0, 1.0 },
	{ 1.0, 1.0, 1.0 },
	{ 139.0 / 255.0, 69.0 / 255.0, 19.0 / 255.0 },
	{ 1.0, 20.0 / 255.0, 147.0 / 255.0 },
	{ 1.0, 165.0 / 255.0, 0.0 }
};

char *theme[6] {
	"jug",
		"globe",
		"crimper",
		"sloper",
		"fingerPocket",
		"pinch"
};

char *types[11] {
	"jug",
		"leftPull",
		"rightPull",
		"undercling",
		"globe",
		"crimper",
		"sloper",
		"sloper_45deg",
		"sloper_neg45deg",
		"fingerPocket",
		"pinch"
};

wall *currentWall;
int maxRoutesPerWall = 10;
int maxRoutesPerHold = 3;

GLfloat myModelMat[4][4] = {
	{ 1, 0, 0, 0 },
	{ 0, 1, 0, 0 },
	{ 0, 0, 1, 0 },
	{ 0, 0, -60, 1 }
};

/* Field of view, aspect and near/far planes for the perspective projection */
float fovy = 45.0;
float aspect = windowWidth / windowHeight;
float zNear = 1.0;
float zFar = 100.0;

/* Information about the texture */
const char* textureFile = "crate.jpg";
unsigned char* textureData;
GLint textureDataLocation;
int textureWidth;
int textureHeight;
int textureComp;
GLuint texture;

/* Information about the texture */
const char* whiteFile = "sphere1.bmp";
unsigned char* whiteData;
GLuint white;
GLint whiteDataLocation;
int whiteWidth;
int whiteHeight;
int whiteComp;

/* Information about the normal */
const char* normalFile = "example_normalmap.png";
unsigned char* normalData;
GLint normalDataLocation;
GLint cubeDataLocation;
int normalWidth;
int normalHeight;
int normalComp;
GLuint normal;

GLuint vao;
GLuint vao2;

GLuint cube_sp;
GLuint cube_map_texture;
GLuint cube_vao;

GLuint monkey_sp;
int monkey_M_location;
int monkey_V_location;
int monkey_P_location;

GLuint suzanne_sp;
int suzanne_M_location;
int suzanne_V_location;
int suzanne_P_location;

int g_point_count = 0;
int g_point_count2 = 0;

const int numOfBugs = 6;
mat4 model_mat[numOfBugs];
int clickedLocation;
bool clicked;
int frameCount[numOfBugs];
float planeAngle = -45.0f;
int score = 0;
int waitFrame = 1000;
int timeElapsed = 0;

float cam_speed = 3.0f; // 1 unit per second
float cam_heading_speed = 5.00f; // 30 degrees per second
float cam_heading = 0.0f; // y-rotation in degrees

mat4 T;
mat4 R;
versor q;

vec4 fwd;
vec4 rgt;
vec4 up;

// camera matrices. it's easier if they are global
mat4 view_mat; 
mat4 proj_mat;
vec3 cam_pos (0.0f, 0.0f, 5.0f);

int cube_V_location;
int cube_P_location;

bool cam_moved = false;
vec3 move;
float cam_yaw = 0.0f; // y-rotation in degrees
float cam_pitch = 0.0f;
float cam_roll = 0.0;

float wallVertices[12];

void drawWall() {
	glBindTexture(GL_TEXTURE_2D, texture);

	float wallVertices[12] =
	{
		-currentWall->width, currentWall->height, 0.0,
		-currentWall->width, -currentWall->height, 0.0,
		currentWall->width, -currentWall->height, 0.0,
		currentWall->width, currentWall->height, 0.0
	};

	glVertexPointer(3, GL_FLOAT, 0, wallVertices);
	glColor3f(0.75, 0.75, 0.75);
	glDrawArrays(GL_POLYGON, 0, 4);
}

void drawHole(int xPos, int yPos, float red, float green, float blue, float radius) {
	xPos -= currentWall->width - 1;
	yPos -= currentWall->height - 1;

	float holeVertices[12] =
	{
		xPos - radius, yPos + radius, 0.1,
		xPos - radius, yPos - radius, 0.1,
		xPos + radius, yPos - radius, 0.1,
		xPos + radius, yPos + radius, 0.1
	};

	glVertexPointer(3, GL_FLOAT, 0, holeVertices);
	glColor3f(red, green, blue);
	glDrawArrays(GL_POLYGON, 0, 4);
}

void drawHolds(int i, int j) {
	int timesUsed = 0;
	if (displayRoute == -1) {
		for (int k = 0; k < maxRoutesPerWall; k++) {
			if (currentWall->holes[i][j].routesUsing[k] != 0) {
				timesUsed++;
				drawHole(i, j, routeColors[k][0], routeColors[k][1], routeColors[k][2], timesUsed*0.2);
			}
		}
	}
	else {
		if (currentWall->holes[i][j].routesUsing[displayRoute] != 0) {
			timesUsed++;
			drawHole(i, j, routeColors[displayRoute][0], routeColors[displayRoute][1], routeColors[displayRoute][2], timesUsed*0.2);
		}
	}
}

void drawHoles() {
	int numberOfColumns = currentWall->height * 2 - 1;
	int numberOfRows = currentWall->width * 2 - 1;

	for (int i = 0; i < numberOfRows; i++) {
		for (int j = 0; j < numberOfColumns; j++) {
			if (currentWall->holes[i][j].isHole) {
				drawHole(i, j, 0.0, 0.0, 0.0, 0.1);
				drawHolds(i, j);
			}
		}
	}
}

void deleteHolds(int index) {
	int numberOfColumns = currentWall->height * 2 - 1;
	int numberOfRows = currentWall->width * 2 - 1;


	for (int i = 0; i < numberOfRows; i++) {
		for (int j = 0; j < numberOfColumns; j++) {
			if (currentWall->holes[i][j].routesUsing[index] > 0) {
				currentWall->holes[i][j].numRoutesUsing--;
			}
			for (int k = index; k < maxRoutesPerWall - 1; k++) {
				currentWall->holes[i][j].routesUsing[k] = currentWall->holes[i][j].routesUsing[k + 1];
			}
			currentWall->holes[i][j].routesUsing[maxRoutesPerWall - 1] = 0;
		}
	}
}

void deleteRoute(int routeIndex) {
	if (routeIndex <= currentWall->numberOfRoutes - 1) {
		deleteHolds(routeIndex);
		for (int k = routeIndex; k < maxRoutesPerWall - 1; k++) {
			currentWall->routeList[routeIndex] = currentWall->routeList[routeIndex + 1];
		}
		currentWall->numberOfRoutes--;
	}
}

void drawText(const char *text, int length, int x, int y) {
	glMatrixMode(GL_PROJECTION);
	double *matrix = new double[16];
	glGetDoublev(GL_PROJECTION_MATRIX, matrix);
	glLoadIdentity();
	glOrtho(0, 800, 0, 600, -5, 5);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();
	glLoadIdentity();
	glRasterPos2i(x, y);
	for (int i = 0; i < length; i++) {
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, (int)text[i]);
	}
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(matrix);
	glMatrixMode(GL_MODELVIEW);
}

void printText() {
	//glUseProgram(0);
	glColor3f(0, 1, 0);
	std::string text = "Mode: ";
	if (mode == 'b') {
		text.append("Add Bouldering Problem");
	}
	else if (mode == 't') {
		text.append("Add Top Rope Route");
	}
	else if (mode == 'v') {
		text.append("View Route");
	}
	else if (mode == 'd') {
		text.append("Delete Route");
	}
	drawText(text.data(), text.size(), 50, 550);

	//glUseProgram(0);
	glColor3f(0, 1, 0);
	std::string timeText;
	char timeBuf[5];
	_itoa(currentWall->numberOfRoutes, timeBuf, 10);
	timeText = "Number of Displayed Routes: ";
	timeText.append(timeBuf);
	drawText(timeText.data(), timeText.size(), 50, 50);

	for (int i = 0; i < currentWall->numberOfRoutes; i++) {
		glColor3f(routeColors[i][0], routeColors[i][1], routeColors[i][2]);
		std::string ratingText = "#";
		char indexBuf[5];
		_itoa(i, indexBuf, 10);
		ratingText.append(indexBuf);

		char ratingBuf[5];
		_itoa(currentWall->routeList[i]->rating, ratingBuf, 10);
		if (currentWall->routeList[i]->rating > 6) {
			ratingText.append(": Rating - 5.");
		}
		else {
			ratingText.append(": Rating - V");
		}
		ratingText.append(ratingBuf);
		drawText(ratingText.data(), ratingText.size(), 500, 550-i*50);
	}
}

/**
*    Function invoked for drawing using OpenGL
*/
void display()
{
	/* Clear the window */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	/* #### Load/set the model view matrix */
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glLoadMatrixf((GLfloat *)myModelMat);

	/* Enable client */
	glEnableClientState(GL_VERTEX_ARRAY);

	drawWall();
	drawHoles();

	printText();

	/* Disable client */
	glDisableClientState(GL_VERTEX_ARRAY);

	/* Force execution of OpenGL commands */
	glFlush();

	/* Swap buffers for animation */
	glutSwapBuffers();
}

/**
*    Function invoked when window system events are not being received
*/
void idle()
{
	/* Redraw the window */
	glutPostRedisplay();
}

int checkForHit(int x, int y) {
	GLuint index;
	glReadPixels(x, windowHeight - y - 1, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);
	return index;
}

char *randomizeTheme(int rating) {
	if ((rating == 0) || (rating == 7) || (rating == 8)) {
		return theme[0];
	}
	else if ((rating == 9) || (rating == 1)) {
		return theme[rand() % 5];
	}
	else {
		return theme[rand() % 6];
	}
}

int* findClosestHole(int x, int y) {
	int *position = (int*)malloc(2 * sizeof(int));

	if (currentWall->holes[x][y].isHole) {
		position[0] = x;
	}
	else if (x > currentWall->width) {
		position[0] = x - 1;
	}
	else {
		position[0] = x + 1;
	}

	position[1] = y;
	return position;
}

int **randomizeImportantPositions() {
	int **positions = (int **)malloc(4 * sizeof(int*));
	int width = 2 * currentWall->width - 2;
	int startingY = 7 + rand() % 2;
	positions[0] = findClosestHole(1 + rand() % width, startingY);
	positions[1] = findClosestHole(1 + rand() % width, currentWall->height / 3.0);
	positions[2] = findClosestHole(1 + rand() % width, currentWall->height*2.0 / 3.0);
	positions[3] = findClosestHole(1 + rand() % width, currentWall->height);
	return positions;
}

char *randomizeHoldType(int rating) {
	if (rating < 9) {
		return types[0];
	}
	else if (rating == 9) {
		return types[rand() % 5];
	}
	else {
		return types[rand() % 6];
	}
}

hold **startRoute(route *newRoute) {
	hold **startingHolds = (hold **)malloc(2 * sizeof(hold*));

	int leftX = newRoute->importantPositions[0][0];
	int leftY = newRoute->importantPositions[0][1];
	startingHolds[0] = &currentWall->holes[leftX][leftY];
	if (!startingHolds[0]->numRoutesUsing) {
		startingHolds[0]->holdType = randomizeHoldType(newRoute->rating);
	}

	int rightX = newRoute->importantPositions[0][0];
	int rightY = newRoute->importantPositions[0][1];
	startingHolds[1] = &currentWall->holes[rightX][rightY];
	if (!startingHolds[1]->numRoutesUsing) {
		startingHolds[1]->holdType = randomizeHoldType(newRoute->rating);
	}

	return startingHolds;
}

char *determineHoldRelationship(char *currentType, char *nextType) {
	if (!strcmp(currentType, "footHold") || !strcmp(nextType, "footHold")) {
		return "impossible";
	}

	if ((!strcmp(currentType, "jug")) || (!strcmp(currentType, "leftPull")) || (!strcmp(currentType, "rightPull")) || (!strcmp(currentType, "undercling"))) {
		if ((!strcmp(nextType, "jug")) || (!strcmp(nextType, "leftPull")) || (!strcmp(nextType, "rightPull")) || (!strcmp(nextType, "undercling"))) {
			return "easy";
		}
		else if ((!strcmp(nextType, "globe")) || (!strcmp(nextType, "sloper")) || (!strcmp(nextType, "sloper_45deg")) || (!strcmp(nextType, "sloper_neg45deg"))) {
			return "medium";
		}
		else if ((!strcmp(nextType, "crimper")) || (!strcmp(nextType, "fingerPocket")) || (!strcmp(nextType, "pinch"))) {
			return "medium";
		}
	}
	else if ((!strcmp(currentType, "globe")) || (!strcmp(currentType, "sloper")) || (!strcmp(currentType, "sloper_45deg")) || (!strcmp(currentType, "sloper_neg45deg"))) {
		if ((!strcmp(nextType, "jug")) || (!strcmp(nextType, "leftPull")) || (!strcmp(nextType, "rightPull")) || (!strcmp(nextType, "undercling"))) {
			return "medium";
		}
		else if ((!strcmp(nextType, "globe")) || (!strcmp(nextType, "sloper")) || (!strcmp(nextType, "sloper_45deg")) || (!strcmp(nextType, "sloper_neg45deg"))) {
			return "medium";
		}
		else if ((!strcmp(nextType, "crimper")) || (!strcmp(nextType, "fingerPocket")) || (!strcmp(nextType, "pinch"))) {
			return "hard";
		}
	}
	else if ((!strcmp(currentType, "crimper")) || (!strcmp(currentType, "fingerPocket")) || (!strcmp(currentType, "pinch"))) {
		if ((!strcmp(nextType, "jug")) || (!strcmp(nextType, "leftPull")) || (!strcmp(nextType, "rightPull")) || (!strcmp(nextType, "undercling"))) {
			return "medium";
		}
		else if ((!strcmp(nextType, "globe")) || (!strcmp(nextType, "sloper")) || (!strcmp(nextType, "sloper_45deg")) || (!strcmp(nextType, "sloper_neg45deg"))) {
			return "hard";
		}
		else if ((!strcmp(nextType, "crimper")) || (!strcmp(nextType, "fingerPocket")) || (!strcmp(nextType, "pinch"))) {
			return "hard";
		}
	}
}

float *calculateMinMaxRadius(route *currentRoute, char *currentType, char *nextType) {
	float *radii = (float*)malloc(2 * sizeof(float));
	float minRadius = 0;
	float maxRadius = 0;
	char *relationship = determineHoldRelationship(currentType, nextType);

	if ((currentRoute->rating == 7) || (currentRoute->rating == 0)) {
		minRadius = 1;
		maxRadius = 2;
	}
	else if (currentRoute->rating == 8) {
		minRadius = 3;
		maxRadius = 5;
	}
	else if ((currentRoute->rating == 9) || (currentRoute->rating == 1)) {
		if (!strcmp(relationship, "easy")) {
			minRadius = 5;
			maxRadius = 7;
		}
		else if (!strcmp(relationship, "medium")) {
			minRadius = 3;
			maxRadius = 5;
		}
		else if (!strcmp(relationship, "hard")) {
			minRadius = 2;
			maxRadius = 4;
		}
	}
	else if ((currentRoute->rating == 10) || (currentRoute->rating == 2)) {
		if (!strcmp(relationship, "easy")) {
			minRadius = 7;
			maxRadius = 9;
		}
		else if (!strcmp(relationship, "medium")) {
			minRadius = 5;
			maxRadius = 7;
		}
		else if (!strcmp(relationship, "hard")) {
			minRadius = 3;
			maxRadius = 5;
		}
	}

	radii[0] = minRadius;
	radii[1] = maxRadius;
	return radii;
}

float calculateMidAngle(hold *currentHand, route *currentRoute, bool nextHand) {
	int nextPos[2];

	if (currentHand->yIndex < currentWall->height / 3.0) {
		nextPos[1] = 1;
	}
	else if (currentHand->yIndex < currentWall->height * 2.0 / 3.0) {
		nextPos[1] = 2;
	}
	else {
		nextPos[1] = 3;
	}

	nextPos[0] = currentRoute->importantPositions[nextPos[1]][0];
	nextPos[1] = currentRoute->importantPositions[nextPos[1]][1];

	float angle = atan2(nextPos[1] - currentHand->yIndex, nextPos[0] - currentHand->xIndex);

	if (nextPos[0] > currentHand->xIndex) {
		if (!nextHand) {
			angle = angle + 0.7854;
		}
	}
	else {
		if (nextHand) {
			angle = angle - 0.7854;
		}
	}

	if ((angle < 0) || (angle > PI)) {
		angle = PI / 2.0;
	}
	return angle;
}

float calculateAngleRange(route *currentRoute, char *currentType, char *nextType) {
	char *relationship = determineHoldRelationship(currentType, nextType);

	if (currentRoute->rating == 7) {
		return atan(0.57735);
	}
	else if (currentRoute->rating == 8) {
		return atan(1.73205);
	}
	else {
		if (!strcmp(relationship, "easy")) {
			return atan(1.73205);
		}
		else if (!strcmp(relationship, "medium")) {
			return atan(1);
		}
		else if (!strcmp(relationship, "hard")) {
			return atan(0.57735);
		}
	}
}

bool passRadii(hold *hand, int i, int j, float *radii) {
	float distance = sqrt(pow(hand->yIndex - j, 2) + pow(hand->xIndex - i, 2));
	return ((distance >= 2 * radii[0]) && (distance <= 2 * radii[1]));
}

bool passAngle(hold *hand, int i, int j, float midAngle, float angleRange) {
	float angle = atan2(j - hand->yIndex, i - hand->xIndex);
	if ((angle < 0) || (angle > PI)) {
		return false;
	}
	return ((angle >= midAngle - angleRange) && (angle <= midAngle + angleRange));
}

bool passFootAngle(hold *hand, int i, int j, float midAngle, float angleRange) {
	float angle = atan2(j - hand->yIndex, i - hand->xIndex);

	return ((angle >= midAngle - angleRange) && (angle <= midAngle + angleRange));
}

bool isPossible(int i, int j, hold *hand, float *radii, float midAngle, float angleRange) {
	int width = 2 * currentWall->width - 1;
	int height = 2 * currentWall->height - 1;
	if ((i > 0) && (j > 0) && (i < width) && (j < height)) {
		bool pass = true;
		pass = pass && currentWall->holes[i][j].isHole;
		if (currentWall->holes[i][j].numRoutesUsing > 0) {
			pass = pass && (strcmp(currentWall->holes[i][j].holdType, "footHold") != 0);
		}
		pass = pass && passRadii(hand, i, j, radii);
		pass = pass && passAngle(hand, i, j, midAngle, angleRange);
		return pass;
	}

	return false;
}

bool checkPossibility(route *currentRoute, hold *possibility, hold *hand, bool nextHand, int i, int j) {
	float *radii = calculateMinMaxRadius(currentRoute, hand->holdType, possibility->holdType);
	float midAngle = calculateMidAngle(hand, currentRoute, nextHand);
	float angleRange = calculateAngleRange(currentRoute, hand->holdType, possibility->holdType);

	return isPossible(i, j, hand, radii, midAngle, angleRange);
}

int *lookForAvailableHold(route *currentRoute, hold *hand, bool nextHand) {
	//give priority to holds currently in route;
	int *availableHold = (int *)malloc(2 * sizeof(int));
	availableHold[0] = -1;
	bool isCrux = (rand() % 4 == 0) && ((currentRoute->rating == 9) || (currentRoute->rating == 10));
	if (isCrux) {
		currentRoute->rating--;
	}

	for (int i = -10; i <= 10; i++) {
		for (int j = -10; j <= 10; j++) {
			int x = hand->xIndex + i;
			int y = hand->yIndex + j;
			if ((x > 0) && (y > 0) && (x < (2 * currentWall->width - 1)) && (y < (currentWall->height - 1))) {
				if ((currentWall->holes[x][y].numRoutesUsing > 0) && (currentWall->holes[x][y].numRoutesUsing < maxRoutesPerHold)) {
					if (checkPossibility(currentRoute, &currentWall->holes[x][y], hand, nextHand, x, y)) {
						availableHold[0] = x;
						availableHold[1] = y;
					}
				}
			}
		}
	}
	if (isCrux) {
		currentRoute->rating++;
	}
	return availableHold;
}

int **findAllPossible(hold *hand, float *radii, float midAngle, float angleRange) {
	int outerRadius = (int)radii[1] + 1;
	int **allPossible = (int **)malloc(outerRadius*outerRadius * sizeof(int *));
	int numberPossible = 0;
	allPossible[0] = (int *)malloc(2 * sizeof(int));

	for (int i = -outerRadius; i <= outerRadius; i++) {
		for (int j = 0; j <= outerRadius; j++) {
			int x = hand->xIndex + i;
			int y = hand->yIndex + j;
			if (isPossible(x, y, hand, radii, midAngle, angleRange)) {
				allPossible[++numberPossible] = (int *)malloc(2 * sizeof(int));
				allPossible[numberPossible][0] = x;
				allPossible[numberPossible][1] = y;
			}
		}
	}

	allPossible[0][0] = numberPossible;
	return allPossible;
}

hold *randomizeHoldPosition(route *currentRoute, hold **hands, bool nextHand, char *holdType) {
	float *radii = calculateMinMaxRadius(currentRoute, hands[!nextHand]->holdType, holdType);
	float midAngle = calculateMidAngle(hands[!nextHand], currentRoute, nextHand);
	float angleRange = calculateAngleRange(currentRoute, hands[!nextHand]->holdType, holdType);

	int **allPossible = findAllPossible(hands[!nextHand], radii, midAngle, angleRange);
	int numberPossible = allPossible[0][0];

	if (numberPossible == 0) {
		int xPos = hands[!nextHand]->xIndex;
		int yPos = hands[!nextHand]->yIndex + calculateMinMaxRadius(currentRoute, hands[!nextHand]->holdType, "jug")[0];
		int max = 2 * currentRoute->importantPositions[3][1] - 2;
		int *pos;
		if (yPos > max) {
			pos = findClosestHole(xPos, max);
		}
		else {
			pos = findClosestHole(xPos, yPos);
		}

		hold *newHold = &currentWall->holes[pos[0]][pos[1]];
		newHold->holdType = "jug";
		return newHold;
	}
	else {
		int random = rand() % numberPossible;
		int xPos = allPossible[random + 1][0];
		int yPos = allPossible[random + 1][1];

		hold *newHold = &currentWall->holes[xPos][yPos];
		newHold->holdType = holdType;
		return newHold;
	}
}

hold *generateNextHandHold(route *currentRoute, hold** hands, bool nextHand) {
	int *index = lookForAvailableHold(currentRoute, hands[!nextHand], nextHand);
	hold *newHold;

	if ((index[0] == -1) || (rand() % 2 == 0)) {
		char *type;
		if (rand() % 2 == 1) {
			type = currentRoute->theme;
		}
		else {
			type = randomizeHoldType(currentRoute->rating);
		}

		newHold = randomizeHoldPosition(currentRoute, hands, nextHand, type);
	}
	else {
		int x = index[0];
		int y = index[1];
		newHold = &currentWall->holes[x][y];
	}

	return newHold;
}

void addHoldsToWall(hold **hands) {
	if (hands[0] != NULL) {
		int xPos = hands[0]->xIndex;
		int yPos = hands[0]->yIndex;
		int currentRoute = currentWall->numberOfRoutes - 1;
		currentWall->holes[xPos][yPos].routesUsing[currentRoute] = 1;
		currentWall->holes[xPos][yPos].numRoutesUsing++;
	}

	if (hands[1] != NULL) {
		int xPos = hands[1]->xIndex;
		int yPos = hands[1]->yIndex;
		int currentRoute = currentWall->numberOfRoutes - 1;
		currentWall->holes[xPos][yPos].routesUsing[currentRoute] = 1;
		currentWall->holes[xPos][yPos].numRoutesUsing++;
	}
}

float *calculateMinMaxRadiusForFoot(route *newRoute, char *relationship) {
	float *radii = (float *)malloc(2 * sizeof(float));

	if ((newRoute->rating == 7) || (newRoute->rating == 0)) {
		radii[0] = 6;
		radii[1] = 8;
	}
	else if (newRoute->rating == 8) {
		if (!strcmp(relationship, "easy")) {
			radii[0] = 6;
			radii[1] = 8;
		}
		else if (!strcmp(relationship, "medium")) {
			radii[0] = 6;
			radii[1] = 8;
		}
		else if (!strcmp(relationship, "hard")) {
			radii[0] = 6;
			radii[1] = 8;
		}
	}
	else if ((newRoute->rating == 9) || (newRoute->rating == 1)) {
		if (!strcmp(relationship, "easy")) {
			radii[0] = 6;
			radii[1] = 8;
		}
		else if (!strcmp(relationship, "medium")) {
			radii[0] = 6;
			radii[1] = 8;
		}
		else if (!strcmp(relationship, "hard")) {
			radii[0] = 6;
			radii[1] = 8;
		}
	}
	else if ((newRoute->rating == 10) || (newRoute->rating == 2)) {
		if (!strcmp(relationship, "easy")) {
			radii[0] = 6;
			radii[1] = 8;
		}
		else if (!strcmp(relationship, "medium")) {
			radii[0] = 6;
			radii[1] = 8;
		}
		else if (!strcmp(relationship, "hard")) {
			radii[0] = 6;
			radii[1] = 8;
		}
	}

	return radii;
}

float calculateMidAngleForFoot(route *newRoute, hold *currentHand, bool nextHand) {
	if ((newRoute->rating == 7) || (newRoute->rating == 8) || (newRoute->rating == 0)) {
		return -1.0*PI / 2.0;
	}
	else if ((newRoute->rating == 9) || (newRoute->rating == 10) || (newRoute->rating == 1) || (newRoute->rating == 2)) {
		float angle = calculateMidAngle(currentHand, newRoute, nextHand);
		angle -= PI;
		return (angle + -1.0*PI / 2.0) / 2.0;
	}
}

float calculateAngleRangeForFoot(route *newRoute, char *relationship) {
	if ((newRoute->rating == 7) || (newRoute->rating == 0)) {
		return PI / 6.0;
	}
	else if ((newRoute->rating == 8) || (newRoute->rating == 9) || (newRoute->rating == 1)) {
		if (!strcmp(relationship, "easy")) {
			return PI / 3.0;
		}
		else if (!strcmp(relationship, "medium")) {
			return PI / 4.0;
		}
		else if (!strcmp(relationship, "hard")) {
			return PI / 6.0;
		}
	}
	else if ((newRoute->rating == 10) || (newRoute->rating == 2)) {
		if (!strcmp(relationship, "easy")) {
			return PI / 3.0;
		}
		else if (!strcmp(relationship, "medium")) {
			return PI / 3.0;
		}
		else if (!strcmp(relationship, "hard")) {
			return PI / 4.0;
		}
	}
}

bool checkPossibleFoot(route *newRoute, float *radii, float midAngle, float angleRange, hold *nextHold, int x, int y) {
	bool pass = true;
	pass = pass && currentWall->holes[x][y].isHole;
	pass = pass && passRadii(nextHold, x, y, radii);
	pass = pass && passFootAngle(nextHold, x, y, midAngle, angleRange);
	pass = pass && (currentWall->holes[x][y].numRoutesUsing < 3);
	return pass;
}

int *lookForAvailableFootHold(route *newRoute, char *relationship, float *radii, float midAngle, float angleRange, int *footHoldIndex, hold *currentHold, hold *nextHold) {
	footHoldIndex[0] = -1;
	int routeNumber = currentWall->numberOfRoutes - 1;

	int outerRadius = (int)radii[1] + 1;
	int **allPossible = (int **)malloc(outerRadius*outerRadius * sizeof(int *));
	int numberPossible = 0;

	for (int i = -15; i <= 15; i++) {
		for (int j = -15; j <= 0; j++) {
			int x = nextHold->xIndex + i;
			int y = nextHold->yIndex + j;
			if ((x > 0) && (y > 0) && (x < (2 * currentWall->width - 1)) && (y < (2 * currentWall->height - 1))) {
				if (checkPossibleFoot(newRoute, radii, midAngle, angleRange, nextHold, x, y)) {
					printf("\tpossible\n");
					if (currentWall->holes[x][y].routesUsing[routeNumber] > 0) {
						footHoldIndex[0] = x;
						footHoldIndex[1] = y;
						printf("\tone currently in route\n");
						free(allPossible);
						return footHoldIndex;
					}
					else if ((currentWall->holes[x][y].numRoutesUsing == 0) || (!strcmp(currentWall->holes[x][y].holdType, "foothold"))) {
						allPossible[numberPossible] = (int*)malloc(2 * sizeof(int));
						allPossible[numberPossible][0] = x;
						allPossible[numberPossible++][1] = y;
					}
				}
			}
		}
	}

	if (numberPossible != 0) {
		printf("\tsome possible\n");
		int random = rand() % numberPossible;
		footHoldIndex[0] = allPossible[random][0];
		footHoldIndex[1] = allPossible[random][1];
	}
	else {
		printf("none possible\n");
	}
	return footHoldIndex;
}

void generateNextFootHold(route *newRoute, hold *currentHold, hold *nextHold, bool nextHand) {
	char *relationship = determineHoldRelationship(currentHold->holdType, nextHold->holdType);
	float *radii = calculateMinMaxRadiusForFoot(newRoute, relationship);
	float midAngle = calculateMidAngleForFoot(newRoute, currentHold, nextHand);
	float angleRange = calculateAngleRangeForFoot(newRoute, relationship);
	int *footHoldIndex = (int *)malloc(2 * sizeof(int));

	footHoldIndex = lookForAvailableFootHold(newRoute, relationship, radii, midAngle, angleRange, footHoldIndex, currentHold, nextHold);

	if (footHoldIndex[0] != -1) {
		int currentRoute = currentWall->numberOfRoutes - 1;
		if (currentWall->holes[footHoldIndex[0]][footHoldIndex[1]].numRoutesUsing == 0) {
			currentWall->holes[footHoldIndex[0]][footHoldIndex[1]].holdType = "footHold";
		}
		if (currentWall->holes[footHoldIndex[0]][footHoldIndex[1]].routesUsing[currentRoute]++ == 0) {
			currentWall->holes[footHoldIndex[0]][footHoldIndex[1]].numRoutesUsing++;
		}
		free(footHoldIndex);
	}
}

void startFeet(route *newRoute, hold **hands) {
	int randomY1 = (rand() % 3) + 1;
	int randomY2 = (rand() % 3) + 1;
	int randomX1 = hands[0]->xIndex - (rand() % 3);
	while ((randomX1 < 2) || (currentWall->holes[randomX1][randomY1].numRoutesUsing > 2)) {
		randomX1++;
	}
	int randomX2 = randomX1 + (rand() % 3) + 4;
	while ((randomX2 > (2 * currentWall->width - 2)) || (currentWall->holes[randomX1][randomY1].numRoutesUsing > 2)) {
		randomX2--;
	}

	int *leftFoot = findClosestHole(randomX1, randomY1);
	int *rightFoot = findClosestHole(randomX2, randomY2);

	int currentRoute = currentWall->numberOfRoutes - 1;
	if (currentWall->holes[leftFoot[0]][leftFoot[1]].numRoutesUsing == 0) {
		currentWall->holes[leftFoot[0]][leftFoot[1]].holdType = "footHold";
	}
	if (currentWall->holes[leftFoot[0]][leftFoot[1]].routesUsing[currentRoute] == 0) {
		currentWall->holes[leftFoot[0]][leftFoot[1]].numRoutesUsing++;
	}
	currentWall->holes[leftFoot[0]][leftFoot[1]].routesUsing[currentRoute] = 1;
	if (currentWall->holes[rightFoot[0]][rightFoot[1]].numRoutesUsing == 0) {
		currentWall->holes[rightFoot[0]][rightFoot[1]].holdType = "footHold";
	}
	if (currentWall->holes[rightFoot[0]][rightFoot[1]].routesUsing[currentRoute] == 0) {
		currentWall->holes[rightFoot[0]][rightFoot[1]].numRoutesUsing++;
	}
	currentWall->holes[rightFoot[0]][rightFoot[1]].routesUsing[currentRoute] = 1;
}

void addFinalHoldToWall(route *currentRoute, hold **hands) {
	int x = currentRoute->importantPositions[3][0];
	int y = currentRoute->importantPositions[3][1] - 2;
	char *type = "jug";

	hold *finalHold = &currentWall->holes[x][y];
	if (finalHold->numRoutesUsing++ == 0) {
		finalHold->holdType = "jug";
	}
	finalHold->routesUsing[currentWall->numberOfRoutes - 1] = 1;

	hands[0] = finalHold;
	addHoldsToWall(hands);
}

void generateHolds(route *newRoute) {
	hold **hands = startRoute(newRoute);
	startFeet(newRoute, hands);
	bool nextHand;

	if (hands[0]->xIndex < currentWall->width) {
		nextHand = 1;
	}
	else {
		nextHand = 0;
	}

	if (newRoute->rating < 7) {
		while (hands[!nextHand]->yIndex < currentWall->height - 1) {
			addHoldsToWall(hands);
			hold *nextHold = generateNextHandHold(newRoute, hands, nextHand);
			generateNextFootHold(newRoute, hands[!nextHand], nextHold, nextHand);
			hands[nextHand] = nextHold;
			nextHand = !nextHand;
		}
	}
	else {
		while (hands[!nextHand]->yIndex < 2 * currentWall->height - 2) {
			addHoldsToWall(hands);
			hold *nextHold = generateNextHandHold(newRoute, hands, nextHand);
			generateNextFootHold(newRoute, hands[!nextHand], nextHold, nextHand);
			hands[nextHand] = nextHold;
			nextHand = !nextHand;
		}

		addFinalHoldToWall(newRoute, hands);
	}
}

void addToRouteList(route *newRoute) {
	int number = currentWall->numberOfRoutes - 1;

	currentWall->routeList[number] = newRoute;
}

void generateRoute(int rating) {
	route * newRoute = currentWall->routeList[currentWall->numberOfRoutes - 1];
	newRoute->rating = rating;
	newRoute->theme = randomizeTheme(rating);
	newRoute->importantPositions = randomizeImportantPositions();
	if (rating == 7) {
		newRoute->importantPositions[1][0] = newRoute->importantPositions[0][0];
		newRoute->importantPositions[2][0] = newRoute->importantPositions[0][0];
		newRoute->importantPositions[3][0] = newRoute->importantPositions[0][0];
	}
	generateHolds(newRoute);
	//addToRouteList(newRoute);
}

void printRouteInfo(FILE *fileID) {
	fprintf(fileID, "\n\Route Info:\n\n");

	for (int i = 0; i < currentWall->numberOfRoutes; i++) {
		fprintf(fileID, "%d:\n", i + 1);
		fprintf(fileID, "\tExpected Rating: %d\n", currentWall->routeList[i]->rating);
	}
}

void printHoldInfo(FILE *fileID) {
	fprintf(fileID, "\n\Hold Info:\n\n");

	int numberOfColumns = currentWall->height * 2 - 1;
	int numberOfRows = currentWall->width * 2 - 1;
	int currentNumber = 1;

	for (int j = numberOfColumns - 1; j >= 0; j--) {
		for (int i = 0; i < numberOfRows; i++) {
			if (currentWall->holes[i][j].numRoutesUsing > 0) {
				fprintf(fileID, "%d:\n", currentNumber++);
				fprintf(fileID, "\tHold Type: %s\n", currentWall->holes[i][j].holdType);
				fprintf(fileID, "\tPosition: %.1fft, %.1fft\n", currentWall->holes[i][j].xIndex / 2.0, currentWall->holes[i][j].yIndex / 2.0);
				fprintf(fileID, "\tRoutes Using: ");
				for (int k = 0; k < currentWall->numberOfRoutes; k++) {
					if (currentWall->holes[i][j].routesUsing[k] > 0) {
						fprintf(fileID, "%d, ", k + 1);
					}
				}
				fprintf(fileID, "\n\n");
			}
		}
	}
}

void printWall() {
	FILE *fileID = fopen("wallInfo.txt", "w");

	fprintf(fileID, "\n\nWall (bottom to top):\n\n");

	int numberOfColumns = currentWall->height * 2 - 1;
	int numberOfRows = currentWall->width * 2 - 1;
	int currentNumber = 1;

	for (int j = numberOfColumns - 1; j >= 0; j--) {
		for (int i = 0; i < numberOfRows; i++) {
			if (currentWall->holes[i][j].numRoutesUsing > 0) {
				fprintf(fileID, "%d", currentNumber++);
			}
			else if (currentWall->holes[i][j].isHole) {
				fprintf(fileID, ".");
			}
			else {
				fprintf(fileID, " ");
			}
		}
		fprintf(fileID, "\n");
	}

	printRouteInfo(fileID);
	printHoldInfo(fileID);
	fclose(fileID);
}

/**
*    Function invoked when an event on a regular keys occur
*/
void keyboard(unsigned char k, int x, int y)
{
	if (k == 27)
	{
		/* Close application if ESC is pressed */
		exit(0);
	}
	else if (k == 'p') {
		printWall();
	}
	else if (k == 'b') {
		mode = 'b';
		displayRoute = -1;
	}
	else if (k == 't') {
		mode = 't';
		displayRoute = -1;
	}
	else if (k == 'd') {
		mode = 'd';
		displayRoute = -1;
	}
	else if (k == 'v') {
		mode = 'v';
		displayRoute = -1;
	}
	else if (currentWall->numberOfRoutes < maxRoutesPerWall) {
		if (mode == 'b') {
			if (k == '0') {
				currentWall->numberOfRoutes++;
				generateRoute(0);
			}
			else if (k == '1') {
				currentWall->numberOfRoutes++;
				generateRoute(1);
			}
			else if (k == '2') {
				currentWall->numberOfRoutes++;
				generateRoute(2);
			}
		}
		else if (mode == 't') {
			if (k == '7') {
				currentWall->numberOfRoutes++;
				generateRoute(7);
			}
			else if (k == '8') {
				currentWall->numberOfRoutes++;
				generateRoute(8);
			}
			else if (k == '9') {
				currentWall->numberOfRoutes++;
				generateRoute(9);
			}
			else if (k == '0') {
				currentWall->numberOfRoutes++;
				generateRoute(10);
			}
		}
		else if (mode == 'v') {
			if (k == '0') {
				displayRoute = 0;
			}
			else if (k == '1') {
				displayRoute = 1;
			}
			else if (k == '2') {
				displayRoute = 2;
			}
			else if (k == '3') {
				displayRoute = 3;
			}
			else if (k == '4') {
				displayRoute = 4;
			}
			else if (k == '5') {
				displayRoute = 5;
			}
			else if (k == '6') {
				displayRoute = 6;
			}
			else if (k == '7') {
				displayRoute = 7;
			}
			else if (k == '8') {
				displayRoute = 8;
			}
			else if (k == '9') {
				displayRoute = 9;
			}
		}
		else if (mode == 'd') {
			if (k == '0') {
				deleteRoute(0);
			}
			else if (k == '1') {
				deleteRoute(1);
			}
			else if (k == '2') {
				deleteRoute(2);
			}
			else if (k == '3') {
				deleteRoute(3);
			}
			else if (k == '4') {
				deleteRoute(4);
			}
			else if (k == '5') {
				deleteRoute(5);
			}
			else if (k == '6') {
				deleteRoute(6);
			}
			else if (k == '7') {
				deleteRoute(7);
			}
			else if (k == '8') {
				deleteRoute(8);
			}
			else if (k == '9') {
				deleteRoute(9);
			}
		}
	}
}

void myMouseFunc(int button, int state, int x, int y)
{
	if(button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
		//if(checkForHit(x /(float) windowWidth, y /(float) windowHeight)) {
		int index = checkForHit(x, y);
		if ((index > 0) && (frameCount[index - 1] > waitFrame)) {
			frameCount[index - 1] = 0;
			if ((index - 1) % 3 == 0) {
				score += 5;
			} else {
				score += 1;
			}
		}
	}
}

void setNormalMap() {
	/* Set the normal map of the model */
	normalData = stbi_load(normalFile, &normalWidth, &normalHeight, &normalComp, STBI_rgb);
	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &normal);
	glBindTexture(GL_TEXTURE_2D, normal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, normalWidth, normalHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, normalData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void setWoodenTexture() {
	/* Set the texture of the model */
	textureData = stbi_load(textureFile, &textureWidth, &textureHeight, &textureComp, STBI_rgb);
	glActiveTexture(GL_TEXTURE2);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void setBlankTexture() {
	/* Set the texture of the model */
	glActiveTexture(GL_TEXTURE3);
	whiteData = stbi_load(whiteFile, &whiteWidth, &whiteHeight, &whiteComp, STBI_rgb);
	glGenTextures(1, &white);
	glBindTexture(GL_TEXTURE_2D, white);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, whiteWidth, whiteHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, whiteData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void setTextures() {
	//setNormalMap();
	//setWoodenTexture();
	//setBlankTexture();	
}

void setMonkeyShader() {
	// shaders for "Suzanne" mesh
	monkey_sp = create_programme_from_files (
		MONKEY_VERT_FILE, MONKEY_FRAG_FILE
	);
	monkey_M_location = glGetUniformLocation (monkey_sp, "M");
	monkey_V_location = glGetUniformLocation (monkey_sp, "V");
	monkey_P_location = glGetUniformLocation (monkey_sp, "P");
	normalDataLocation = glGetUniformLocation(monkey_sp, "normalData");
	textureDataLocation = glGetUniformLocation(monkey_sp, "textureData");
	cubeDataLocation = glGetUniformLocation(monkey_sp, "cube_texture");
	clickedLocation = glGetUniformLocation(monkey_sp, "clicked");

	suzanne_sp = create_programme_from_files(
		MONKEY_REFRACT_VERT_FILE, MONKEY_REFRACT_FRAG_FILE
		);
	suzanne_M_location = glGetUniformLocation(suzanne_sp, "M");
	suzanne_V_location = glGetUniformLocation(suzanne_sp, "V");
	suzanne_P_location = glGetUniformLocation(suzanne_sp, "P");
	normalDataLocation = glGetUniformLocation(suzanne_sp, "normalData");
	textureDataLocation = glGetUniformLocation(suzanne_sp, "textureData");
	cubeDataLocation = glGetUniformLocation(suzanne_sp, "cube_texture");
	clickedLocation = glGetUniformLocation(suzanne_sp, "clicked");
}

void setShaders() {
	setMonkeyShader();
}

void setRenderingDefaults() {
	/*---------------------------SET RENDERING DEFAULTS---------------------------*/
	glUseProgram (monkey_sp);
	glUniformMatrix4fv (monkey_V_location, 1, GL_FALSE, view_mat.m);
	glUniformMatrix4fv (monkey_P_location, 1, GL_FALSE, proj_mat.m);
	glUniform1i(cubeDataLocation, 0);
	glUniform1i(normalDataLocation, 1);
	glUniform1i(textureDataLocation, 2);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normal);

	glUseProgram(suzanne_sp);
	glUniformMatrix4fv(suzanne_V_location, 1, GL_FALSE, view_mat.m);
	glUniformMatrix4fv(suzanne_P_location, 1, GL_FALSE, proj_mat.m);
	glUniform1i(cubeDataLocation, 0);
	glUniform1i(normalDataLocation, 1);
	glUniform1i(textureDataLocation, 2);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, normal);

	glUseProgram (cube_sp);
	glUniformMatrix4fv (cube_V_location, 1, GL_FALSE, R.m);
	glUniformMatrix4fv (cube_P_location, 1, GL_FALSE, proj_mat.m);
	// unique model matrix for each sphere
	for (int i = 0; i < numOfBugs; i++) {
		model_mat[i] = identity_mat4();
	}
	
	glEnable (GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc (GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable (GL_CULL_FACE); // cull face
	glCullFace (GL_BACK); // cull back face
	glFrontFace (GL_CCW); // set counter-clock-wise vertex order to mean the front
	glClearColor (0.2, 0.2, 0.2, 1.0); // grey background to help spot mistakes
}

bool isHole(int x, int y) {
	if (x % 2 == y % 2) {
		return true;
	}
	return false;
}

void findHoleSpots() {
	int numberOfColumns = currentWall->height * 2 - 1;
	int numberOfRows = currentWall->width * 2 - 1;

	for (int i = 0; i < numberOfRows; i++) {
		for (int j = 0; j < numberOfColumns; j++) {
			currentWall->holes[i][j].isHole = isHole(i, j);
			currentWall->holes[i][j].numRoutesUsing = 0;
			currentWall->holes[i][j].xIndex = i;
			currentWall->holes[i][j].yIndex = j;
			currentWall->holes[i][j].routesUsing = (int*)malloc(maxRoutesPerWall*sizeof(int));
			for (int k = 0; k < maxRoutesPerWall; k++) {
				currentWall->holes[i][j].routesUsing[k] = 0;
			}
		}
	}
}

void initHoles() {
	int numberOfColumns = currentWall->height * 2;
	int numberOfRows = currentWall->width * 2;
	currentWall->holes = (hold**)malloc(numberOfRows* sizeof(hold*));

	for (int i = 0; i < numberOfRows; i++) {
		currentWall->holes[i] = (hold *)malloc(numberOfColumns*sizeof(hold));
	}

	findHoleSpots();
}

void initRoutes() {
	currentWall->routeList = (route **)malloc(maxRoutesPerWall*sizeof(route*));

	for (int i = 0; i < maxRoutesPerWall; i++) {
		currentWall->routeList[i] = (route *)malloc(sizeof(route));
	}
}

void initWall() {
	currentWall = (wall *)malloc(sizeof(wall));
	currentWall->height = 21;
	currentWall->width = 7;
	currentWall->numberOfRoutes = 0;
	initHoles();
	initRoutes();
}


/**
*    Set OpenGL initial state
*/
void init()
{
	/* Set clear color */
	glClearColor(0.0, 0.0, 0.0, 0.0);

	/* Enable the depth buffer */
	glEnable(GL_DEPTH_TEST);

	/* Set the texture of the model */
	textureData = stbi_load(textureFile, &textureWidth, &textureHeight, &textureComp, STBI_rgb);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth, textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, textureData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glActiveTexture(GL_TEXTURE0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fovy, aspect, zNear, zFar);
	
	srand(time(NULL));

	initWall();
}


/**
*    Main function
*/
int main(int argc, char **argv)
{
	/* Initialize the GLUT window */
	glutInit(&argc, argv);
	glutInitWindowSize(windowWidth, windowHeight);
	glutInitWindowPosition(30, 30);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutCreateWindow("CS 334 - Final Project: Procedural Generation of Rock Climbing Routes");

	/* Set OpenGL initial state */
	init();

	/* Callback functions */
	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);

	/* Start the main GLUT loop */
	/* NOTE: No code runs after this */
	glutMainLoop();
}


//ex cred ideas
//1. crosshairs instead of cursor