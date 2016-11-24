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

#include "CameraPose.h"

namespace Ubitrack { namespace Drivers {


CameraPose::CameraPose( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_push ( "PushInput", *this, boost::bind( &CameraPose::poseIn, this, _1, 1 ))
	, m_pull ( "PullInput", *this )
{
}

/** render the object */
void CameraPose::draw( Measurement::Timestamp& t, int parity )
{
	if ( m_pull.isConnected() ) 
		poseIn( m_pull.get( t ), 0 );

	glMatrixMode( GL_MODELVIEW );
	boost::mutex::scoped_lock l( m_poseLock );
	glMultMatrixd( m_pose );
}

bool CameraPose::hasWaitingEvents()
{
	if ( m_pull.isConnected() ) return false;
	return ( m_push.getQueuedEvents() > 0 );
}

/**
 * callback from Pose port
 * @param pose current transformation
 */
void CameraPose::poseIn( const Ubitrack::Measurement::Pose& pose, int redraw )
{
	m_lastUpdateTime = pose.time();
	Ubitrack::Math::Pose invpose = ~(*pose);
	Ubitrack::Math::Matrix< double, 4, 4 > m( invpose.rotation(), invpose.translation() );
	double* tmp =  m.content();

	boost::mutex::scoped_lock l( m_poseLock );
	for ( int i = 0; i < 16; i++ ) m_pose[i] = tmp[i];

	// the camera pose has changed, so redraw the world
	if (redraw) {
		LOG4CPP_DEBUG( logger, "CameraPose: calling invalidate()" );
		m_pModule->invalidate();
	}
}


} } // namespace Ubitrack::Drivers

