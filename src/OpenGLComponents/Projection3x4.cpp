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

#include "Projection3x4.h"
#include <utAlgorithm/Projection.h>

namespace Ubitrack { namespace Drivers {

Projection3x4::Projection3x4( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
{
	if ( subgraph->hasEdge( "Input" ) )
	{
		// new behaviour
		Graph::UTQLSubgraph::EdgePtr pEdge = subgraph->getEdge( "Input" );
		if ( pEdge->hasAttribute( "mode" ) && pEdge->getAttributeString( "mode" ) == "pull" )
			m_pPull.reset( new PullConsumer< Ubitrack::Measurement::Matrix3x4 >( "Input", *this ) );
		else
			m_pPush.reset( new PushConsumer< Ubitrack::Measurement::Matrix3x4 >( "Input", *this, boost::bind( &Projection3x4::inputIn, this, _1, 1 ) ) );
	}
	else
	{
		// legacy behaviour
		m_pPull.reset( new PullConsumer< Ubitrack::Measurement::Matrix3x4 >( "PullInput", *this ) );
		m_pPush.reset( new PushConsumer< Ubitrack::Measurement::Matrix3x4 >( "PushInput", *this, boost::bind( &Projection3x4::inputIn, this, _1, 1 ) ) );
	}
}

/** render the object */
void Projection3x4::draw( Measurement::Timestamp& time, int parity )
{
	// use projection in stereo mode only if correct eye
	if ( ( m_stereoEye == stereoEyeRight && parity ) || ( m_stereoEye == stereoEyeLeft && !parity ) )
		return;
		
	if ( m_pPull && m_pPull->isConnected() )
		inputIn( m_pPull->get(time), 0 );

	LOG4CPP_TRACE( logger, "Updating projection matrix to:" << std::endl << m_projection );
	glMatrixMode( GL_PROJECTION );
	glLoadMatrixd( m_projection.content() );
	glMatrixMode( GL_MODELVIEW );
}

/**
 * callback from Pose input port
 * @param pose input pose
 * @param redraw trigger a redraw? (only if called from push-input)
 */
void Projection3x4::inputIn( const Measurement::Matrix3x4& m, int redraw )
{
	m_lastUpdateTime = m.time();
	Math::Matrix< double, 3, 4 > mat3x4( *(m.get()) );
	
	// convert 3x4 to 4x4 matrix
	m_projection = Algorithm::projectionMatrixToOpenGL( 0, m_pModule->m_width, 0, m_pModule->m_height, m_pModule->m_near, m_pModule->m_far, mat3x4 );
	
	// the pose has changed, so redraw the world
	if (redraw) m_pModule->invalidate();
}

} } // namespace Ubitrack::Drivers

