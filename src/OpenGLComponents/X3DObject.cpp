#ifdef X3DRENDERER_ENABLED

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

#include "X3DObject.h"

namespace Ubitrack { namespace Drivers {

X3DObject::X3DObject( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: TrackedObject( name, subgraph, componentKey, pModule )
	, m_occlusionOnly( false )
{
	// load object path
	Graph::UTQLSubgraph::NodePtr objectNode = subgraph->getNode( "Object" );
	if ( !objectNode )
		UBITRACK_THROW( "No Object node in RenderModule configuration" );

	std::string path = objectNode->getAttribute( "virtualObjectX3DPath" ).getText();
	if ( path.length() == 0)
		UBITRACK_THROW( "VirtualObject component with empty virtualObjectX3DPath  attribute" );

	if ( objectNode->hasAttribute( "occlusionOnly" ) && objectNode->getAttribute( "occlusionOnly" ).getText() == "true" )
		m_occlusionOnly = true;
		
	// load x3d
	m_doc = boost::shared_ptr< TiXmlDocument >( new TiXmlDocument( path ) );
	m_x3d = boost::shared_ptr< X3DRender >( new X3DRender() );
	m_doc->LoadFile();
}

/** render the object, if up-to-date tracking information is available */
void X3DObject::draw3DContent( Measurement::Timestamp& t, int parity )
{
	// Remember old blend mode
	GLboolean blendMode[4];
	
	LOG4CPP_DEBUG( logger, "X3DObject::draw3DContent() for timestamp " << t );
	// render only into z-buffer?
	if ( m_occlusionOnly ) 
	{
		glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
		glGetBooleanv(GL_COLOR_WRITEMASK, blendMode);
	}

	m_doc->Accept( m_x3d.get() );

	// Reset old blend mode
	if ( m_occlusionOnly ) 
		glColorMask( blendMode[0], blendMode[1], blendMode[2], blendMode[3] );
}

} } // namespace Ubitrack::Drivers

#endif // X3DRENDERER_ENABLED