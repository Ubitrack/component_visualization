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
 * Implementation of the error visualization component
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include "PositionErrorVisualization.h"

#ifdef HAVE_LAPACK

#include <math.h>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/bindings/lapack/syev.hpp>
#include <log4cpp/Category.hh>

//static log4cpp::Category& logger( log4cpp::Category::getInstance( "Render.PositionErrorVisualization" ) );

using namespace Ubitrack::Math;
namespace ublas = boost::numeric::ublas;
namespace lapack = boost::numeric::bindings::lapack;


namespace Ubitrack { namespace Drivers {

PositionErrorVisualization::PositionErrorVisualization( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph,
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: TrackedObject( name, subgraph, componentKey, pModule )
	, m_errorPushPort( "ErrorInput", *this, boost::bind( &PositionErrorVisualization::receiveError, this, _1 ) )
{
	double scaling = 3.0;
	subgraph->m_DataflowAttributes.getAttributeData( "scaling", scaling );
	m_posEllipsoid.setScaling( scaling );
}


void PositionErrorVisualization::draw3DContent( Measurement::Timestamp& t, int )
{
	LOG4CPP_DEBUG( logger, "Drawing ellipsoids" );

	// save old state
	GLboolean oldCullMode;
	glGetBooleanv( GL_CULL_FACE, &oldCullMode );

	// set new state
	glEnable( GL_CULL_FACE );

	glColor3f( 0.3f, 0.9f, 0.9f );
	m_posEllipsoid.draw();

	// restore old state
	if ( !oldCullMode )
		glDisable( GL_CULL_FACE );
}


void PositionErrorVisualization::receiveError( const Ubitrack::Measurement::ErrorPosition& error )
{
	LOG4CPP_DEBUG( logger, "Received error position" );
	LOG4CPP_TRACE( logger, error->value << ", " << error->covariance );

	m_posEllipsoid.setPosition( error->value );
	m_posEllipsoid.setCovariance( error->covariance );
}


} } // namespace Ubitrack::Drivers

#endif // HAVE_LAPACK
