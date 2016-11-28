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

#include <utVision/OpenCLManager.h>

#include "RenderModule.h"
#include "tools.h"

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
#include "CustomGeometry.h"
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
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <iomanip>
#include <math.h>



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


//OCL
#ifdef HAVE_OPENCL
#include <opencv2/core/ocl.hpp>
#ifdef __APPLE__
#include "OpenCL/cl_gl.h"
#else
#include "CL/cl_gl.h"
#endif
#endif

#include "utVisualization/utRenderAPI.h"


namespace Ubitrack { namespace Drivers {

using namespace Dataflow;
using namespace Visualization;


class CameraHandleImpl : public CameraHandle {

public:
	CameraHandleImpl(std::string& _name, int _width, int _height, VirtualCamera* _handle)
	: CameraHandle(_name, _width, _height, _handle)
	, m_last_xpos( 0. )
	, m_last_ypos( 0. )
	{
	}

	~CameraHandleImpl()
	{
	}

	virtual bool setup(boost::shared_ptr<VirtualWindow>& window) {
		bool ret = CameraHandle::setup(window);
		if (m_pVirtualCamera != NULL) {
			VirtualCamera::ComponentList objects = m_pVirtualCamera->getAllComponents();
			for ( VirtualCamera::ComponentList::iterator i = objects.begin(); i != objects.end(); i++ ) {
				(*i)->setup();
				(*i)->glInit();
			}
		}
		return ret;
	}

	virtual void teardown() {
		if (m_pVirtualCamera != NULL) {
			VirtualCamera::ComponentList objects = m_pVirtualCamera->getAllComponents();
			for ( VirtualCamera::ComponentList::iterator i = objects.begin(); i != objects.end(); i++ ) {
				(*i)->glCleanup();
			}
		}
		CameraHandle::teardown();
	}

	virtual void render(int ellapsed_time) {
		if (m_pVirtualCamera != NULL) {
			m_pVirtualCamera->display(ellapsed_time);
		}
	}

	virtual void on_keypress(int key, int scancode, int action, int mods) {
		if (m_pVirtualCamera != NULL) {
			m_pVirtualCamera->keyboard((unsigned char) key, (int) m_last_xpos, (int) m_last_ypos);
		}
	}

	virtual void on_render(int ellapsed_time) {
		if (m_pVirtualCamera != NULL) {
//			m_pVirtualCamera->display(ellapsed_time);
		}
	}

	virtual void on_cursorpos(double xpos, double ypos) {
		m_last_xpos = xpos;
		m_last_ypos = ypos;
	}

private:
	double m_last_xpos, m_last_ypos;

};




int VirtualCamera::setup()
{
	if (!m_isSetupComplete) {
		LOG4CPP_INFO( logger, "VirtualCamera: setup" );

/** FULLSCREEN NOT MIGRATED .
	// make full screen?
	if ( m_moduleKey.m_bFullscreen ) {
#ifdef    _WIN32
		Math::Vector< int, 2 > newSize = makeWindowFullscreen( m_moduleKey, m_moduleKey.m_monitorPoint );
		m_width = newSize( 0 );
		m_height = newSize( 1 );
#else
		//glutFullScreen();
		// glfw supports screen-size windows: http://www.glfw.org/docs/latest/window.html#window_windowed_full_screen
#endif
	}

**/

		// setup is done in the CameraHandle setup method.
		m_isSetupComplete = true;

	}
	return 1;
}


void VirtualCamera::invalidate( VirtualObject* caller )
{
//	if (m_redraw) return;
	// check if this is the last incoming update of several concurrent ones
	ComponentList objects = getAllComponents();
	for ( ComponentList::iterator i = objects.begin(); i != objects.end(); i++ ) {
		if ((*i).get() == caller) continue;
		if ((*i)->hasWaitingEvents()) return;
	}
//	m_redraw = 1;
	LOG4CPP_DEBUG( logger, "invalidate(): Waking up main thread" );
	RenderManager::singleton().notify_ready();
}


/** Cleans up the specified component, blocks until the job has been completed on the GL task */
void VirtualCamera::cleanup( VirtualObject* vo )
{

	// this method is called from the VirtualCameraModule in order to signal that it's done
	// for now, all components are cleaned in the teardown of the VirtualCamera

	// dynamic deregistration only makes sense, if dynamic registration works,
	// but this seems not to be the case for the existing GLUT implementation
}


void VirtualCamera::redraw( )
{
	// TODO: fix stereo view handling
	// TODO: make minmum fps configureable (currently 2fps)
	if ((!m_redraw) && (m_stereoRenderPasses == stereoRenderNone && m_lastRedrawTime + 500000000L < Measurement::now()  )) return; 
//	glutSetWindow( m_winHandle );
//	LOG4CPP_TRACE( logger, "render(): calling glutPostRedisplay" );
//	glutPostRedisplay();
	m_redraw = 0;

}

bool VirtualCamera::isSetupComplete() {
	return m_isSetupComplete;
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
	std::string window_name(m_moduleKey.c_str());

	// XXX can this be simplified ??
	boost::shared_ptr<CameraHandleImpl> cam( new CameraHandleImpl(window_name, m_width, m_height, this));
	boost::shared_ptr<CameraHandle> cam_ = boost::dynamic_pointer_cast<CameraHandle>(cam);
	m_winHandle = RenderManager::singleton().register_camera(cam_);
}


VirtualCamera::~VirtualCamera()
{

	LOG4CPP_DEBUG( logger, "~VirtualCamera(): Destroying module for module key '" << m_moduleKey << "'...");

	// need to remove ourselves from the render-manager
	RenderManager::singleton().unregister_camera(m_winHandle);
	// more cleanup needed ?

	LOG4CPP_DEBUG( logger, "~VirtualCamera(): module destroyed" );
}


void VirtualCamera::keyboard( unsigned char key, int x, int y )
{
	LOG4CPP_DEBUG( logger, "keyboard(): " << key << ", " << x << ", " << y );

	/** accessing the alt-modifier needs a change in the function signature.
	if (glutGetModifiers() & GLUT_ACTIVE_ALT)
	{
		switch ( key )
		{
			case 'f': 
				#ifdef	_WIN32
					// need to work around freeglut for multi-monitor fullscreen
					makeWindowFullscreen( m_moduleKey, Math::Vector< int, 2 >( 0xFFFF, 0xFFFF ) );
				#else
//					glutFullScreen();
				#endif
				break;
			case 'i': m_info = !m_info; break;
			case 'v': m_doSync = !m_doSync; break;
			case 's': m_parity = !m_parity; break;
			// case 'q': delete this; break;
		}
	}
	else
	**/
	{
		m_lastKey = key;
		if ( x < 0 || y < 0 || x > m_width || y > m_height )
			m_lastMousePos = Math::Vector< double, 2 >( 0.5, 0.5 );
		else
			m_lastMousePos = Math::Vector< double, 2 >( double( x ) / m_width, double( y ) / m_height );
	}
//	glutPostRedisplay();
}


unsigned char VirtualCamera::getLastKey() {
	unsigned char tmp = m_lastKey;
	m_lastKey = 0;
	return tmp;
}


Math::Vector< double, 2 > VirtualCamera::getLastMousePos() {
	return m_lastMousePos;
}




void VirtualCamera::display(int ellapsed_time)
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
	int curtime = (int)(ellapsed_time);
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

		drawString( text.str() );
//		for ( unsigned int i = 0; i < text.str().length(); i++ )
//			glutBitmapCharacter( GLUT_BITMAP_8_BY_13, text.str()[i] );

		glPixelZoom( xzoom, yzoom );

		glPopMatrix();
		glEnable( GL_LIGHTING );
	}

	// wait for the screen refresh

	// XXX commented out for now .. maybe needs extra api
	m_vsync.wait( m_doSync );
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
//	glutPostRedisplay();
}


boost::shared_ptr< VirtualObject > VirtualCamera::createComponent( const std::string& type, const std::string& name, 
	boost::shared_ptr< Graph::UTQLSubgraph > pConfig, const VirtualObjectKey& key, VirtualCamera* pModule )
{
    LOG4CPP_DEBUG( logger, "createComponent(): called");
	
	//if ( type == "Transparency" )
	//	return boost::shared_ptr< VirtualObject >( new Transparency( name, pConfig, key, pModule ) );
	if ( type == "CustomGeometry" )
		return boost::shared_ptr< VirtualObject >( new CustomGeometry( name, pConfig, key, pModule ) );
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
	renderComponents.push_back( "CustomGeometry" );
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

