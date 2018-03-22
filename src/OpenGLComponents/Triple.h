#ifndef TRIPLE_H
#define TRIPLE_H

#include <utVisualization/OpenGLWrapper.h>

#include <iostream>
#include <math.h>


template< typename Type > class Triple {

	public:

		Triple operator-() {
			Triple tmp;
			tmp.a = -a;
			tmp.b = -b;
			tmp.c = -c;
			return tmp;
		}

		Triple operator-(Triple in) {
			Triple tmp;
			tmp.a = a - in.a;
			tmp.b = b - in.b;
			tmp.c = c - in.c;
			return tmp;
		}

		Triple operator&(Triple in){
			Triple tmp;
			tmp.a = b*in.c - c*in.b;
			tmp.b = c*in.a - a*in.c;
			tmp.c = a*in.b - b*in.a;
			return tmp;
		}


		void normalize() { double tmp = length(); if (tmp == 0.0) return; a = a/tmp; b = b/tmp; c = c/tmp; }

		void set( Type _a, Type _b, Type _c ) { a = _a; b = _b; c = _c; }

		double length() { return sqrt(a*a+b*b+c*c); }

		Type a,b,c;
};


typedef Triple< GLfloat > Vector;
typedef Triple< GLuint  > Triangle;


template< typename Type > std::ostream& operator<< ( std::ostream& s, const Triple<Type>& t ) {
	return s << "[ " << t.a << " " << t.b << " " << t.c << " ]";
}

#endif

