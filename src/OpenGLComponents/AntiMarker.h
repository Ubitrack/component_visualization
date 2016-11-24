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

#ifndef _ANTIMARKER_H_
#define _ANTIMARKER_H_

#include "RenderModule.h"

namespace Ubitrack { namespace Drivers {


/**
 * @ingroup driver_components
 * Antimarker component.
 *
 * Marker + Antimarker => nothing.
 *
 * This component takes a list of four 2D positions (as provided, e.g.,
 * by the MarkerTracker via its "Corners" port. The given quad is enlarged
 * by an optional factor (default 6%) and background image color is sampled
 * at the four corner points. Finally, an interpolated colored quad is drawn,
 * thereby eradicating the marker.
 */

class AntiMarker
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
	AntiMarker( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
		const VirtualObjectKey& componentKey, VirtualCamera* pModule );

	/** render the object */
	virtual void draw( Measurement::Timestamp& t, int parity );

	virtual bool hasWaitingEvents();

protected:

	/**
	 * callback from Pose port
	 * @param pose current transformation
	 */
	void positionIn( const Ubitrack::Measurement::PositionList2& pos, int parity );

	// positionlist input
	PushConsumer< Ubitrack::Measurement::PositionList2 > m_push;

	std::vector< Math::Vector< double, 2 > > m_data;
	boost::mutex m_posLock;
	double m_factor;
};


} } // namespace Ubitrack::Drivers

#endif

