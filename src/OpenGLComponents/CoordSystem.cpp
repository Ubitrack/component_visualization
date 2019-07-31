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
 * @file
 * Implementation of the coordinate system visualization component
 *
 * @author Ulrich Eck <ulrich.eck@tum.de>
 */

#include "CoordSystem.h"

// should be replaced ..
#ifdef __APPLE__
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif


#include <math.h>
#include <log4cpp/Category.hh>

//static log4cpp::Category& logger( log4cpp::Category::getInstance( "Render.PoseErrorVisualization" ) );

using namespace Ubitrack::Math;


namespace Ubitrack { namespace Drivers {


CoordSystemVisualization::CoordSystemVisualization( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph,
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: TrackedObject( name, subgraph, componentKey, pModule )
{
	double axisLength = 0.1;
	std::string rgbString;
	subgraph->m_DataflowAttributes.getAttributeData( "axisLength", axisLength );
	subgraph->m_DataflowAttributes.getAttributeData("RGB", rgbString);

	if (subgraph->m_DataflowAttributes.hasAttribute("PositionRGB"))
		rgbString = subgraph->m_DataflowAttributes.getAttributeString("PositionRGB");
	else
		rgbString = "0.8 0.8 0.0";

	{
		std::istringstream inStream(rgbString);
		
		inStream >> m_posColor[0];
		inStream >> m_posColor[1];
		inStream >> m_posColor[2];
			
	}


	m_axis_x =  Math::Vector< double, 3 >( axisLength, 0, 0 );
	m_axis_y =  Math::Vector< double, 3 >( 0, axisLength, 0 );
	m_axis_z =  Math::Vector< double, 3 >( 0, 0, axisLength );
}


void CoordSystemVisualization::draw3DContent( Measurement::Timestamp& t, int )
{
	LOG4CPP_DEBUG( logger, "Drawing Coordinate System" );

	// save old state
	GLboolean oldCullMode;
	glGetBooleanv( GL_CULL_FACE, &oldCullMode );

	GLboolean oldLineSmooth = glIsEnabled( GL_LINE_SMOOTH );

	GLfloat oldLineWidth;
	glGetFloatv( GL_LINE_WIDTH, &oldLineWidth );

	// set new state
	glEnable( GL_CULL_FACE );
	glLineWidth( 4 );
	glEnable( GL_LINE_SMOOTH );

	glColor3f(m_posColor[0], m_posColor[1], m_posColor[2]);
	glBegin( GL_LINES );
		glVertex3d( 0, 0, 0 );
		glVertex3d( m_axis_x( 0 ), m_axis_x( 1 ), m_axis_x( 2 ) );
	glEnd();

	// y-axis rotation error
	glColor3f( 0.0f, 1.0f, 0.0f );
	glBegin( GL_LINES );
		glVertex3d( 0, 0, 0 );
		glVertex3d( m_axis_y( 0 ), m_axis_y( 1 ), m_axis_y( 2 ) );
	glEnd();

	// z-axis rotation error
	glColor3f( 0.0f, 0.0f, 1.0f );
	glBegin( GL_LINES );
		glVertex3d( 0, 0, 0 );
		glVertex3d( m_axis_z( 0 ), m_axis_z( 1 ), m_axis_z( 2 ) );
	glEnd();

	// restore old state
	if ( !oldCullMode )
		glDisable( GL_CULL_FACE );

	if ( !oldLineSmooth )
		glDisable( GL_LINE_SMOOTH );
	glLineWidth( oldLineWidth );
}


} } // namespace Ubitrack::Drivers

