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

#ifndef _BACKGROUNDIMAGE_H_
#define _BACKGROUNDIMAGE_H_

#ifdef WIN32
#include <utUtil/CleanWindows.h>
#endif

#include "RenderModule.h"
#include <utVision/Image.h>
#include <utVision/TextureUpdate.h>


namespace Ubitrack { namespace Drivers {


/**
 * @ingroup driver_components
 * Component for planar background images.
 * Provides two push-in ports for images.
 */
class BackgroundImage
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
	BackgroundImage( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
		const VirtualObjectKey& componentKey, VirtualCamera* pModule );

	~BackgroundImage();
		
	/** render the object */
	virtual void draw( Measurement::Timestamp& t, int num );

	/**
	 * callback from Image port
	 * passes image to the parent module
	 * @param img an image Measurement
	 */
	void imageIn( const Ubitrack::Measurement::ImageMeasurement& img, int num = 0 );

	/** deletes OpenGL state */
    virtual void glCleanup();

	/** check whether there is an image waiting in the queue */
	virtual bool hasWaitingEvents();

protected:

	Ubitrack::Measurement::ImageMeasurement m_background[2];
	boost::mutex m_imageLock[2];

	// variables for textured drawing
	bool m_bUseTexture;
	Vision::TextureUpdate m_textureUpdate;


	Ubitrack::Dataflow::PushConsumer< Ubitrack::Measurement::ImageMeasurement > m_image0;
	Ubitrack::Dataflow::PushConsumer< Ubitrack::Measurement::ImageMeasurement > m_image1;


};

} } // namespace Ubitrack::Drivers

#endif

