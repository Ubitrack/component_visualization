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

#include "VideoSync.h"

void VideoSync::alarm( int ) { cont = 1; }
int  VideoSync::cont = 0;

int VideoSync::getFrame() { return frame; }

#ifdef _WIN32

	VideoSync::VideoSync( int fps ) {
		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALFARPROC)wglGetProcAddress( "wglSwapIntervalEXT" );
		state = 0;
		frame = 0;
	}

	void VideoSync::wait( int flag ) {
		frame++;
		if (flag == state) return;
		if (wglSwapIntervalEXT) wglSwapIntervalEXT( flag );
		state = flag;
	}

	int VideoSync::getRetrace() { return frame; }

#elif __APPLE__

	VideoSync::VideoSync( int fps ) {
		state = 0;
		frame = 0;
	}

	void VideoSync::wait( int flag ) {
		frame++;
		if (flag == state) return;
		const GLint tmp = flag;
		CGLSetParameter( CGLGetCurrentContext(), kCGLCPSwapInterval, &tmp );
		state = flag;
	}

	int VideoSync::getRetrace() { return frame; }

#else

	#include <signal.h>
	#include <unistd.h>
	#include <sys/time.h>

	/**
	 * create a video sync object
	 *
	 * @param fps framerate to synchronize ( 0 = hardware synchronization )
	 *
	 */
	VideoSync::VideoSync( int fps )
	{
		mode = fps;
		frame = 0;

		glXGetVideoSyncSGI  = (pglXGetVideoSyncSGI)  glXGetProcAddress( (const GLubyte*)"glXGetVideoSyncSGI"  );
		glXWaitVideoSyncSGI = (pglXWaitVideoSyncSGI) glXGetProcAddress( (const GLubyte*)"glXWaitVideoSyncSGI" );

		/*if (mode) {
			int usec = 1000000 / fps;
			struct itimerval delay = { { 0, usec }, { 0, usec } };
			signal( SIGALRM, alarm );
			setitimer( ITIMER_REAL, &delay, 0 );
		}*/
	}

	int VideoSync::getRetrace()
	{
		//if (mode) return frame;
		unsigned int retraceCount = 0;
		if (glXGetVideoSyncSGI) glXGetVideoSyncSGI(&retraceCount);
		return retraceCount;
	}

	void VideoSync::wait( int flag )
	{
		frame++;
		if (!flag) return;
		//if (mode) { while (!cont) usleep(100); cont = 0; return; }
		unsigned int retraceCount = getRetrace();
		if (glXWaitVideoSyncSGI) glXWaitVideoSyncSGI(2, (retraceCount+1)%2, &retraceCount);
	}

#endif

