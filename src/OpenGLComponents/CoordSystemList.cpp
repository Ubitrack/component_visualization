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



#include "CoordSystemList.h"

namespace Ubitrack { namespace Drivers {


        CoordSystemListVisualization::CoordSystemListVisualization( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph,
                                const VirtualObjectKey& componentKey, VirtualCamera* pModule )
                : VirtualObject( name, subgraph, componentKey, pModule )
                , m_push ( "PushInput", *this, boost::bind( &CoordSystemListVisualization::dataIn, this, _1 ))
        {
            double axisLength = 0.1;
            std::string rgbString;
            subgraph->m_DataflowAttributes.getAttributeData( "axisLength", axisLength );
            subgraph->m_DataflowAttributes.getAttributeData("RGB", rgbString);

            if (subgraph->m_DataflowAttributes.hasAttribute("PositionRGB"))
                rgbString = subgraph->m_DataflowAttributes.getAttributeString("PositionRGB");
            else
                rgbString = "0.8 0.8 0.0";

            {
                std::istringstream inStream(rgbString);

                inStream >> m_posColor[0];
                inStream >> m_posColor[1];
                inStream >> m_posColor[2];

            }


            m_axis_x =  Math::Vector< double, 3 >( axisLength, 0, 0 );
            m_axis_y =  Math::Vector< double, 3 >( 0, axisLength, 0 );
            m_axis_z =  Math::Vector< double, 3 >( 0, 0, axisLength );
        }

/** render the object */
        void CoordSystemListVisualization::draw( Measurement::Timestamp& t, int parity )
        {
            boost::mutex::scoped_lock l( m_lock );
            glMatrixMode( GL_MODELVIEW );

            for(Ubitrack::Math::Pose pose : *m_data) {
                glPushMatrix();


                Ubitrack::Math::Matrix< double, 4, 4 > m( pose.rotation(), pose.translation() );
                double* tmp =  m.content();

                glMultMatrixd( tmp );

                drawConent( t, parity );

                glPopMatrix();
            }



        }

        void CoordSystemListVisualization::drawConent( Measurement::Timestamp&, int parity ) {
            // save old state
            GLboolean oldCullMode;
            glGetBooleanv( GL_CULL_FACE, &oldCullMode );

            GLboolean oldLineSmooth = glIsEnabled( GL_LINE_SMOOTH );

            GLfloat oldLineWidth;
            glGetFloatv( GL_LINE_WIDTH, &oldLineWidth );

            // set new state
            glEnable( GL_CULL_FACE );
            glLineWidth( 4 );
            glEnable( GL_LINE_SMOOTH );

            glColor3f(m_posColor[0], m_posColor[1], m_posColor[2]);
            glBegin( GL_LINES );
            glVertex3d( 0, 0, 0 );
            glVertex3d( m_axis_x( 0 ), m_axis_x( 1 ), m_axis_x( 2 ) );
            glEnd();

            // y-axis rotation error
            glColor3f( 0.0f, 1.0f, 0.0f );
            glBegin( GL_LINES );
            glVertex3d( 0, 0, 0 );
            glVertex3d( m_axis_y( 0 ), m_axis_y( 1 ), m_axis_y( 2 ) );
            glEnd();

            // z-axis rotation error
            glColor3f( 0.0f, 0.0f, 1.0f );
            glBegin( GL_LINES );
            glVertex3d( 0, 0, 0 );
            glVertex3d( m_axis_z( 0 ), m_axis_z( 1 ), m_axis_z( 2 ) );
            glEnd();

            // restore old state
            if ( !oldCullMode )
                glDisable( GL_CULL_FACE );

            if ( !oldLineSmooth )
                glDisable( GL_LINE_SMOOTH );
            glLineWidth( oldLineWidth );
        }

        bool CoordSystemListVisualization::hasWaitingEvents()
        {
            return ( m_push.getQueuedEvents() > 0 );
        }


/**
 * callback from Pose port
 * @param pose current transformation
 */
        void CoordSystemListVisualization::dataIn( const Ubitrack::Measurement::PoseList& pos )
        {
            boost::mutex::scoped_lock l( m_lock );
            m_data = pos;

            // redraw the world
            m_pModule->invalidate();
        }


    } } // namespace Ubitrack::Drivers

