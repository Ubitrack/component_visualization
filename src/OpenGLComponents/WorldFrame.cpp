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
 * @file Component which renders a large coordinate frame including a
 * grid in the x/z-plane representing the horizon.
 *
 * @author Guanzhou Wang <wangguanzzz@googlemail.com>
 * @author Peter Keitler <keitler@in.tum.de>
 */

#include "WorldFrame.h"

namespace Ubitrack { namespace Drivers {	
WorldFrame::WorldFrame( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph,
						const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: TrackedObject( name, subgraph, componentKey, pModule )
	, m_width ( 5 )
	, m_height ( 5 )
	, m_size ( 1 )
	, m_thickness ( 0.5 )
	, m_bInitialized ( false )
{
	// read parameters
	Graph::UTQLSubgraph::NodePtr objectNode = subgraph->getNode( "Object" );
	if ( !objectNode )
		UBITRACK_THROW( "No Object node in RenderModule configuration" );

	objectNode->getAttributeData( "gridCountX", m_width );
	objectNode->getAttributeData( "gridCountZ", m_height );
	objectNode->getAttributeData( "size", m_size );
	objectNode->getAttributeData( "thickness", m_thickness );
	if ( objectNode->hasAttribute( "rgba" ) ) 
	{
		try 
		{
			std::string rgbaChars = objectNode->getAttribute( "rgba" ).getText();
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

	time=0;
}


WorldFrame::~WorldFrame()
{
}


void WorldFrame::draw3DContent( Measurement::Timestamp& t, int )
{             
	int CONE1 = 200;
	int CONE2 = 270;
	double len = m_size/2;

	glEnable ( GL_LINE_SMOOTH );
	
	//x
	glColor4f( 1.0, 0.0, 0.0, 1.0 );
	glBegin(GL_LINES);
	glVertex3f((float)-len,0.0f,0.0f);
	glVertex3f((float)len,0.0f,0.0f);
	glEnd();

	glPushMatrix();
	glTranslatef((float)len,0.0,0.0f);
	glRotatef(90.0f,0.0f,90.0f,0.0f);
	glutWireCone((float)(m_size/CONE1),(float)(m_size/CONE2),10.0f,10.0f);
	glPopMatrix();

	//y
	glColor4f( 0.0, 1.0, 0.0, 1.0 );
	glBegin(GL_LINES);
	glVertex3f(0.0f,(float)-len,0.0f);
	glVertex3f(0.0f,(float)len,0.0f);
	glEnd();

	glPushMatrix();
	glTranslatef(0.0f,(float)len,0.0f);
	glRotatef(90.0f,-1.0f,0.0f,0.0f);
	glutWireCone((float)(m_size/CONE1),(float)(m_size/CONE2),10.0f,10.0f);
	glPopMatrix();

	//z
	glColor4f( 0.0, 0.0, 1.0, 1.0 );
	glBegin(GL_LINES);
	glVertex3f(0,0,(float)-len);
	glVertex3f(0,0,(float)len);
	glEnd();

	glPushMatrix();
	glTranslatef(0,0,(float)len);
	glutWireCone(m_size/CONE1,m_size/CONE2,10,10);
	glPopMatrix();

	//draw the dash lines
	glLineWidth( (float)m_thickness );
	glColor4f( (float)m_rgba[0], (float)m_rgba[1], (float)m_rgba[2], (float)m_rgba[3] );
	for(float i=(float)len/m_width;i<len;(float)(i+=(float)len/m_width)){
		glEnable(GL_LINE_STIPPLE);
		glLineStipple (0, 0x0F0F);
		glBegin(GL_LINES);
		glVertex3f(i,0.0f,(float)-len);
		glVertex3f(i,0.0f,(float)len);
		glEnd();
	}
	for(float i=-(float)len/m_width;i>-len;(float)(i-=(float)len/m_width)){
		glEnable(GL_LINE_STIPPLE);
		glLineStipple (0, 0x0F0F);
		glBegin(GL_LINES);
		glVertex3f(i,0.0f,(float)-len);
		glVertex3f(i,0.0f,(float)len);
		glEnd();
	}
	for(float i=(float)len/m_height;i<len;(float)(i+=(float)len/m_height)){
		glBegin(GL_LINES);
		glVertex3f((float)-len,0.0,i);
		glVertex3f((float)len,0.0,i);
		glEnd();
	}
	for(float i=-(float)len/m_height;i>-len;(float)(i-=(float)len/m_height)){
		glBegin(GL_LINES);
		glVertex3f((float)-len,0.0,i);
		glVertex3f((float)len,0.0,i);
		glEnd();
	 }
	 glDisable(GL_LINE_STIPPLE);

	 glDisable ( GL_LINE_SMOOTH );
}

} } // namespace Ubitrack::Drivers


