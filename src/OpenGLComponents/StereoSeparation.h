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

#ifndef _STEREOSEPARATION_H_
#define _STEREOSEPARATION_H_

#include "RenderModule.h"

namespace Ubitrack { namespace Drivers {


/**
 * @ingroup driver_components
 * Component for StereoSeparation
 * Provides input pose and two offset poses for each rendering pass.
 * Currently only color separation is supported.
 *
 * @author Nicolas Heuser (heuser@in.tum.de)
 */
class StereoSeparation
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
	StereoSeparation( const std::string& name, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, 
		const VirtualObjectKey& componentKey, VirtualCamera* pModule );

	/** render the object */
	virtual void draw( Measurement::Timestamp& time, int parity );

protected:

	/**
	 * callback from Pose input port
	 * @param pose input pose
	 * @param redraw trigger a redraw? (only if called from push-input)
	 */
	void poseIn( const Ubitrack::Measurement::Pose& pose, int redraw );

	/**
	 * callback from PoseA port
	 * @param pose offset pose A
	 * @param redraw trigger a redraw? (only if called from push-input)
	 */
	void poseAIn( const Ubitrack::Measurement::Pose& pose, int redraw );

	/**
	 * callback from PoseB port
	 * @param pose offset pose B
	 * @param redraw trigger a redraw? (only if called from push-input)
	 */
	void poseBIn( const Ubitrack::Measurement::Pose& pose, int redraw );

	// pose input
	PushConsumer< Measurement::Pose >	m_pushInput;
	PullConsumer< Measurement::Pose >	m_pullInput;

	PushConsumer< Measurement::Pose >	m_pushA;
	PullConsumer< Measurement::Pose >	m_pullA;

	PushConsumer< Measurement::Pose >	m_pushB;
	PullConsumer< Measurement::Pose >	m_pullB;

	/** Output port of the component. */
	PushSupplier< Measurement::Pose > 	m_outputPort;

	Math::Pose				m_poseInput;
	Math::Pose				m_pose_offsetA;
	Math::Pose				m_pose_offsetB;

	bool					m_colorMaskA[4];
	bool					m_colorMaskB[4];

	Measurement::Timestamp			m_lastUpdateTime;
};

} } // namespace Ubitrack::Drivers

#endif

