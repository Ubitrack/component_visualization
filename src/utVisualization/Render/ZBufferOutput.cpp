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

#include "ZBufferOutput.h"

namespace Ubitrack { namespace Drivers {

ZBufferOutput::ZBufferOutput( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_port( "Output", *this )
	, m_width(0)
	, m_height(0)
{
}

/** render the object */
void ZBufferOutput::draw( Measurement::Timestamp&, int parity )
{
	unsigned char* data;
	if ((m_width != m_pModule->m_width) || (m_height != m_pModule->m_height))
	{
		m_width  = m_pModule->m_width;
		m_height = m_pModule->m_height;
		if ((m_zBuffer) && (m_zBuffer->imageData)) delete (unsigned char*)(m_zBuffer->imageData);
		data = new unsigned char[m_width*m_height];
		m_zBuffer = boost::shared_ptr< Vision::Image >( new Vision::Image( m_width, m_height, 1, data, IPL_DEPTH_8U ) );
	}

	m_zBuffer->iplImage()->origin = 1;
	data = (unsigned char*)m_zBuffer->imageData;
	glReadPixels(0, 0, m_width, m_height, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, data ); 

	/*for(int a = 0; a < m_width*m_height; a++)
	{
		if (data[a] != 255) std::cout << (int)data[a] << " ";
		//if(data[a] == 1.0) data[a] = 0.0;
		//if(data[a]  > 0.9) data[a] = 0.5;
	}*/

	m_port.send( Measurement::ImageMeasurement( Measurement::now(), m_zBuffer ) );
}

} } // namespace Ubitrack::Drivers

