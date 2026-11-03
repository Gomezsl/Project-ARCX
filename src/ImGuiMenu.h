#pragma once
namespace Menu {
void Render(int width, int height);          // llamar en cada eglSwapBuffers (contexto GL activo)
void OnTouchRaw(int a, int b, int c, int d); // puente desde el hook de toque del juego
void Shutdown();
}
