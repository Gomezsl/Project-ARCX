#pragma once
namespace Menu {
void Render(int width, int height);          // llamar en cada eglSwapBuffers (contexto GL activo)
bool OnTouchRaw(int a, int b, int c, int d); // hook de toque; devuelve true si el menu consume el evento
void Shutdown();
}
