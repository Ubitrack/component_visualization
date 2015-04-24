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

//

#ifdef HAVE_GLEW
	#include "GL/glew.h"
#endif

#include "GL/freeglut.h"

#include "RenderModule.h"

log4cpp::Category& logger( log4cpp::Category::getInstance( "Drivers.Render" ) );
log4cpp::Category& loggerEvents( log4cpp::Category::getInstance( "Ubitrack.Events.Drivers.Render" ) );

#ifdef HAVE_OPENCV
	#include "BackgroundImage.h"
	#include "ZBufferOutput.h"
	#include "ImageOutput.h"
#endif

#ifdef HAVE_LAPACK
	#include "PoseErrorVisualization.h"
	#include "PositionErrorVisualization.h"
#endif

#ifdef HAVE_COIN
	#include "InventorObject.h"
#endif

//#include "Transparency.h"
#include "X3DObject.h"
#include "VectorfieldViewer.h"
#include "AntiMarker.h"
#include "PointCloud.h"
#include "Intrinsics.h"
#include "CameraPose.h"
#include "ButtonOutput.h"
#include "DropShadow.h"
#include "WorldFrame.h"
#include "Skybox.h"
#include "DirectionLine.h"
#include "Projection.h"
#include "Projection3x4.h"
#include "StereoSeparation.h"
#include "Cross2D.h"
#include "Fullscreen.h"
#include "StereoRendering.h"

#include <utUtil/Exception.h>
#include <utUtil/OS.h>
#include <boost/scoped_ptr.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <iomanip>
#include <math.h>

//OCL
#include <opencv2/core/ocl.hpp>
#include <CL/cl_gl.h>
#include <GL/glut.h>

namespace Ubitrack { namespace Drivers {

using namespace Dataflow;


// some unavoidable globals to manage the GLUT main loop and its handlers
std::deque< VirtualCamera* > g_setup;
std::map< std::string, int > g_names;
std::map< int, VirtualCamera* > g_modules;
std::set< VirtualObject* > g_cleanup_components;
boost::scoped_ptr< boost::thread > g_glutThread;
boost::mutex g_globalMutex;
boost::condition g_setup_performed;
boost::condition g_continue;
boost::condition g_cleanup_done;
#ifdef __APPLE__
	bool g_glutInitialized = false;
#endif

int g_run = 1;

// fake command line for GLUT (yes, the library _is_ 10 years old)
int   g_argc   = 1;
char* g_argv[] = { "VirtualCamera", 0 };



void g_mainloop()
{
	LOG4CPP_DEBUG( logger, "g_mainloop(): Render thread started" );

	boost::mutex::scoped_lock lock( g_globalMutex );

#ifdef __APPLE__
	if ( !g_glutInitialized )
	{	
		g_glutInitialized = true;
#else
	if ( !glutGet(GLUT_INIT_STATE) )
	{
#endif
		glutInit( &g_argc, g_argv );
		glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
		glutSetOption( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION );
	}

	while (g_run)
	{
		// are there any setup functions pending?
		if ( g_setup.size() > 0 ) {
			LOG4CPP_DEBUG( logger, "g_mainloop(): Calling setup()..." );

			// try to setup the first window in the queue
			VirtualCamera* tmp = g_setup.front(); 
			g_setup.pop_front();
			// if this failed, push it to the end of the queue
			if ( !(tmp->setup()) ) {
				g_setup.push_back( tmp );
				LOG4CPP_DEBUG( logger, "g_mainloop(): setup() failed, scheduling component to be set up again..." );
			}
			else {
				LOG4CPP_DEBUG( logger, "g_mainloop(): setup() successful" );
			}
			
			// let GLUT do its thing..
			glutMainLoopEvent();
			if ( g_setup.size() == 0 )
			{
				LOG4CPP_DEBUG( logger, "g_mainloop(): setup() all setup() activities performed" );
				g_setup_performed.notify_all();
			}
		}

		// are there any component cleanup functions pending?
		if ( ! g_cleanup_components.empty() ) 
		{
			LOG4CPP_DEBUG( logger, "g_mainloop(): Cleaning up GL context of all pending components..." );
			while ( ! g_cleanup_components.empty() )
			{
				VirtualObject * voPtr = *(g_cleanup_components.begin());
				voPtr->glCleanup();
				g_cleanup_components.erase( voPtr );

				// let GLUT do its thing..
				glutMainLoopEvent();
			}
			g_cleanup_done.notify_all();
			LOG4CPP_DEBUG( logger, "g_mainloop(): Cleaning done" );
		}
		
		// check
		// - if a redraw is needed for any window
		// - if any windows have been deleted

		std::map< int,VirtualCamera* >::iterator pos = g_modules.begin();
		std::map< int,VirtualCamera* >::iterator end = g_modules.end();

		while ( pos != end )
		{
			LOG4CPP_TRACE( logger, "g_mainloop(): iterating module with key '" << pos->first << "'..." );
		
			if ( pos->second )
			{
				LOG4CPP_TRACE( logger, "g_mainloop(): Call redraw()" );
			
				pos->second->redraw();
				pos++;
			} 
			else 
			{
				LOG4CPP_DEBUG( logger, "g_mainloop(): Destroying GL window with handle " << pos->first << "..." );

				glutDestroyWindow( pos->first );
				g_modules.erase( pos++ );

				// let GLUT do its thing..
				glutMainLoopEvent();

				LOG4CPP_DEBUG( logger, "g_mainloop(): GL window destroyed, " << g_modules.size() << " modules remaining" );

				// quit thread when last module is destroyed
				if ( g_modules.empty() )
				{
					glutMainLoopEvent();
					LOG4CPP_DEBUG( logger, "g_mainloop(): Render thread stopped" );
					return;
				}
			}
		}	

		// let GLUT do its thing..
		glutMainLoopEvent();

		// Be nice to the rest of the system. g_glutThread->yield() doesn't have any noticeable effect
		// with timeslices of the size that is common today. 5 ms maxes the frame rate at 200 Hz, but 
		// that should be sufficient for most kinds of hardware.
		// FIXME: probably needs to be increased to at least 60 Hz again for frame-sequential stereo
		// Can this only be done when stereo is actually used? Otherwise this will be a waste of processing time.(DP)
		g_continue.timed_wait( lock, boost::posix_time::milliseconds(100) );
		
		LOG4CPP_TRACE( logger, "g_mainloop(): timed_wait finished" );
	}
}


void g_display()
{
	VirtualCamera* win = g_modules[ glutGetWindow() ];
	if ( win ) win->display();
}


void g_keyboard( unsigned char key, int x, int y )
{
	VirtualCamera* win = g_modules[ glutGetWindow() ];
	if ( win ) win->keyboard( key, x, y );
}


void g_reshape( int w, int h )
{
	VirtualCamera* win = g_modules[ glutGetWindow() ];
	if ( win ) win->reshape( w, h );
}



int VirtualCamera::setup()
{
    LOG4CPP_DEBUG( logger, "setup(): Starting setup of window for module key " << m_moduleKey );

	// enable stencil buffer?
	if ( m_moduleKey.m_bEnableStencil )
		glutInitDisplayMode( GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE | GLUT_STENCIL );    
	
	if ( !m_moduleKey.m_sGameMode.empty() )
	{
		glutGameModeString( m_moduleKey.m_sGameMode.c_str() );
		m_winHandle = glutEnterGameMode();
	}
	else if ( m_moduleKey.substr(0,3) == "Sub" ) 
	{
		// sub-window -> check if the parent has already been created
		std::string parent = m_moduleKey.substr(3);
		if (g_names.find( parent ) == g_names.end()) return 0;
		// parent is there, so create the subwindow
		m_winHandle = glutCreateSubWindow( g_names[parent], 0, 0, m_width, m_height );
	} 
	else 
	{
		// create new top level window, 
		glutInitWindowSize( m_width, m_height );
		m_winHandle = glutCreateWindow( m_moduleKey.c_str() );
	}

    LOG4CPP_DEBUG( logger, "setup(): Window handle is " << m_winHandle );

	// store this-pointer and window handle
	g_modules[ m_winHandle ] = this;
	g_names[ m_moduleKey ] = m_winHandle;
	
	LOG4CPP_DEBUG( logger, "setup(): module '" << this->m_moduleKey << "' with key '" << m_winHandle << "' added to list" );

	// make full screen?
	if ( m_moduleKey.m_bFullscreen )
		#ifdef	_WIN32
			{
			Math::Vector< int, 2 > newSize = makeWindowFullscreen( m_moduleKey, m_moduleKey.m_monitorPoint );
			m_width = newSize( 0 );
			m_height = newSize( 1 );
			}
		#else
			glutFullScreen();
		#endif
	
	// GLEW provides access to OpenGL extensions
	#ifdef HAVE_GLEW
		glewInit();
	#endif

	// GL: enable and set colors
	glEnable( GL_COLOR_MATERIAL );
	glClearColor( 0.0, 0.0, 0.0, 1.0 ); // TODO: make this configurable (but black is best for optical see-through ar!)

	// GL: enable and set depth parameters
	glEnable( GL_DEPTH_TEST );
	glClearDepth( 1.0 );

	// GL: disable backface culling
	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glDisable( GL_CULL_FACE );

	// GL: light parameters
	GLfloat light_pos[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	GLfloat light_amb[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLfloat light_dif[] = { 0.9f, 0.9f, 0.9f, 1.0f };

	// GL: enable lighting
	glLightfv( GL_LIGHT0, GL_POSITION, light_pos );
	glLightfv( GL_LIGHT0, GL_AMBIENT,  light_amb );
	glLightfv( GL_LIGHT0, GL_DIFFUSE,  light_dif );
	glEnable( GL_LIGHTING );
	glEnable( GL_LIGHT0 );

	// GL: bitmap handling
	glPixelStorei( GL_PACK_ALIGNMENT,   1 );
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

	// GL: alpha blending
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable( GL_BLEND );

	// GL: misc stuff
	glShadeModel( GL_SMOOTH );
	glEnable( GL_NORMALIZE );

	// make functions known to GLUT
	glutKeyboardFunc( g_keyboard );
	glutDisplayFunc ( g_display  );
	glutReshapeFunc ( g_reshape  );

	initCL();

	ComponentList objects = getAllComponents();
	for ( ComponentList::iterator i = objects.begin(); i != objects.end(); i++ )
	{
        (*i)->glInit();
    }
     
	return 1;
}

void VirtualCamera::initCL()
{
	
	//Get all Platforms and select a GPU one
	cl_uint numPlatforms;
	clGetPlatformIDs (65536, NULL, &numPlatforms); 
	LOG4CPP_INFO( logger, "Platforms detected: " << numPlatforms );
 
	cl_platform_id* platformIDs;
	platformIDs = new cl_platform_id[numPlatforms];
 
	cl_int err = clGetPlatformIDs(numPlatforms, platformIDs, NULL);
	if(err != CL_SUCCESS)
	{
		LOG4CPP_INFO( logger, "error at clGetPlatformIDs :" << err );
	}
		
	cl_uint selectedPlatform =0;  // simply take first platform(index 0), code for taking correct one is long and not postet here
 
	cl_platform_id selectedPlatformID = platformIDs[selectedPlatform];
	delete[] platformIDs; 	
 
	cl_device_id selectedDeviceID; 
	//Select a GPU device
	err = clGetDeviceIDs(selectedPlatformID, CL_DEVICE_TYPE_GPU, 1, &selectedDeviceID, NULL);
	if(err != CL_SUCCESS)
	{
		LOG4CPP_INFO( logger, "error at clGetDeviceIDs :" << err );
	}

	char cDeviceNameBuffer[1024];
	clGetDeviceInfo (selectedDeviceID, CL_DEVICE_NAME, sizeof(char) *  1024, cDeviceNameBuffer, NULL);
	LOG4CPP_INFO( logger, ": Device Name: "		<< cDeviceNameBuffer );

	
	//Get a context with OpenGL connection
	cl_context_properties props[] = { 
		CL_GL_CONTEXT_KHR, 
		(cl_context_properties)wglGetCurrentContext(), 
		CL_WGL_HDC_KHR, 
		(cl_context_properties) wglGetCurrentDC(), 
		CL_CONTEXT_PLATFORM, 
		(cl_context_properties) selectedPlatformID, 
		0};
 
	cl_int status = 0;

	m_clContext = clCreateContext(props,1, &selectedDeviceID, NULL, NULL, &err);
	if(!m_clContext || err!= CL_SUCCESS)
	{
		LOG4CPP_INFO( logger,  "error at clCreateContext :" << err );
	}
	
	m_clCommandQueue = clCreateCommandQueue(m_clContext, selectedDeviceID, 0,&err);
	if(!m_clCommandQueue || err!= CL_SUCCESS)
	{
		LOG4CPP_INFO( logger, "error at clCreateCommandQueue :" << err );
	}
	
	cv::ocl::Context& oclContext = cv::ocl::Context::getDefault(false);
	oclContext.initContextFromHandle(selectedPlatformID, m_clContext, selectedDeviceID);	
}

void VirtualCamera::invalidate( VirtualObject* caller )
{
	if (m_redraw) return;
	// check if this is the last incoming update of several concurrent ones
	ComponentList objects = getAllComponents();
	for ( ComponentList::iterator i = objects.begin(); i != objects.end(); i++ ) {
		if ((*i).get() == caller) continue;
		if ((*i)->hasWaitingEvents()) return;
	}
	m_redraw = 1;
	LOG4CPP_DEBUG( logger, "invalidate(): Waking up main thread" );
	g_continue.notify_all();
}


/** Cleans up the specified component, blocks until the job has been completed on the GL task */
void VirtualCamera::cleanup( VirtualObject* vo )
{
	LOG4CPP_DEBUG( logger, "cleanup(): Lock mutex" );

	// Scoped lock
	boost::mutex::scoped_lock l( g_globalMutex );

	// Enqueue component for cleanup of GL context
	g_cleanup_components.insert( vo );

	LOG4CPP_DEBUG( logger, "cleanup(): Waking up GL thread" );

	// Wake up GL thread and wait until cleanup is done
	g_continue.notify_all();
	while ( g_cleanup_components.find( vo ) != g_cleanup_components.end() )
	{
		LOG4CPP_DEBUG( logger, "cleanup(): Block until GL context of component has been cleaned up" );

		// This will unlock the lock until wait returns so that the GL task my acquire the lock...
		g_cleanup_done.timed_wait ( l, boost::posix_time::milliseconds (25) );

		LOG4CPP_TRACE( logger, "cleanup(): Next trial, yet unprocessed components: " << g_cleanup_components.size() );
	}

	LOG4CPP_DEBUG( logger, "cleanup(): Cleanup of component's GL context done" );
}


void VirtualCamera::redraw( )
{
	// TODO: fix stereo view handling
	// TODO: make minmum fps configureable (currently 2fps)
	if ((!m_redraw) && (m_stereoRenderPasses == stereoRenderNone && m_lastRedrawTime + 500000000L < Measurement::now()  )) return; 
	glutSetWindow( m_winHandle );
	LOG4CPP_TRACE( logger, "redraw(): calling glutPostRedisplay" );
	glutPostRedisplay();
	m_redraw = 0;
}


VirtualCamera::VirtualCamera( const VirtualCameraKey& key, boost::shared_ptr< Graph::UTQLSubgraph >, FactoryHelper* pFactory )
	: Module< VirtualCameraKey, VirtualObjectKey, VirtualCamera, VirtualObject >( key, pFactory )
	, m_width(key.m_width)
	, m_height(key.m_height)
	, m_near(key.m_near)
	, m_far(key.m_far)
	, m_winHandle(0)
	, m_redraw(1)
	, m_doSync(0)
	, m_parity(0)
	, m_info(0)
	, m_lasttime(0)
	, m_lastframe(0)
	, m_fps(0)
	, m_lastRedrawTime(0)
	, m_vsync()
	, m_stereoRenderPasses( stereoRenderNone )
{
	LOG4CPP_DEBUG( logger, "VirtualCamera(): Creating module for module key '" << m_moduleKey << "'...");

	// lock access to globals
	boost::mutex::scoped_lock l( g_globalMutex );

	LOG4CPP_DEBUG( logger, "VirtualCamera(): Access to virtual camera map acquired");

	// schedule the setup function for this window
	g_setup.push_back( this );

	// if there's no thread yet, init GLUT library first and create a new control thread
	// Note: this thread must do ALL OpenGL operations for all VirtualCameras!
	// Most GL implementation do not look kindly on context sharing between threads!
	if ( g_glutThread.get() == 0 ) {
		LOG4CPP_DEBUG( logger, "VirtualCamera(): Creating single GL thread...");

		g_glutThread.reset( new boost::thread( boost::bind( g_mainloop ) ) );

		LOG4CPP_DEBUG( logger, "VirtualCamera(): Single GL thread created");
	}
}


VirtualCamera::~VirtualCamera()
{
	bool bKillThread = false;

	LOG4CPP_DEBUG( logger, "~VirtualCamera(): Destroying module for module key '" << m_moduleKey << "'...");
	
	{
		// lock access to globals and remove the stored this-pointer
		boost::mutex::scoped_lock lock( g_globalMutex );
		
		LOG4CPP_DEBUG( logger, "~VirtualCamera(): Access to virtual camera map acquired, destroying window with handle '" << m_winHandle << "'");
		
		/* 
		 * Ensure that all pending setup() activities are executed first. This is necessary for example during component 
		 * creation when all constructors are called sequentially and some component throws an exception so that the data flow
		 * management instance stops calling constructors and starts calling destructors.
		 */
		while ( ! g_setup.empty() ) 
		{
			LOG4CPP_DEBUG( logger, "~VirtualCamera(): waiting for scheduled setup() activities to terminate" );
			
			g_setup_performed.timed_wait( lock, boost::posix_time::milliseconds(100) );
		}
		
		g_modules[ m_winHandle ] = 0;
		g_names.erase( m_moduleKey );

		// kill thread if this was the last window
		if ( g_names.empty() ) {
			bKillThread = true;
		}
	}

	// the thread should die with the last render window
	if ( bKillThread )
	{
		LOG4CPP_DEBUG( logger, "~VirtualCamera(): Waiting for render thread to stop..." );

		g_glutThread->join();
		boost::mutex::scoped_lock l( g_globalMutex );
		g_glutThread.reset();

		LOG4CPP_DEBUG( logger, "~VirtualCamera(): Render thread stopped" );
	}

	LOG4CPP_DEBUG( logger, "~VirtualCamera(): module destroyed" );
}


void VirtualCamera::keyboard( unsigned char key, int x, int y )
{
	LOG4CPP_DEBUG( logger, "keyboard(): " << key << ", " << x << ", " << y );
	
	if (glutGetModifiers() & GLUT_ACTIVE_ALT)
	{
		switch ( key )
		{
			case 'f': 
				#ifdef	_WIN32
					// need to work around freeglut for multi-monitor fullscreen
					makeWindowFullscreen( m_moduleKey, Math::Vector< int, 2 >( 0xFFFF, 0xFFFF ) );
				#else
					glutFullScreen();
				#endif
				break;
			case 'i': m_info = !m_info; break;
			case 'v': m_doSync = !m_doSync; break;
			case 's': m_parity = !m_parity; break;
			// case 'q': delete this; break;
		}
	}
	else
	{
		m_lastKey = key;
		if ( x < 0 || y < 0 || x > m_width || y > m_height )
			m_lastMousePos = Math::Vector< double, 2 >( 0.5, 0.5 );
		else
			m_lastMousePos = Math::Vector< double, 2 >( double( x ) / m_width, double( y ) / m_height );
	}
	glutPostRedisplay();
}


unsigned char VirtualCamera::getLastKey() {
	unsigned char tmp = m_lastKey;
	m_lastKey = 0;
	return tmp;
}


Math::Vector< double, 2 > VirtualCamera::getLastMousePos() {
	return m_lastMousePos;
}




void VirtualCamera::display()
{
	m_lastRedrawTime = Measurement::now();

	// get frame counters and parity
	int parity = 0;
	int curframe = m_vsync.getFrame();
	if ( m_stereoRenderPasses == stereoRenderSequential )
	{
		// compute parity for frame sequential stereo
		int retrace  = m_vsync.getRetrace();
		parity = (retrace%2 == m_parity); // Nick: This seems to be broken on linux
	}

	// predict a little bit (only for pull inputs)
	Measurement::Timestamp imageTime( Measurement::now() + 5000000L );

	// clear buffers
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// create a perspective projection matrix
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( m_moduleKey.m_fov, ((double)m_width/(double)m_height), m_moduleKey.m_near, m_moduleKey.m_far );

	// clear model-view transformation
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	// calculate fps
	int curtime = glutGet( GLUT_ELAPSED_TIME );
	if ((curtime - m_lasttime) >= 1000) {
		m_fps = (1000.0*(curframe-m_lastframe))/((double)(curtime-m_lasttime));
		m_lasttime  = curtime;
		m_lastframe = curframe;
	}

	LOG4CPP_TRACE( logger, "display(): Redrawing.." );

	// iterate over all components (already sorted by priority thanks to std::map)
	ComponentList objects = getAllComponents();
	for ( ComponentList::iterator i = objects.begin(); i != objects.end(); i++ )
	{
		try
		{
			(*i)->draw( imageTime, parity ); // Parity = 0 if not frame sequential
		}
		catch( const Util::Exception& e )
		{
			LOG4CPP_NOTICE( loggerEvents, "display(): Exception in main loop from component " << (*i)->getName() << ": " << e );
		}
	}

	if ( m_stereoRenderPasses == stereoRenderSingle ) 
	{
		// 2nd rendering pass for stereo separation when both eyes are to be rendered into a single image
		// Only makes sense with color mask stereo separation.
		glClear( GL_DEPTH_BUFFER_BIT ); // Let color buffer intact, only clear depth information.
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity(); // Reset transformation stack.

		for ( ComponentList::iterator i = objects.begin(); i != objects.end(); i++ )
		{
			try
			{        
				(*i)->draw( imageTime, 1 ); // Parity = 1
			}
			catch( const Util::Exception& e )
			{
				LOG4CPP_NOTICE( loggerEvents, "display(): Exception in main loop from component " << (*i)->getName() << ": " << e );
			}
		}
	}

	// print info string
	if (m_info) {
  
		std::ostringstream text;
		text << std::fixed << std::showpoint << std::setprecision(2);
		text << "FPS: " << m_fps;
		text << " VSync: " << (m_doSync?"on":"off");

		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		glMatrixMode( GL_PROJECTION ); 
		glPushMatrix();
		glLoadIdentity();
		gluOrtho2D( 0, m_width, 0, m_height );
		glDisable( GL_LIGHTING );

		GLfloat xzoom, yzoom;
		glGetFloatv( GL_ZOOM_X, &xzoom );
		glGetFloatv( GL_ZOOM_Y, &yzoom );

		glPixelZoom( 1.0, 1.0 );

		glColor4f( 1.0, 0.0, 0.0, 1.0 );
		glRasterPos2i( 10, m_height-23 );

		for ( unsigned int i = 0; i < text.str().length(); i++ )
			glutBitmapCharacter( GLUT_BITMAP_8_BY_13, text.str()[i] );

		glPixelZoom( xzoom, yzoom );

		glPopMatrix();
		glEnable( GL_LIGHTING );
	}

	// wait for the screen refresh
	m_vsync.wait( m_doSync );
	
	// put current buffer into display
	LOG4CPP_TRACE( logger, "display(): Swapping buffers.." );
	glutSwapBuffers();
}


void VirtualCamera::reshape( int w, int h )
{
	LOG4CPP_DEBUG( logger, "reshape(): new size: " << w << "x" << h );
	
	// store new window size for reference
	m_width  = w;
	m_height = h;

	// set a whole-window viewport
	glViewport( 0, 0, m_width, m_height );

	// invalidate display
	glutPostRedisplay();
}


boost::shared_ptr< VirtualObject > VirtualCamera::createComponent( const std::string& type, const std::string& name, 
	boost::shared_ptr< Graph::UTQLSubgraph > pConfig, const VirtualObjectKey& key, VirtualCamera* pModule )
{
    LOG4CPP_DEBUG( logger, "createComponent(): called");
	
	//if ( type == "Transparency" )
	//	return boost::shared_ptr< VirtualObject >( new Transparency( name, pConfig, key, pModule ) );
	if ( type == "X3DObject" )
		return boost::shared_ptr< VirtualObject >( new X3DObject( name, pConfig, key, pModule ) );
	else if ( type == "VectorfieldViewer" )
		return boost::shared_ptr< VirtualObject >( new VectorfieldViewer( name, pConfig, key, pModule ) );
	else if ( type == "AntiMarker" )
		return boost::shared_ptr< VirtualObject >( new AntiMarker( name, pConfig, key, pModule ) );
	else if ( type == "PointCloud" )
		return boost::shared_ptr< VirtualObject >( new PointCloud( name, pConfig, key, pModule ) );
	else if ( type == "Intrinsics" )
		return boost::shared_ptr< VirtualObject >( new Intrinsics( name, pConfig, key, pModule ) );
	else if ( type == "CameraPose" )
		return boost::shared_ptr< VirtualObject >( new CameraPose( name, pConfig, key, pModule ) );
	else if ( type == "ButtonOutput" )
		return boost::shared_ptr< VirtualObject >( new ButtonOutput( name, pConfig, key, pModule ) );
	else if ( type == "DropShadow" )
		return boost::shared_ptr< VirtualObject >( new DropShadow( name, pConfig, key, pModule ) );
	else if ( type == "WorldFrame" )
		return boost::shared_ptr< VirtualObject >( new WorldFrame( name, pConfig, key, pModule ) );	
  	else if ( type == "Skybox" )
        return boost::shared_ptr< VirtualObject >( new Skybox( name, pConfig, key, pModule ) );	
  	else if ( type == "DirectionLine" )
		return boost::shared_ptr< VirtualObject >( new DirectionLine( name, pConfig, key, pModule ) );	
	else if ( type == "Projection" )
		return boost::shared_ptr< VirtualObject >( new Projection( name, pConfig, key, pModule ) );
	else if ( type == "Projection3x4" )
		return boost::shared_ptr< VirtualObject >( new Projection3x4( name, pConfig, key, pModule ) );
	else if ( type == "StereoSeparation" )
		return boost::shared_ptr< VirtualObject >( new StereoSeparation( name, pConfig, key, pModule ) );
	else if ( type == "StereoRendering" )
		return boost::shared_ptr< VirtualObject >( new StereoRendering( name, pConfig, key, pModule ) );
	else if ( type == "Cross2D" )
		return boost::shared_ptr< VirtualObject >( new Cross2D( name, pConfig, key, pModule ) );

	#ifdef HAVE_OPENCV
	else if ( type == "ImageOutput" )
		return boost::shared_ptr< VirtualObject >( new ImageOutput( name, pConfig, key, pModule ) );
	else if ( type == "ZBufferOutput" )
		return boost::shared_ptr< VirtualObject >( new ZBufferOutput( name, pConfig, key, pModule ) );
	else if ( type == "BackgroundImage" )
		return boost::shared_ptr< VirtualObject >( new BackgroundImage( name, pConfig, key, pModule ) );
	#endif

	#ifdef HAVE_LAPACK
	else if ( type == "PoseErrorVisualization" )
		return boost::shared_ptr< VirtualObject >( new PoseErrorVisualization( name, pConfig, key, pModule ) );
	else if ( type == "PositionErrorVisualization" )
		return boost::shared_ptr< VirtualObject >( new PositionErrorVisualization( name, pConfig, key, pModule ) );
	#endif

    #ifdef HAVE_COIN
	else if ( type == "InventorObject" )
		return boost::shared_ptr< VirtualObject >( new InventorObject( name, pConfig, key, pModule ) );
	#endif

	UBITRACK_THROW( "Class " + type + " not supported by render module" );

    LOG4CPP_DEBUG( logger, "createComponent(): done");
}


/** register module at factory */
UBITRACK_REGISTER_COMPONENT( ComponentFactory* const cf )
{
	std::vector< std::string > renderComponents;
	//renderComponents.push_back( "Transparency" );
	renderComponents.push_back( "X3DObject" );
	renderComponents.push_back( "VectorfieldViewer" );
	renderComponents.push_back( "AntiMarker" );
	renderComponents.push_back( "PointCloud" );
	renderComponents.push_back( "Intrinsics" );
	renderComponents.push_back( "CameraPose" );
	renderComponents.push_back( "ButtonOutput" );
	renderComponents.push_back( "DropShadow" );
    renderComponents.push_back( "WorldFrame" );
    renderComponents.push_back( "Skybox" );
    renderComponents.push_back( "DirectionLine" );
	renderComponents.push_back( "Projection" );
	renderComponents.push_back( "Projection3x4" );
	renderComponents.push_back( "StereoSeparation" );
	renderComponents.push_back( "StereoRendering" );
	renderComponents.push_back( "Cross2D" );
	#ifdef HAVE_OPENCV
		renderComponents.push_back( "ImageOutput" );
		renderComponents.push_back( "ZBufferOutput" );
		renderComponents.push_back( "BackgroundImage" );
	#endif

	#ifdef HAVE_LAPACK
		renderComponents.push_back( "PoseErrorVisualization" );
		renderComponents.push_back( "PositionErrorVisualization" );
	#endif
	#ifdef HAVE_COIN
		renderComponents.push_back( "InventorObject" );
	#endif
	cf->registerModule< VirtualCamera > ( renderComponents );
}


} } // namespace Ubitrack::Drivers

