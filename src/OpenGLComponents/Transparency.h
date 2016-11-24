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
 * Renders everything transparently.
 *
 * @author Peter Keitler <keitler@in.tum.de>
 */ 
 
#ifndef __Transparency_h_INCLUDED__
#define __Transparency_h_INCLUDED__

#include <boost/scoped_ptr.hpp>


#include <utMath/Scalar.h>
#include "RenderModule.h"

namespace Ubitrack { namespace Drivers {

/**
 * @ingroup driver_components
 * Component for forcing a transparent rendering
 */
class Transparency
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
	Transparency( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
		const VirtualObjectKey& componentKey, VirtualCamera* pModule );

	~Transparency();
		
	/** render the object */
	virtual void draw( Measurement::Timestamp&, int );

	/** check if there are events waiting for this component */
	virtual bool hasWaitingEvents();

protected:
	/**
	 * Callback from Distance port
	 * @param new alpha value
	 * @param redraw trigger a redraw? (only if called from push-input)
	 */
	void alphaIn( const Ubitrack::Measurement::Distance &a );

	/** internal alpha value */
	Math::Scalar< double > alpha;

	/** alpha value input */
	boost::scoped_ptr< PushConsumer< Ubitrack::Measurement::Distance > > m_pPush;
};


} } // namespace Ubitrack::Drivers

#endif
