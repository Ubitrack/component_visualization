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


#include "StereoRendering.h"


// should be replaced ..
#ifdef __APPLE__
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif


namespace Ubitrack { namespace Drivers {


StereoRendering::StereoRendering( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > pSubgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, pSubgraph, componentKey, pModule )
	, m_stereoOffset( 0 )
	, m_stencilWidth( -1 )
	, m_stencilHeight( -1 )
{
	Graph::UTQLSubgraph::NodePtr pNode = pSubgraph->getNode( "Camera" );
	
	std::string sStereoType = pNode->getAttributeString( "stereoType" );
	if ( sStereoType == "lineSequential" )
	{
		m_stereoType = stereoLineSequential;
		getModule().setStereoRenderPasses( VirtualCamera::stereoRenderSingle );
	}
	else if ( sStereoType == "frameSequential" )
	{
		m_stereoType = stereoFrameSequential;
		getModule().setStereoRenderPasses( VirtualCamera::stereoRenderSequential );
	}
	else if ( sStereoType == "redGreen" )
	{
		m_stereoType = stereoRedGreen;
		getModule().setStereoRenderPasses( VirtualCamera::stereoRenderSingle );
	}
	else if ( sStereoType == "redBlue" )
	{
		m_stereoType = stereoRedBlue;
		getModule().setStereoRenderPasses( VirtualCamera::stereoRenderSingle );
	}
	else
		UBITRACK_THROW( "Invalid stereoType attribute" );
		
	pNode->getAttributeData( "stereoOffset", m_stereoOffset );
}

/** render the object */
void StereoRendering::draw( Measurement::Timestamp&, int parity )
{
	switch ( m_stereoType )
	{
	case stereoRedGreen:
		LOG4CPP_TRACE( logger, "glColorMask( " << (GLboolean)parity << ", " << (GLboolean)!parity << ", false, true" );
		glColorMask( parity, !parity, false, true );
		break;

	case stereoRedBlue:
		LOG4CPP_TRACE( logger, "glColorMask( " << (GLboolean)parity << ", false, " << (GLboolean)!parity << ", true )" );
		glColorMask( parity, false, !parity, true );
		break;
		
	case stereoLineSequential:
		glEnable( GL_STENCIL_TEST );
		
		if ( getModule().m_width != m_stencilWidth || getModule().m_height != m_stencilHeight )
			initStencilBuffer();
			
		glStencilFunc( GL_EQUAL, parity ? 1 : 0, 0x01 );
		break;

	default:
		// TODO: other stereo methods
		break;
	}
	
	if ( m_stereoOffset > 0 )
	{
		// non-calibrated simple stereo: add offset to projection matrix (will not work with spaam)
		glMatrixMode( GL_PROJECTION );
		glTranslated( ( parity ? 0.5 : -0.5 ) * m_stereoOffset, 0, 0 );
		glMatrixMode( GL_MODELVIEW );
	}
}


void StereoRendering::initStencilBuffer()
{
	LOG4CPP_DEBUG( logger, "initializing stencil buffer for line sequential stereo )" );
		
	m_stencilWidth = getModule().m_width;
	m_stencilHeight = getModule().m_height;
	
	// store the projection matrix
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();

	// create a 2D-orthogonal projection matrix
	glLoadIdentity();
	gluOrtho2D( 0.0, m_stencilWidth, 0.0, m_stencilHeight );
			
	// initialize every other line in the stencil buffer with 1
	GLfloat fOldLineWidth = 1.0;
	glGetFloatv( GL_LINE_WIDTH, &fOldLineWidth );
	glLineWidth( 1.0 );

	glStencilMask( 0x01 );
	glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
	glStencilFunc( GL_ALWAYS, 1, 0x01 );
	glClearStencil( 0 );
	glClear( GL_STENCIL_BUFFER_BIT );

	glBegin( GL_LINES );
	for ( int i = 0; i < m_stencilHeight; i += 2 ) {
		glVertex2f( 0, i );
		glVertex2f( m_stencilWidth, i );
	}
	glEnd();

	// reset stencil operations
	glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );

	glLineWidth( fOldLineWidth );

	// reset projection matrix
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
}

} } // namespace Ubitrack::Drivers

