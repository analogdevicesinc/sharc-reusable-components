///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 29 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __MAINFRAME_H__
#define __MAINFRAME_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/gauge.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class mainFrame
///////////////////////////////////////////////////////////////////////////////
class mainFrame : public wxFrame
{
	private:

	protected:
		wxMenuBar* m_menubar;
		wxPanel* m_panel1;
		wxStaticText* m_staticStatusText;
		wxStaticText* status;
		wxStaticText* m_staticPortText;
		wxChoice* m_commPortChoice;
		wxButton* load;
		wxTextCtrl* fileName;
		wxButton* Update;
		wxGauge* m_gauge1;
		wxPanel* m_panel2;
		wxButton* close;

		// Virtual event handlers, overide them in your derived class
		virtual void mainFrameOnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void m_commPortChoiceOnLeftDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void loadOnButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void UpdateOnButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void closeOnButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		mainFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 500,246 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL|wxBORDER_SUNKEN );

		~mainFrame();

};

#endif //__MAINFRAME_H__
