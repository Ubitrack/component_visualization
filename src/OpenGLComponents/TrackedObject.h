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

#ifndef __TrackedObject_h_INCLUDED__
#define __TrackedObject_h_INCLUDED__

#include <boost/scoped_ptr.hpp>
#include "RenderModule.h"

namespace Ubitrack { namespace Drivers {


/**
 * @ingroup driver_components
 * Base class for tracked objects. 
 * Implements push and pull ports and sets the model-view matrix.
 */
class TrackedObject
	: public VirtualObject
{
public:

	/**
	 * Constructor
	 * @param name edge name
	 * @param config component configuration
	 * @param componentKey the unique identifier for this component
	 * @param pModule parent object
	 */
	TrackedObject( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
		const VirtualObjectKey& componentKey, VirtualCamera* pModule )
		: VirtualObject( name, subgraph, componentKey, pModule )
	{
		if ( subgraph->hasEdge( "Input" ) )
		{
			// new behaviour (Input port is either push or pull)
			Graph::UTQLSubgraph::EdgePtr pEdge = subgraph->getEdge( "Input" );
			if ( pEdge->hasAttribute( "mode" ) && pEdge->getAttributeString( "mode" ) == "pull" )
				m_pPull.reset( new PullConsumer< Ubitrack::Measurement::Pose >( "Input", *this ) );
			else
				m_pPush.reset( new PushConsumer< Ubitrack::Measurement::Pose >( "Input", *this, boost::bind( &TrackedObject::poseIn, this, _1, 1 ) ) );
		}
		else
		{
			// legacy behaviour (two separate ports for push and pull)
			m_pPull.reset( new PullConsumer< Ubitrack::Measurement::Pose >( "PullInput", *this ) );
			m_pPush.reset( new PushConsumer< Ubitrack::Measurement::Pose >( "PushInput", *this, boost::bind( &TrackedObject::poseIn, this, _1, 1 ) ) );
		}
	}

	/** 
	 * override this method to do the actual OpenGL drawing.
	 * The model-view matrix should already be set correctly.
	 */
	virtual void draw3DContent( Measurement::Timestamp& t, int parity ) = 0;
	
	/** render the object, if up-to-date tracking information is available */
	virtual void draw( Measurement::Timestamp& t, int parity )
	{
		if ( m_pPull && m_pPull->isConnected() ) 
			poseIn( m_pPull->get( t ), 0 );

		// remove object if no measurements in the last second
		// TODO: make this configurable
		if (t > m_lastUpdateTime + 1000000000L) return;

		glMatrixMode( GL_MODELVIEW );
		glPushMatrix();
		{
			boost::mutex::scoped_lock l( m_poseLock );
			glMultMatrixd( m_pose );
		}

		draw3DContent( t, parity );
		
		glPopMatrix();
	}

	virtual bool hasWaitingEvents()
	{
		return m_pPush && m_pPush->getQueuedEvents() > 0;
	}

protected:

	/**
	 * callback from Pose port
	 * @param pose current transformation
	 * @param redraw trigger a redraw? (only if called from push-input)
	 */
	void poseIn( const Ubitrack::Measurement::Pose& pose, int redraw )
	{
		m_lastUpdateTime = pose.time();
		Ubitrack::Math::Matrix< double, 4, 4 > m( pose->rotation(), pose->translation() );
		double* tmp =  m.content();

		boost::mutex::scoped_lock l( m_poseLock );
		for ( int i = 0; i < 16; i++ ) m_pose[i] = tmp[i];

		// the pose has changed, so redraw the world
		if (redraw) { 
			LOG4CPP_DEBUG( logger, "TrackedObject: calling invalidate()" );
			m_pModule->invalidate();
		}
	}

	// pose input
	boost::scoped_ptr< PushConsumer< Ubitrack::Measurement::Pose > > m_pPush;
	boost::scoped_ptr< PullConsumer< Ubitrack::Measurement::Pose > > m_pPull;

	double m_pose[16];
	boost::mutex m_poseLock;
};


} } // namespace Ubitrack::Drivers

#endif

