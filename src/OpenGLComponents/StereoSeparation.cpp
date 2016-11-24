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


#include "StereoSeparation.h"

namespace Ubitrack { namespace Drivers {


StereoSeparation::StereoSeparation( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
	const VirtualObjectKey& componentKey, VirtualCamera* pModule )
	: VirtualObject( name, subgraph, componentKey, pModule )
	, m_pushInput( "PushInput", *this, boost::bind( &StereoSeparation::poseIn, this, _1, 1 ) )
	, m_pullInput( "PullInput", *this)
	, m_pushA( "PushInputA", *this, boost::bind( &StereoSeparation::poseAIn, this, _1, 1 ) )
	, m_pullA( "PullInputA", *this)
	, m_pushB( "PushInputB", *this, boost::bind( &StereoSeparation::poseBIn, this, _1, 1 ) )
	, m_pullB( "PullInputB", *this)
	, m_outputPort( "Output", *this )
{
	// ToDo: Make this configurable!!!
	m_colorMaskA[0] = false;	//red
	m_colorMaskA[1] = true;		//green
	m_colorMaskA[2] = false;	//blue
	m_colorMaskA[3] = true;		//alpha

	m_colorMaskB[0] = true;
	m_colorMaskB[1] = false;
	m_colorMaskB[2] = false;
	m_colorMaskB[3] = true;
}

/** render the object */
void StereoSeparation::draw( Measurement::Timestamp& time, int parity )
{
	if (m_pullInput.isConnected())
		poseIn( m_pullInput.get(time), 0);

	if (parity == 0) {
		if (m_pullA.isConnected())
			poseAIn( m_pullA.get(time), 0);
		glColorMask(m_colorMaskA[0], m_colorMaskA[1], m_colorMaskA[2], m_colorMaskA[3]);
		Measurement::Pose pose(time, m_poseInput*m_pose_offsetA );
		m_outputPort.send( pose );
	} else {
		if (m_pullB.isConnected())
			poseBIn( m_pullB.get(time), 0);
		glColorMask(m_colorMaskB[0], m_colorMaskB[1], m_colorMaskB[2], m_colorMaskB[3]);
		Measurement::Pose pose(time, m_poseInput*m_pose_offsetB );
		m_outputPort.send( pose );
	}
}

/**
 * callback from Pose input port
 * @param pose input pose
 * @param redraw trigger a redraw? (only if called from push-input)
 */
void StereoSeparation::poseIn( const Ubitrack::Measurement::Pose& pose, int redraw )
{
	m_lastUpdateTime = pose.time();
	m_poseInput = *(pose.get());
	// the pose has changed, so redraw the world
	if (redraw) m_pModule->invalidate();
}

/**
 * callback from PoseA port
 * @param pose offset pose A
 * @param redraw trigger a redraw? (only if called from push-input)
 */
void StereoSeparation::poseAIn( const Ubitrack::Measurement::Pose& pose, int redraw )
{
	m_lastUpdateTime = pose.time();
	m_pose_offsetA = *(pose.get());
	// the pose has changed, so redraw the world
	if (redraw) m_pModule->invalidate();
}

/**
 * callback from PoseB port
 * @param pose offset pose B
 * @param redraw trigger a redraw? (only if called from push-input)
 */
void StereoSeparation::poseBIn( const Ubitrack::Measurement::Pose& pose, int redraw )
{
	m_lastUpdateTime = pose.time();
	m_pose_offsetB = *(pose.get());
	// the pose has changed, so redraw the world
	if (redraw) m_pModule->invalidate();
}

} } // namespace Ubitrack::Drivers

