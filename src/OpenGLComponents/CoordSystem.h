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
 * Error visualizing component as part of the render driver
 *
 * @author Ulrich Eck <ulrich.eck@tum.de>
 */ 
 
#ifndef __CoordSystemVisualization_h_INCLUDED__
#define __CoordSystemVisualization_h_INCLUDED__

#include "TrackedObject.h"

namespace Ubitrack { namespace Drivers {

/**
 * @ingroup driver_components
 * Component for coordinate system visualization.
 */
class CoordSystemVisualization
	: public TrackedObject
{
public:

	/**
	 * Constructor
	 * @param name edge name
	 * @param config component configuration
	 * @param componentKey the unique identifier for this component
	 * @param pModule parent object
	 */
	CoordSystemVisualization( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
		const VirtualObjectKey& componentKey, VirtualCamera* pModule );

	/** render the object */
	virtual void draw3DContent( Measurement::Timestamp&, int );

protected:

	Math::Vector< double, 3 > m_axis_x;
	Math::Vector< double, 3 > m_axis_y;
	Math::Vector< double, 3 > m_axis_z;

	float m_posColor[3];

	Measurement::Timestamp m_lastPose;
};


} } // namespace Ubitrack::Drivers

#endif
