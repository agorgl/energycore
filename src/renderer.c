#include <energycore/renderer.h>
#include <glad/glad.h>

void renderer_render()
{
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
