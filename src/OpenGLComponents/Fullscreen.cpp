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

/**
 * @file
 * Implementation of a drop shadow component
 *
 * @author Daniel Pustka <daniel.pustka@in.tum.de>
 */

#include <log4cpp/Category.hh>

#include "Fullscreen.h"

#ifdef _WIN32
#include <utUtil/CleanWindows.h>
#endif

static log4cpp::Category& logger( log4cpp::Category::getInstance( "Drivers.Render" ) );

namespace Ubitrack { namespace Drivers {	

Math::Vector< int, 2 > makeWindowFullscreen( const std::string& sWindowName, const Math::Vector< int, 2 >& monitorPoint )
{
#ifdef _WIN32
	// first, find the window
	HWND hWnd = FindWindow( 0, sWindowName.c_str() );
	if ( !hWnd )
	{
		LOG4CPP_ERROR( logger, "No window found having name \"" << sWindowName << "\"" );
		return Math::Vector< int, 2 >( 640, 480 );
	}
	
	// now find the monitor
	HMONITOR hMonitor;
	if ( monitorPoint( 0 ) != 0xFFFF )
	{
		POINT p = { monitorPoint( 0 ), monitorPoint( 1 ) };
		hMonitor = MonitorFromPoint( p, MONITOR_DEFAULTTONEAREST );
	}
	else
		hMonitor = MonitorFromWindow( hWnd, MONITOR_DEFAULTTONEAREST );
	
	if ( !hMonitor)
	{
		LOG4CPP_ERROR( logger, "No monitor found at " << monitorPoint );
		return Math::Vector< int, 2 >( 640, 480 );
	}

	// get monitor size
	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof( monitorInfo );
	GetMonitorInfo( hMonitor, &monitorInfo );
	RECT& rcMon = monitorInfo.rcMonitor;
	Math::Vector< int, 2 > monitorSize( rcMon.right - rcMon.left, rcMon.bottom - rcMon.top );
	
	LOG4CPP_INFO( logger, "Maximizing window " << sWindowName << " on screen " << monitorInfo.szDevice 
		<< ", topLeft=(" << rcMon.left << "," << rcMon.top
		<< "), bottomRight=(" << rcMon.right << "," << rcMon.bottom << ")"	);
	
	// expand window size to fit around border -- duplicated code from freeglut glutFullScreen()
	AdjustWindowRect( &monitorInfo.rcMonitor, WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, FALSE );

	LOG4CPP_DEBUG( logger, "Outer window: topLeft=(" << rcMon.left << "," << rcMon.top
		<< "), bottomRight=(" << rcMon.right << "," << rcMon.bottom << ")"	);
	
	// remove window border -- does not work because freeglut assumes window border in size calculations!
	// LONG_PTR wndStyle = GetWindowLongPtr( hWnd, GWL_STYLE );
	// wndStyle &= ~( WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX | WS_SYSMENU | WS_THICKFRAME );
	// SetWindowLongPtr( hWnd, GWL_STYLE, wndStyle );
	
	// resize window to cover monitor
	SetWindowPos( hWnd, HWND_TOP, rcMon.left, rcMon.top, rcMon.right - rcMon.left, rcMon.bottom - rcMon.top, 
		/*SWP_FRAMECHANGED | */SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING | SWP_NOZORDER );
		
	return monitorSize;
#else
	// dummy
	return Math::Vector< int, 2 >( 640, 480 );
#endif

}


} } // namespace Ubitrack::Drivers

