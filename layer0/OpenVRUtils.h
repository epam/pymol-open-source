#ifndef _H_OpenVRUtils
#define _H_OpenVRUtils

#include "os_gl.h"

namespace OpenVRUtils {

GLuint CompileProgram(char const* vertexShader,  char const* fragmentShader, char const* attributes[] = 0);

GLuint LoadTexture(unsigned width, unsigned height, unsigned char const* ptr);
GLuint LoadTexture(char const* filename);

void VectorNormalize(float v[]);
void VectorCrossProduct(float const v1[], float const v2[], float cross[]);

void MatrixFastInverseGLGL(float const srcGL44[], float dstGL44[]);
void MatrixFastInverseVRGL(float const srcVR34[], float dstGL44[]);
void MatrixCopyVRGL(float const srcVR34[], float dstGL44[]);

} // namespace OpenVRUtils

#endif // _H_OpenVRUtils
