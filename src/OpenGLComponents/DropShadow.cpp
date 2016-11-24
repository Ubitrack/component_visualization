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
 * Implementation of a drop shadow component
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include "DropShadow.h"


namespace Ubitrack { namespace Drivers {

DropShadow::DropShadow( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph,
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: TrackedObject( name, subgraph, componentKey, pModule )
	, m_width( 1 )
	, m_height( 1 )
	, m_bInitialized( false )
{
	// read parameters
	Graph::UTQLSubgraph::NodePtr objectNode = subgraph->getNode( "Object" );
	if ( !objectNode )
		UBITRACK_THROW( "No Object node in RenderModule configuration" );

	objectNode->getAttributeData( "shadowWidth", m_width );
	objectNode->getAttributeData( "shadowHeight", m_height );
}


DropShadow::~DropShadow()
{
	// TODO: delete texture
}


void DropShadow::draw3DContent( Measurement::Timestamp& t, int )
{
	if ( !m_bInitialized )
	{
		// init opengl
		glEnable( GL_TEXTURE_2D );
		glGenTextures( 1, &m_texture );
		glBindTexture( GL_TEXTURE_2D, m_texture );

		// create the shadow texture
		const double b = 0.5; // arbitrary value >= 0
		unsigned char texData[ 4 * 64 * 64 ];
		for ( unsigned y = 0; y < 64; y++ )
			for ( unsigned x = 0; x < 64; x++ )
			{
				double dist = sqrt( (x-31.0) * (x-31.0) + (y-31.0) * (y-31.0) ) / 31.0;
				dist = ( dist * dist + b * dist ) / ( 1.0 + b );
				dist = std::max( std::min( dist, 1.0 ), 0.5 );
				texData[ 4 * ( 64 * y + x )     ] = 0;
				texData[ 4 * ( 64 * y + x ) + 1 ] = 0;
				texData[ 4 * ( 64 * y + x ) + 2 ] = 0;
				texData[ 4 * ( 64 * y + x ) + 3 ] = 255 - int( dist * 255 );
			}

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData );

		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		
		m_bInitialized = true;
	}
	
	// save old state
	GLboolean oldCullMode;
	glGetBooleanv( GL_CULL_FACE, &oldCullMode );
	
	GLboolean oldBlendMode;
	glGetBooleanv( GL_BLEND, &oldBlendMode );
	
	GLboolean oldLightingMode;
	glGetBooleanv( GL_LIGHTING, &oldLightingMode );

	GLint oldTexEnvMode;
	glGetTexEnviv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &oldTexEnvMode );
	
	// set new state
	glEnable( GL_TEXTURE_2D );
	glEnable( GL_CULL_FACE );
	glEnable( GL_BLEND );
	glDisable( GL_LIGHTING );
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	// draw shadow
	glBindTexture( GL_TEXTURE_2D, m_texture );
	glBegin( GL_TRIANGLE_STRIP );
		glNormal3d( 0, 0, 1 );
		glTexCoord2d( 0, 0 );
		glVertex3d( -m_width / 2, -m_height / 2, 0 );
		
		glNormal3d( 0, 0, 1 );
		glTexCoord2d( 1, 0 );
		glVertex3d(  m_width / 2, -m_height / 2, 0 );
		
		glNormal3d( 0, 0, 1 );
		glTexCoord2d( 0, 1 );
		glVertex3d( -m_width / 2,  m_height / 2, 0 );
		
		glNormal3d( 0, 0, 1 );
		glTexCoord2d( 1, 1 );
		glVertex3d(  m_width / 2,  m_height / 2, 0 );
	glEnd();

	// restore old state
	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, oldTexEnvMode );
	
	if ( !oldBlendMode )
		glDisable( GL_BLEND );
		
	if ( !oldCullMode )
		glDisable( GL_CULL_FACE );
		
	if ( oldLightingMode )
		glEnable( GL_LIGHTING );

	glPopMatrix();
}

} } // namespace Ubitrack::Drivers

