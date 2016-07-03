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

#ifdef HAVE_OPENCL
#include <opencv2/core/ocl.hpp>
#ifdef __APPLE__
    #include "OpenCL/cl_gl.h"
#else
    #include "CL/cl_gl.h"
#endif
#endif

#include <GL/glut.h>

namespace Ubitrack { namespace Drivers {

BackgroundImage::BackgroundImage( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_bUseTexture( true )
	, m_bTextureInitialized( false )
	, m_image0( "Image1", *this, boost::bind( &BackgroundImage::imageIn, this, _1, 0 ))
	, m_image1( "Image2", *this, boost::bind( &BackgroundImage::imageIn, this, _1, 1 ))
#ifdef DO_TIMING
	, m_textureUpdateTimer( "TextureUpdateTimer", "Ubitrack.Timing" )
	, m_counter(0)
#endif
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

	if ( m_bTextureInitialized ) {
 		glBindTexture( GL_TEXTURE_2D, 0 );
 		glDisable( GL_TEXTURE_2D );
 		glDeleteTextures( 1, &m_texture );
 	}
}

/** render the object */
void BackgroundImage::draw( Measurement::Timestamp& t, int num )
{
    if (!getModule().isSetupComplete()) {
        return;
    }

	// Disable transparency for background image. The Transparency
	// module might have enabled global transparency for the virtual
	// scene.  We have to restore this state below.
	glDisable( GL_BLEND );

    // access OCL Manager and initialize if needed
	Vision::OpenCLManager& oclManager = Vision::OpenCLManager::singleton();
	static bool isInitialized = false;
	if (!isInitialized)
	{
        if (oclManager.isEnabled()) {
            oclManager.initializeOpenGL();
        }
		isInitialized = true;
		return;
	}
	

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

    // if OpenCL is enabled and image is on GPU, then use OCL codepath
    bool image_isOnGPU = oclManager.isEnabled() & m_background[num]->isOnGPU();
	
	// find out texture format
	GLenum imgFormat = GL_LUMINANCE;
	int numOfChannels = 1;
	switch ( m_background[num]->pixelFormat() ) {
		case Vision::Image::LUMINANCE:
			imgFormat = GL_LUMINANCE;
			numOfChannels = 1;
			break;
		case Vision::Image::RGB:
			numOfChannels = 3;
			imgFormat = GL_RGB;
			break;
#ifndef GL_BGR_EXT
		case Vision::Image::BGR: imgFormat = GL_RGB; numOfChannels = 3; break;
#else
		case Vision::Image::BGR:
			numOfChannels = 3;
			imgFormat = GL_BGR_EXT;
			break;
#endif
		case Vision::Image::RGBA:
			numOfChannels = 4;
			imgFormat = GL_RGBA;
			break;
		case Vision::Image::BGRA:
			numOfChannels = 4;
			imgFormat = GL_BGRA;
			break;
		default:
			// Log Error ?
			break;
	}

	if ( !m_bUseTexture )
	{
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
		glDrawPixels( m_background[num]->width(), m_background[num]->height(), imgFormat, GL_UNSIGNED_BYTE, m_background[num]->Mat().data );
	}
	else
	{
		glEnable(GL_TEXTURE_2D);
		if ( !m_bTextureInitialized )
		{
			
			m_bTextureInitialized = true;
			
			// generate power-of-two sizes
			m_pow2Width = 1;
			while ( m_pow2Width < (unsigned)m_background[ num ]->width() )
				m_pow2Width <<= 1;
			
			m_pow2Height = 1;
			while ( m_pow2Height < (unsigned)m_background[ num ]->height() )
				m_pow2Height <<= 1;
		
			glGenTextures( 1, &m_texture );
			glBindTexture( GL_TEXTURE_2D, m_texture );
		
			// define texture parameters
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
		
			// load empty texture image (defines texture size)
			glTexImage2D( GL_TEXTURE_2D, 0, numOfChannels, m_pow2Width, m_pow2Height, 0, imgFormat, GL_UNSIGNED_BYTE, 0 );
			LOG4CPP_DEBUG( logger, "glTexImage2D( width=" << m_pow2Width << ", height=" << m_pow2Height << " ): " << glGetError() );
		
			LOG4CPP_INFO( logger, "initalized texture ( " << imgFormat << " ) GPU? " << image_isOnGPU);


            if (image_isOnGPU) {
#ifdef HAVE_OPENCL
                //Get an image Object from the OpenGL texture
                cl_int err;
                m_clImage = clCreateFromGLTexture2D( oclManager.getContext(), CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, m_texture, &err);
                if (err != CL_SUCCESS)
                {
                    LOG4CPP_ERROR( logger, "error at  clCreateFromGLTexture2D:" << err );
                }
				// @todo the allocation is done here on GPU without caring about possible copy or assignment operations later
                //we need to have an RGBA or monochrome image
                if (m_background[num]->channels() == 3 || m_background[num]->channels() == 4)
                {
                    m_convertedImage.reset(new cv::UMat(m_background[ num ]->uMat().size(), CV_8UC4));
                } else if (m_background[num]->channels() == 1 )
                {
                    m_convertedImage.reset(new cv::UMat(m_background[ num ]->uMat().size(), CV_8UC1));
                } else {
					LOG4CPP_ERROR( logger, "Invalid channel-size when creating converted image:" << m_background[num]->channels() );
				}
#endif
            }


		}
#ifdef DO_TIMING
		{
			UBITRACK_TIME( m_textureUpdateTimer );
#endif

        if (image_isOnGPU) {
#ifdef HAVE_OPENCL
			// @todo we really need an image-format flag and pixeltype on the image class ..
            //we need to have an RGBA or monochrome image
			if (m_background[num]->channels() == 1) {
				LOG4CPP_DEBUG(logger, "XXX U8 " <<  m_background[num]->uMat().elemSize());
				if (m_background[num]->uMat().elemSize() == 2) {
					m_background[num]->uMat().convertTo(*m_convertedImage, CV_8U, 0.00390625);
				} else {
					*m_convertedImage = m_background[num]->uMat();
				}
				numOfChannels = 1;
			} else if (m_background[num]->channels() == 2) {
				LOG4CPP_DEBUG(logger, "XXX U16");
				// @todo fix me: currently we're down-casting 16bit to 8bit monochrome when using gpu-image.
				m_background[num]->uMat().convertTo(*m_convertedImage, CV_8U, 0.00390625);
				numOfChannels = 1;
			} else if (m_background[num]->channels() == 3)
            {
                if (imgFormat == GL_BGR_EXT) {
                    LOG4CPP_DEBUG(logger, "XXX BGR");
                    cv::cvtColor(m_background[num]->uMat(), *m_convertedImage, cv::COLOR_BGR2RGBA);
                } else if (imgFormat == GL_RGB) {
                    LOG4CPP_DEBUG(logger, "XXX RGB");
                    cv::cvtColor(m_background[num]->uMat(), *m_convertedImage, cv::COLOR_RGB2RGBA);
                } else {
                    LOG4CPP_ERROR( logger, "Error: received incompatible format for conversion to RGBA: " << imgFormat);
                }
                imgFormat = GL_RGBA;
                numOfChannels = 4;
            } else  if (m_background[num]->channels() == 4) {
                LOG4CPP_DEBUG(logger, "XXX 4CHAN");
                *m_convertedImage = m_background[num]->uMat();
            } else {
				LOG4CPP_ERROR(logger, "Unkown channel-size: " << m_background[num]->channels());
			}

            cl_mem clBuffer = (cl_mem) m_convertedImage->handle(cv::ACCESS_READ);
            cl_command_queue commandQueue = oclManager.getCommandQueue();

            cl_int err;

//#ifdef __APPLE__
//            glFlushRenderAPPLE();
//#else
            err = clEnqueueAcquireGLObjects(commandQueue, 1, &m_clImage, 0, NULL, NULL);
            if(err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clEnqueueAcquireGLObjects:" << err );
            }
//#endif
            size_t offset = 0;
            size_t dst_origin[3] = {0, 0, 0};
            size_t region[3] = {static_cast<size_t>(m_convertedImage->size().width), static_cast<size_t>(m_convertedImage->size().height), 1};

            err = clEnqueueCopyBufferToImage(commandQueue, clBuffer, m_clImage, 0, dst_origin, region, 0, NULL, NULL);
            if(err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clEnqueueAcquireGLObjects:" << err );
            }

            if (err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clEnqueueCopyBufferToImage:" << err );
            }

//#ifdef __APPLE__
//            err = clFlush(commandQueue);
//            if (err != CL_SUCCESS)
//            {
//                LOG4CPP_ERROR( logger, "error at  clFlush:" << err );
//            }
//#else
            err = clEnqueueReleaseGLObjects(commandQueue, 1, &m_clImage, 0, NULL, NULL);
            if(err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clEnqueueReleaseGLObjects:" << err );
            }
//#endif

            err = clFinish(commandQueue);
            if (err != CL_SUCCESS)
            {
                LOG4CPP_ERROR( logger, "error at  clFinish:" << err );
            }
#else
            LOG4CPP_ERROR( logger, "Image isOnGPU but OpenCL is disabled!!");
#endif
        } else {
            // load image into texture
            glBindTexture( GL_TEXTURE_2D, m_texture );
#ifdef DO_TIMING
            {
			UBITRACK_TIME( m_textureUpdateTimer );
#endif
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, m_background[ num ]->width(), m_background[ num ]->height(),
                    imgFormat, GL_UNSIGNED_BYTE, m_background[ num ]->Mat().data );
#ifdef DO_TIMING
            }
#endif

        }

		glBindTexture(GL_TEXTURE_2D, m_texture);

		glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

		// display textured rectangle
		double y0 = m_background[ num ]->origin() ? 0 : m_height;
		double y1 = m_height - y0;
		double tx = double( m_background[ num ]->width() ) / m_pow2Width;
		double ty = double( m_background[ num ]->height() ) / m_pow2Height;

		// draw two triangles
		glBegin( GL_TRIANGLE_STRIP );
		glTexCoord2d(  0, ty ); glVertex2d(       0, y1 );
		glTexCoord2d(  0,  0 ); glVertex2d(       0, y0 );
		glTexCoord2d( tx, ty ); glVertex2d( m_width, y1 );
		glTexCoord2d( tx,  0 ); glVertex2d( m_width, y0 );
		glEnd();

		glBindTexture(GL_TEXTURE_2D, 0);
 
		glDisable( GL_TEXTURE_2D );
#ifdef DO_TIMING
		}
		m_counter++;
		if(m_counter > 10){
			LOG4CPP_INFO(logger, "measuremts: " << m_textureUpdateTimer );
			m_counter = 0;
		}
#endif 
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

	if(img->depth() == IPL_DEPTH_32F){
        // @todo should this computation be moved to the rendering thread ?
        // also this does assume the image is on CPU !!!
		boost::shared_ptr<Ubitrack::Vision::Image> p(new Ubitrack::Vision::Image(img->width(), img->height(), 1, IPL_DEPTH_8U ));
		float* depthData = (float*) img->Mat().data;
		unsigned char* up =(unsigned char*) p->Mat().data;
		LOG4CPP_INFO(logger, "copy data");
		for(unsigned int i=0;i<img->width()*img->height();i++)
			if(depthData[i] != depthData[i])
				up[i] = 0;
			else 
				up[i] = depthData[i]*255;
		
		m_background[num] = Ubitrack::Measurement::ImageMeasurement(img.time(), p);
	} else {
		m_background[num] = img;
	}
	m_pModule->invalidate( this );
}

/** check whether there is an image waiting in the queue */
bool BackgroundImage::hasWaitingEvents()
{
	return ( (m_image0.getQueuedEvents() > 0) || (m_image1.getQueuedEvents() > 0) );
}

} } // namespace Ubitrack::Drivers

