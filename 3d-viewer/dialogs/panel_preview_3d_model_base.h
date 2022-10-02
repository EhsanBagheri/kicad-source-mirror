///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/spinbutt.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/slider.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PREVIEW_3D_MODEL_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PREVIEW_3D_MODEL_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticText1;
		wxTextCtrl* xscale;
		wxSpinButton* m_spinXscale;
		wxStaticText* m_staticText2;
		wxTextCtrl* yscale;
		wxSpinButton* m_spinYscale;
		wxStaticText* m_staticText3;
		wxTextCtrl* zscale;
		wxSpinButton* m_spinZscale;
		wxStaticText* m_staticText11;
		wxTextCtrl* xrot;
		wxSpinButton* m_spinXrot;
		wxStaticText* m_staticText21;
		wxTextCtrl* yrot;
		wxSpinButton* m_spinYrot;
		wxStaticText* m_staticText31;
		wxTextCtrl* zrot;
		wxSpinButton* m_spinZrot;
		wxStaticText* m_staticText12;
		wxTextCtrl* xoff;
		wxSpinButton* m_spinXoffset;
		wxStaticText* m_staticText22;
		wxSpinButton* m_spinYoffset;
		wxStaticText* m_staticText32;
		wxTextCtrl* zoff;
		wxSpinButton* m_spinZoffset;
		wxSlider* m_opacity;
		wxTextCtrl* m_boardThicknessCtrl;
		wxStaticText* m_boardThicknessUnits;
		wxBoxSizer* m_SizerPanelView;
		wxBitmapButton* m_bpvISO;
		wxBitmapButton* m_bpvLeft;
		wxBitmapButton* m_bpvRight;
		wxBitmapButton* m_bpvFront;
		wxBitmapButton* m_bpvBack;
		wxBitmapButton* m_bpvTop;
		wxBitmapButton* m_bpvBottom;
		wxBitmapButton* m_bpUpdate;

		// Virtual event handlers, override them in your derived class
		virtual void onMouseWheelScale( wxMouseEvent& event ) { event.Skip(); }
		virtual void updateOrientation( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDecrementScale( wxSpinEvent& event ) { event.Skip(); }
		virtual void onIncrementScale( wxSpinEvent& event ) { event.Skip(); }
		virtual void onMouseWheelRot( wxMouseEvent& event ) { event.Skip(); }
		virtual void onDecrementRot( wxSpinEvent& event ) { event.Skip(); }
		virtual void onIncrementRot( wxSpinEvent& event ) { event.Skip(); }
		virtual void onMouseWheelOffset( wxMouseEvent& event ) { event.Skip(); }
		virtual void onDecrementOffset( wxSpinEvent& event ) { event.Skip(); }
		virtual void onIncrementOffset( wxSpinEvent& event ) { event.Skip(); }
		virtual void onOpacitySlider( wxCommandEvent& event ) { event.Skip(); }
		virtual void onMouseWheelBoardThickness( wxMouseEvent& event ) { event.Skip(); }
		virtual void updateBoardThickness( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DISO( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DLeft( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DRight( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DFront( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DBack( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DTop( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DBottom( wxCommandEvent& event ) { event.Skip(); }
		virtual void View3DUpdate( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxTextCtrl* yoff;

		PANEL_PREVIEW_3D_MODEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PREVIEW_3D_MODEL_BASE();

};

