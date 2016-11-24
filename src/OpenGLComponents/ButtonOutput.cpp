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

#include "ButtonOutput.h"


namespace Ubitrack { namespace Drivers {

ButtonOutput::ButtonOutput( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_port( "Output", *this )
	, m_posPort( "Position", *this )
{
}

void ButtonOutput::draw( Measurement::Timestamp& t, int parity )
{
	unsigned char key = m_pModule->getLastKey();
	if (!key) return;

    LOG4CPP_DEBUG( logger, "Button '" << key << "' pressed, push event with ID " << (int)key << " (" << Math::Scalar<int>( (int)key ) << ")" );
	
	Measurement::Button tmp( t, (int)key );
	m_port.send( tmp );
	m_posPort.send( Measurement::Position2D( t, m_pModule->getLastMousePos() ) );
}

} } // namespace Ubitrack::Drivers

