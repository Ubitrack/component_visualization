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



#ifdef HAVE_GLEW
	#include "GL/glew.h"
#endif

#include "PointCloud.h"

namespace Ubitrack { namespace Drivers {


PointCloud::PointCloud( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_push ( "PushInput", *this, boost::bind( &PointCloud::dataIn, this, _1 ))
	, m_ttl(1.0)
	, m_size(5.0)
	, m_setup(1)
{
	Graph::UTQLSubgraph::NodePtr objectNode = subgraph->getNode( "PointCloud" );
	if ( !objectNode )
		UBITRACK_THROW( "No PointCloud node in RenderModule configuration" );

	objectNode->getAttributeData( "TTL",  m_ttl  );
	objectNode->getAttributeData( "size", m_size );

	std::string color = objectNode->getAttribute( "rgba" ).getText();
	std::istringstream cparse(color);
	cparse >> m_color[0] >> m_color[1] >> m_color[2] >> m_color[3];
	if (!cparse) m_color[0] = m_color[1] = m_color[2] = m_color[3] = 1.0;
}

/** render the object */
void PointCloud::draw( Measurement::Timestamp&, int parity )
{
	boost::mutex::scoped_lock l( m_lock );

	// throw out old stuff
	Measurement::Timestamp current = Measurement::now();
	if (m_ttl > 0.0)
		while ( !m_data.empty() && ((current - m_data.front().time()) > m_ttl*1000000000.0) )
			m_data.pop_front();

	glColor4dv( m_color );
	glPointSize( (float)m_size );

	if (m_setup)
	{
		// lots of nice-looking extras
		glEnable( GL_POINT_SMOOTH );
		glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );
		m_setup = 0;

		#ifdef HAVE_GLEW
			float coeffs[] = {0.0, 1.0, 0.0};
			glPointParameterfvARB( GL_POINT_DISTANCE_ATTENUATION, coeffs );
			glPointParameterfARB( GL_POINT_SIZE_MAX, m_size );
			glPointParameterfARB( GL_POINT_SIZE_MIN,    1.0 );
			glPointParameterfARB( GL_POINT_FADE_THRESHOLD_SIZE, m_size );
		#endif
	}

	// render the lot
	glEnableClientState(GL_VERTEX_ARRAY);
	for ( unsigned int i = 0; i < m_data.size(); i++ )
	{
		std::vector< Math::Vector< double, 3 > >* ptr = m_data[i].get();
		glVertexPointer( 3, GL_DOUBLE, sizeof(Math::Vector< double, 3 >), (*ptr)[0].data().begin() );
		glDrawArrays( GL_POINTS, 0, m_data[i]->size() );
	}
}

bool PointCloud::hasWaitingEvents()
{
	return ( m_push.getQueuedEvents() > 0 );
}


/**
 * callback from Pose port
 * @param pose current transformation
 */
void PointCloud::dataIn( const Ubitrack::Measurement::PositionList& pos )
{
	boost::mutex::scoped_lock l( m_lock );
	m_data.push_back( pos );

	// redraw the world
	m_pModule->invalidate();
}


} } // namespace Ubitrack::Drivers

