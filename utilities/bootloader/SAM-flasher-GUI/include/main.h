/**
 * Copyright (c) 2021 - Analog Devices Inc. All Rights Reserved.
 * This software is proprietary and confidential to Analog Devices, Inc.
 * and its licensors.
 *
 * This software is subject to the terms and conditions of the license set
 * forth in the project LICENSE file. Downloading, reproducing, distributing or
 * otherwise using the software constitutes acceptance of the license. The
 * software may not be used except as expressly authorized under the license.
 */

#ifndef _main_h
#define _main_h

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/msgqueue.h>
#include <wx/event.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>

#include "mainFrame.h"

#define APP_NAME               "SAM Flasher"
#define MY_APP_VERSION_STRING  "0.0.5"

class FlasherMainFrame : public mainFrame
{
    public:
        FlasherMainFrame(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString);
        ~FlasherMainFrame();

    public:

    protected:
        void OnTimer(wxTimerEvent &event);
        void handlerMenuOpen(wxCommandEvent & event);
        void handlerMenuExit(wxCommandEvent & event);
        void closeOnButtonClick( wxCommandEvent& event );
        void loadOnButtonClick( wxCommandEvent& event );
        void UpdateOnButtonClick( wxCommandEvent& event );
        void m_commPortChoiceOnLeftDown( wxMouseEvent& event );
        void SetStatus(wxString status);
        void UpdateComPorts(bool findCLDPort);
        void UpdateButtonEnable(void);

    protected:
        wxString ldrPath;
        wxArrayString commPorts;
        bool comPortOK;
        bool ldrOK;

    protected:
        wxTimer m_timer;
};

class FlasherApp : public wxApp
{
    public:
        virtual bool OnInit();
};

#endif
