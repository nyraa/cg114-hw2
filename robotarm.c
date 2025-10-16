#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "readstl.h"

#define LINKS_FILE_PREFIX "links/link"
#define NUM_LINKS 5

static GLfloat windowWidth  = 100.0f;  // world-coord half-width or height (depends on aspect)
static GLfloat windowHeight = 100.0f;

uint32_t numTriangles[NUM_LINKS];
struct Triangle* links[NUM_LINKS];
GLfloat origins[NUM_LINKS][3] = {
    {0, -10, 0},
    {0, 20, 0},
    {0, 40, 0},
    {32.5, 120, 0},
    {0, 115, 0}
};
GLfloat linkColors [NUM_LINKS][3] = {
    {1.0f, 0.0f, 0.0f},
    {1.0f, 0.5f, 0.0f},
    {1.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 1.0f, 1.0f}
};

void RenderScene(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();
    glScalef(0.3f, 0.3f, 0.3f);
    for (int i = 0; i < NUM_LINKS; ++i)
    {
        // draw link
        glColor3f(linkColors[i][0], linkColors[i][1], linkColors[i][2]);
        glTranslatef(origins[i][0], origins[i][1], origins[i][2]);
        glBegin(GL_TRIANGLES);
        for (uint32_t j = 0; j < numTriangles[i]; ++j)
        {
            glNormal3fv(links[i][j].normal);
            glVertex3fv(links[i][j].vertex1);
            glVertex3fv(links[i][j].vertex2);
            glVertex3fv(links[i][j].vertex3);
        }
        glEnd();
    }

    glPopMatrix();

    glutSwapBuffers();
}

void SetupRC(void)
{
    // Clear color: blue
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);

    /* Lighting setup */
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat ambientLight[]  = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat diffuseLight[]  = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat specularLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat lightPos[]      = { 100.0f, 100.0f, 100.0f, 1.0f }; // positional light

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    /* Let glColor* calls set the material diffuse color */
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    /* Set default specular and shininess */
    GLfloat matSpecular[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0f);
}

void ChangeSize(int w, int h)
{
    if (h == 0) h = 1;

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    GLfloat aspect = (GLfloat)w / (GLfloat)h;

    if (w <= h)
    {
        windowWidth  = 100.0f;
        windowHeight = 100.0f / aspect;
        glOrtho(-100.0, 100.0, -windowHeight, windowHeight, -1000.0, 1000.0);
    }
    else
    {
        windowWidth  = 100.0f * aspect;
        windowHeight = 100.0f;
        glOrtho(-windowWidth, windowWidth, -100.0, 100.0, -1000.0, 1000.0);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void SpecialKeys(int key, int x, int y)
{
    glutPostRedisplay();
}

void loadSTL()
{
    for (int i = 0; i < NUM_LINKS; ++i)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s%d.stl", LINKS_FILE_PREFIX, i + 1);
        numTriangles[i] = readBinSTL(filename, &links[i]);
        printf("Loaded %s with %d triangles\n", filename, numTriangles[i]);
    }
}

void Cleanup(void)
{
    for (int i = 0; i < NUM_LINKS; ++i)
    {
        if (links[i] != NULL)
        {
            free(links[i]);
        }
    }
}

int main(int argc, char* argv[])
{
    loadSTL();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Display STL");

    glutDisplayFunc(RenderScene);
    glutReshapeFunc(ChangeSize);
    glutSpecialFunc(SpecialKeys);

    SetupRC();
    glutMainLoop();
    Cleanup();
    return 0;
}