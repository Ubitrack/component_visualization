#ifndef TUPLE_H
#define TUPLE_H

#include <utVision/OpenGLPlatform.h>

#include <iostream>


template< typename Type > class Tuple {
	public:
		void set( Type _a, Type _b ) { a = _a; b = _b; }
		Type a,b;
};


typedef Tuple< GLfloat > TexVec;


template< typename Type > std::ostream& operator<< ( std::ostream& s, const Tuple<Type>& t ) {
	return s << "[ " << t.a << " " << t.b << " ]";
}

template< typename Type > bool operator!=( const Tuple<Type>& self, const Tuple<Type> &other) {
	return ((self.a != other.a) || (self.b != other.b));
}

#endif

