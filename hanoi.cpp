#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <iostream>
#include <stdlib.h>
#include <stddef.h>
#include <vector>
#include <cmath>
#include <sstream>
#include <string>
#include <unistd.h>
#include "load_and_bind_texture.h"
#include "draw_text.h"

//defining constants
//radius of the tower's central core
#define TOWER_RAD 0.025f
//height of the tower's core
#define TOWER_HEIGHT 1.0f
//distance between adjacent tower centres
#define TOWER_SPACING 0.9f
//radius of the tower's base disk
#define BASE_RAD 0.4f
//thickness of tower's base
#define BASE_HEIGHT 0.05f
//radius of the hole in the centre of each disk
#define HOLE_RAD 0.03f
//number of slices to use when making solids
#define SLICES 64
//number of stacks to use when making solids
#define STACKS 64
//height at which disks move to when moving between towers
#define MOVING_HEIGHT 1.1f
//number of microseconds to sleep for during animation
#define ANI_UP_TIME 10000
//number of degrees the camera rotates by
#define ANGLE_INCREMENT 10
//the value by which animation speed in increased or decreased by
#define SPEED_STEP 0.01f
//base animation speed (1x speed)
#define DEFAULT_ANI_SPEED 0.06f

std::vector<int> diskPos;
std::vector<std::vector <float> > diskColours;
float diskHeight = 0.1f, halfHeight, movingX, movingZ, vertAngle = 0,
			horAngle = 0, animationSpeed = DEFAULT_ANI_SPEED, targetHeight,
			targetX = 0, diskDChange = 0;
int numberOfDisks = 7, diskToMove, targetTower, sizeError = 0, moveCounter;
bool movingDisk = false, animating = false;
float light_position[] = { 2.0, 2.0, 2.0, 0.0 };
unsigned int texture = 0;

//draws a single empty tower at the given x and y
void drawTower(float x, float y)
{
	//create and set material for towers
	float mat_ambient[] = { 0.05, 0.05, 0.05, 1.0 };
	float mat_diffuse[] = { 0.75, 0.75, 0.75, 1.0 };
	float mat_specular[] = { 0.7, 0.7, 0.7, 1.0 };
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);

	//enable texturing
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_2D);

	//set texturing to account for lighting
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glPushMatrix();
		//translate to tower coordinates
		glTranslatef(x, y, 0);
		GLUquadricObj* q_obj = gluNewQuadric();
		gluQuadricNormals(q_obj, GLU_SMOOTH);
		//draw the tower core
		gluCylinder(q_obj, TOWER_RAD, TOWER_RAD, TOWER_HEIGHT, SLICES, STACKS);

		glPushMatrix();
			//translate to the top of the tower and add a cap
			glTranslated(0.0, 0.0, TOWER_HEIGHT);
			gluSphere(q_obj, TOWER_RAD, SLICES, STACKS);
		glPopMatrix();

		//draw the top side of the base
		gluDisk(q_obj, 0, BASE_RAD, SLICES, STACKS);

		glPushMatrix();
			glTranslatef(0, 0, -BASE_HEIGHT);
			glPushMatrix();
				glRotated(180.0, 1.0, 0.0, 0.0);
				//draw the bottom side of the base
				gluDisk(q_obj, 0, BASE_RAD, SLICES, STACKS);
			glPopMatrix();
			//draw the sides of the base
			gluCylinder(q_obj, BASE_RAD, BASE_RAD, BASE_HEIGHT, SLICES, STACKS);
		glPopMatrix();

	glPopMatrix();

	//disable texturing
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_2D);
}

//sets the material of the disk to its corresponding material
void setDiskColour(int diskNo)
{
	//retrieve material properties from the colour vector
	float mat_diffuse[] = { diskColours[diskNo][0], diskColours[diskNo][1],
													diskColours[diskNo][2], 1.0 };
	float mat_specular[] = { diskColours[diskNo][3], diskColours[diskNo][4],
													diskColours[diskNo][5], 1.0 };
	float mat_ambient[] = { diskColours[diskNo][6], diskColours[diskNo][7],
													diskColours[diskNo][8], 1.0 };

	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
}

//fill out the colour vector depending on the disk's size
void createDiskColours()
{
	//empty the colour vector
	diskColours.clear();
	//for each disk
	for (size_t i = 0; i < numberOfDisks; i++)
	{
		std::vector<float> diskColour;
		//calculate hue
		float hue = (360 * i) / (numberOfDisks);
		//calculate the partial colour for the disk
		float partialColour = 1 - std::abs(fmod(hue / 60.0f, 2) - 1);

		/*depending on hue bracket, one of RGB is set to 1, and another to
		partialColour*/
		//the first 3 elements are diffuse for RGB in that order
		if (hue >= 0 && hue < 60)
		{
			diskColour.push_back(1.0f);
			diskColour.push_back(partialColour);
			diskColour.push_back(0);
		}
		else if (hue < 120)
		{
			diskColour.push_back(partialColour);
			diskColour.push_back(1.0f);
			diskColour.push_back(0);
		}
		else if (hue < 180)
		{
			diskColour.push_back(0);
			diskColour.push_back(1.0f);
			diskColour.push_back(partialColour);
		}
		else if (hue < 240)
		{
			diskColour.push_back(0);
			diskColour.push_back(partialColour);
			diskColour.push_back(1.0f);
		}
		else if (hue < 300)
		{
			diskColour.push_back(partialColour);
			diskColour.push_back(0);
			diskColour.push_back(1.0f);
		}
		else
		{
			diskColour.push_back(1.0f);
			diskColour.push_back(0);
			diskColour.push_back(partialColour);
		}
		//add specular RGB based off the diffuse colour
		diskColour.push_back((diskColour[0] * 0.3f) + 0.4f);
		diskColour.push_back((diskColour[1] * 0.3f) + 0.4f);
		diskColour.push_back((diskColour[2] * 0.3f) + 0.4f);
		//add ambient RGB based off diffuse colour
		diskColour.push_back(diskColour[0] * 0.05f);
		diskColour.push_back(diskColour[1] * 0.05f);
		diskColour.push_back(diskColour[2] * 0.05f);
		//add colour vector for this disk to diskColours
		diskColours.push_back(diskColour);
	}
}

//draws a single hanoi disk with given radius at given coordinates
void drawHanoiDisk(float x, float y, float z, float radius)
{
	//subtracting halfHeight to account for torus radius on the outer edge
	radius -= halfHeight;
	glPushMatrix();
		//translate to disk centre
		glTranslatef(x, y, z);
		GLUquadricObj* q_obj = gluNewQuadric();
		gluQuadricNormals(q_obj, GLU_SMOOTH);

		//draw the outside rim
		glPushMatrix();
			//translate half height so torus is in between the top and bottom disk
			glTranslatef(0, 0, halfHeight);
			glutSolidTorus(halfHeight, radius, SLICES, STACKS);
		glPopMatrix();

		//draw the top cap
		glPushMatrix();
			glTranslated(0.0, 0.0, diskHeight);
			gluDisk(q_obj, HOLE_RAD, radius, SLICES, STACKS);
		glPopMatrix();

		//draw the bottom cap
		glPushMatrix();
			glRotated(180.0, 1.0, 0.0, 0.0);
			gluDisk(q_obj, HOLE_RAD, radius, SLICES, STACKS);
		glPopMatrix();

		//draw the inside of the hole
		gluQuadricOrientation(q_obj, GLU_INSIDE);
		gluCylinder(q_obj, HOLE_RAD, HOLE_RAD, diskHeight, SLICES, STACKS);

	glPopMatrix();
}

//draws the hanoi disks in their correct positions
void drawDisksOnTowers()
{
	//reset disk diameter to size of tower base
	float diskDiam = BASE_RAD;
	//create an array to represent the current height of the top disk on each
	//tower
	float currHeight[3] = { 0, 0, 0 };
	/*as a larger disk cannot be on top of a smaller disk, simply draw disk in
	decreasing order of size (index of diskPos is propotional to the size of the
	disk, i.e. diskPos[0] is the smallest disk and diskPos[n] is the largest
	disk)
	*/
	for (int diskNo = numberOfDisks - 1; diskNo >= 0; diskNo--)
	{
		//get the tower the disk indexed by diskNo is on
		int pillar = diskPos[diskNo];
		//set the correct material for the disk
		setDiskColour(diskNo);
		if (pillar == 0)
		{
			//if the disk is not on any pillar it must be moving, so use moving coords
			drawHanoiDisk(movingX, 0, movingZ, diskDiam);
		}
		else
		{
			//otherwise place disk on top of top disk of that pillar
			drawHanoiDisk(0.9f * (2 - pillar), 0, currHeight[pillar - 1], diskDiam);
			//increment height to top disk on the pillar
			currHeight[pillar - 1] += diskHeight;
		}
		//decrease diskDiameter for the next disk
		diskDiam -= diskDChange;
	}
}

//returns true if all disks are either on tower 2 or 3 (have all been moved)
bool checkWin()
{
	//if 0 disks in play, the player cannot win
	if (numberOfDisks <= 0)
		return false;
	//get position of the smallest disk
	int tower = diskPos[0];
	//if that disk is on tower 1 or moving then the player has not won
	if (tower == 1 || tower == 0)
		return false;
	/*if any single disk is not on the same tower as disk 0, then the player has
	not won (i.e. not all disks on the same tower)
	*/
	for (int i = 1; i < diskPos.size(); i++)
		if (diskPos[i] != tower)
			return false;
	//otherwise player has won
	return true;
}

//draws all instructions to the screen
void drawText()
{
	//set colour to white and disable lighting
	glColor3f(1, 1, 1);
	glDisable(GL_LIGHTING);
	//if a player has attempted to move a disk onto a smaller disk
	if (sizeError)
		draw_text(230, 800, "Disk bigger than top disk on that tower!");
	//create string to describe animation speed and move counter
	std::stringstream stream;
	stream  << (animationSpeed/DEFAULT_ANI_SPEED);
	std::string str = "Use '<'/'>' to change animation speed. Currently: " + stream.str() + "x";

	std::stringstream stream2;
	stream2  << moveCounter;
	std::string str2 = "Number of moves taken: " + stream2.str();
	//print text to the screen using draw_text.h
	draw_text(10, 960, str2.c_str());
	draw_text(10, 220, str.c_str());
	draw_text(10, 180, "Use the arrow keys to move the camera.");
	draw_text(10, 140, "Use '1'/'2'/'3' to pick up disk from/place disk on tower.");
	draw_text(10, 100, "Use '+'/'-' to change the number of disks (resets).");
	draw_text(10, 60, "Use 'r' to reset the game.");
	draw_text(10, 20, "Use 'q' to quit.");


	//check if the player has won, if so tell the player
	if (checkWin())
		draw_text(400, 800, "You solved it!");

	//renable lighting
	glEnable(GL_LIGHTING);
}

//the main display func
void display()
{
	//clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//position camera
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 3, 2.5, // eye position
						0, 0, 0.5, // reference point
						0, 0, 1  // up vector
	);

	//rotate world to simulate rotating camera around world
	glRotatef(vertAngle, 1, 0, 0);
	glRotatef(horAngle, 0, 0, 1);

	//reset light position
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	//draw pillars into scene
	drawTower(TOWER_SPACING, 0);
	drawTower(0, 0);
	drawTower(-TOWER_SPACING, 0);

	//draw disks on each pillar
	drawDisksOnTowers();

	//draw text
	drawText();

	glutSwapBuffers();
}

//resets the game
void reset()
{
	//reset diskPos vector to have numberOfDisks disks all on tower 1
	diskPos.assign(numberOfDisks, 1);
	/*calculate disk height (numberOfDisks + 2 is used to ensure torus edge of
	disks does not clip into the central hole of the disk as this ensures the
	ratio of the smallest disk is bigger than half its height (the radius of the
	torus) for any number of disks)
	*/
	diskHeight = TOWER_HEIGHT / (numberOfDisks + 2);
	halfHeight = diskHeight / 2;
	//calculate diameter change between disks
	diskDChange = (BASE_RAD - TOWER_RAD) / (numberOfDisks + 2);
	//fill out colour vector for all disks
	createDiskColours();
	//no disk is moving from a reset
	movingDisk = false;
	//reset move counter
	moveCounter = 0;
	glutPostRedisplay();
}

//adds a disk to the game
void addDisk()
{
	numberOfDisks++;
	reset();
}

//removes a disk from the game
void removeDisk()
{
	if (numberOfDisks > 0)
		numberOfDisks--;
	reset();
}

//keeps redisplays occuring to ensure no frame is left unloaded
void idle()
{
  usleep(ANI_UP_TIME);
	glutPostRedisplay();
}

//moves the moving disk (diskPos == 0) to the moving height
void moveUp()
{
	//block keyboard input
	animating = true;
  usleep(ANI_UP_TIME);
	//move disk up by animation speed
	movingZ += animationSpeed;
	//check if exceeding moving height
	if (movingZ > MOVING_HEIGHT)
	{
		//when reached moving height stop moving up
		glutIdleFunc(idle);
		//set Z incase of overshooting
		movingZ = MOVING_HEIGHT;
		//enable keyboard input
		animating = false;
	}
	glutPostRedisplay();
}

//moves a disk down to be the top disk of the target tower
void moveDown()
{
	//block keyboard input
	animating = true;
  usleep(ANI_UP_TIME);
	//move Z down
	movingZ -= animationSpeed;
	//check if overshooting
	if (movingZ < targetHeight)
	{
		//stop moving down
		glutIdleFunc(idle);
		//set disk to be on tower
		diskPos[diskToMove] = targetTower;
		//no disk is moving and keyboard input is renabled
		movingDisk = false;
		animating = false;
	}
	glutPostRedisplay();
}

//move floating disk to above the target tower
void moveAcross()
{
	//block keyboard input
	animating = true;
  usleep(ANI_UP_TIME);
	//get distance to target
	float dist = movingX - targetX;
	//check if the disk is within one speed step of the target
	if (dist < animationSpeed && -dist < animationSpeed)
	{
		//set position to target
		movingX = targetX;
		glutPostRedisplay();
		//move disk down onto tower
		glutIdleFunc(moveDown);
	}
	else
	{
		//if not move towards the target
		if (movingX < targetX)
			movingX += animationSpeed;
		else
			movingX -= animationSpeed;
	}
	glutPostRedisplay();
}

//returns the height of the topmost disk of a given tower
float getHeightOfTopDisk(int tower)
{
	//count the number of disks on the tower
	int disksOnTower = 0;
	for (int i = 0; i < numberOfDisks; i++)
		if (diskPos[i] == tower)
			disksOnTower++;
	//return the count multiplied by disk height
	return disksOnTower * diskHeight;
}

//reduce the number of disk size errors by one
void disableDiskErrorFlag(int)
{
	sizeError--;
	glutPostRedisplay();
}

//handle the keypress corresponding to moving a disk (1/2/3)
void moveDisk(int tower)
{
	//get height of the top disk of the tower
	float topDiskHeight = getHeightOfTopDisk(tower);
	/*get the top disk of the tower by increasing topDiskOfTower until a value is
	found that is on the tower
	*/
	int topDiskOfTower = 0;
	while (topDiskOfTower < numberOfDisks && diskPos[topDiskOfTower] != tower)
		topDiskOfTower++;

	//if a disk is already moving (i.e. floating)
	if (movingDisk)
	{
		//then check if it fits on the destination tower
		if (diskToMove < topDiskOfTower)
		{
			//if so handle moving it onto the tower
			targetHeight = topDiskHeight;
			targetTower = tower;
			switch (targetTower)
			{
			case(1): targetX = TOWER_SPACING; break;
			case(2): targetX = 0; break;
			case(3): targetX = -TOWER_SPACING; break;
			}
			//move disk across
			glutIdleFunc(moveAcross);
		}
		else
		{
			//otherwise increase size error (displays error message)
			sizeError++;
			//in 2 seconds remove an error
			glutTimerFunc(2000, disableDiskErrorFlag, 0);
		}
	}
	else
	{
		//disk is on a tower already, so handle moving it up
		//if no disk is on tower then just return and do nothing
		if (topDiskOfTower >= numberOfDisks)
			return;
		//set moving flag to true
		movingDisk = true;
		//set the moving disk index and set its current Z
		diskToMove = topDiskOfTower;
		movingZ = topDiskHeight - diskHeight;
		//set its current X based off the tower it's on
		switch (tower)
		{
		case(1): movingX = TOWER_SPACING; break;
		case(2): movingX = 0; break;
		case(3): movingX = -TOWER_SPACING; break;
		}
		//set the disk to not be on a tower
		diskPos[diskToMove] = 0;
		//increase move moveCounter
		moveCounter++;
		//move the disk up
		glutIdleFunc(moveUp);
	}
}

//handles keyboard inputs
void keyboard(unsigned char key, int, int)
{
	//if a disk is currently animating ignore keyboard input
	if (!animating)
		switch (key)
		{
		case '+': addDisk(); break;
		case '-': removeDisk(); break;
		case 'r':
		case 'R': reset(); break;
		case '1': moveDisk(1); break;
		case '2': moveDisk(2); break;
		case '3': moveDisk(3); break;
		case 'q':
		case 'Q': exit(1); break;
		case '>': animationSpeed += SPEED_STEP; break;
		case '<': animationSpeed -= SPEED_STEP;	if (animationSpeed <= 0) animationSpeed = SPEED_STEP; break;
		}

	glutPostRedisplay();
}

//handles camera rotations
void special(int key, int, int)
{
	switch (key)
	{
	case GLUT_KEY_UP: vertAngle += ANGLE_INCREMENT; break;
	case GLUT_KEY_DOWN: vertAngle -= ANGLE_INCREMENT; break;
	case GLUT_KEY_LEFT: horAngle += ANGLE_INCREMENT; break;
	case GLUT_KEY_RIGHT: horAngle -= ANGLE_INCREMENT; break;
	}
	glutPostRedisplay();
}

//handles reshaping the window
void reshape(int w, int h)
{
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//for perspective projection use the GLU helper
	//take FOV, ASPECT RATIO, NEAR, FAR
	gluPerspective(40.0, 1.0f, 1.0, 5.0);
}

//initialises texture and lighting
void init()
{
	//reset game for the first time (i.e. intialises game)
	reset();
	//create lighting
	float light_ambient[] = { 0.1, 0.1, 0.1, 1.0 };
	float light_diffuse[] = { 0.5, 0.5, 0.5, 1.0 };

	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0);

	glFrontFace(GL_CW);

	//enable the lights
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	glEnable(GL_AUTO_NORMAL);
	glEnable(GL_NORMALIZE);
	glEnable(GL_DEPTH_TEST);

	//enable shading to be smooth
	glShadeModel(GL_SMOOTH);

	//initialise material
	float mat_ambient[] = { 0.05, 0.05, 0.05, 1.0 };
	float mat_diffuse[] = { 0.75, 0.75, 0.75, 1.0 };
	float mat_specular[] = { 0.7, 0.7, 0.7, 1.0 };
	float mat_shininess[] = { 10.0 };
	glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	//initialise texture
	texture = load_and_bind_texture("./wood-grain.png");
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
}

int main(int argc, char* argv[])
{
	//initialise openGL
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(1024, 1024);
	glutInitWindowPosition(0, 0);

	glutCreateWindow("Towers of Hanoi");

	//set up functions
	glutKeyboardFunc(keyboard);
	glutSpecialFunc(special);
	glutReshapeFunc(reshape);
	glutDisplayFunc(display);
	glutIdleFunc(idle);

	init();

	glutMainLoop();

	return 0;
}
