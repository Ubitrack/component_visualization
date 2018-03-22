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

#include "AntiMarker.h"

// should be replaced ..
#ifdef __APPLE__
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif

namespace Ubitrack { namespace Drivers {

AntiMarker::AntiMarker( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_push ( "PushInput", *this, boost::bind( &AntiMarker::positionIn, this, _1, 1 ))
	, m_factor( 0.06 )
{
	Graph::UTQLSubgraph::NodePtr objectNode = subgraph->getNode( "Object" );
	if (objectNode) objectNode->getAttributeData( "factor", m_factor );
}

/** render the object */
void AntiMarker::draw( Measurement::Timestamp& t, int parity )
{
	if (t > m_lastUpdateTime + 350000000L) return;
	boost::mutex::scoped_lock l( m_posLock );
	if (m_data.size() != 4) return;

	glMatrixMode(GL_MODELVIEW ); glPushMatrix(); glLoadIdentity();
	glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
	
	GLint vport[4]; glGetIntegerv( GL_VIEWPORT, vport );
	gluOrtho2D( vport[0], vport[0]+vport[2], vport[1], vport[1]+vport[3] );

	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	double d1x = m_factor*(m_data[0][0] - m_data[2][0]); double d2x = m_factor*(m_data[1][0] - m_data[3][0]);
	double d1y = m_factor*(m_data[0][1] - m_data[2][1]); double d2y = m_factor*(m_data[1][1] - m_data[3][1]);

	m_data[0][0] += d1x; m_data[0][1] += d1y; m_data[1][0] += d2x; m_data[1][1] += d2y;
	m_data[2][0] -= d1x; m_data[2][1] -= d1y; m_data[3][0] -= d2x; m_data[3][1] -= d2y;

	unsigned char colors[4][3];

	glReadPixels( (GLint)m_data[0][0], (GLint)m_data[0][1], 1, 1, GL_RGB, GL_UNSIGNED_BYTE, colors[0] );
	glReadPixels( (GLint)m_data[1][0], (GLint)m_data[1][1], 1, 1, GL_RGB, GL_UNSIGNED_BYTE, colors[1] );
	glReadPixels( (GLint)m_data[2][0], (GLint)m_data[2][1], 1, 1, GL_RGB, GL_UNSIGNED_BYTE, colors[2] );
	glReadPixels( (GLint)m_data[3][0], (GLint)m_data[3][1], 1, 1, GL_RGB, GL_UNSIGNED_BYTE, colors[3] );

	glBegin(GL_QUADS);
		glColor3ubv( colors[0] ); glVertex2f( (float)m_data[0][0], (float)m_data[0][1] );
		glColor3ubv( colors[1] ); glVertex2f( (float)m_data[1][0], (float)m_data[1][1] );
		glColor3ubv( colors[2] ); glVertex2f( (float)m_data[2][0], (float)m_data[2][1] );
		glColor3ubv( colors[3] ); glVertex2f( (float)m_data[3][0], (float)m_data[3][1] );
	glEnd();

	glEnable(GL_LIGHTING);
	glEnable(GL_DEPTH_TEST);

	glPopMatrix(); glMatrixMode(GL_MODELVIEW); glPopMatrix();
}

bool AntiMarker::hasWaitingEvents()
{
	//if ( m_pull.isConnected() ) return false;
	return ( m_push.getQueuedEvents() > 0 );
}

void AntiMarker::positionIn( const Ubitrack::Measurement::PositionList2& pos, int parity )
{
	m_lastUpdateTime = pos.time();
	boost::mutex::scoped_lock l( m_posLock );
	m_data = *pos;
	// the camera pose has changed, so redraw the world
	m_pModule->invalidate();
}

} } // namespace Ubitrack::Drivers
