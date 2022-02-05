
#pragma once
#include <tuple>
#include <string>

#include "touchdesigner/TOP_CPlusPlusBase.h"

namespace uma {
class FrameBuffer
{
public:
	FrameBuffer();
	virtual ~FrameBuffer();

	virtual void bind();
	virtual void unbind();
	virtual const std::string& allocateSingleColorTexture(int width, int height, int internalformat, int format, int type);
	virtual void clear();

	virtual GLuint getFboId() const;
	virtual GLuint getColorTextureId() const;
	virtual int getWidth() const;
	virtual int getHeight() const;

protected:
	virtual const std::tuple<bool, std::string>& checkFboStatus();

	int width_, height_;
	bool is_allocated_;
	GLuint id_;
	GLuint colorid_;
};
}
