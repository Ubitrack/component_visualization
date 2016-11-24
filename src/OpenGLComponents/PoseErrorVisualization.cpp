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

#include "PoseErrorVisualization.h"

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

namespace {

/**
 * Computes the jacobian for converting the rotational error into a position ellipsoid
 */
void rotErrorJacobian( Math::Matrix< double, 3, 3 >& j, const Math::Vector< double, 3 >& pos )
{
	j( 0, 0 ) = 0.0;
	j( 0, 1 ) = 2 * pos( 2 );
	j( 0, 2 ) = -2 * pos( 1 );
	j( 1, 0 ) = -2 * pos( 2 );
	j( 1, 1 ) = 0.0;
	j( 1, 2 ) = 2 * pos( 0 );
	j( 2, 0 ) = 2 * pos( 1 );
	j( 2, 1 ) = -2 * pos( 0 );
	j( 2, 2 ) = 0.0;
}

} // anonymous namespace

PoseErrorVisualization::PoseErrorVisualization( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph,
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: TrackedObject( name, subgraph, componentKey, pModule )
	, m_errorPushPort( "ErrorInput", *this, boost::bind( &PoseErrorVisualization::receiveError, this, _1 ) )
{
	double scaling = 3.0;
	double axisLength = 0.1;
	
	subgraph->m_DataflowAttributes.getAttributeData( "scaling", scaling );
	subgraph->m_DataflowAttributes.getAttributeData( "axisLength", axisLength );

	m_posEllipsoid.setScaling( scaling );
	m_rotXEllipsoid.setScaling( scaling );
	m_rotYEllipsoid.setScaling( scaling );
	m_rotZEllipsoid.setScaling( scaling );

	m_rotXEllipsoid.setPosition( Math::Vector< double, 3 >( axisLength, 0, 0 ) );
	m_rotYEllipsoid.setPosition( Math::Vector< double, 3 >( 0, axisLength, 0 ) );
	m_rotZEllipsoid.setPosition( Math::Vector< double, 3 >( 0, 0, axisLength ) );
}


void PoseErrorVisualization::draw3DContent( Measurement::Timestamp& t, int )
{
	LOG4CPP_DEBUG( logger, "Drawing ellipsoids" );

	// save old state
	GLboolean oldCullMode;
	glGetBooleanv( GL_CULL_FACE, &oldCullMode );

	GLboolean oldLineSmooth = glIsEnabled( GL_LINE_SMOOTH );

	GLfloat oldLineWidth;
	glGetFloatv( GL_LINE_WIDTH, &oldLineWidth );

	// set new state
	glEnable( GL_CULL_FACE );
	glLineWidth( 1 );
	glEnable( GL_LINE_SMOOTH );

	// position error
	glColor3f( 0.8f, 0.8f, 0.0f );
	m_posEllipsoid.draw();

	// x-axis rotation error
	glColor3f( 0.8f, 0.0f, 0.0f );
	m_rotXEllipsoid.draw();

	glBegin( GL_LINES );
		glVertex3d( 0, 0, 0 );
		glVertex3d( m_rotXEllipsoid.position()( 0 ), m_rotXEllipsoid.position()( 1 ), m_rotXEllipsoid.position()( 2 ) );
	glEnd();

	// y-axis rotation error
	glColor3f( 0.0f, 0.8f, 0.0f );
	m_rotYEllipsoid.draw();

	glBegin( GL_LINES );
		glVertex3d( 0, 0, 0 );
		glVertex3d( m_rotYEllipsoid.position()( 0 ), m_rotYEllipsoid.position()( 1 ), m_rotYEllipsoid.position()( 2 ) );
	glEnd();

	// z-axis rotation error
	glColor3f( 0.0f, 0.0f, 0.8f );
	m_rotZEllipsoid.draw();

	glBegin( GL_LINES );
		glVertex3d( 0, 0, 0 );
		glVertex3d( m_rotZEllipsoid.position()( 0 ), m_rotZEllipsoid.position()( 1 ), m_rotZEllipsoid.position()( 2 ) );
	glEnd();

	// restore old state
	if ( !oldCullMode )
		glDisable( GL_CULL_FACE );

	if ( !oldLineSmooth )
		glDisable( GL_LINE_SMOOTH );
	glLineWidth( oldLineWidth );
}


void PoseErrorVisualization::receiveError( const Ubitrack::Measurement::ErrorPose& error )
{
	LOG4CPP_DEBUG( logger, "Received error pose" );
	LOG4CPP_TRACE( logger, *error );

	boost::mutex::scoped_lock l( m_poseLock );

	// rotate the position covariance into the target coordinate frame
	Matrix< double, 3, 3 > j( ~error->rotation() );
	Matrix< double, 3, 3 > tmp( ublas::prod( j, ublas::subrange( error->covariance(), 0, 3, 0, 3 ) ) );
	Matrix< double, 3, 3 > posError( ublas::prod( tmp, ublas::trans( j ) ) );

	LOG4CPP_TRACE( logger, "Position error: " << std::endl << posError );
	m_posEllipsoid.setCovariance( posError );

	// x rotation error
	rotErrorJacobian( j, m_rotXEllipsoid.position() );
	noalias( tmp ) = ublas::prod( j, ublas::subrange( error->covariance(), 3, 6, 3, 6 ) );
	noalias( posError ) = ublas::prod( tmp, ublas::trans( j ) );

	LOG4CPP_TRACE( logger, "X rotation error: " << std::endl << posError );
	m_rotXEllipsoid.setCovariance( posError );

	// y rotation error
	rotErrorJacobian( j, m_rotYEllipsoid.position() );
	noalias( tmp ) = ublas::prod( j, ublas::subrange( error->covariance(), 3, 6, 3, 6 ) );
	noalias( posError ) = ublas::prod( tmp, ublas::trans( j ) );

	LOG4CPP_TRACE( logger, "Y rotation error: " << std::endl << posError );
	m_rotYEllipsoid.setCovariance( posError );

	// z rotation error
	rotErrorJacobian( j, m_rotZEllipsoid.position() );
	noalias( tmp ) = ublas::prod( j, ublas::subrange( error->covariance(), 3, 6, 3, 6 ) );
	noalias( posError ) = ublas::prod( tmp, ublas::trans( j ) );

	LOG4CPP_TRACE( logger, "Z rotation error: " << std::endl << posError );
	m_rotZEllipsoid.setCovariance( posError );
}


} } // namespace Ubitrack::Drivers

#endif // HAVE_LAPACK
