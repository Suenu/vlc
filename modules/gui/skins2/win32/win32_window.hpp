/*****************************************************************************
 * win32_window.hpp
 *****************************************************************************
 * Copyright (C) 2003 VideoLAN
 * $Id: win32_window.hpp,v 1.1 2004/01/03 23:31:34 asmax Exp $
 *
 * Authors: Cyril Deguet     <asmax@via.ecp.fr>
 *          Olivier Teuli�re <ipkiss@via.ecp.fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

#ifndef WIN32_WINDOW_HPP
#define WIN32_WINDOW_HPP

#include "../src/os_window.hpp"
#include <windows.h>
#include <ole2.h>   // LPDROPTARGET


/// Win32 implementation of OSWindow
class Win32Window: public OSWindow
{
    public:
        Win32Window( intf_thread_t *pIntf, GenericWindow &rWindow,
                     HINSTANCE hInst, HWND hParentWindow,
                     bool dragDrop, bool playOnDrop );
        virtual ~Win32Window();

        // Show the window
        virtual void show( int left, int top );

        // Hide the window
        virtual void hide();

        /// Move and resize the window
        virtual void moveResize( int left, int top, int width, int height );

        /// Bring the window on top
        virtual void raise();

        /// Set the opacity of the window (0 = transparent, 255 = opaque)
        virtual void setOpacity( uint8_t value );

        /// Toggle the window on top
        virtual void toggleOnTop( bool onTop );

        /// Getter for the window handle
        HWND getHandle() const { return m_hWnd; }

    private:
        /// Window handle
        HWND m_hWnd;
        /// Indicates whether the window handles drag&drop events
        bool m_dragDrop;
        /// Drop target
        LPDROPTARGET m_pDropTarget;
        bool m_mm;
};


#endif
