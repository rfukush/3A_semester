#include <GL/glut.h>

#include <math.h>

const float DEG2RAD = 3.14159/180;
 
void DrawCircle(double radius)
{
  int i;
  
  glBegin(GL_LINE_LOOP);
 
  for (i=0; i < 360; i++)
    {
      float degInRad = i*DEG2RAD;
      glVertex3f( cos(degInRad)*radius, 0.0, sin(degInRad)*radius );
    }
 
  glEnd();
}

void DrawPlanet(double radius )
{
  glPushMatrix();
  {
    glRotatef ( 90.0, 1.0, 0.0, 0.0);
    glutWireSphere(radius, 20, 20);
  }
  glPopMatrix();
}
