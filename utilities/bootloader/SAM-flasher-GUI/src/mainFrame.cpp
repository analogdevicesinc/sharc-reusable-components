///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 29 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "mainFrame.h"

///////////////////////////////////////////////////////////////////////////

mainFrame::mainFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 500,-1 ), wxDefaultSize );

	m_menubar = new wxMenuBar( 0 );
	this->SetMenuBar( m_menubar );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 1, 0, 0 );
	fgSizer1->AddGrowableCol( 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL|wxBORDER_SIMPLE );
	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer2->AddGrowableCol( 1 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticStatusText = new wxStaticText( m_panel1, wxID_ANY, wxT("Status:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticStatusText->Wrap( -1 );
	m_staticStatusText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	fgSizer2->Add( m_staticStatusText, 0, wxALIGN_RIGHT|wxALL, 5 );

	status = new wxStaticText( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	status->Wrap( -1 );
	fgSizer2->Add( status, 0, wxALL, 5 );

	m_staticPortText = new wxStaticText( m_panel1, wxID_ANY, wxT("Comm Port:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticPortText->Wrap( -1 );
	m_staticPortText->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	fgSizer2->Add( m_staticPortText, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	wxArrayString m_commPortChoiceChoices;
	m_commPortChoice = new wxChoice( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_commPortChoiceChoices, 0|wxTAB_TRAVERSAL );
	m_commPortChoice->SetSelection( 0 );
	fgSizer2->Add( m_commPortChoice, 0, wxALL|wxEXPAND, 5 );

	load = new wxButton( m_panel1, wxID_ANY, wxT("Load"), wxDefaultPosition, wxDefaultSize, 0|wxTAB_TRAVERSAL );
	fgSizer2->Add( load, 0, wxALL, 5 );

	fileName = new wxTextCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY );
	fgSizer2->Add( fileName, 0, wxALL|wxEXPAND, 5 );

	Update = new wxButton( m_panel1, wxID_ANY, wxT("Update"), wxDefaultPosition, wxDefaultSize, 0|wxTAB_TRAVERSAL );
	fgSizer2->Add( Update, 0, wxALL, 5 );

	m_gauge1 = new wxGauge( m_panel1, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL );
	m_gauge1->SetValue( 0 );
	fgSizer2->Add( m_gauge1, 0, wxALL|wxEXPAND, 5 );


	m_panel1->SetSizer( fgSizer2 );
	m_panel1->Layout();
	fgSizer2->Fit( m_panel1 );
	fgSizer1->Add( m_panel1, 1, wxEXPAND | wxALL, 5 );

	m_panel2 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer3->AddGrowableCol( 0 );
	fgSizer3->SetFlexibleDirection( wxBOTH );
	fgSizer3->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	close = new wxButton( m_panel2, wxID_ANY, wxT("Close"), wxDefaultPosition, wxDefaultSize, 0|wxTAB_TRAVERSAL );
	fgSizer3->Add( close, 0, wxALL, 5 );


	m_panel2->SetSizer( fgSizer3 );
	m_panel2->Layout();
	fgSizer3->Fit( m_panel2 );
	fgSizer1->Add( m_panel2, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( fgSizer1 );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( mainFrame::mainFrameOnClose ) );
	m_commPortChoice->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( mainFrame::m_commPortChoiceOnLeftDown ), NULL, this );
	load->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( mainFrame::loadOnButtonClick ), NULL, this );
	Update->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( mainFrame::UpdateOnButtonClick ), NULL, this );
	close->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( mainFrame::closeOnButtonClick ), NULL, this );
}

mainFrame::~mainFrame()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( mainFrame::mainFrameOnClose ) );
	m_commPortChoice->Disconnect( wxEVT_LEFT_DOWN, wxMouseEventHandler( mainFrame::m_commPortChoiceOnLeftDown ), NULL, this );
	load->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( mainFrame::loadOnButtonClick ), NULL, this );
	Update->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( mainFrame::UpdateOnButtonClick ), NULL, this );
	close->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( mainFrame::closeOnButtonClick ), NULL, this );

}
