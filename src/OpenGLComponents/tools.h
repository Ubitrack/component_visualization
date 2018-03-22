#ifndef AUX_H
#define AUX_H

#include <tinyxml.h>

#include <sstream>
#include <vector>
#include <map>

// GLAD
#include <glad/glad.h>

// #ifdef _WIN32
// 	#include <utUtil/CleanWindows.h>
// 	#include <GL/gl.h>
// 	#include <GL/glu.h>
// #elif __APPLE__
// 	#include <OpenGL/OpenGL.h>
// 	#include <OpenGL/glu.h>
// #else
// 	#include <GL/gl.h>
// 	#include <GL/glu.h>
// 	#include <GL/glx.h>
// #endif


#include "Tuple.h"
#include "Triple.h"


/*#include <stdio.h>
#define glError() { \
        GLenum err = glGetError(); \
        while (err != GL_NO_ERROR) { \
                fprintf(stderr, "glError: %s caught at %s:%u\n", (char *)gluErrorString(err), __FILE__, __LINE__); \
                err = glGetError(); \
        } \
}*/

GLfloat unproject(int screen_x, int screen_y, Vector* click, Vector* origin, GLfloat screen_z = -1.0);


void drawTexturedSphere( GLdouble radius, GLint slices, GLint stacks );
void drawTexturedCylinder( GLdouble radius, GLdouble height, GLint slices, GLint stacks );
void drawTexturedCone( GLdouble base, GLdouble height, GLint slices, GLint stacks );
void drawTexturedBox( GLdouble x, GLdouble y, GLdouble z );

void drawWireCone(GLdouble base, GLdouble height, GLint slices, GLint stacks);

void drawString( std::string text );



void loadTexture( const char* url, bool repeatS, bool repeatT );


int parseAttribute( const TiXmlAttribute* attrib, const std::string& name, bool* res );
int parseAttribute( const TiXmlAttribute* attrib, const std::string& name, std::string* res );

int parseAttribute(
	const TiXmlAttribute* attrib,
	const std::string& name,
	double* res0,
	double* res1 = 0,
	double* res2 = 0,
	double* res3 = 0
);




void process( std::istringstream& stream, std::vector< Triangle >* storage );
void process( std::istringstream& stream, std::vector< Vector   >* storage );
void process( std::istringstream& stream, std::vector< TexVec   >* storage );


template< class Type > std::vector< Type >* getList(
	const TiXmlElement* element, 
	std::map< const TiXmlElement*, std::vector< Type > >& storage
) {
	typename std::map< const TiXmlElement*, std::vector< Type > >::iterator it = storage.find( element );
	if ( it != storage.end() ) return &(it->second);
	return 0;
}

template< class Type> void printList( std::vector< Type >* foo ) {
	if (!foo) return;
	typename std::vector<Type>::iterator pos = foo->begin();
	typename std::vector<Type>::iterator end = foo->end();
	for ( ; pos != end; pos++) std::cout << *pos << " ";
	std::cout << std::endl;
}

template< class Type > std::vector< Type >* parseList(
	const TiXmlElement* element, 
	const TiXmlAttribute* attrib, 
	const std::string& name,
	std::map< const TiXmlElement*, std::vector< Type > >& storage
) {
	if ( attrib->Name() != name ) return 0;
	std::vector< Type >* tmp = getList< Type >( element, storage );
	if (tmp) return tmp;
	std::istringstream stream( attrib->Value() );
	storage[element].clear();
	tmp = &(storage[element]);
	process( stream, tmp );
	return tmp;
}

#endif

