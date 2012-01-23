/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_pcb_text.h
 * @brief TEXTE_PCB class definition.
 */

#ifndef CLASS_PCB_TEXT_H
#define CLASS_PCB_TEXT_H

#include <class_board_item.h>
#include <PolyLine.h>


class LINE_READER;
class EDA_DRAW_PANEL;


class TEXTE_PCB : public BOARD_ITEM, public EDA_TEXT
{
public:
    TEXTE_PCB( BOARD_ITEM* parent );

    // Do not create a copy constructor.  The one generated by the compiler is adequate.

    ~TEXTE_PCB();

    const wxPoint GetPosition() const           // is an overload
    {
        return m_Pos;   // within EDA_TEXT
    }

    void SetPosition( const wxPoint& aPos )     // is an overload
    {
        m_Pos = aPos;   // within EDA_TEXT
    }

    /**
     * Function Move
     * move this object.
     * @param aMoveVector - the move vector for this object.
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }

    /**
     * Function Rotate
     * Rotate this object.
     * @param aRotCentre - the rotation point.
     * @param aAngle - the rotation angle in 0.1 degree.
     */
    virtual void Rotate( const wxPoint& aRotCentre, double aAngle );

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * @param aCentre - the rotation point.
     */
    virtual void Flip( const wxPoint& aCentre );

    /* duplicate structure */
    void Copy( TEXTE_PCB* source );

    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int aDrawMode,
               const wxPoint& offset = ZeroOffset );

    // File Operations:
    int ReadTextePcbDescr( LINE_READER* aReader );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_ITEM.
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTest( const wxPoint& refPos )
    {
        return TextHitTest( refPos );
    }

    /**
     * Function HitTest (overloaded)
     * tests if the given EDA_RECT intersect this object.
     * @param refArea the given EDA_RECT to test
     * @return bool - true if a hit, else false
     */
    bool HitTest( EDA_RECT& refArea )
    {
        return TextHitTest( refArea );
    }

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT("PTEXT");
    }

    /**
     * Function TransformShapeWithClearanceToPolygon
     * Convert the track shape to a closed polygon
     * Used in filling zones calculations
     * Circles and arcs are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aClearanceValue = the clearance around the pad
     * @param aCircleToSegmentsCount = the number of segments to approximate a circle
     * @param aCorrectionFactor = the correction to apply to circles radius to keep
     * clearance when the circle is approximated by segment bigger or equal
     * to the real clearance value (usually near from 1.0)
     */
    void TransformShapeWithClearanceToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                                               int                    aClearanceValue,
                                               int                    aCircleToSegmentsCount,
                                               double                 aCorrectionFactor );

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  add_text_xpm; }

    virtual EDA_RECT GetBoundingBox() const { return GetTextBox(); };

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;
#endif

private:
    virtual EDA_ITEM* doClone() const;
};

#endif  // #define CLASS_PCB_TEXT_H
