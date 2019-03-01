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

// TODO: implement module start/stop mechanics

/**
 * @ingroup vision_components
 * @file
 * Implements a window to display images, based on HighGUI.
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 * @author Christian Waechter <christian.waechter@in.tum.de> (modified)
 */

#include <string>
#include <iostream>
#include <deque>

#include <boost/version.hpp>
#include <boost/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>
#include <log4cpp/Category.hh>

#include <utDataflow/PushConsumer.h>
#include <utDataflow/PushSupplier.h>
#include <utDataflow/PullSupplier.h>
#include <utDataflow/Module.h>
#include <utDataflow/ComponentFactory.h>
#include <utMeasurement/Measurement.h>
#include <utUtil/OS.h>
#include <utVision/Image.h>

#include <opencv2/highgui.hpp>

static log4cpp::Category& logger( log4cpp::Category::getInstance( "Ubitrack.Vision.HighguiWindow" ) );

using namespace Ubitrack;
using namespace Ubitrack::Vision;
using namespace Ubitrack::Dataflow;

void on_mouse( int event, int x, int y, int flags, void* param );
void on_trackbar( int event, void* param );

namespace Ubitrack { namespace Drivers {

/** the component key -- reads the window title */
class HWCKey
	: public std::string
{
public:
	/** extract window title from configuration */
	HWCKey( boost::shared_ptr< Graph::UTQLSubgraph > subgraph )
	{
		std::string sTitle = subgraph->m_DataflowAttributes.getAttribute( "windowTitle" ).getText();
		if ( sTitle.length() == 0 )
			UBITRACK_THROW( "Configuration for HighguiWindow component has empty 'title' attribute" );
		*static_cast< std::string* >( this ) = sTitle;
	}
};

// forward declarations
class HighguiWindow;
class HighguiWindowModule;

typedef Module< SingleModuleKey, HWCKey, HighguiWindowModule, HighguiWindow > ModuleBase;



/**
 * The module -- only responsible for a thread that calls cvGetKey every 100ms
 */
class HighguiWindowModule
	: public ModuleBase
{
public:
	/** constructor, creates thread */
	HighguiWindowModule( const SingleModuleKey& key, boost::shared_ptr< Graph::UTQLSubgraph >, FactoryHelper* pFactory )
		: ModuleBase( key, pFactory )
		, m_bStop( false )
		, m_nWindows( 0 )
	{}

	/** destructor, stops thread */
	~HighguiWindowModule()
	{}

	virtual void stopModule()
	{
		m_bStop = true;
		if ( m_pThread )
		{
			m_pThread->join();
		}
	}

	virtual void startModule()
	{
		m_pThread.reset( new boost::thread( boost::bind( &HighguiWindowModule::threadProc, this ) ) );
		m_bStop = false;
	}

	/** creates a new window */
	void createWindow( const std::string& name )
	{
		boost::mutex::scoped_lock l( m_queueMutex );
		m_queue.push_back( boost::bind( &HighguiWindowModule::myCreateWindow, this, name ) );
		m_queueCondition.notify_all();
	}
	
	void addMouseCallback( const std::string& name, HighguiWindow* p )
	{
		// getComponent( HWCKey( name ) ) ;
		boost::mutex::scoped_lock l( m_queueMutex );
		m_queue.push_back( boost::bind( cvSetMouseCallback, name.c_str(), on_mouse, p ) );
		m_queueCondition.notify_all();
	}
	
	void addTrackbar( int & initVal, unsigned maxVal, const std::string& winName, const std::string &trackBarName, HighguiWindow* p  )
	{
#if CV_MAJOR_VERSION > 1
		boost::mutex::scoped_lock l( m_queueMutex );
		m_queue.push_back( boost::bind( cvCreateTrackbar2, trackBarName.c_str(), winName.c_str(), &initVal, maxVal, on_trackbar, p ) );
		m_queueCondition.notify_all();
#endif
	}

	/** shows an image */
	void showImage( const std::string& name, const boost::shared_ptr< Image > pImage )
	{
		boost::mutex::scoped_lock l( m_queueMutex );
		m_queue.push_back( boost::bind( &HighguiWindowModule::myShowImage, this, name, pImage ) );
		m_queueCondition.notify_all();
	}


protected:
	// thread main loop
	void threadProc();

	void myCreateWindow( const std::string& name )
	{
		cvNamedWindow( name.c_str(), 0 );//CV_WINDOW_AUTOSIZE );
		m_nWindows++;
	}

	// @todo ->Mat() member function of image should be read-only, and ->WriteableMat() should be non-const
//	void myShowImage( const std::string& name, const boost::shared_ptr< Image > pImage )
	void myShowImage( const std::string& name, boost::shared_ptr< Image > pImage )
	{
		IplImage cvimg = pImage->Mat();				
		cvimg.origin = pImage->origin();		
		cvShowImage( name.c_str(), &cvimg );
	}

	// the thread
	boost::scoped_ptr< boost::thread > m_pThread;

	// stop the thread?
	bool m_bStop;

	// number of windows open
	int m_nWindows;

	// a mutex object to synchronize highgui calls
	boost::mutex m_queueMutex;

	// notification when the queue has changed
	boost::condition m_queueCondition;

	// A queue to store procedure calls. Why? Because in Win32, all procedure calls that
	// modify a window should come from the same thread. Big debugging otherwise. So we
	// store the calls that the window thread should make in a single event queue.
	typedef std::deque< boost::function< void() > > QueueType;
	QueueType m_queue;
};



/**
 * @ingroup vision_components
 * A Component that displays images in a OpenCV HighGUI window.
 *
 * @par Input Ports
 * PushConsumer< Ubitrack::Measurement::ImageMeasurement > port with name "Input".
 *
 * @par Output Ports
 * PushSupplier< Ubitrack::Measurement::Button > port with name "Button"
 *
 * @par Example Configuration
 * \verbatim 
 */
class HighguiWindow
	: public HighguiWindowModule::Component
{
public:
	/** constructor, creates window */
	HighguiWindow( const std::string& sName, boost::shared_ptr< Graph::UTQLSubgraph > subgraph, const HWCKey& componentKey, HighguiWindowModule* pModule )
		: HighguiWindowModule::Component( sName, componentKey, pModule )
		, uiMaxValue( 10 )
		, iValue( 5 )
		, bNorm ( true )
		, m_positionPort( "MousePosition", *this )
		, m_trackbarButtonPort( "TrackBarPosition", *this )
		, m_trackbarDistancePort( "TrackBarRatio", *this )
		, m_inPort( "Input", *this, boost::bind( &HighguiWindow::pushImage, this, _1 ) )
		, m_buttonPort( "Button", *this )
	{
		getModule().createWindow( getKey() );
		getModule().addMouseCallback( getKey(), this );
		
		if( subgraph->m_DataflowAttributes.hasAttribute( "maxValue" ) )
		{
			subgraph->m_DataflowAttributes.getAttributeData( "maxValue", uiMaxValue );
			subgraph->m_DataflowAttributes.getAttributeData( "initValue", iValue );
			
			int numBars = 3;
			// for( unsigned i = 0; i < numBars, ++i )
			m_trackBarValues.resize( numBars );
			m_trackbarNames.push_back( "Value" );
			getModule().addTrackbar( iValue, uiMaxValue, getKey(), m_trackbarNames[0], this );
		}
	}
	
	
	/** destructor, destroys window */
	~HighguiWindow()
	{}

	
	/* signals a button event */
	void signalButton(char nKey)
	{ 
		Measurement::Timestamp t = Ubitrack::Measurement::now();
		m_buttonPort.send( Measurement::Button(t, int( nKey )) ); 
	}

    unsigned int uiMaxValue;
	int iValue;
    bool bNorm;

	// the ports
	Dataflow::PushSupplier< Measurement::Position2D > m_positionPort;
	Dataflow::PushSupplier< Measurement::Button > m_trackbarButtonPort;
	Dataflow::PushSupplier< Measurement::Distance > m_trackbarDistancePort;
	
	std::vector< std::string > m_trackbarNames;
	
protected:
	/** method that receives events and displays the image */
	void pushImage( const Measurement::ImageMeasurement& m )
	{
		getModule().showImage( getKey(), m );
	}
	
	std::vector< int* > m_trackBarValues;

	/// Image input port
	Dataflow::PushConsumer< Measurement::ImageMeasurement > m_inPort;
	/// Button input port
	Dataflow::PushSupplier< Measurement::Button > m_buttonPort;
};



void HighguiWindowModule::threadProc()
{
	while ( !m_bStop )
	{
		// get next event from queue
		boost::function< void() > nextCall;
		{
			boost::mutex::scoped_lock l( m_queueMutex );
			if ( m_queue.empty() )
			{
				// wait for event or message dispatching timeout
				boost::xtime xt;
#if BOOST_VERSION >= 105000
				boost::xtime_get( &xt, boost::TIME_UTC_ );
#else
				boost::xtime_get( &xt, boost::TIME_UTC );
#endif
				xt.nsec += 100000000;
				xt.sec += xt.nsec / 1000000000;
				xt.nsec %= 1000000000;
				m_queueCondition.timed_wait( l, xt );
			}

			// comment of this change (CW@2013-05-24):
			// previously there was one highgui function executed
			// with cvWaitKey(10) after each call. -> lead to a huge
			// delay when dealing with multiple images/windows at the same time. 
			// therefore I changed to many calls (until pileline is empty) 
			// and calling then cvWaitkey, hope that helps and works in common
			// cases with one camera as well.
			while ( !m_queue.empty() )
			{
				nextCall = m_queue.front();
				m_queue.pop_front();
				if ( nextCall ) 
					nextCall();
			}
		}
		
		int nKey = cvWaitKey( 5 );

		// let the opencv message dispatching do something		
#ifndef _WIN32 // added from CW@2013-05-24: since this necessary on linux only, we dont do it on windows
		if ( m_nWindows ) 
		{
			// ATTENTION: has to be at least 2, otherwise highgui does not process any events on Linux. This
			// has to do with the timed_wait() call above (Pete, 2010-08-17)
			nKey = cvWaitKey( 2 );
		}
#endif
		
		// when a button is pushed, signal a button event
		if ( nKey != -1 )
		{
			LOG4CPP_DEBUG( logger, "Received Key" );
			ComponentList comps = getAllComponents();
			for ( ComponentList::iterator it = comps.begin(); it != comps.end(); it++ )
				(*it)->signalButton(nKey);
		}
	}

	cvWaitKey( 2 );
	cvDestroyAllWindows();
	cvWaitKey( 2 );
}


	
} } // namespace Ubitrack::Driver


void on_mouse( int event, int x, int y, int flags, void* param )
{
	switch( flags )
    {
    //case CV_EVENT_LBUTTONDOWN:
	case CV_EVENT_FLAG_LBUTTON:
        {
			Ubitrack::Drivers::HighguiWindow* wind = static_cast< Ubitrack::Drivers::HighguiWindow* > ( param );
			Math::Vector< double, 2 > cursor ( x, y );
			Ubitrack::Measurement::Timestamp t = Ubitrack::Measurement::now();
			wind->m_positionPort.send( Ubitrack::Measurement::Position2D( t, cursor ) );
		}
	}
}


void on_trackbar( int event, void* param ) 
{
	Ubitrack::Drivers::HighguiWindow* wind = static_cast< Ubitrack::Drivers::HighguiWindow* > ( param );
	Ubitrack::Measurement::Timestamp t = Ubitrack::Measurement::now();
	int iVal = cvGetTrackbarPos( wind->m_trackbarNames[ 0 ].c_str(), wind->getKey().c_str() );
	
	LOG4CPP_DEBUG( logger, "Trackbar changed, new value: " << iVal );

	if ( wind->m_trackbarButtonPort.isConnected() ) 
	{
		wind->m_trackbarButtonPort.send( Measurement::Button( t, iVal ) );
	}
	if ( wind->m_trackbarDistancePort.isConnected() ) 
	{
		double dVal = (double)iVal;
		if ( wind->bNorm )
			dVal /= wind->uiMaxValue;
		wind->m_trackbarDistancePort.send( Measurement::Distance( t, dVal ) );
	}
}



UBITRACK_REGISTER_COMPONENT( Dataflow::ComponentFactory* const cf ) {
	cf->registerModule< Ubitrack::Drivers::HighguiWindowModule > ( "HighguiWindow" );
}
