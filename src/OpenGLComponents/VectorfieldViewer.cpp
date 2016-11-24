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

#include "VectorfieldViewer.h"

#include <fstream>

namespace Ubitrack { namespace Drivers {

VectorfieldViewer::VectorfieldViewer( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: TrackedObject( name, subgraph, componentKey, pModule )
{
	// load object path
	/*Graph::UTQLSubgraph::NodePtr objectNode = subgraph->getNode( "Object" );
	if ( !objectNode )
		UBITRACK_THROW( "No Object node in RenderModule configuration" );

	std::string path = objectNode->getAttribute( "virtualObjectX3DPath" ).getText();
	if ( path.length() == 0)
		UBITRACK_THROW( "VirtualObject component with empty virtualObjectX3DPath  attribute" );

	if ( objectNode->hasAttribute( "occlusionOnly" ) && objectNode->getAttribute( "occlusionOnly" ).getText() == "true" )
		m_occlusionOnly = true;*/

	std::ifstream field("vfield.dump");
	unsigned char tmp1,tmp2;
	double dx,dy,dz;
	while (field) {
		field >> tmp1 >> dx >> dy >> dz >> tmp2;
		if ((tmp1 != '[') || (tmp2 != ']')) UBITRACK_THROW( "Vectorfield parse error" );
		m_pos.push_back( Math::Vector< double, 3 >(dx,dy,dz) );
		field >> tmp1 >> dx >> dy >> dz >> tmp2;
		if ((tmp1 != '[') || (tmp2 != ']')) UBITRACK_THROW( "Vectorfield parse error" );
		m_val.push_back( Math::Vector< double, 3 >(dx,dy,dz) );
	}
}

/** render the object, if up-to-date tracking information is available */
void VectorfieldViewer::draw3DContent( Measurement::Timestamp& t, int parity )
{
	double scale = 0.05;
	double offset = -0.10;
	int size = m_pos.size();
	glColor4f(1.0,0.0,0.0,1.0);
	glBegin(GL_LINES);
	for (int i = 0; i < size; i++) {
		glVertex3d( m_pos[i][0], m_pos[i][1]+offset, m_pos[i][2] );
		glVertex3d( m_pos[i][0]+m_val[i][0]*scale, m_pos[i][1]+offset+m_val[i][1]*scale, m_pos[i][2]+m_val[i][2]*scale );
	}
	glEnd();

	glPointSize( 3.0 );
	glBegin(GL_POINTS);
	for (int i = 0; i < size; i++) 
		glVertex3d( m_pos[i][0], m_pos[i][1]+offset, m_pos[i][2] );
	glEnd();
}

} } // namespace Ubitrack::Drivers

