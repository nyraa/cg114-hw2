#include "gltools.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "math3d.h"
#include "stopwatch.hpp"

#include "readstl.h"

#define LINKS_FILE_PREFIX "links/link"
#define NUM_LINKS 5

static GLfloat windowWidth  = 100.0f;  // world-coord half-width or height (depends on aspect)
static GLfloat windowHeight = 100.0f;

uint32_t numTriangles[NUM_LINKS];
// struct Triangle *links[NUM_LINKS];
float *links[NUM_LINKS];
float *normals[NUM_LINKS];
const GLfloat linkOrigins[NUM_LINKS][3] = {
    {0, -180, 0},
    {0, 20, 0},
    {0, 40, 0},
    {32.5, 120, 0},
    {0, 115, 0}
};
const GLfloat linkColors[NUM_LINKS][3] = {
    {1.0f, 0.0f, 0.0f},     // link 0 red
    {1.0f, 0.5f, 0.0f},     // link 1 orange
    {1.0f, 1.0f, 0.0f},     // link 2 yellow
    {0.0f, 1.0f, 0.0f},     // link 3 green
    {0.0f, 1.0f, 1.0f}      // link 4 cyan
};
GLfloat linkRotate[NUM_LINKS] = {0};
const GLfloat linkRotateAxis[NUM_LINKS][3] = {
    {0.0f, 0.0f, 0.0f},     // place holder
    {0.0f, 1.0f, 0.0f},     // link 0-1 y axis
    {1.0f, 0.0f, 0.0f},     // link 1-2 x axis
    {1.0f, 0.0f, 0.0f},     // link 2-3 x axis
    {0.0f, 1.0f, 0.0f}      // link 3-4 y axis
};

GLfloat radius = 0.0f;
GLfloat clawLength = 0.0f;

GLfloat sphereRadius = 81.0f;
GLfloat sphereCenter[4] = {-200.0f, -99.0f, 200.0f, 1.0f};

M3DVector3f groundPoints[3] = {
    { -30.0f, -180.0f, -20.0f },
    {  -30.0f, -180.0f, 20.0f },
    {    40.0f, -180.0f,  20.0f }
};
M3DMatrix44f shadowMatrix;

#define NUM_TEXTURES 2
GLuint textureIDs[NUM_TEXTURES];

enum DrawMode
{
    Default,
    VertexArray,
    VBO
};

DrawMode currentDrawMode = Default;

float getPointToSegmentDistance(const GLfloat p[3], const GLfloat a[3], const GLfloat b[3])
{
    M3DVector3f ap, ab;
    m3dSubtractVectors3(ap, p, a);
    m3dSubtractVectors3(ab, b, a);
    float abLengthSq = m3dGetVectorLength(ab);
    abLengthSq *= abLengthSq;
    float t = (m3dDotProduct(ap, ab)) / abLengthSq;
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    M3DVector3f projection;
    projection[0] = a[0] + t * ab[0];
    projection[1] = a[1] + t * ab[1];
    projection[2] = a[2] + t * ab[2];
    M3DVector3f diff;
    m3dSubtractVectors3(diff, p, projection);
    return m3dGetVectorLength(diff);
}

void DrawRobotArm(int colorMode)
{
    // push matrix for arm rotation and base translation
    glPushMatrix();
    for (int i = 0; i < NUM_LINKS; ++i)
    {
        // draw link
        if (colorMode)
        {
            glColor3f(linkColors[i][0], linkColors[i][1], linkColors[i][2]);
        }
        else
        {
            glColor3f(0.0f, 0.0f, 0.0f);
        }
        glTranslatef(linkOrigins[i][0], linkOrigins[i][1], linkOrigins[i][2]);
        glRotatef(linkRotate[i], linkRotateAxis[i][0], linkRotateAxis[i][1], linkRotateAxis[i][2]);

        glBindTexture(GL_TEXTURE_2D, textureIDs[0]);
        switch(currentDrawMode)
        {
            case Default:
                glBegin(GL_TRIANGLES);
                for (uint32_t j = 0; j < numTriangles[i]; ++j)
                {
                    glNormal3fv(&normals[i][j * 3]);
                    glTexCoord2f(0.0f, 0.0f);
                    glVertex3fv(&links[i][j * 9]);
                    glTexCoord2f(1.0f, 0.0f);
                    glVertex3fv(&links[i][j * 9 + 3]);
                    glTexCoord2f(0.5f, 1.0f);
                    glVertex3fv(&links[i][j * 9 + 6]);
                }
                glEnd();
                break;
            case VertexArray:
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_NORMAL_ARRAY);
                glVertexPointer(3, GL_FLOAT, 0, &links[i][0]);
                glNormalPointer(GL_FLOAT, 0, &normals[i][0]);
                glDrawArrays(GL_TRIANGLES, 0, numTriangles[i] * 3);
                glDisableClientState(GL_VERTEX_ARRAY);
                glDisableClientState(GL_NORMAL_ARRAY);
                break;
            case VBO:
                {
                    GLuint vboIDs[2];
                    glGenBuffers(2, vboIDs);
                    // vertex buffer
                    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[0]);
                    glBufferData(GL_ARRAY_BUFFER, numTriangles[i] * sizeof(struct Triangle), links[i], GL_STATIC_DRAW);
                    glEnableClientState(GL_VERTEX_ARRAY);
                    glVertexPointer(3, GL_FLOAT, sizeof(struct Triangle), (GLvoid *)offsetof(struct Triangle, vertex1));
                    // normal buffer
                    glBindBuffer(GL_ARRAY_BUFFER, vboIDs[1]);
                    glBufferData(GL_ARRAY_BUFFER, numTriangles[i] * sizeof(struct Triangle), links[i], GL_STATIC_DRAW);
                    glEnableClientState(GL_NORMAL_ARRAY);
                    glNormalPointer(GL_FLOAT, sizeof(struct Triangle), (GLvoid *)offsetof(struct Triangle, normal));
                    // draw
                    glDrawArrays(GL_TRIANGLES, 0, numTriangles[i] * 3);
                    // cleanup
                    glDisableClientState(GL_VERTEX_ARRAY);
                    glDisableClientState(GL_NORMAL_ARRAY);
                    glBindBuffer(GL_ARRAY_BUFFER, 0);
                    glDeleteBuffers(2, vboIDs);
                }
                break;
        }
    }
    // pop arm rotation and base translation
    glPopMatrix();
}

void RenderScene(void)
{
    static int iFrames = 0;
    static CStopWatch frameTimer;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);

    glPushMatrix();
    #define SCALE 0.2f
    glScalef(SCALE, SCALE, SCALE);
    #undef SCALE
    glRotatef(30.0f, 1.0f, 0.0f, 0.0f);     // rotate x
    glRotatef(-30.0f, 0.0f, 1.0f, 0.0f);    // rotate y

    // draw robot arm shadow
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glPushMatrix();
    glMultMatrixf((GLfloat *)shadowMatrix);
    DrawRobotArm(0);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    // draw target sphere shadow
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glPushMatrix();
    glMultMatrixf((GLfloat *)shadowMatrix);
    glTranslatef(sphereCenter[0], sphereCenter[1], sphereCenter[2]);
    glColor3f(0.0f, 0.0f, 0.0f);
    glutSolidSphere(sphereRadius, 30, 30);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    // draw robot arm
    glBindTexture(GL_TEXTURE_2D, textureIDs[0]);
    DrawRobotArm(1);

    // calculate claw segment transform matrix
    M3DMatrix44f transformMatrix, currentMatrix, tempMatrix;
    m3dLoadIdentity44(currentMatrix);
    for (int i = 0; i < NUM_LINKS; ++i)
    {
        m3dTranslationMatrix44(transformMatrix, linkOrigins[i][0], linkOrigins[i][1], linkOrigins[i][2]);
        m3dMatrixMultiply44(tempMatrix, currentMatrix, transformMatrix);
        m3dRotationMatrix44(transformMatrix, m3dDegToRad(linkRotate[i]), linkRotateAxis[i][0], linkRotateAxis[i][1], linkRotateAxis[i][2]);
        m3dMatrixMultiply44(currentMatrix, tempMatrix, transformMatrix);
    }
    
    // calculate claw segment positions
    M3DVector4f originPos = {0.0f, 0.0f, 0.0f, 1.0f}, clawPos;
    m3dTransformVector4(clawPos, originPos, currentMatrix);
    // add claw length to get claw end position
    M3DVector4f clawEndPos;
    m3dTranslationMatrix44(transformMatrix, 0.0f, clawLength, 0.0f);
    m3dMatrixMultiply44(tempMatrix, currentMatrix, transformMatrix);
    m3dTransformVector4(clawEndPos, originPos, tempMatrix);

    // calculate distance from claw segment to sphere center
    float distToSphere = getPointToSegmentDistance(sphereCenter, clawPos, clawEndPos);

    // draw target sphere
    glPushMatrix();
    glTranslatef(sphereCenter[0], sphereCenter[1], sphereCenter[2]);
    // switch color if claw touches sphere
    if (distToSphere <= sphereRadius)
    {
        glColor3ub(151, 160, 155);
        glBindTexture(GL_TEXTURE_2D, textureIDs[1]);
    }
    else
    {
        glColor3ub(161, 113, 111);
        glBindTexture(GL_TEXTURE_2D, textureIDs[0]);
    }

    // glutSolidSphere(sphereRadius, 30, 30);
    gltDrawSphere(sphereRadius, 30, 30);
    glPopMatrix();

    // draw claw segment for debugging
    // glDisable(GL_LIGHTING);
    // glDisable(GL_DEPTH_TEST);
    // glLineWidth(5.0f);
    // glColor3f(1.0f, 1.0f, 1.0f);
    // glBegin(GL_LINES);
    // glVertex3f(clawPos[0], clawPos[1], clawPos[2]);
    // glVertex3f(clawEndPos[0], clawEndPos[1], clawEndPos[2]);
    // glEnd();
    // glEnable(GL_DEPTH_TEST);
    // glEnable(GL_LIGHTING);

    // draw workspace sphere
    // translate to first origin as sphere center
    glTranslatef(linkOrigins[0][0], linkOrigins[0][1], linkOrigins[0][2]);

    // disable lighting for the sphere
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // enable blending for transparency
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glColor4f(1.0f, 1.0f, 1.0f, 0.4f);

    // glutSolidSphere(radius, 30, 30);
    gltDrawSphere(radius, 30, 30);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glPopMatrix();

    glutSwapBuffers();

    iFrames++;

    // Do periodic frame rate calculation
    if(iFrames == 100)
    {
        float fps;
        char cBuffer[64];
        
        fps = 100.0f / frameTimer.GetElapsedSeconds();
        if(currentDrawMode == VertexArray)
            sprintf(cBuffer,"Robot Arm with Vertex Array %.1f fps", fps);
        else
            sprintf(cBuffer,"Robot Arm with VBO %.1f fps", fps);
            
        glutSetWindowTitle(cBuffer);
        
        frameTimer.Reset();
        iFrames = 0;
    }

}

void SetupRC(void)
{
    // Clear color: blue
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);

    // lighting setup
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat ambientLight[]  = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat diffuseLight[]  = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat specularLight[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat lightPos[]      = { 400.0f, 400.0f, 200.0f, 0.0f }; // positional light

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // Let glColor* calls set the material diffuse color
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Set default specular and shininess
    GLfloat matSpecular[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0f);

    // calculate shadow projection matrix
    M3DVector4f planeEq;
    m3dGetPlaneEquation(planeEq, groundPoints[0], groundPoints[1], groundPoints[2]);
    m3dMakePlanarShadowMatrix(shadowMatrix, planeEq, lightPos);

    // read texture
    glGenTextures(NUM_TEXTURES, textureIDs);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    GLbyte *pBytes;
    GLint iWidth, iHeight, iComponents;
    GLenum eFormat;

    glBindTexture(GL_TEXTURE_2D, textureIDs[0]);
    pBytes = gltLoadTGA("brick.tga", &iWidth, &iHeight, &iComponents, &eFormat);
    glTexImage2D(GL_TEXTURE_2D, 0, iComponents, iWidth, iHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBytes);
    free(pBytes);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, textureIDs[1]);
    pBytes = gltLoadTGA("grass.tga", &iWidth, &iHeight, &iComponents, &eFormat);
    glTexImage2D(GL_TEXTURE_2D, 0, iComponents, iWidth, iHeight, 0, eFormat, GL_UNSIGNED_BYTE, pBytes);
    free(pBytes);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void TimerFunction(int value)
{
    glutPostRedisplay();
    glutTimerFunc(3, TimerFunction, 1);
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
}

void HandleKey(unsigned char key, int x, int y)
{
    GLfloat rotateStep = 5.0f;
    switch (key)
    {
        case 'q': case 'Q':
            linkRotate[1] -= rotateStep;
            break;
        case 'a': case 'A':
            linkRotate[1] += rotateStep;
            break;
        case 'w': case 'W':
            linkRotate[2] += rotateStep;
            break;
        case 's': case 'S':
            linkRotate[2] -= rotateStep;
            break;
        case 'e': case 'E':
            linkRotate[3] += rotateStep;
            break;
        case 'd': case 'D':
            linkRotate[3] -= rotateStep;
            break;
        case 'r': case 'R':
            linkRotate[4] -= rotateStep;
            break;
        case 'f': case 'F':
            linkRotate[4] += rotateStep;
            break;
    }
    for (int i = 1; i < NUM_LINKS; ++i)
    {
        if (linkRotate[i] >= 360)
            linkRotate[i] -= 360;
        else if (linkRotate[i] < 0)
            linkRotate[i] += 360;
    }
    glutPostRedisplay();
}

void ProcessMenu(int value)
{
    currentDrawMode = (DrawMode)value;
    glutPostRedisplay();
}

void loadSTL()
{
    for (int i = 0; i < NUM_LINKS; ++i)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s%d.stl", LINKS_FILE_PREFIX, i + 1);
        numTriangles[i] = readBinSTL(filename, &links[i], &normals[i]);
        printf("Loaded %s with %d triangles\n", filename, numTriangles[i]);
    }
}

void ShutdownRC(void)
{
    glDeleteTextures(NUM_TEXTURES, textureIDs);
    for (int i = 0; i < NUM_LINKS; ++i)
    {
        if (links[i] != NULL)
        {
            free(links[i]);
        }
    }
}

int main(int argc, char *argv[])
{
    loadSTL();
    // calculate radius
    // from root to claw origin
    for (int i = 1; i < NUM_LINKS; ++i)
    {
        radius += m3dGetVectorLength(linkOrigins[i]);
    }
    // add claw length
    // find claw length from STL data (link 4)
    GLfloat maxY = -INFINITY;
    GLfloat minY = INFINITY;
    for (uint32_t j = 0; j < numTriangles[4]; ++j)
    {
        GLfloat v1y = links[4][j * 9 + 1];
        GLfloat v2y = links[4][j * 9 + 4];
        GLfloat v3y = links[4][j * 9 + 7];
        GLfloat vMaxY = fmaxf(v1y, fmaxf(v2y, v3y));
        GLfloat vMinY = fminf(v1y, fminf(v2y, v3y));
        if (vMaxY > maxY) maxY = vMaxY;
        if (vMinY < minY) minY = vMinY;
    }
    clawLength = maxY - minY;
    radius += clawLength;
    printf("Calculated workspace radius: %.2f\n", radius);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Robot Arm");

    glutDisplayFunc(RenderScene);
    glutReshapeFunc(ChangeSize);
    glutKeyboardFunc(HandleKey);

    glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("Immediate Mode", Default);
    glutAddMenuEntry("Vertex Array", VertexArray);
    glutAddMenuEntry("VBO", VBO);
    glutAttachMenu(GLUT_RIGHT_BUTTON);

    SetupRC();
    glutTimerFunc(33, TimerFunction, 1);
    glutMainLoop();
    ShutdownRC();
    return 0;
}