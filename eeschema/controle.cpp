/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fctsys.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <eeschema_id.h>
#include <hotkeys.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>
#include <sch_view.h>


bool LIB_VIEW_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, EDA_KEY aHotKey )
{
    bool eventHandled = true;

    // Filter out the 'fake' mouse motion after a keyboard movement
    if( !aHotKey && m_movingCursorWithKeyboard )
    {
        m_movingCursorWithKeyboard = false;
        return false;
    }

    wxPoint pos = aPosition;
    GeneralControlKeyMovement( aHotKey, &pos, true );

    // Update cursor position.
    m_canvas->CrossHairOn( aDC );
    SetCrossHairPosition( pos, true );

    if( aHotKey )
    {
        SCH_SCREEN* screen = GetScreen();

        if( screen->GetCurItem() && screen->GetCurItem()->GetEditFlags() )
            eventHandled = OnHotKey( aDC, aHotKey, aPosition, screen->GetCurItem() );
        else
            eventHandled = OnHotKey( aDC, aHotKey, aPosition, NULL );
    }

    UpdateStatusBar();    // Display cursor coordinates info.

    return eventHandled;
}
