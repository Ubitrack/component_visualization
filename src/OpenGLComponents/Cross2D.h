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

#ifndef _CROSS2D_H_
#define _CROSS2D_H_

#include <boost/scoped_ptr.hpp>
#include "RenderModule.h"

namespace Ubitrack { namespace Drivers {


/**
 * @ingroup driver_components
 * Component for planar 2D crosses (e.g. for HMD calibration).
 */
class Cross2D
	: public VirtualObject
{
public:

	/**
	 * Constructor
	 * @param name edge name
	 * @param config component configuration
	 * @param componentKey the unique identifier for this component
	 * @param pModule parent object
	 */
	Cross2D( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
		const VirtualObjectKey& componentKey, VirtualCamera* pModule );

	/** render the object */
	virtual void draw( Measurement::Timestamp& t, int num );

	/**
	 * callback from position port
	 * @param pos position measurement
	 */
	void crossPositionIn( const Ubitrack::Measurement::Position2D& pos );

	/** check whether there are events waiting in the queue */
	virtual bool hasWaitingEvents();

protected:
	boost::mutex m_positionLock;
	Ubitrack::Measurement::Position2D m_crossPosition;
	boost::scoped_ptr< Ubitrack::Dataflow::PushConsumer< Ubitrack::Measurement::Position2D > > m_pInPositionPush;
	boost::scoped_ptr< Ubitrack::Dataflow::PullConsumer< Ubitrack::Measurement::Position2D > > m_pInPositionPull;

};

} } // namespace Ubitrack::Drivers

#endif

