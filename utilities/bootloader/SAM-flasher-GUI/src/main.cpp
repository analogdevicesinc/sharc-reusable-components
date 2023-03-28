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

/*
 * Linux:
 *   cat /etc/udev/rules.d/99-SAM-Flasher.rules
 *   KERNEL=="ttyACM0", MODE="0666"
 *   apt remove modemmanager
 *
 * Windows:
 *
 * License:
 *   MIT: https://github.com/nerves-project/nerves_uart/tree/master/src
 *   Lua: https://github.com/elua/elua/tree/master/src/serial
 */

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/msgqueue.h>
#include <wx/event.h>

#include "main.h"
#include "mainFrame.h"
#include "flash_cmd.h"
#include "serial.h"
#include "uart_enum.h"

#include <stdio.h>
#include <unistd.h>

#ifdef __MINGW32__
#define SAM_COM_DESCR       "CLD"
#else
#define SAM_COM_DESCR       "SHARC"
#endif

#define TIMER_PERIOD_MS   (100)

void FlasherMainFrame::SetStatus(wxString txt)
{
    status->SetLabel(txt);
}

void FlasherMainFrame::UpdateButtonEnable(void)
{
    if (comPortOK && ldrOK) {
        Update->Enable();
    } else {
        Update->Disable();
    }
}

void FlasherMainFrame::UpdateComPorts(bool findCLDPort)
{
    struct serial_info *ports;
    wxArrayString choices;
    size_t i;

    /* Clear out the choice arrays */
    m_commPortChoice->Clear();
    commPorts.Clear();
    choices.Clear();

    /* Enumerate the serial ports */
    ports = find_serialports();
    if (ports == NULL) {
        SetStatus("No serial ports!");
        comPortOK = false;
    } else {
        while (ports) {
            choices.Add(wxString::Format("%s (%s)",
                 ports->description ? ports->description : ports->name, ports->name));
            commPorts.Add(ports->name);
            ports = ports->next;
        }
        serial_info_free_list(ports);

        m_commPortChoice->Set(choices);
        m_commPortChoice->SetSelection(0);

        /* Preselect first CLD device if desired */
        if (findCLDPort && !choices.IsEmpty()) {
            for (i = 0; i < choices.GetCount(); i++) {
                if (choices.Item(i).StartsWith(SAM_COM_DESCR)) {
                    m_commPortChoice->SetSelection(i);
                    break;
                }
            }
        }
        SetStatus("Ready");
        comPortOK = true;
    }

    UpdateButtonEnable();
}

void FlasherMainFrame::handlerMenuOpen(wxCommandEvent & event)
{
    wxFileDialog
        openFileDialog(this, _("Load LDR File"),
            "", "",
            "Files (*.ldr)|*.ldr", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        return;
    }

    ldrPath = openFileDialog.GetPath();
    fileName->SetValue(openFileDialog.GetFilename());

    ldrOK = true;

    UpdateButtonEnable();
}

void FlasherMainFrame::handlerMenuExit(wxCommandEvent & event)
{
    Close();
}

FlasherMainFrame::FlasherMainFrame(wxWindow* parent, wxWindowID id, const wxString& title) : mainFrame( parent, id, title)
{
    wxMenu *pMenu;

    /* Set the application log output */
    delete wxLog::SetActiveTarget(new wxLogStderr(NULL));

    /* Setup the application menus */
    pMenu = new wxMenu();
    pMenu->Append(wxID_EXIT, _T("&Exit"));
    pMenu->Bind(wxEVT_COMMAND_MENU_SELECTED, &FlasherMainFrame::handlerMenuExit, this, wxID_EXIT);
    m_menubar->Append(pMenu, _T("&File"));

    /* Init some widgets */
    m_gauge1->SetRange(100);
    Update->Disable();

    /* Init some variables */
    ldrPath = wxS("");
    comPortOK = false;
    ldrOK = false;

    /* Enumerate the serial ports and pre-select the first CLD port */
    UpdateComPorts(true);

    /* Bind a generic timer event to our handler */
    m_timer.Bind(wxEVT_TIMER, &FlasherMainFrame::OnTimer, this, m_timer.GetId());

    /* Start the HMI timer */
    m_timer.Start(TIMER_PERIOD_MS);

    /* Fit the HMI to the controls */
    Fit();
}

void FlasherMainFrame::OnTimer(wxTimerEvent &event)
{
    m_timer.Start(TIMER_PERIOD_MS);
}

FlasherMainFrame::~FlasherMainFrame()
{
}

void FlasherMainFrame::UpdateOnButtonClick( wxCommandEvent& event )
{
    ser_handle serial = (ser_handle)-1;

    int ok;
    int done;

    int percent;
    int old_percent;

    FILE *f;
    int fileLen, progLen, readLen;
    unsigned char dBuf[512];

    char success[256];

    wxString commPortName;
    char *commPortStr;

    ok = 0;

    /* Open LDR file and get its length */
    f = fopen(ldrPath, "rb");
    if (f) {
        fseek(f, 0, SEEK_END);
        fileLen = (int)ftell(f);
        if (fileLen < 0) {
            fileLen = 0;
            ok = 0;
        } else {
            fseek(f, 0, SEEK_SET);
            ok = 1;
        }
    } else {
        ok = 0;
    }
    if (!ok) {
        SetStatus("Unable to open .ldr file!");
    }

    /* Open selected comm port */
    if (ok) {
        commPortName = commPorts.Item(m_commPortChoice->GetSelection());
        commPortStr = strdup(commPortName.ToAscii());

        serial = ser_open(commPortStr);

        ok = (serial != (ser_handle)-1);
        if (ok) {
            ser_setup(serial, 115200, SER_DATABITS_8, SER_PARITY_NONE, SER_STOPBITS_1);
            ser_set_timeout_ms(serial, 1000);
        } else {
            SetStatus("Invalid serial port!");
        }

        free(commPortStr);
    }

    /* Ping bootloader */
    if (ok) {
        send_nop(serial);
        ok = waitfor(serial, CMD_ID_NOP);
        if (ok) {
            SetStatus("SAM Bootloader detected...");
        } else {
            SetStatus("SAM Bootloader not detected!");
        }
    }

    /* Erase application */
    if (ok) {
        SetStatus("Erasing flash...");
        wxTheApp->SafeYield(this, true);
        send_erase_application(serial);
        old_percent = percent = 0;
        do {
            ok = waitfor(serial, CMD_ID_ERASE_STATUS);
            if (ok) {
                done = process_erase_status(&percent);
                if (percent != old_percent) {
                    old_percent = percent;
                    m_gauge1->SetValue(percent);
                }
                wxTheApp->SafeYield(this, true);
            }
        } while (ok && !done);
    }

    /* Program application */
    if (ok) {
        SetStatus("Programming flash...");
        wxTheApp->SafeYield(this, true);
        old_percent = percent = 0;
        progLen = 0;
        ok = 1;
        while (ok && ((readLen = fread(dBuf, 1, 256, f)) > 0)) {
            send_program(serial, progLen, dBuf, readLen);
            ok = waitfor(serial, CMD_ID_PROGRAM_ACK);
            if (ok) {
                ok = process_program_ack(progLen);
                if (ok) {
                    progLen += readLen;
                    percent = (progLen * 100) / fileLen;
                    if (percent != old_percent) {
                        old_percent = percent;
                        m_gauge1->SetValue(percent);
                    }
                }
                wxTheApp->SafeYield(this, true);
            }
        }

        if ((ok) && (fileLen == progLen)) {
            sprintf(success, "Success! Programmed %d bytes.", progLen);
            SetStatus(success);
        } else {
            SetStatus("Programming failed!");
        }

    }

    /* Close ldr file */
    if (f) {
        fclose(f);
    }

    /* Close serial port */
    ser_close(serial);
}

void FlasherMainFrame::loadOnButtonClick( wxCommandEvent& event )
{
    wxCommandEvent evt;
    handlerMenuOpen(evt);
}

void FlasherMainFrame::closeOnButtonClick( wxCommandEvent& event )
{
    Close();
}

void FlasherMainFrame::m_commPortChoiceOnLeftDown( wxMouseEvent& event )
{
    UpdateComPorts(true);
    event.Skip();
}

bool FlasherApp::OnInit()
{
    FlasherMainFrame *frame = new FlasherMainFrame(NULL, wxID_ANY, APP_NAME " - " MY_APP_VERSION_STRING);
#ifdef __MINGW32__
    frame->SetIcon( wxICON(aaaaaaaa) );
#endif
    frame->Show(true);
    return true;
}

wxIMPLEMENT_APP(FlasherApp);
