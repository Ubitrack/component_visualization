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
 * Implementation 3x3 covariance visualization
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include "ErrorEllipsoid.h"

#ifdef HAVE_LAPACK

#include <math.h>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/bindings/lapack/syev.hpp>
#include <log4cpp/Category.hh>

//static log4cpp::Category& logger( log4cpp::Category::getInstance( "Render.PoseErrorVisualization" ) );

using namespace Ubitrack::Math;
namespace ublas = boost::numeric::ublas;
namespace lapack = boost::numeric::bindings::lapack;


namespace Ubitrack { namespace Drivers {

ErrorEllipsoid::ErrorEllipsoid( const Math::Vector< double, 3 >& position, double scaling )
	: m_position( position )
	, m_scaling( scaling )
	, m_rotation( ublas::identity_matrix< double >( 4, 4 ) )
{
	m_pQuadric = gluNewQuadric();
}


ErrorEllipsoid::~ErrorEllipsoid()
{
	gluDeleteQuadric( m_pQuadric );
}


void ErrorEllipsoid::draw()
{
	// set transformation
	//glMatrixMode( GL_MODELVIEW );
	glPushMatrix();
	glTranslated( m_position( 0 ), m_position( 1 ), m_position( 2 ) );
	glMultMatrixd( m_rotation.content() );
	glScaled( m_sizes( 0 ), m_sizes( 1 ), m_sizes( 2 ) );

	// draw sphere
	gluSphere( m_pQuadric, 1.0, 20, 10 );

	glPopMatrix();
}


void ErrorEllipsoid::setCovariance( const Math::Matrix< double, 3, 3 >& covariance )
{
	ublas::subrange( m_rotation, 0, 3, 0, 3 ) = covariance;
	ublas::matrix_range< Math::Matrix< double, 4, 4 > > upperLeft( m_rotation, ublas::range( 0, 3 ), ublas::range( 0, 3 ) );
	lapack::syev( 'V', 'U', upperLeft, m_sizes, lapack::minimal_workspace() );
	for ( unsigned i = 0; i < 3; i++ )
		if ( m_sizes( i ) > 0 )
			m_sizes( i ) = sqrt( m_sizes( i ) ) * m_scaling;
		else
			m_sizes( i )  = 0;

	LOG4CPP_TRACE( logger, "Size = " << m_sizes );
}


} } // namespace Ubitrack::Drivers

#endif // HAVE_LAPACK
