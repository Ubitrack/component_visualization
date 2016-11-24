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

#ifndef __Skybox_h_INCLUDED__
#define __Skybox_h_INCLUDED__

#include "RenderModule.h"

#include <stdio.h>
#include <stdlib.h>


#ifdef __APPLE__
	#include <OpenGL/OpenGL.h>
	#include <GLUT/glut.h>
#else
        #include <GL/gl.h>			// Header File For The OpenGL32 Library
        #include <GL/glu.h>			// Header File For The GLu32 Library
#endif



namespace Ubitrack { namespace Drivers {


/**
 * @ingroup driver_components
 * Base class for tracked objects. 
 * Implements push and pull ports and sets the model-view matrix.
 */
class Skybox
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
	Skybox( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
		const VirtualObjectKey& componentKey, VirtualCamera* pModule )
		: VirtualObject( name, subgraph, componentKey, pModule )
		, m_push ( "PushInput", *this, boost::bind( &Skybox::poseIn, this, _1, 1 ))
		, m_pull ( "PullInput", *this ){
                 objectNode = subgraph->getNode( "Skybox" );
                 };
	
	
	/*draw the skybox of different sizes*/
void Draw_Skybox(float x, float y, float z, float width, float height, float length)
{
	// Center the Skybox around the given x,y,z position
	x = x - width  / 2;
	y = y - height / 2;
	z = z - length / 2;


	// Draw Front side
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glBegin(GL_QUADS);	
	
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x,		  y,		z+length);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x,		  y+height, z+length);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x+width, y+height, z+length); 
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x+width, y,		z+length);
		
	glEnd();

	// Draw Back side
	glBindTexture(GL_TEXTURE_2D, texture[1]);
	glBegin(GL_QUADS);		
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x+width, y,		z);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x+width, y+height, z); 
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x,		  y+height,	z);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x,		  y,		z);
		
	glEnd();

	// Draw Left side
	glBindTexture(GL_TEXTURE_2D, texture[2]);
	glBegin(GL_QUADS);		
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x,		  y+height,	z);	
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x,		  y+height,	z+length); 
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x,		  y,		z+length);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x,		  y,		z);		
	glEnd();

	// Draw Right side
	glBindTexture(GL_TEXTURE_2D, texture[3]);
	glBegin(GL_QUADS);		
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x+width, y,		z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x+width, y,		z+length);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x+width, y+height,	z+length); 
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x+width, y+height,	z);
	glEnd();

	// Draw Up side
	glBindTexture(GL_TEXTURE_2D, texture[4]);
	glBegin(GL_QUADS);		
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x+width, y+height, z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x+width, y+height, z+length); 
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x,		  y+height,	z+length);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x,		  y+height,	z);
	glEnd();

	// Draw Down side
	glBindTexture(GL_TEXTURE_2D, texture[5]);
	glBegin(GL_QUADS);		
	
		glTexCoord2f(0.0f, 0.0f); glVertex3f(x,		  y,		z);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(x,		  y,		z+length);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(x+width, y,		z+length); 
		glTexCoord2f(0.0f, 1.0f); glVertex3f(x+width, y,		z);
	
	glEnd();

}
  
	/** render the object, if up-to-date tracking information is available */
	virtual void draw( Measurement::Timestamp& t, int parity )
	{
            
		if ( m_pull.isConnected() ) 
			poseIn( m_pull.get( t ), 0 );

		// remove object if no measurements in the last second
		// TODO: make this configurable
		if (t > m_lastUpdateTime + 1000000000L) return; 
		
		
	   /* glEnable(GL_TEXTURE_2D);
	    if(texture!=0&&texture<100)*/
	    
		glMatrixMode( GL_MODELVIEW );
		glPushMatrix();
		{
			boost::mutex::scoped_lock l( m_poseLock );
			glMultMatrixd( m_pose );
		}
		
        glDisable( GL_DEPTH_TEST );
        glColor4d( 1.0, 1.0, 1.0, 1.0);
		glEnable(GL_TEXTURE_2D);
        Draw_Skybox(0,0,0,1,1,1);
		
		glPopMatrix();
	}

	virtual bool hasWaitingEvents()
	{
		if ( m_pull.isConnected() ) return false;
		return ( m_push.getQueuedEvents() > 0 );
	}

    virtual void glInit()
    {
		if ( !objectNode )
			UBITRACK_THROW( "No Object node in RenderModule configuration" );
    
		std::string path0 = objectNode->getAttribute( "front" ).getText();
		std::string path1 = objectNode->getAttribute( "back" ).getText();
		std::string path2 = objectNode->getAttribute( "left" ).getText();
		std::string path3 = objectNode->getAttribute( "right" ).getText();
		std::string path4 = objectNode->getAttribute( "up" ).getText();
		std::string path5 = objectNode->getAttribute( "down" ).getText();
    
		texture[0]=LoadTextureRAW(path0.c_str(), true);
		texture[1]=LoadTextureRAW(path1.c_str(), true);
		texture[2]=LoadTextureRAW(path2.c_str(), true);
		texture[3]=LoadTextureRAW(path3.c_str(), true);
		texture[4]=LoadTextureRAW(path4.c_str(), true);
		texture[5]=LoadTextureRAW(path5.c_str(), true);
    } 
    

    
    // for testing 
    // load a 256x256 RGB .RAW file as a texture
GLuint LoadTextureRAW( const char * filename, int wrap )
{
  GLuint texture;
  int width, height;
  unsigned char * data;
  FILE * file;

  // open texture data
  file = fopen( filename, "rt" );
  if ( file == NULL ) return 0;

  // allocate buffer
  width = 256;
  height = 256;
  data = (unsigned char *)malloc( width * height * 3 );

  // read texture data
  fread( data, width * height * 3, 1, file );
  fclose( file );

  // allocate a texture name
  glGenTextures( 1, &texture );

  // select our current texture
  glBindTexture( GL_TEXTURE_2D, texture );

  // select modulate to mix texture with color for shading
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

  // when texture area is small, bilinear filter the closest MIP map
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                   GL_LINEAR_MIPMAP_NEAREST );
  // when texture area is large, bilinear filter the first MIP map
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

  // if wrap is true, the texture wraps over at the edges (repeat)
  //       ... false, the texture ends at the edges (clamp)
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                   wrap ? (float)GL_REPEAT : (float)GL_CLAMP );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                   wrap ? (float)GL_REPEAT : (float)GL_CLAMP );

  // build our texture MIP maps
  gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width,
    height, GL_RGB, GL_UNSIGNED_BYTE, data );
    
  // free buffer
  free( data );
  return texture;
}

protected:

	/**
	 * callback from Pose port
	 * @param pose current transformation
	 * @param redraw trigger a redraw? (only if called from push-input)
	 */
	void poseIn( const Ubitrack::Measurement::Rotation& pose, int redraw )
	{
		m_lastUpdateTime = pose.time();
		Ubitrack::Math::Matrix< double, 3, 3 > m( *pose);
		double* tmp =  m.content();

		boost::mutex::scoped_lock l( m_poseLock );
		
	
        m_pose[0] = tmp[0];
        m_pose[1] = tmp[1];
        m_pose[2] = tmp[2];
        m_pose[3]=0;
        m_pose[4] = tmp[3];
        m_pose[5] = tmp[4];
        m_pose[6] = tmp[5];
        m_pose[7] = 0;
        m_pose[8] = tmp[6];
        m_pose[9] = tmp[7];
        m_pose[10] = tmp[8];
        m_pose[11] = 0;
        m_pose[12] = 0;
        m_pose[13] = 0;
        m_pose[14] = 0;
        m_pose[15] = 1;
        
        LOG4CPP_DEBUG( logger, ""<<m_pose[0]<<","<<m_pose[1]<<","<<m_pose[2] );
		
        // the pose has changed, so redraw the world
		if (redraw) m_pModule->invalidate();
	}

	// pose input
	PushConsumer< Ubitrack::Measurement::Rotation > m_push;
	PullConsumer< Ubitrack::Measurement::Rotation > m_pull;

	double m_pose[16];
	boost::mutex m_poseLock;
	// texture of the skybox
	GLuint texture[6];
	Graph::UTQLSubgraph::NodePtr objectNode;
};


} } // namespace Ubitrack::Drivers

#endif

