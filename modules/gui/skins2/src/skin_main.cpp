/*****************************************************************************
 * skin_main.cpp
 *****************************************************************************
 * Copyright (C) 2003 VideoLAN
 * $Id: skin_main.cpp,v 1.1 2004/01/03 23:31:34 asmax Exp $
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

#include <stdlib.h>
#include "generic_window.hpp"
#include "dialogs.hpp"
#include "os_factory.hpp"
#include "os_loop.hpp"
#include "window_manager.hpp"
#include "var_manager.hpp"
#include "vlcproc.hpp"
#include "theme.hpp"
#include "theme_loader.hpp"
#include "../commands/async_queue.hpp"


//---------------------------------------------------------------------------
// Exported interface functions.
//---------------------------------------------------------------------------
#ifdef WIN32_SKINS
extern "C" __declspec( dllexport )
    int __VLC_SYMBOL( vlc_entry ) ( module_t *p_module );
#endif


//---------------------------------------------------------------------------
// Local prototypes.
//---------------------------------------------------------------------------
static int  Open  ( vlc_object_t * );
static void Close ( vlc_object_t * );
static void Run   ( intf_thread_t * );


//---------------------------------------------------------------------------
// Open: initialize interface
//---------------------------------------------------------------------------
static int Open( vlc_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t *)p_this;

    // Allocate instance and initialize some members
    p_intf->p_sys = (intf_sys_t *) malloc( sizeof( intf_sys_t ) );
    if( p_intf->p_sys == NULL )
    {
        msg_Err( p_intf, "out of memory" );
        return( VLC_ENOMEM );
    };

    p_intf->pf_run = Run;

    // Suscribe to messages bank
    p_intf->p_sys->p_sub = msg_Subscribe( p_intf );

    p_intf->p_sys->p_input = NULL;
    p_intf->p_sys->p_playlist = (playlist_t *)vlc_object_find( p_intf,
        VLC_OBJECT_PLAYLIST, FIND_ANYWHERE );
    if( p_intf->p_sys->p_playlist == NULL )
    {
        msg_Err( p_intf, "No playlist object found" );
        return VLC_EGENERIC;
    }

    // Initialize "singleton" objects
    p_intf->p_sys->p_logger = NULL;
    p_intf->p_sys->p_queue = NULL;
    p_intf->p_sys->p_dialogs = NULL;
    p_intf->p_sys->p_osFactory = NULL;
    p_intf->p_sys->p_osLoop = NULL;
    p_intf->p_sys->p_varManager = NULL;
    p_intf->p_sys->p_vlcProc = NULL;

    // No theme yet
    p_intf->p_sys->p_theme = NULL;

    // Initialize singletons
    if( AsyncQueue::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "Cannot initialize AsyncQueue" );
        return VLC_EGENERIC;
    }
    if( OSFactory::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "Cannot initialize OSFactory" );
        return VLC_EGENERIC;
    }
    if( VarManager::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "Cannot instanciate VarManager" );
        return VLC_EGENERIC;
    }
    if( VlcProc::instance( p_intf ) == NULL )
    {
        msg_Err( p_intf, "Cannot initialize VLCProc" );
        return VLC_EGENERIC;
    }
    Dialogs::instance( p_intf );

    return( VLC_SUCCESS );
}

//---------------------------------------------------------------------------
// Close: destroy interface
//---------------------------------------------------------------------------
static void Close( vlc_object_t *p_this )
{
    intf_thread_t *p_intf = (intf_thread_t *)p_this;

    // Destroy "singleton" objects
    VlcProc::destroy( p_intf );
    OSFactory::instance( p_intf )->destroyOSLoop();
    OSFactory::destroy( p_intf );
    Dialogs::destroy( p_intf );
    AsyncQueue::destroy( p_intf );
    VarManager::destroy( p_intf );

    if( p_intf->p_sys->p_input )
    {
        vlc_object_release( p_intf->p_sys->p_input );
    }

    if( p_intf->p_sys->p_playlist )
    {
        vlc_object_release( p_intf->p_sys->p_playlist );
    }

   // Unsuscribe to messages bank
    msg_Unsubscribe( p_intf, p_intf->p_sys->p_sub );

    // Destroy structure
    free( p_intf->p_sys );
}


//---------------------------------------------------------------------------
// Run: main loop
//---------------------------------------------------------------------------
static void Run( intf_thread_t *p_intf )
{
    // Load a theme
    ThemeLoader *pLoader = new ThemeLoader( p_intf );
    char *skin_last = config_GetPsz( p_intf, "skin_last2" );

    if( skin_last == NULL || !pLoader->load( skin_last ) )
    {
        // Too bad, it failed. Let's try with the default theme
#ifdef WIN32_SKINS
        string default_dir = (string)p_intf->p_libvlc->psz_vlcpath +
                             "\\skins\\default\\theme.xml";
        if( !pLoader->load( default_dir ) )
#else
        string user_skin = (string)p_intf->p_vlc->psz_homedir +
                           "/" + CONFIG_DIR + "/skins/default/theme.xml";

        string default_skin = (string)DATA_PATH + "/skins/default/theme.xml";
        if( !pLoader->load( user_skin ) && !pLoader->load( default_skin ) )
#endif
        {
            // Last chance: the user can select a new theme file (blocking call)
            Dialogs *pDialogs = Dialogs::instance( p_intf );
            pDialogs->showChangeSkin();
        }
    }
    delete pLoader;

    if( skin_last )
    {
        free( skin_last );
    }

    // Get the instance of OSLoop
    OSLoop *loop = OSFactory::instance( p_intf )->getOSLoop();
    loop->run();

    // Delete the theme
    if( p_intf->p_sys->p_theme )
    {
        delete p_intf->p_sys->p_theme;
        p_intf->p_sys->p_theme = NULL;
    }
}

//---------------------------------------------------------------------------
// Module descriptor
//---------------------------------------------------------------------------
#define DEFAULT_SKIN        N_("Last skin used")
#define DEFAULT_SKIN_LONG   N_("Select the path to the last skin used")

vlc_module_begin();
// XXX
    add_string( "skin_last2", "", NULL, DEFAULT_SKIN, DEFAULT_SKIN_LONG,
                VLC_TRUE );
    set_description( _("Skinnable Interface") );
    set_capability( "interface", 30 );
    set_callbacks( Open, Close );
    set_program( "svlc" );
vlc_module_end();

