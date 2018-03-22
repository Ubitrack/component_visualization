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
 * Visualizes a single 3x3 error covariance
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */ 
 
#ifndef __ERRORELLIPSOID_H_INCLUDED__
#define __ERRORELLIPSOID_H_INCLUDED__



#ifdef HAVE_LAPACK

#include "RenderModule.h"

// should be replaced ..
#ifdef __APPLE__
	#include <OpenGL/glu.h>
#else
	#include <GL/glu.h>
#endif

namespace Ubitrack { namespace Drivers {

/**
 * Draws 3x3 covariances as ellipsoids
 */
class ErrorEllipsoid
{
public:
	/**
	 * Create a new error ellipsoid at the given position
	 * @param position the position of the ellipsoid
	 * @param scaling the scaling of the ellipsoid in multiples of sigma
	 */
	ErrorEllipsoid( const Math::Vector< double, 3 >& position = ( Math::Vector< double, 3 >( 0, 0, 0 ) ), double scaling = 3.0 );

	/** destructor */
	~ErrorEllipsoid();

	/**
	 * sets the covariance matrix
	 */
	void setCovariance( const Math::Matrix< double, 3, 3 >& covariance );

	/** renders the ellipsoid */
	void draw();

	/** returns the position */
	const Math::Vector< double, 3 >& position() const
	{ return m_position; }

	/** sets the scaling */
	void setScaling( double scaling )
	{ m_scaling = scaling; }

	/** sets the position */
	void setPosition( const Math::Vector< double, 3 >& position )
	{ m_position = position; }

protected:
	Math::Vector< double, 3 > m_position;
	double m_scaling;

	// result of the decomposition
	Math::Vector< double, 3 > m_sizes;
	Math::Matrix< double, 4, 4 > m_rotation;

	GLUquadricObj* m_pQuadric;
};

} } // namespace Ubitrack::Drivers

#endif // HAVE_LAPACK
#endif
