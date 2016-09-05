/**
*	CS 334 - Fundamentals of Computer Graphics
*	Initial framework for assignment 1
*/

#include <iostream>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdlib.h>
#include "GL/glut.h"

/* Constant values */
const float PI = 3.14159265359;

/* Window information */
float windowWidth = 1200;
float windowHeight = 800;

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
	{0.0, 1.0, 0.0},
	{0.0, 0.0, 1.0},
	{1.0, 1.0, 0.0},
	{1.0, 0.0, 1.0},
	{0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0},
	{139.0/255.0, 69.0/255.0, 19.0/255.0},
	{1.0, 20.0/255.0, 147.0/255.0},
	{1.0, 165.0/255.0, 0.0}
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

/* #### The modelview matrix to load; remember OpenGL is row-major and not column major */
/* You do not necessarily need to use this matrix */
GLfloat myModelMat[4][4] = {
{ 1, 0, 0, 0 },
{ 0, 1, 0, 0 },
{ 0, 0, 1, 0 },
{ 0, 0, -60, 1 }
};


/* #### Define variables for the camera here... */
float xPosition = 0;
float zPosition = 0;
float xRotation = 0;
float yRotation = 0;
float zRotation = 0;

/* Field of view, aspect and near/far planes for the perspective projection */
float fovy = 45.0;
float aspect = windowWidth / windowHeight;
float zNear = 1.0;
float zFar = 100.0;

float wallVertices[12];

void drawWall() {
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
	for (int k = 0; k < maxRoutesPerWall; k++) {
		if (currentWall->holes[i][j].routesUsing[k] != 0) {
			timesUsed++;
			drawHole(i, j, routeColors[k][0], routeColors[k][1], routeColors[k][2], timesUsed*0.2);
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

/**
*    Function invoked for drawing using OpenGL
*/
void display()
{
    static int frameCount=0;

	/* #### frame count, might come in handy for animations */
	frameCount++;
	
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

char *randomizeTheme(int rating) {
	if (rating < 9) {
		return theme[0];
	}
	else if (rating == 9) {
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

	if (currentRoute->rating == 7) {
		minRadius = 1;
		maxRadius = 2;
	}
	else if (currentRoute->rating == 8) {
		minRadius = 3;
		maxRadius = 5;
	}
	else if (currentRoute->rating == 9) {
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
	else if (currentRoute->rating == 10) {
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
	return ((distance >= 2*radii[0]) && (distance <= 2*radii[1]));
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
	
	for (int i = -10; i <= 10; i++) {
		for (int j = -10; j <= 10; j++) {
			int x = hand->xIndex + i;
			int y = hand->yIndex + j;
			if ((x > 0) && (y > 0) && (x < (2*currentWall->width-1)) && (y < (currentWall->height-1))) {
				if ((currentWall->holes[x][y].numRoutesUsing > 0) && (currentWall->holes[x][y].numRoutesUsing < maxRoutesPerHold)) {
					if (checkPossibility(currentRoute, &currentWall->holes[x][y], hand, nextHand, x, y)) {
						availableHold[0] = x;
						availableHold[1] = y;
					}
				}
			}
		}
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

	if (newRoute->rating == 7) {
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
	else if (newRoute->rating == 9) {
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
	else if (newRoute->rating == 10) {
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
	if ((newRoute->rating == 7) || (newRoute->rating == 8)) {
		return -1.0*PI/2.0;
	}
	else if ((newRoute->rating == 9) || (newRoute->rating == 10)) {
		float angle = calculateMidAngle(currentHand, newRoute, nextHand);
		angle -= PI;
		return (angle + -1.0*PI / 2.0) / 2.0;
	}
}

float calculateAngleRangeForFoot(route *newRoute, char *relationship) {
	if (newRoute->rating == 7) {
		return PI/6.0;
	}
	else if ((newRoute->rating == 8) || (newRoute->rating == 9)) {
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
	else if (newRoute->rating == 10) {
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
			if ((x > 0) && (y > 0) && (x < (2 * currentWall->width - 1)) && (y < (2*currentWall->height - 1))) {
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

	while (hands[!nextHand]->yIndex < 2*currentWall->height-2) {
		addHoldsToWall(hands);
		hold *nextHold = generateNextHandHold(newRoute, hands, nextHand);
		generateNextFootHold(newRoute, hands[!nextHand], nextHold, nextHand);
		hands[nextHand] = nextHold;
		nextHand = !nextHand;
	}

	addFinalHoldToWall(newRoute, hands);
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
				fprintf(fileID, "\tPosition: %.1fft, %.1fft\n", currentWall->holes[i][j].xIndex/2.0, currentWall->holes[i][j].yIndex/2.0);
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

	for (int j = numberOfColumns-1; j >= 0; j--) {
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
*    #### Function invoked when an event on regular keys occur
*/
void keyboard(unsigned char k, int x, int y)
{
    /* Show which key was pressed */
    //std::cout << "Pressed \"" << k << "\" ASCII: " << (int)k << std::endl;

	if (k == 27)
    {
        /* Close application if ESC is pressed */
        exit(0);
	}
	else if (k == 'p') {
		printWall();
	}
	else if (currentWall->numberOfRoutes < maxRoutesPerWall) {
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
}


/**
*	#### Function invoked when an event on special keys occur
*/
void special(int key, int x, int y) 
{
	if(key == GLUT_KEY_UP) 
	{
		zPosition += 0.25;
	}
	else if(key == GLUT_KEY_DOWN) 
	{
		zPosition -= 0.25;
	}
	else if(key == GLUT_KEY_RIGHT) 
	{
		xPosition -= 0.25;
	}
	else if(key == GLUT_KEY_LEFT) 
	{
		xPosition += 0.25;
	}
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
*	Set OpenGL initial state
*/
void init()
{
    /* Set clear color */
    glClearColor(1.0, 1.0, 1.0, 0.0);

    /* Enable the depth buffer */
    glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fovy, aspect, zNear, zFar);

	srand(time(NULL));

	initWall();
}


/**
*	Main function
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
	glutSpecialFunc(special);

    /* Start the main GLUT loop */
    /* NOTE: No code runs after this */
    glutMainLoop();
}