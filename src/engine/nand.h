#pragma once
// Lectura de ficheros de la NAND de la Wii (solo lectura). En Dolphin lee la NAND emulada (User/Wii/...).
#include <gctypes.h>

namespace nand {

// Lee un fichero entero, p. ej. "/shared2/FaceLib/RFL_Res.dat". Devuelve un búfer alineado a 32 bytes que hay que
// liberar con free(), o nullptr si no existe o no se puede leer. `size` recibe el tamaño en bytes.
u8* readFile(const char* path, u32* size);

s32 lastError();  // último código de error de ISFS (<0), para diagnóstico
u8* listDir(const char* path, u32* size);

}  // namespace nand
