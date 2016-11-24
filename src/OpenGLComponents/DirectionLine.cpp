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

#ifdef HAVE_GLEW
	#include "GL/glew.h"
#endif

#include "DirectionLine.h"
#include "tools.h"

namespace Ubitrack { namespace Drivers {


DirectionLine::DirectionLine( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
							  const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_target_port( "TargetPosition", *this, boost::bind( &DirectionLine::dataIn, this, _1, 1 ))
	, m_source_port( "SourcePosition", *this )
	, m_thickness ( 0.5 )
{
	// read parameters
	subgraph->m_DataflowAttributes.getAttributeData( "thickness", m_thickness );
	if ( subgraph->m_DataflowAttributes.hasAttribute( "rgba" ) ) 
	{
		try 
		{
			std::string rgbaChars = subgraph->m_DataflowAttributes.getAttribute( "rgba" ).getText();
			std::istringstream rgbaString( rgbaChars );
			for (int i=0; i < 4; ++i)
				rgbaString >> m_rgba[i];
		}
		catch( ... ) 
		{
			UBITRACK_THROW( "Invalid value for attribute 'rgba'" );
		}
	}
	else 
	{
		m_rgba[0] = 1.0;
		m_rgba[1] = 1.0;
		m_rgba[2] = 1.0;
		m_rgba[3] = 1.0;
	}
}


/** render the object */
void DirectionLine::draw( Measurement::Timestamp& t, int parity )
{
	boost::mutex::scoped_lock l( m_lock );

	glEnable ( GL_LINE_STIPPLE );
	glEnable ( GL_LINE_SMOOTH );
	
	glLineWidth( (float)m_thickness );

	// TODO Dummy cone, if not rendered, the color of the line below will be wrong!
	drawWireCone( 1.0, 1.0, 1, 1 );

	glColor4f( (float)m_rgba[0], (float)m_rgba[1], (float)m_rgba[2], (float)m_rgba[3] );
	glLineStipple ( 1, 0x0F0F );
	glBegin(GL_LINES);
	glVertex3f( (float)m_source_position[0], (float)m_source_position[1], (float)m_source_position[2] );
	glVertex3f( (float)m_target_position[0], (float)m_target_position[1], (float)m_target_position[2] );
	glEnd();
	glDisable( GL_LINE_STIPPLE );
	glDisable ( GL_LINE_SMOOTH );
}


bool DirectionLine::hasWaitingEvents()
{
	return ( m_target_port.getQueuedEvents() > 0 );
}


void DirectionLine::dataIn( const Ubitrack::Measurement::Position& pos, int redraw )
{
	boost::mutex::scoped_lock l( m_lock );

	m_target_position = *pos;
	m_source_position = *(m_source_port.get( pos.time() ));
	
	// redraw the world
	m_pModule->invalidate();

	// the pose has changed, so redraw the world
	if (redraw)
		m_pModule->invalidate();
}


} } // namespace Ubitrack::Drivers

