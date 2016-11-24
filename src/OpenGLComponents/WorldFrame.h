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
 * Component which renders a large coordinate frame including a
 * grid in the x/z-plane representing the horizon.
 *
 * @author Guanzhou Wang <wangguanzzz@googlemail.com>
 * @author Peter Keitler <keitler@in.tum.de>
 */ 
 
#ifndef __WorldFrame_h_INCLUDED__
#define __WorldFrame_h_INCLUDED__



#include "TrackedObject.h"

#define GL_CLAMP_TO_EDGE	0x812F

namespace Ubitrack { namespace Drivers {

/**
 * @ingroup driver_components
 * Component for pose error visualization.
 */
class WorldFrame
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
	WorldFrame( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
		const VirtualObjectKey& componentKey, VirtualCamera* pModule );

	~WorldFrame();
		
	/** render the object */
	virtual void draw3DContent( Measurement::Timestamp&, int );
	
	GLuint LoadTextureRAW( const char * filename, int wrap );
	


protected:
	double m_width;
	double m_height;
	double m_size;
	double m_thickness;
	double m_rgba[4];
	bool m_bInitialized;
	int time;
};


} } // namespace Ubitrack::Drivers

#endif
