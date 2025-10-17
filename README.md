# Engine2D-Test
**👋ola eu estava tentando criar um projeto que quase simula um motor de uma Engine2D entao modifique por vontade próprio.**

_Como compilar e executar

- Linux (com SDL2): instale libsdl2-dev (ex.: apt install libsdl2-dev), então:
  - g++ engine2d.cpp -lSDL2 -std=c++17 -O2 -o engine2d
  - Coloque um arquivo player.bmp no mesmo diretório ou o engine cria um fallback.
  - ./engine2d

- Windows: use MSYS2/MinGW ou Visual Studio. Link com SDL2 (lib e include) conforme seu ambiente._