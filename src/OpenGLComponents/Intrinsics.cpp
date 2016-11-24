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

#include "Intrinsics.h"
#include <utAlgorithm/Projection.h>

namespace Ubitrack { namespace Drivers {

Intrinsics::Intrinsics( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_calibWidth( 320.0 )
	, m_calibHeight( 240.0 )
	, m_port( "Intrinsics", *this )
{
	subgraph->m_DataflowAttributes.getAttributeData( "calibWidth", m_calibWidth );
	subgraph->m_DataflowAttributes.getAttributeData( "calibHeight", m_calibHeight );
}

/** render the object */
void Intrinsics::draw( Measurement::Timestamp& time, int parity )
{
	// use projection in stereo mode only if correct eye
	if ( ( m_stereoEye == stereoEyeRight && parity ) || ( m_stereoEye == stereoEyeLeft && !parity ) )
		return;
		
	m_intrinsics = *(m_port.get( time )); //getTime() );

	double l = 0;
	double r = m_calibWidth;
	double b = 0;
	double t = m_calibHeight;
	double n = m_pModule->m_near;
	double f = m_pModule->m_far;

	// create a perspective projection matrix
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	Math::Matrix< double, 4, 4 > m = Ubitrack::Algorithm::projectionMatrixToOpenGL( l, r, b, t, n, f, m_intrinsics );
	glMultMatrixd( m.content() );
	glMatrixMode( GL_MODELVIEW );
}


} } // namespace Ubitrack::Drivers

