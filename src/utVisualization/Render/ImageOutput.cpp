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

#include "ImageOutput.h"

namespace Ubitrack { namespace Drivers {

ImageOutput::ImageOutput( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_port( "Output", *this )
	, m_width(0)
	, m_height(0)
{
}

/** render the object */
void ImageOutput::draw( Measurement::Timestamp&, int parity )
{
	if (parity) return;

	unsigned char* data;
	if ((m_width != m_pModule->m_width) || (m_height != m_pModule->m_height))
	{
		m_width  = m_pModule->m_width;
		m_height = m_pModule->m_height;
		if ((m_image) && (m_image->iplImage()->imageData)) delete[] (unsigned char*)(m_image->iplImage()->imageData);
		data = new unsigned char[m_width*m_height*3];
		m_image = boost::shared_ptr< Vision::Image >( new Vision::Image( m_width, m_height, 3, data, IPL_DEPTH_8U ) );
	}

	data = (unsigned char*)m_image->iplImage()->imageData;
	glReadPixels( 0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, data ); 
	m_image->iplImage()->origin = 1;

	m_port.send( Measurement::ImageMeasurement( Measurement::now(), m_image ) );
}

} } // namespace Ubitrack::Drivers

