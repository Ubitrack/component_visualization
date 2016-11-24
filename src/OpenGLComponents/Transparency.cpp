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
 * Implementation of transparent rendering
 *
 * @author Peter Keitler <keitler@in.tum.de>
 */

#include "Transparency.h"


// #ifdef HAVE_GLEW
	// #include "GL/glew.h"
// #endif
//removed by CW (16.7.2012) -- compile errror on windows

namespace Ubitrack { namespace Drivers {

Transparency::Transparency( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph,
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
{
	LOG4CPP_DEBUG( logger, "Transparency::Transparency(), initialize alpha to " << alpha );

	alpha = 0.5;
	m_pPush.reset( new PushConsumer< Ubitrack::Measurement::Distance >( "Input", *this, boost::bind( &Transparency::alphaIn, this, _1 ) ) );
}


Transparency::~Transparency()
{
}


void Transparency::draw( Measurement::Timestamp& t, int parity )
{
//added win32-guard by CW (16.7.2012): gl-functions seem to be unavailable on windows systems
#ifndef WIN32 
#ifdef HAVE_GLEW
	LOG4CPP_DEBUG( logger, "Transparency::draw(), set alpha to " << alpha << " for timestamp " << t );

	// Enable global transparency for the virtual scene. This affects
	// all render components except the BackgroundVideo component.
	glBlendColor ( 0.0, 0.0, 0.0, alpha );
	glBlendFunc ( GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA );
#else
	LOG4CPP_ERROR( logger, "Transparency::draw() has no effect since 'glew' is not installed" );
#endif
#endif

}


void Transparency::alphaIn( const Ubitrack::Measurement::Distance &a ) 
{
	if ( *a < 0.0 )
		alpha = 0.0;
	else if ( *a > 1.0 )
		alpha = 1.0;
	else
		alpha = *a;

	LOG4CPP_DEBUG( logger, "Transparency::alphaIn() " << a );
}


bool Transparency::hasWaitingEvents()
{
	LOG4CPP_DEBUG( logger, "Transparency::hasWaitingEvents(), return " << (m_pPush && m_pPush->getQueuedEvents() > 0) );
	return 1;
}




} } // namespace Ubitrack::Drivers

