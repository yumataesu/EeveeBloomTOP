#include "Quad.h"

using namespace uma;

Quad::Quad()
	: vao_(0)
	, vertex_vbo_(0)
	, texcoord_vbo_(0)
	, ebo_(0)
{}


Quad::~Quad()
{
	clear();
}


void Quad::clear()
{
	glDeleteVertexArrays(1, &vao_);
	glDeleteBuffers(1, &vertex_vbo_);
	glDeleteBuffers(1, &texcoord_vbo_);
	glDeleteBuffers(1, &ebo_);
}


void Quad::create()
{
	clear();

	GLfloat vertices[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f
	};

	GLfloat texcoords[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	int indices[] = { 0, 1, 2, 0, 3, 2 };


	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vertex_vbo_);
	glGenBuffers(1, &texcoord_vbo_);
	glGenBuffers(1, &ebo_);


	glBindVertexArray(vao_);

	// Position attribute
	glBindBuffer(GL_ARRAY_BUFFER, vertex_vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

	// Texcoord attribute
	glBindBuffer(GL_ARRAY_BUFFER, texcoord_vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);

	// Element Array Buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


void Quad::draw() const
{
	glBindVertexArray(vao_);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}