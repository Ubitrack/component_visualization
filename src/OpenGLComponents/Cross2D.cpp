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

#include "Cross2D.h"

// should be replaced ..
#ifdef __APPLE__
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif

namespace Ubitrack { namespace Drivers {

Cross2D::Cross2D( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
{
	if ( subgraph->hasEdge( "Input" ) )
	{
		// new behaviour
		Graph::UTQLSubgraph::EdgePtr pEdge = subgraph->getEdge( "Input" );
		if ( pEdge->hasAttribute( "mode" ) && pEdge->getAttributeString( "mode" ) == "pull" )
			m_pInPositionPull.reset( new PullConsumer< Ubitrack::Measurement::Position2D >( "Input", *this ) );
		else
			m_pInPositionPush.reset( new PushConsumer< Ubitrack::Measurement::Position2D >( "Input", *this, boost::bind( &Cross2D::crossPositionIn, this, _1 ) ) );
	}
	else
	{
		// legacy behaviour
		m_pInPositionPush.reset( new PushConsumer< Ubitrack::Measurement::Position2D >( "PushInput", *this, boost::bind( &Cross2D::crossPositionIn, this, _1 ) ) );
		m_pInPositionPull.reset( new PullConsumer< Ubitrack::Measurement::Position2D >( "PullInput", *this ) );
	}
}

/** render the object */
void Cross2D::draw( Measurement::Timestamp& t, int num )
{
	LOG4CPP_DEBUG( logger, "Cross2D::draw" );

	if ( m_pInPositionPull && m_pInPositionPull->isConnected() )
	{
		LOG4CPP_DEBUG( logger, "pulling for cross position" );
		m_crossPosition = m_pInPositionPull->get( t );
	}

	if ( !m_crossPosition )
		return;
		
	GLfloat x, y;
	{
	boost::mutex::scoped_lock l( m_positionLock );
	x = static_cast< float >( (*m_crossPosition)( 0 ) );
	y = static_cast< float >( (*m_crossPosition)( 1 ) );
	}
	
	int m_width  = m_pModule->m_width;
	int m_height = m_pModule->m_height;
	
	LOG4CPP_DEBUG( logger, "drawing cross at [ " << x << ", " << y << " ], screen size: [ " << m_width << ", " << m_height << " ]" );
	
	// store the projection matrix
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();

	// create a 2D-orthogonal projection matrix
	glLoadIdentity();
	gluOrtho2D( 0.0, m_width, 0.0, m_height );

	// prepare fullscreen bitmap without fancy extras
	glDisable( GL_DEPTH_TEST );

	// save line width
	GLfloat fOldLineWidth = 1.0;
	glGetFloatv( GL_LINE_WIDTH, &fOldLineWidth );

	// draw the cross
	glColor3f( 1.0, 1.0, 0.0 );
	glLineWidth( 3.0 );
	glFlush();

	glBegin(GL_LINES);

	glVertex2f( x - 10, y );
	glVertex2f( x - 2,  y );
	glVertex2f( x + 2,  y );
	glVertex2f( x + 10, y );
	glVertex2f( x, y - 10 );
	glVertex2f( x, y - 2 );
	glVertex2f( x, y + 2 );
	glVertex2f( x, y + 10 );
	
	glEnd();

	// restore the GL state
	glLineWidth( fOldLineWidth );
	glEnable( GL_DEPTH_TEST );
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
}

void Cross2D::crossPositionIn( const Ubitrack::Measurement::Position2D& pos )
{
	LOG4CPP_DEBUG( logger, "received cross position " << pos );
	boost::mutex::scoped_lock l( m_positionLock );
	m_crossPosition = pos;
	m_pModule->invalidate( this );
}

bool Cross2D::hasWaitingEvents()
{
	return m_pInPositionPush && m_pInPositionPush->getQueuedEvents() > 0;
}

} } // namespace Ubitrack::Drivers
