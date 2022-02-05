#include "FrameBuffer.h"

using namespace uma;

FrameBuffer::FrameBuffer()
	: id_(0)
	, colorid_(0)
	, width_(2)
	, height_(2)
	, is_allocated_(false)
{}


FrameBuffer::~FrameBuffer()
{
	clear();
}


void FrameBuffer::bind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, id_);
}


void FrameBuffer::unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


const std::string& FrameBuffer::allocateSingleColorTexture(int width, int height, int internalformat, int format, int type)
{
	clear();

	glGenFramebuffers(1, &id_);
	glBindFramebuffer(GL_FRAMEBUFFER, id_);

	glGenTextures(1, &colorid_);
	glBindTexture(GL_TEXTURE_2D, colorid_);
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorid_, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	auto result = checkFboStatus();
	is_allocated_ = std::get<0>(result);
	if (is_allocated_)
	{
		width_ = width;
		height_ = height;
	}

	return std::get<1>(result);
}


void FrameBuffer::clear()
{
	glDeleteFramebuffers(1, &id_);
	id_ = 0;
	glDeleteTextures(1, &colorid_);
	colorid_ = 0;
	width_ = 2;
	height_ = 2;
	is_allocated_ = false;
}


const std::tuple<bool, std::string>& FrameBuffer::checkFboStatus()
{
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		return std::make_tuple(true, "FRAMEBUFFER_COMPLETE - OK");
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		return std::make_tuple(false, "FRAMEBUFFER_INCOMPLETE_ATTACHMENT");
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		return std::make_tuple(false, "FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
	case GL_FRAMEBUFFER_UNSUPPORTED:
		return std::make_tuple(false, "FRAMEBUFFER_UNSUPPORTED");
	default:
		return std::make_tuple(false, "UNKNOWN ERROR");
	}
}


GLuint FrameBuffer::getFboId() const { return id_; }

GLuint FrameBuffer::getColorTextureId() const { return colorid_; }

int FrameBuffer::getWidth() const { return width_; }

int FrameBuffer::getHeight() const { return height_; }
