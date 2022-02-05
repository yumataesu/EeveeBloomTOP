#pragma once

#include "touchdesigner/TOP_CPlusPlusBase.h"

namespace uma {
class Quad
{
public:
	Quad();
	virtual ~Quad();

	void clear();
	void create();
	void draw() const;

protected:
	GLuint vao_;
	GLuint vertex_vbo_, texcoord_vbo_, ebo_;
};
}

