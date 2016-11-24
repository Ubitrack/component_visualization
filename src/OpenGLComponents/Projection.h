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

#ifndef _PROJECTION_H_
#define _PROJECTION_H_

#include <boost/scoped_ptr.hpp>
#include "RenderModule.h"

namespace Ubitrack { namespace Drivers {

/**
 * @ingroup driver_components
 * Component for manipulating the projection matrix.
 * Provides a pull-in port for a 4x4 matrix, manipulates the OpenGL projection stack.
 *
 * @author Nicolas Heuser (heuser@in.tum.de)
 */
class Projection
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
	Projection( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
		const VirtualObjectKey& componentKey, VirtualCamera* pModule );

	/** render the object */
	virtual void draw( Measurement::Timestamp& time, int parity );

protected:

	/**
	 * callback from Pose input port
	 * @param pose input pose
	 * @param redraw trigger a redraw? (only if called from push-input)
	 */
	void inputIn( const Measurement::Matrix4x4& m, int redraw );

	Ubitrack::Math::Matrix< double, 4, 4 > m_projection;

	boost::scoped_ptr< PushConsumer< Measurement::Matrix4x4 > > m_pPush;
	boost::scoped_ptr< PullConsumer< Measurement::Matrix4x4 > > m_pPull;
};

} } // namespace Ubitrack::Drivers

#endif

