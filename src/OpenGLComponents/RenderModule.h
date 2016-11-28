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
 * @file
 * OpenGL based X3D render component/viewer
 *
 * @par Example Query
 * @verbatim
<Pattern name="MyQery">
	<Input>
		<Node name="Camera">
			<Predicate>id=="someId"</Predicate>
		</Node>
		<Node name="Object">
			<Predicate>renderable=="true"&amp;&amp;virtualObjectX3DPath!=""</Predicate>
		</Node>
		<Edge name="PushInput" source="Camera" destination="Object">
			<Predicate>type=="6D"&amp;&amp;mode=="push"</predicate>
		</Edge>
		
		<!-- this is optional -->
		<Node name="ImagePlane"/>
		<Edge name="Image" source="Camera" destination="ImagePlane">
			<Predicate>type=="image"&aml;&amp;mode=="push"</Predicate>
		</Edge>
	</Input>
	<DataflowConfiguration>
		<UbitrackLib class="RenderModule"/>
	</DataflowConfiguration>
</Pattern> 
@endverbatim
 * The ImagePlane+Image part is optional and only required for the background image.
 *
 * @par Example Dataflow Configuration
 * @verbatim
<Pattern name="Renderer" id="Renderer1">
	<Input>
		<Node name="Camera" id="Camera1">
			<Attribute name="virtualCameraFov" value="30"/>
			<Attribute name="virtualCameraNear" value="0.01"/>
			<Attribute name="virtualCameraFar" value="40"/>
			<Attribute name="virtualCameraWidth" value="640"/>
			<Attribute name="virtualCameraHeight" value="480"/>
			<Attribute name="virtualCameraStereo" value="0.0"/>
		</Node>
		<Node name="Object" id="Object1">
			<Attribute name="virtualObjectX3DPath" value="..."/>
		</Node>
		<Edge name="PushInput" source="Camera" destination="Object" pattern-ref="..." edge-ref="..."/>
		
		<!-- this is optional -->
		<Node name="ImagePlane"/>
		<Edge name="Image" source="Camera" destination="ImagePlane" pattern-ref="..." edge-ref="..."/>
	</Input>
	<DataflowConfiguration>
		<UbitrackLib class="RenderModule"/>
	</DataflowConfiguration>
</Pattern> 
@endverbatim
 * @author Florian Echtler <echtler@in.tum.de>
 */


#ifndef __RenderModule_h_INCLUDED__
#define __RenderModule_h_INCLUDED__

#include <string>
#include <map>
#include <cstdlib>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include <log4cpp/Category.hh>

#include <utDataflow/PullConsumer.h>
#include <utDataflow/PushConsumer.h>
#include <utDataflow/PushSupplier.h>
#include <utDataflow/Component.h>
#include <utDataflow/Module.h>
#include <utMeasurement/Measurement.h>
#include <utMath/Matrix.h>

#include "VideoSync.h"

//opencl context
#ifdef HAVE_OPENCL
#ifdef __APPLE__
#include "OpenCL/opencl.h"
#else
#include "CL/cl.h"
#endif
#endif


using namespace Ubitrack::Dataflow;

extern log4cpp::Category& logger;


namespace Ubitrack { namespace Drivers {

// forward declaration
class VirtualObject;

// TODO: implement start/stop mechanism

/**
 * @ingroup driver_components
 * Module key for viewer component.
 * Represents the window that will be opened.
 */
class VirtualCameraKey
	: public NodeIdKey
{
public:

	/**
	 * Constructor
	 * If the configuration is buggy, defaults will be used.
	 * @param cc a configuration object
	 */
	VirtualCameraKey( boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
		: NodeIdKey( subgraph, "Camera" )
		, m_bFullscreen( false )
		, m_monitorPoint( Math::Vector< int, 2 >( 0, 0 ) )
		, m_bEnableStencil( false )
	{
		// some sane defaults
		m_fov  = 30;
		m_near = 0.01;
		m_far  = 100;

		m_width  = 800;
		m_height = 600;
		
		Graph::UTQLSubgraph::NodePtr cameraNode = subgraph->getNode( "Camera" );
		if ( cameraNode )
		{
			cameraNode->getAttributeData( "virtualCameraFov", m_fov );
			cameraNode->getAttributeData( "virtualCameraNear", m_near );
			cameraNode->getAttributeData( "virtualCameraFar", m_far );
			cameraNode->getAttributeData( "virtualCameraWidth", m_width );
			cameraNode->getAttributeData( "virtualCameraHeight", m_height );
		
			m_bFullscreen = cameraNode->getAttributeString( "virtualCameraFullscreen" ) == "true";
			cameraNode->getAttributeData( "virtualCameraMonitorX", m_monitorPoint( 0 ) );
			cameraNode->getAttributeData( "virtualCameraMonitorY", m_monitorPoint( 1 ) );
			m_sGameMode = cameraNode->getAttributeString( "virtualCameraGameMode" );
			
			// normally handlede by stereorendering, but we need it at module initialization
			m_bEnableStencil = cameraNode->getAttributeString( "stereoType" ) == "lineSequential";
		}
	}

	double m_fov, m_near, m_far;
	int m_width, m_height;
	
	bool m_bFullscreen;
	Math::Vector< int, 2 > m_monitorPoint;
	
	std::string m_sGameMode;
	
	bool m_bEnableStencil;
};


/**
 * @ingroup driver_components
 * Component key for virtual objects.
 */
class VirtualObjectKey
	: public SubgraphIdKey
{
	public:

		/// set priority based on dataflow class as stored in the subgraph by the dataflow network
		VirtualObjectKey( boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
			: SubgraphIdKey( subgraph )
			, m_priority( 100 ) // default priority for render objects
		{
			// lower priority for occlusion objects
			if ( subgraph->hasNode( "Object" ) && subgraph->getNode( "Object" )->hasAttribute( "occlusionOnly" ) && 
				subgraph->getNode( "Object" )->getAttribute( "occlusionOnly" ).getText() == "true" )
				m_priority -= 25;
		
			std::string& dfclass = subgraph->m_DataflowClass;
			
			if ( dfclass == "Intrinsics"       ) m_priority =  10;
			if ( dfclass == "CameraPose"       ) m_priority =  10;
			if ( dfclass == "Projection"       ) m_priority =  10;
			if ( dfclass == "Projection3x4"    ) m_priority =  10;
			if ( dfclass == "StereoRendering"  ) m_priority =  20;
			if ( dfclass == "BackgroundImage"  ) m_priority =  40;
			if ( dfclass == "Transparency"     ) m_priority =  45;
			if ( dfclass == "Cross2D"          ) m_priority =  50;
			if ( dfclass == "AntiMarker"       ) m_priority =  50;
			if ( dfclass == "StereoSeparation" ) m_priority =  50;
			if ( dfclass == "DropShadow"       ) m_priority = 150;
			if ( dfclass == "ImageOutput"      ) m_priority = 200;
			if ( dfclass == "ButtonOutput"     ) m_priority = 200;
			if ( dfclass == "ZBufferOutput"    ) m_priority = 200;
		}

		// compare priorities first, then string contents
		bool operator<( const VirtualObjectKey& b ) const
		{
			if (m_priority == b.m_priority) return ((SubgraphIdKey)(*this) < (SubgraphIdKey)b);
			return (m_priority < b.m_priority);
		}

	private:

		int m_priority;
};


/**
 * @ingroup driver_components
 * Module for virtual OpenGL camera.
 * Opens window and does the rendering.
 */
class VirtualCamera
	: public Module< VirtualCameraKey, VirtualObjectKey, VirtualCamera, VirtualObject >
{
public:

	/**
	 * Constructor
	 * @param key which VirtualCamera am I?
	 * @param pFactory internal parameter from ComponentFactory
	 */
	VirtualCamera( const VirtualCameraKey& key, boost::shared_ptr< Graph::UTQLSubgraph >, FactoryHelper* pFactory );

	/** Destructor */
	~VirtualCamera();

	/** keyboard callback */
	void keyboard( unsigned char key, int x, int y );

	/** button output helper function */
	unsigned char getLastKey();

	/** button output helper function */
	Math::Vector< double, 2 > getLastMousePos();

	/** reshape callback */
	void reshape( int w, int h );

	/** display callback */
	void display(int ellapsed_time);

	/** callback from the VirtualObjects if world has changed */
	void invalidate( VirtualObject* caller = 0 );

	/** setup for GL context, called from main GL thread _only_ */
	int setup();

	/** cleanup GL context, called from main GL thread _only_ */
	void cleanup( VirtualObject* vo );

	/** redraw GL context, called from main GL thread _only_ */
	void redraw();

	/** isSetupComplete ? **/
	bool isSetupComplete();

	/** create new components. Necessary to support multiple component types. */
	boost::shared_ptr< VirtualObject > createComponent( const std::string& type, const std::string& name, 
		boost::shared_ptr< Graph::UTQLSubgraph > pConfig, const VirtualObjectKey& key, VirtualCamera* pModule );
		
	/** should not be public, but fast access from components needed */
	// FIXME: use friends instead? Getter method?
	int m_width, m_height;
	double m_near, m_far;
	
	/** define how stereo is rendered */
	enum StereoRenderPasses { stereoRenderNone, stereoRenderSequential, stereoRenderSingle };

	/** called by components to set stereo mode */
	void setStereoRenderPasses( StereoRenderPasses srp )
	{ m_stereoRenderPasses = srp; }

protected:

	int m_winHandle, m_redraw, m_doSync, m_parity, m_info, m_lasttime, m_lastframe;
	unsigned char m_lastKey;
	Math::Vector< double, 2 > m_lastMousePos;
	double m_fps;
	Measurement::Timestamp m_lastRedrawTime;

	VideoSync m_vsync;
	
	StereoRenderPasses m_stereoRenderPasses;

	static int m_window;

	bool m_isSetupComplete;

};


/**
 * @ingroup driver_components
 * Abstract component class for OpenGL render objects.
 * Provides a push-in port for poses and a push-in port for images.
 */
class VirtualObject
	: public VirtualCamera::Component
{
public:

	/**
	 * Constructor
	 * @param name edge name
	 * @param config component configuration
	 * @param componentKey the unique identifier for this component
	 * @param pModule parent object
	 */
	VirtualObject( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, const VirtualObjectKey& componentKey, VirtualCamera* pModule )
		: VirtualCamera::Component( name, componentKey, pModule )
		, m_stereoEye( stereoEyeAll )
	{
		// render only on one side?
		if ( subgraph->hasNode( "ImagePlane" ) && subgraph->getNode( "ImagePlane" )->hasAttribute( "stereoEye" ) )
			m_stereoEye = subgraph->getNode( "ImagePlane" )->getAttributeString( "stereoEye" ) == "left" ? stereoEyeLeft : stereoEyeRight;

		bCleanup = false;
	}

	virtual void stop()
	{
		// Trigger the GL cleanup first. This blocks until actions have been performed on behalf of the GL task.
		getModule().cleanup( this );
		
		// Then invoke stop() in superclass
		VirtualCamera::Component::stop();
	}

	virtual int setup() {
		return getModule().setup();
	}

	/** Initialization of GL context, gets called on the GL thread only. */
    virtual void glInit()
    {}

	/** Cleanup of GL context, gets called on the GL thread only. */
    virtual void glCleanup()
    {}

	/** render the object */
	virtual void draw( Measurement::Timestamp& t, int parity )
	{}

	/** check if there are events waiting for this component */
	virtual bool hasWaitingEvents( )
	{
		return false;
	}
	
	Measurement::Timestamp getTime()
	{ return m_lastUpdateTime; }

protected:

	/** stereo configuration. Some objects only want to be drawn on one side */
	enum { stereoEyeAll, stereoEyeLeft, stereoEyeRight } m_stereoEye;
	
	Measurement::Timestamp m_lastUpdateTime;

	bool bCleanup;
};

} } // namespace Ubitrack::Drivers

#endif

