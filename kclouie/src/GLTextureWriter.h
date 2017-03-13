#ifndef __GLTextureWriter__
#define __GLTextureWriter__ 

#include <string>
#include "Texture.h"
#include <glfw/glfw3.h>
/**
 * GLTextureWriter outputs a three channel (GL_RGB) 
 * image to a png file given by file name.
 *
 * Contact kpidding@calpoly.edu for any support questions!
 */
namespace GLTextureWriter
{
	bool WriteImage(std::shared_ptr<Texture> texture, std::string fileName);
	bool WriteImage(const Texture & texture, std::string fileName);
	bool WriteImage(GLint textureHandle, std::string fileName);
}
#endif