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

#include "BackgroundImage.h"
#include <utVision/OpenCLManager.h>
#include <utUtil/TracingProvider.h>

#ifdef HAVE_OPENCL
#include <opencv2/core/ocl.hpp>
#ifdef __APPLE__
    #include "OpenCL/cl_gl.h"
#else
    #include "CL/cl_gl.h"
#endif
#endif


namespace Ubitrack { namespace Drivers {

BackgroundImage::BackgroundImage( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_bUseTexture( true )
	, m_image0( "Image1", *this, boost::bind( &BackgroundImage::imageIn, this, _1, 0 ))
	, m_image1( "Image2", *this, boost::bind( &BackgroundImage::imageIn, this, _1, 1 ))
{
	if ( subgraph->m_DataflowAttributes.getAttributeString( "useTexture" ) == "false" ) 
	{
		m_bUseTexture = false;
	}
}

BackgroundImage::~BackgroundImage()
{
}


/** deletes OpenGL state */
void BackgroundImage::glCleanup()
{
	LOG4CPP_DEBUG( logger, "glCleanup() called" );
	m_textureUpdate.cleanupTexture();
}

/** render the object */
void BackgroundImage::draw( Measurement::Timestamp& t, int num )
{
    if (!getModule().isSetupComplete()) {
		LOG4CPP_INFO(logger, "Waiting for setup of RenderModule");
        return;
    }

	// Disable transparency for background image. The Transparency
	// module might have enabled global transparency for the virtual
	// scene.  We have to restore this state below.
	glDisable( GL_BLEND );

	// use image in stereo mode only if correct eye
	if ( ( m_stereoEye == stereoEyeRight && num ) || ( m_stereoEye == stereoEyeLeft && !num ) )
		return;

	// check if we have an image to display as background
	if ( m_background[num].get() == 0 ) return;


	int m_width  = m_pModule->m_width;
	int m_height = m_pModule->m_height;

	// store the projection matrix
	glMatrixMode( GL_PROJECTION );
	glPushMatrix();

	// create a 2D-orthogonal projection matrix
	glLoadIdentity();
	gluOrtho2D( 0.0, m_width, 0.0, m_height );

	// prepare fullscreen bitmap without fancy extras
	GLboolean bLightingEnabled = glIsEnabled( GL_LIGHTING );
	glDisable( GL_LIGHTING );
	glDisable( GL_DEPTH_TEST );
	
	// lock it to avoid random crashes
	boost::mutex::scoped_lock l( m_imageLock[num] );

	if ( !m_bUseTexture )
	{

		// find out texture format
		GLenum imgFormat = GL_LUMINANCE;
		GLenum pixelSize = GL_UNSIGNED_BYTE;
		switch ( m_background[num]->pixelFormat() ) {
			case Vision::Image::LUMINANCE:
				imgFormat = GL_LUMINANCE;
				break;
			case Vision::Image::RGB:
				imgFormat = GL_RGB;
				break;
	#ifndef GL_BGR_EXT
			case Vision::Image::BGR:
				imgFormat = GL_RGB;
				break;
			case Vision::Image::BGRA:
				imgFormat = GL_BGRA;
				break;
	#else
			case Vision::Image::BGR:
				imgFormat = GL_BGR_EXT;
				break;
			case Vision::Image::BGRA:
				imgFormat = GL_BGRA_EXT;
				break;
	#endif
			case Vision::Image::RGBA:
				imgFormat = GL_RGBA;
				break;
			default:
				// Log Error ?
				break;
		}

		// glDrawPixels version
		glDisable( GL_TEXTURE_2D );

		if ( m_background[num]->origin() ) {
			glRasterPos2i( 0, 0 );
			glPixelZoom(
				((float)m_width /(float)m_background[num]->width() )*1.0000001f,
				((float)m_height/(float)m_background[num]->height())*1.0000001f
			);
		} else {
			glRasterPos2i( 0, m_height-1 );
			glPixelZoom(
				 ((float)m_width /(float)m_background[num]->width())*1.0000001f,
				-((float)m_height/(float)m_background[num]->height())*1.0000001f
			);
		}
		glDrawPixels( m_background[num]->width(), m_background[num]->height(), imgFormat, pixelSize, m_background[num]->Mat().data );
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		if ( !m_textureUpdate.m_bTextureInitialized )
		{
			m_textureUpdate.initializeTexture(m_background[ num ]);
		}

		m_textureUpdate.updateTexture(m_background[num]);

#ifdef ENABLE_EVENT_TRACING
		TRACEPOINT_MEASUREMENT_RECEIVE(getEventDomain(), m_background[num].time(), getName().c_str(), "TextureUpdated")
#endif

		glBindTexture(GL_TEXTURE_2D, m_textureUpdate.m_texture);

		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

		// display textured rectangle
		double y0 = m_background[ num ]->origin() ? 0 : m_height;
		double y1 = m_height - y0;
		double tx = double( m_background[ num ]->width() ) / m_textureUpdate.m_pow2Width;
		double ty = double( m_background[ num ]->height() ) / m_textureUpdate.m_pow2Height;

		// draw two triangles
		glBegin( GL_TRIANGLE_STRIP );
		glTexCoord2d(  0, ty ); glVertex2d(       0, y1 );
		glTexCoord2d(  0,  0 ); glVertex2d(       0, y0 );
		glTexCoord2d( tx, ty ); glVertex2d( m_width, y1 );
		glTexCoord2d( tx,  0 ); glVertex2d( m_width, y0 );
		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
 
		glDisable( GL_TEXTURE_2D );
	}
	
	// change timestamp to image time
	t = m_background[num].time();
	
	// restore opengl state
	glEnable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
	if ( bLightingEnabled )
		glEnable( GL_LIGHTING );
	
	glPopMatrix();
	glMatrixMode( GL_MODELVIEW );
}


/**
 * callback from Image port
 * passes image to the parent module
 * @param img an image Measurement
 */
void BackgroundImage::imageIn( const Ubitrack::Measurement::ImageMeasurement& img, int num )
{
	LOG4CPP_DEBUG( logger, "received background image with timestamp " << img.time() );
	boost::mutex::scoped_lock l( m_imageLock[num] );

	m_background[num] = img;
	m_pModule->invalidate( this );
}

/** check whether there is an image waiting in the queue */
bool BackgroundImage::hasWaitingEvents()
{
	return ( (m_image0.getQueuedEvents() > 0) || (m_image1.getQueuedEvents() > 0) );
}

} } // namespace Ubitrack::Drivers

