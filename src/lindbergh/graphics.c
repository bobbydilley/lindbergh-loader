#include <GL/freeglut.h>
#include <dlfcn.h>

FGAPI int FGAPIENTRY glutEnterGameMode()
{
  glutCreateWindow("SEGA Lindbergh");
  return 1;
}

FGAPI void FGAPIENTRY glutLeaveGameMode()
{
  glutDestroyWindow(glutGetWindow());
  return;
}
