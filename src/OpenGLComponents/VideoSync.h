/*
 * Ubitrack - Library for Ubiquitous Tracking
 * Copyright 2006, Technische Universitaet Muenchen, and individual
 * contributors as indicated by the @authors tag. See the 
 * copyright.txt in the distribution for a full listing of individual
 * contributors.
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this software; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA, or see the FSF site: http://www.fsf.org.
 */

/**
 * @ingroup driver_components
 * helper class to synchronize OpenGL redraws
 * to the screen refresh frequency
 *
 * @author Florian Echtler <echtler@in.tum.de>
 */

#ifdef _WIN32
	#include <GL/gl.h>
	#include <GL/glu.h>
	#include <utUtil/CleanWindows.h>
#elif __APPLE__
	#include <OpenGL/OpenGL.h>
	#include <OpenGL/glu.h>
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
	#include <GL/glx.h>
#endif

class VideoSync {

	public:

		VideoSync( int fps = 0 );

		void wait( int flag );

		int getFrame();
		int getRetrace();

		static void alarm(int);

	private:

		int state, frame, mode;
		static int cont;

#ifdef _WIN32

		typedef BOOL (APIENTRY *PFNWGLSWAPINTERVALFARPROC)( int );

		PFNWGLSWAPINTERVALFARPROC wglSwapIntervalEXT;

#elif __APPLE__


#else

		typedef int (*pglXGetVideoSyncSGI)(unsigned int* count);
		typedef int (*pglXWaitVideoSyncSGI)(int divisor, int remainder, unsigned int* count );

		pglXGetVideoSyncSGI  glXGetVideoSyncSGI;
		pglXWaitVideoSyncSGI glXWaitVideoSyncSGI;

#endif

};

