/* 
Copyright (C) 2010 Ricardo Pescuma Domenecci

This is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this file; see the file license.txt.  If
not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  
*/

#include "commons.h"


// Prototypes ///////////////////////////////////////////////////////////////////////////


PLUGININFOEX pluginInfo = {
	sizeof(PLUGININFOEX),
#ifdef UNICODE
	"SIP Client Test (Unicode)",
#else
	"SIP Client Test (Ansi)",
#endif
	PLUGIN_MAKE_VERSION(0,1,1,0),
	"SIP Client example",
	"Ricardo Pescuma Domenecci",
	"pescuma@miranda-im.org",
	"© 2010 Ricardo Pescuma Domenecci",
	"http://pescuma.org/miranda/sip",
	UNICODE_AWARE,
	0,		//doesn't replace anything built-in
#ifdef UNICODE
	{ 0xcfbcbfac, 0x7f26, 0x489f, { 0x91, 0x7e, 0xc6, 0x7f, 0xc2, 0x2e, 0x55, 0xfd } } // {CFBCBFAC-7F26-489f-917E-C67FC22E55FD}
#else
	{ 0x97506685, 0x6de4, 0x47ef, { 0x84, 0x61, 0x5f, 0xbd, 0xcf, 0xd0, 0x8f, 0x0 } } // {97506685-6DE4-47ef-8461-5FBDCFD08F00}
#endif
};

#define MIID_SIP_CLI { 0xa8a11cca, 0x5545, 0x43d2, { 0xb0, 0x5c, 0x55, 0x5a, 0xde, 0xe1, 0x11, 0x2f } }



HINSTANCE hInst;
PLUGINLINK *pluginLink;
MM_INTERFACE mmi;
UTF8_INTERFACE utfi;

static INT_PTR ModulesLoaded(WPARAM wParam, LPARAM lParam);
static INT_PTR PreShutdown(WPARAM wParam, LPARAM lParam);

static BOOL CALLBACK TestDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

static INT_PTR Service_Call(WPARAM wParam, LPARAM lParam); 
static INT_PTR Service_Answer(WPARAM wParam, LPARAM lParam); 
static INT_PTR Service_Hold(WPARAM wParam, LPARAM lParam);
static INT_PTR Service_Drop(WPARAM wParam, LPARAM lParam);
static INT_PTR Service_SendDTMF(WPARAM wParam, LPARAM lParam);


// Functions ////////////////////////////////////////////////////////////////////////////


extern "C" BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) 
{
	hInst = hinstDLL;
	return TRUE;
}


extern "C" __declspec(dllexport) PLUGININFO* MirandaPluginInfo(DWORD mirandaVersion) 
{
	pluginInfo.cbSize = sizeof(PLUGININFO);
	return (PLUGININFO*) &pluginInfo;
}


extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD mirandaVersion)
{
	pluginInfo.cbSize = sizeof(PLUGININFOEX);
	return &pluginInfo;
}


static const MUUID interfaces[] = { MIID_SIP_CLI, MIID_LAST };
extern "C" __declspec(dllexport) const MUUID* MirandaPluginInterfaces(void)
{
	return interfaces;
}


extern "C" int __declspec(dllexport) Load(PLUGINLINK *link) 
{
	pluginLink = link;

	CHECK_VERSION("SIP_CLI")

	// TODO Assert results here
	mir_getMMI(&mmi);
	mir_getUTFI(&utfi);

	HookEvent(ME_SYSTEM_MODULESLOADED, ModulesLoaded);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, PreShutdown);

	return 0;
}


extern "C" int __declspec(dllexport) Unload(void) 
{
	return 0;
}



SIP_CLIENT *cli;
HANDLE hEvent;
HWND hwnd;

void Callback(void *param, int callId, int state, int flags, const TCHAR *host_port)
{
	char tmp[16];

	VOICE_CALL vc = {0};
	vc.cbSize = sizeof(vc);
	vc.moduleName = MODULE_NAME;
	vc.id = itoa(callId, tmp, 10);
	vc.flags = flags;
	vc.hContact = NULL;
	//vc.ptszName = name;
	vc.ptszNumber = host_port;
	vc.state = state;
	
	NotifyEventHooks(hEvent, (WPARAM) &vc, 0);
	
}

static INT_PTR Service_Call(WPARAM wParam, LPARAM lParam)
{
	// Not implemented yet
	return -1;
}

static INT_PTR Service_Answer(WPARAM wParam, LPARAM lParam)
{
	const char *id = (const char *) wParam;
	if (id == NULL)
		return -1;

	return cli->AnswerCall(cli, atoi(id));
}

static INT_PTR Service_Hold(WPARAM wParam, LPARAM lParam)
{
	const char *id = (const char *) wParam;
	if (id == NULL)
		return -1;
	
	return cli->HoldCall(cli, atoi(id));
}

static INT_PTR Service_Drop(WPARAM wParam, LPARAM lParam)
{
	const char *id = (const char *) wParam;
	if (id == NULL)
		return -1;
	
	return cli->DropCall(cli, atoi(id));
}

static INT_PTR Service_SendDTMF(WPARAM wParam, LPARAM lParam)
{
	const char *id = (const char *) wParam;
	if (id == NULL)
		return -1;
	
	return cli->SendDTMF(cli, atoi(id), (TCHAR) lParam);
}

static INT_PTR ModulesLoaded(WPARAM wParam, LPARAM lParam) 
{	
	SIP_REGISTRATION reg = {0};
	reg.cbSize = sizeof(SIP_REGISTRATION);
	reg.name = MODULE_NAME;
	reg.stun.host = _T("stun01.sipphone.com");
	reg.stun.port = 3478;
	reg.callback = Callback;
	
	cli = (SIP_CLIENT *) CallService(MS_SIP_REGISTER, (WPARAM) &reg, 0);
	if (cli == NULL)
	{
		MessageBox(NULL, _T("Error registering with SIP"), _T("SIP_CLI"), MB_OK | MB_ICONERROR);
		return 0;
	}

	char tmp[512];
	mir_snprintf(tmp, MAX_REGS(tmp), "SIP_CLI%s", PE_VOICE_CALL_STATE);
	hEvent = CreateHookableEvent(tmp);

	mir_snprintf(tmp, MAX_REGS(tmp), "SIP_CLI%s", PS_VOICE_CALL);
	CreateServiceFunction(tmp, &Service_Call);
	mir_snprintf(tmp, MAX_REGS(tmp), "SIP_CLI%s", PS_VOICE_ANSWERCALL);
	CreateServiceFunction(tmp, &Service_Answer);
	mir_snprintf(tmp, MAX_REGS(tmp), "SIP_CLI%s", PS_VOICE_DROPCALL);
	CreateServiceFunction(tmp, &Service_Drop);
	mir_snprintf(tmp, MAX_REGS(tmp), "SIP_CLI%s", PS_VOICE_HOLDCALL);
	CreateServiceFunction(tmp, &Service_Hold);
	mir_snprintf(tmp, MAX_REGS(tmp), "SIP_CLI%s", PS_VOICE_SEND_DTMF);
	CreateServiceFunction(tmp, &Service_SendDTMF);

	VOICE_MODULE vm = {0};
	vm.cbSize = sizeof(vm);
	vm.description = _T("SIP Client Test");
	vm.name = "SIP_CLI";
	// TODO vm.icon
	vm.flags = VOICE_CAPS_VOICE;
	CallService(MS_VOICESERVICE_REGISTER, (WPARAM) &vm, 0);

	hwnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DLG), NULL, TestDlgProc);
	ShowWindow(hwnd, SW_SHOWNORMAL);

	return 0;
}


static INT_PTR PreShutdown(WPARAM wParam, LPARAM lParam)
{
	if (cli)
		CallService(MS_SIP_UNREGISTER, (WPARAM) cli, 0);
	
	return 0;
}


static BOOL CALLBACK TestDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_INITDIALOG:
		{
			TranslateDialogDefault(hwndDlg);
			
			TCHAR text[1024];
			mir_sntprintf(text, MAX_REGS(text), TranslateT("UDP:  %s : %d\nTCP:  %s : %d\nTLS:  %s : %d"),
				cli->udp_host, cli->udp_port, 
				cli->tcp_host, cli->tcp_port, 
				cli->tls_host, cli->tls_port);
			SendDlgItemMessage(hwndDlg, IDC_DATA, WM_SETTEXT, 0, (LPARAM) text);

			SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, CB_ADDSTRING, 0, (LPARAM) _T("UDP"));
			SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, CB_ADDSTRING, 0, (LPARAM) _T("TCP"));
			SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, CB_ADDSTRING, 0, (LPARAM) _T("TLS"));
			SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, CB_SETCURSEL, 0, 0);
			
			return TRUE;
		}
		
		case WM_COMMAND:
		{
			switch(wParam)
			{
				case IDC_CALL:
				{
					TCHAR host[512];
					GetDlgItemText(hwndDlg, IDC_HOST, host, MAX_REGS(host));
					int port = GetDlgItemInt(hwndDlg, IDC_PORT, NULL, TRUE);
					int protocol = SendDlgItemMessage(hwndDlg, IDC_PROTOCOL, CB_GETCURSEL, 0, 0) + 1;

					int callId = cli->Call(cli, host, port, protocol);
					if (callId < 0)
						MessageBox(NULL, _T("Error making call"), _T("SIP_CLI"), MB_OK | MB_ICONERROR);

					break;
				}
			}
			break;
		}
	}
	
	return FALSE;
}
