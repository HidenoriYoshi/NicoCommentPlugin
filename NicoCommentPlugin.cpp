/********************************************************************************
 Copyright (C) 2014 Append Huang <Append@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/


//#include "Main.h"
//#include "OBSApi.h"
#include "NicoCommentPlugin.h"
extern "C" __declspec(dllexport) bool LoadPlugin();
extern "C" __declspec(dllexport) void UnloadPlugin();
extern "C" __declspec(dllexport) CTSTR GetPluginName();
extern "C" __declspec(dllexport) CTSTR GetPluginDescription();
extern "C" __declspec(dllexport) void ConfigPlugin(HWND);

LocaleStringLookup *pluginLocale = NULL;
HINSTANCE hinstMain = NULL;

ImageSource* STDCALL CreateTextSource(XElement *data)
{
    if(!data)
        return NULL;

    return new NicoCommentPlugin(data);
}

int CALLBACK FontEnumProcThingy(ENUMLOGFONTEX *logicalData, NEWTEXTMETRICEX *physicalData, DWORD fontType, ConfigTextSourceInfo *configInfo)
{
    if(fontType == TRUETYPE_FONTTYPE) //HomeWorld - GDI+ doesn't like anything other than truetype
    {
        configInfo->fontNames << logicalData->elfFullName;
        configInfo->fontFaces << logicalData->elfLogFont.lfFaceName;
    }

    return 1;
}

void DoCancelStuff(HWND hwnd)
{
    ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
    ImageSource *source = API->GetSceneImageSource(configInfo->lpName);
    //XElement *data = configInfo->data;

    if(source)
        source->UpdateSettings();
}

UINT FindFontFace(ConfigTextSourceInfo *configInfo, HWND hwndFontList, CTSTR lpFontFace)
{
    UINT id = configInfo->fontFaces.FindValueIndexI(lpFontFace);
    if(id == INVALID)
        return INVALID;
    else
    {
        for(UINT i=0; i<configInfo->fontFaces.Num(); i++)
        {
            UINT targetID = (UINT)SendMessage(hwndFontList, CB_GETITEMDATA, i, 0);
            if(targetID == id)
                return i;
        }
    }

    return INVALID;
}

UINT FindFontName(ConfigTextSourceInfo *configInfo, HWND hwndFontList, CTSTR lpFontFace)
{
    return configInfo->fontNames.FindValueIndexI(lpFontFace);
}

CTSTR GetFontFace(ConfigTextSourceInfo *configInfo, HWND hwndFontList)
{
    UINT id = (UINT)SendMessage(hwndFontList, CB_GETCURSEL, 0, 0);
    if(id == CB_ERR)
        return NULL;

    UINT actualID = (UINT)SendMessage(hwndFontList, CB_GETITEMDATA, id, 0);
    return configInfo->fontFaces[actualID];
}

INT_PTR CALLBACK ConfigureTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool bInitializedDialog = false;

    switch(message)
    {
        case WM_INITDIALOG:
            {
				//Initialization
                ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)lParam;
                SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)configInfo);
                LocalizeWindow(hwnd, pluginLocale);
                XElement *data = configInfo->data;

                // Do Fonts stuff at first-----------------------------------------

                HDC hDCtest = GetDC(hwnd);
                LOGFONT lf;
                zero(&lf, sizeof(lf));
                EnumFontFamiliesEx(hDCtest, &lf, (FONTENUMPROC)FontEnumProcThingy, (LPARAM)configInfo, 0);

				HWND hwndFonts = GetDlgItem(hwnd, IDC_FONT);
				HWND hwndNicknameFonts = GetDlgItem(hwnd, IDC_NICKNAMEFONT);
                for(UINT i=0; i<configInfo->fontNames.Num(); i++)
                {
                    int id = (int)SendMessage(hwndFonts, CB_ADDSTRING, 0, (LPARAM)configInfo->fontNames[i].Array());
                    SendMessage(hwndFonts, CB_SETITEMDATA, id, (LPARAM)i);
                    id = (int)SendMessage(hwndNicknameFonts, CB_ADDSTRING, 0, (LPARAM)configInfo->fontNames[i].Array());
                    SendMessage(hwndNicknameFonts, CB_SETITEMDATA, id, (LPARAM)i);
                }

                CTSTR lpFont = data->GetString(TEXT("font"));
                UINT id = FindFontFace(configInfo, hwndFonts, lpFont);
                if(id == INVALID)
                    id = (UINT)SendMessage(hwndFonts, CB_FINDSTRINGEXACT, -1, (LPARAM)TEXT("Arial"));
                SendMessage(hwndFonts, CB_SETCURSEL, id, 0);

				CTSTR lpNicknameFont = data->GetString(TEXT("nicknameFont"));
                id = FindFontFace(configInfo, hwndNicknameFonts, lpNicknameFont);
                if(id == INVALID)
                    id = (UINT)SendMessage(hwndNicknameFonts, CB_FINDSTRINGEXACT, -1, (LPARAM)TEXT("Arial"));
                SendMessage(hwndNicknameFonts, CB_SETCURSEL, id, 0);

				//Groupbox.Design-----------------------------------------
				UINT basewidth=0;
				UINT baseheight=0;
				OBSGetBaseSize(basewidth, baseheight);
                SendMessage(GetDlgItem(hwnd, IDC_EXTENTWIDTH),  UDM_SETRANGE32, 32, 2048);
                SendMessage(GetDlgItem(hwnd, IDC_EXTENTWIDTH),  UDM_SETPOS32, 0, data->GetInt(TEXT("extentWidth"),  basewidth));
				SendMessage(GetDlgItem(hwnd, IDC_EXTENTHEIGHT), UDM_SETRANGE32, 32, 2048);
                SendMessage(GetDlgItem(hwnd, IDC_EXTENTHEIGHT), UDM_SETPOS32, 0, data->GetInt(TEXT("extentHeight"), baseheight));
                SendMessage(GetDlgItem(hwnd, IDC_NUMOFLINES), UDM_SETRANGE32, 3, 10);
                SendMessage(GetDlgItem(hwnd, IDC_NUMOFLINES),  UDM_SETPOS32, 0, data->GetInt(TEXT("NumOfLines"), 5));
				SendMessage(GetDlgItem(hwnd, IDC_SCROLLSPEED), UDM_SETRANGE32, 5, 100); //only positive
                SendMessage(GetDlgItem(hwnd, IDC_SCROLLSPEED), UDM_SETPOS32, 0, data->GetInt(TEXT("scrollSpeed"), 10));
                CCSetColor(GetDlgItem(hwnd, IDC_BACKGROUNDCOLOR), data->GetInt(TEXT("backgroundColor"), 0xFF000000));
                SendMessage(GetDlgItem(hwnd, IDC_BACKGROUNDOPACITY), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_BACKGROUNDOPACITY), UDM_SETPOS32, 0, data->GetInt(TEXT("backgroundOpacity"), 0));
                SendMessage(GetDlgItem(hwnd, IDC_POINTFILTERING), BM_SETCHECK, data->GetInt(TEXT("pointFiltering"), 0) != 0 ? BST_CHECKED : BST_UNCHECKED, 0);

                //Groupbox.Message-----------------------------------------
                SendMessage(GetDlgItem(hwnd, IDC_TEXTSIZE), UDM_SETRANGE32, 5, 2048);
                SendMessage(GetDlgItem(hwnd, IDC_TEXTSIZE), UDM_SETPOS32, 0, data->GetInt(TEXT("fontSize"), 48));
                CCSetColor(GetDlgItem(hwnd, IDC_COLOR), data->GetInt(TEXT("color"), 0xFFFFFFFF));
                SendMessage(GetDlgItem(hwnd, IDC_TEXTOPACITY), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_TEXTOPACITY), UDM_SETPOS32, 0, data->GetInt(TEXT("textOpacity"), 100));
                SendMessage(GetDlgItem(hwnd, IDC_BOLD), BM_SETCHECK, data->GetInt(TEXT("bold"), 1) ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwnd, IDC_ITALIC), BM_SETCHECK, data->GetInt(TEXT("italic"), 0) ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwnd, IDC_UNDERLINE), BM_SETCHECK, data->GetInt(TEXT("underline"), 0) ? BST_CHECKED : BST_UNCHECKED, 0);

				bool bCheckedOutline = data->GetInt(TEXT("useOutline"), 1) != 0;
                SendMessage(GetDlgItem(hwnd, IDC_USEOUTLINE), BM_SETCHECK, bCheckedOutline ? BST_CHECKED : BST_UNCHECKED, 0);
                EnableWindow(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS_EDIT), bCheckedOutline);
                EnableWindow(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS), bCheckedOutline);
                EnableWindow(GetDlgItem(hwnd, IDC_OUTLINECOLOR), bCheckedOutline);
                EnableWindow(GetDlgItem(hwnd, IDC_OUTLINEOPACITY_EDIT), bCheckedOutline);
                EnableWindow(GetDlgItem(hwnd, IDC_OUTLINEOPACITY), bCheckedOutline);

                SendMessage(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS), UDM_SETRANGE32, 1, 20);
                SendMessage(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS), UDM_SETPOS32, 0, data->GetInt(TEXT("outlineSize"), 5));
                CCSetColor(GetDlgItem(hwnd, IDC_OUTLINECOLOR), data->GetInt(TEXT("outlineColor"), 0xFF000000));
                SendMessage(GetDlgItem(hwnd, IDC_OUTLINEOPACITY), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_OUTLINEOPACITY), UDM_SETPOS32, 0, data->GetInt(TEXT("outlineOpacity"), 100));

				//Groupbox.Nickname----------------------------------------
				bool bCheckedNickname = data->GetInt(TEXT("useNickname"), 1) != 0;
				SendMessage(GetDlgItem(hwnd, IDC_USENICKNAME), BM_SETCHECK, bCheckedNickname ? BST_CHECKED : BST_UNCHECKED, 0);
				EnableWindow(GetDlgItem(hwnd, IDC_USENICKNAMECOLOR), bCheckedNickname);
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEFALLBACKCOLOR), bCheckedNickname);
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEFONT), bCheckedNickname);
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMESIZE), bCheckedNickname);
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOPACITY), bCheckedNickname);
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEBOLD), bCheckedNickname);
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEITALIC), bCheckedNickname);
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEUNDERLINE), bCheckedNickname);
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEUSEOUTLINE), bCheckedNickname);

                CCSetColor(GetDlgItem(hwnd, IDC_NICKNAMEFALLBACKCOLOR), data->GetInt(TEXT("NicknameFallbackColor"), 0xFFFFFF00));
				bool bCheckedUseNicknameColor = data->GetInt(TEXT("useNicknameColor"), 1) != 0;
				SendMessage(GetDlgItem(hwnd, IDC_USENICKNAMECOLOR), BM_SETCHECK, bCheckedUseNicknameColor ? BST_CHECKED : BST_UNCHECKED, 0);
				SendMessage(GetDlgItem(hwnd, IDC_NICKNAMESIZE), UDM_SETRANGE32, 5, 2048);
                SendMessage(GetDlgItem(hwnd, IDC_NICKNAMESIZE), UDM_SETPOS32, 0, data->GetInt(TEXT("nicknameSize"), 48));
				SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOPACITY), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOPACITY), UDM_SETPOS32, 0, data->GetInt(TEXT("nicknameOpacity"), 100));
				SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEBOLD), BM_SETCHECK, data->GetInt(TEXT("nicknameBold"), 1) ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEITALIC), BM_SETCHECK, data->GetInt(TEXT("nicknameItalic"), 0) ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEUNDERLINE), BM_SETCHECK, data->GetInt(TEXT("nicknameUnderline"), 0) ? BST_CHECKED : BST_UNCHECKED, 0);

                bool bCheckedNicknameOutline = data->GetInt(TEXT("nicknameUseOutline"), 1) != 0;
                SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEUSEOUTLINE), BM_SETCHECK, bCheckedNicknameOutline ? BST_CHECKED : BST_UNCHECKED, 0);

                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEAUTOCOLOR), bCheckedNickname && bCheckedUseNicknameColor && bCheckedNicknameOutline);
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINECOLOR), bCheckedNickname && bCheckedNicknameOutline);
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINETHICKNESS_EDIT), bCheckedNickname && bCheckedNicknameOutline);
                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINETHICKNESS), bCheckedNickname && bCheckedNicknameOutline);
                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEOPACITY_EDIT), bCheckedNickname && bCheckedNicknameOutline);
                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEOPACITY), bCheckedNickname && bCheckedNicknameOutline);

                CCSetColor(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINECOLOR), data->GetInt(TEXT("nicknameOutlineColor"), 0xFF000000));
                SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINETHICKNESS), UDM_SETRANGE32, 1, 20);
                SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINETHICKNESS), UDM_SETPOS32, 0, data->GetInt(TEXT("nicknameOutlineSize"), 5));
                SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEOPACITY), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEOPACITY), UDM_SETPOS32, 0, data->GetInt(TEXT("nicknameOutlineOpacity"), 100));
				SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEAUTOCOLOR), BM_SETCHECK, data->GetInt(TEXT("nicknameOutlineAutoColor"), 1) ? BST_CHECKED : BST_UNCHECKED, 0);
				//Groupbox.Connection----------------------------------------
                HWND hwndIRCServer = GetDlgItem(hwnd, IDC_IRCSERVER);
                SendMessage(hwndIRCServer, CB_ADDSTRING, 0, (LPARAM)L"Twitch (irc.chat.twitch.tv)");
                SendMessage(hwndIRCServer, CB_ADDSTRING, 0, (LPARAM)L"Twitch (irc.twitch.tv)");
                int iServer = data->GetInt(TEXT("iServer"), 0);
                ClampVal(iServer, 0, 1);
                SendMessage(hwndIRCServer, CB_SETCURSEL, iServer, 0);

                HWND hwndIRCPort = GetDlgItem(hwnd, IDC_PORT);
                SendMessage(hwndIRCPort, CB_ADDSTRING, 0, (LPARAM)L"6667");
				SendMessage(hwndIRCPort, CB_ADDSTRING, 0, (LPARAM)L"80");
                int iPort = data->GetInt(TEXT("iPort"), 0);
                ClampVal(iServer, 0, 2);
                SendMessage(hwndIRCPort, CB_SETCURSEL, iPort, 0);

				SetWindowText(GetDlgItem(hwnd, IDC_CHANNEL), data->GetString(TEXT("Channel")));

				bool bUseJustinfan = data->GetInt(TEXT("useJustinfan"), 1) != 0;
                SendMessage(GetDlgItem(hwnd, IDC_USEJUSTINFAN), BM_SETCHECK, bUseJustinfan ? BST_CHECKED : BST_UNCHECKED, 0);

				SetWindowText(GetDlgItem(hwnd, IDC_NICKNAME), data->GetString(TEXT("Nickname")));
				SetWindowText(GetDlgItem(hwnd, IDC_PASSWORD), data->GetString(TEXT("Password")));
				EnableWindow(GetDlgItem(hwnd, IDC_NICKNAME), !bUseJustinfan);
				EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), !bUseJustinfan);

				//Finish-------------------------------------------------------------------------

                bInitializedDialog = true;

                return TRUE;
            }

        case WM_DESTROY:
            bInitializedDialog = false;
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {	//FONT ONLY
                case IDC_FONT:
                case IDC_NICKNAMEFONT:
                    if(bInitializedDialog)
                    {
                        if(HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
                        {
                            ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                            if(!configInfo) break;

                            String strFont;
                            if(HIWORD(wParam) == CBN_SELCHANGE)
                                strFont = GetFontFace(configInfo, (HWND)lParam);
                            else
                            {
                                UINT id = FindFontName(configInfo, (HWND)lParam, GetEditText((HWND)lParam));
                                if(id != INVALID)
                                    strFont = configInfo->fontFaces[id];
                            }

                            ImageSource *source = API->GetSceneImageSource(configInfo->lpName);
                            if(source && strFont.IsValid()) {
								switch(LOWORD(wParam)) {
									case IDC_FONT:
										source->SetString(TEXT("font"), strFont);
										break;
									case IDC_NICKNAMEFONT:
										source->SetString(TEXT("nicknameFont"), strFont);
										break;
								}
							}
                        }
                    }
                    break;

				//COLOR SELECTION
                case IDC_BACKGROUNDCOLOR:
                case IDC_COLOR:
                case IDC_OUTLINECOLOR:
				case IDC_NICKNAMEFALLBACKCOLOR:
				case IDC_NICKNAMEOUTLINECOLOR:
                    if(bInitializedDialog)
                    {
                        DWORD color = CCGetColor((HWND)lParam);

                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;
                        ImageSource *source = API->GetSceneImageSource(configInfo->lpName);
                        if(source)
                        {
                            switch(LOWORD(wParam))
                            {
                                case IDC_BACKGROUNDCOLOR:		source->SetInt(TEXT("backgroundColor"), color); break;
                                case IDC_COLOR:					source->SetInt(TEXT("color"), color); break;
                                case IDC_OUTLINECOLOR:			source->SetInt(TEXT("outlineColor"), color); break;
								case IDC_NICKNAMEFALLBACKCOLOR:	source->SetInt(TEXT("NicknameFallbackColor"), color); break;
								case IDC_NICKNAMEOUTLINECOLOR:	source->SetInt(TEXT("nicknameOutlineColor"), color); break;
                            }
                        }
                    }
                    break;

				//TEXT EDIT
				//ExtentSize: change only after press "OK"
				case IDC_NUMOFLINES_EDIT:
                case IDC_SCROLLSPEED_EDIT:
                case IDC_BACKGROUNDOPACITY_EDIT:
				case IDC_TEXTSIZE_EDIT:
				case IDC_TEXTOPACITY_EDIT:
                case IDC_OUTLINETHICKNESS_EDIT:
                case IDC_OUTLINEOPACITY_EDIT:
				case IDC_NICKNAMESIZE_EDIT:
				case IDC_NICKNAMEOPACITY_EDIT:
                case IDC_NICKNAMEOUTLINETHICKNESS_EDIT:
                case IDC_NICKNAMEOUTLINEOPACITY_EDIT:
                    if(HIWORD(wParam) == EN_CHANGE && bInitializedDialog)
                    {
                        int val = (int)SendMessage(GetWindow((HWND)lParam, GW_HWNDNEXT), UDM_GETPOS32, 0, 0);

                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;

                        ImageSource *source = API->GetSceneImageSource(configInfo->lpName);
                        if(source)
                        {
                            switch(LOWORD(wParam))
                            {
								case IDC_NUMOFLINES_EDIT:				source->SetInt(TEXT("NumOfLines"), val); break;
								case IDC_SCROLLSPEED_EDIT:				source->SetInt(TEXT("scrollSpeed"), val); break;
                                case IDC_BACKGROUNDOPACITY_EDIT:		source->SetInt(TEXT("backgroundOpacity"), val); break;
								case IDC_TEXTSIZE_EDIT:					source->SetInt(TEXT("fontSize"), val); break;
                                case IDC_TEXTOPACITY_EDIT:				source->SetInt(TEXT("textOpacity"), val); break;
                                case IDC_OUTLINETHICKNESS_EDIT:			source->SetFloat(TEXT("outlineSize"), (float)val); break;
								case IDC_OUTLINEOPACITY_EDIT:			source->SetInt(TEXT("outlineOpacity"), val); break;
								case IDC_NICKNAMESIZE_EDIT:				source->SetInt(TEXT("nicknameSize"), val); break;
								case IDC_NICKNAMEOPACITY_EDIT:			source->SetInt(TEXT("nicknameOpacity"), val); break;
								case IDC_NICKNAMEOUTLINETHICKNESS_EDIT:	source->SetFloat(TEXT("nicknameOutlineSize"), (float)val); break;
								case IDC_NICKNAMEOUTLINEOPACITY_EDIT:	source->SetInt(TEXT("nicknameOutlineOpacity"), val); break;
                            }
                        }
                    }
                    break;

				//Checkbox
				//case IDC_POINTFILTERING: Effect After Press OK
                case IDC_BOLD:
                case IDC_ITALIC:
                case IDC_UNDERLINE:
                case IDC_USEOUTLINE:
				case IDC_USENICKNAME:
				case IDC_USENICKNAMECOLOR:
                case IDC_NICKNAMEBOLD:
                case IDC_NICKNAMEITALIC:
                case IDC_NICKNAMEUNDERLINE:
				case IDC_NICKNAMEUSEOUTLINE:
				case IDC_NICKNAMEOUTLINEAUTOCOLOR:
				case IDC_USEJUSTINFAN:
                    if(HIWORD(wParam) == BN_CLICKED && bInitializedDialog)
                    {
                        BOOL bChecked = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;

                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;
						XElement *data = configInfo->data;

                        ImageSource *source = API->GetSceneImageSource(configInfo->lpName);
                        if(source)
                        {
                            switch(LOWORD(wParam))
                            {
                                case IDC_BOLD:						source->SetInt(TEXT("bold"), bChecked); break;
                                case IDC_ITALIC:					source->SetInt(TEXT("italic"), bChecked); break;
                                case IDC_UNDERLINE:					source->SetInt(TEXT("underline"), bChecked); break;
                                case IDC_USEOUTLINE:				source->SetInt(TEXT("useOutline"), bChecked); break;
								case IDC_USENICKNAME:				source->SetInt(TEXT("useNickname"), bChecked); break;
								case IDC_USENICKNAMECOLOR:			source->SetInt(TEXT("useNicknameColor"), bChecked); break;
				                case IDC_NICKNAMEBOLD:				source->SetInt(TEXT("nicknameBold"), bChecked); break;
				                case IDC_NICKNAMEITALIC:			source->SetInt(TEXT("nicknameItalic"), bChecked); break;
				                case IDC_NICKNAMEUNDERLINE:			source->SetInt(TEXT("nicknameUnderline"), bChecked); break;
								case IDC_NICKNAMEUSEOUTLINE:		source->SetInt(TEXT("nicknameUseOutline"), bChecked); break;
								case IDC_NICKNAMEOUTLINEAUTOCOLOR:	source->SetInt(TEXT("nicknameOutlineAutoColor"), bChecked); break;
								case IDC_USEJUSTINFAN:		/*source->SetInt(TEXT("useJustinfan"), bChecked);*/ break; //Effect after OK
                            }
                        }
						//Enable or Disable object
                        if(LOWORD(wParam) == IDC_USEOUTLINE)
                        {
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS_EDIT), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTLINECOLOR), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTLINEOPACITY_EDIT), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTLINEOPACITY), bChecked);
                        }

						if(LOWORD(wParam) == IDC_USENICKNAME)
                        {
							BOOL bCheckedNickname = bChecked;
							EnableWindow(GetDlgItem(hwnd, IDC_USENICKNAMECOLOR), bCheckedNickname);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEFALLBACKCOLOR), bCheckedNickname);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEFONT), bCheckedNickname);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMESIZE), bCheckedNickname);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMESIZE_EDIT), bCheckedNickname);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOPACITY), bCheckedNickname);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOPACITY_EDIT), bCheckedNickname);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEBOLD), bCheckedNickname);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEITALIC), bCheckedNickname);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEUNDERLINE), bCheckedNickname);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEUSEOUTLINE), bCheckedNickname);

							//BOOL bCheckedNickname = SendMessage(GetDlgItem(hwnd, IDC_USENICKNAME), BM_GETCHECK, 0, 0) == BST_CHECKED;
							BOOL bCheckedUseNicknameColor = SendMessage(GetDlgItem(hwnd, IDC_USENICKNAMECOLOR), BM_GETCHECK, 0, 0) == BST_CHECKED;
			                BOOL bCheckedNicknameOutline = SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEUSEOUTLINE), BM_GETCHECK, 0, 0) == BST_CHECKED;
			                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEAUTOCOLOR), bCheckedNickname && bCheckedUseNicknameColor && bCheckedNicknameOutline);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINECOLOR), bCheckedNickname && bCheckedNicknameOutline);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINETHICKNESS_EDIT), bCheckedNickname && bCheckedNicknameOutline);
			                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINETHICKNESS), bCheckedNickname && bCheckedNicknameOutline);
			                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEOPACITY_EDIT), bCheckedNickname && bCheckedNicknameOutline);
			                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEOPACITY), bCheckedNickname && bCheckedNicknameOutline);
                        }

						if(LOWORD(wParam) == IDC_NICKNAMEUSEOUTLINE)
						{
							BOOL bCheckedNickname = SendMessage(GetDlgItem(hwnd, IDC_USENICKNAME), BM_GETCHECK, 0, 0) == BST_CHECKED;
							BOOL bCheckedUseNicknameColor = SendMessage(GetDlgItem(hwnd, IDC_USENICKNAMECOLOR), BM_GETCHECK, 0, 0) == BST_CHECKED;					
			                BOOL bCheckedNicknameOutline = bChecked;
			                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEAUTOCOLOR), bCheckedNickname && bCheckedUseNicknameColor && bCheckedNicknameOutline);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINECOLOR), bCheckedNickname && bCheckedNicknameOutline);
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINETHICKNESS_EDIT), bCheckedNickname && bCheckedNicknameOutline);
			                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINETHICKNESS), bCheckedNickname && bCheckedNicknameOutline);
			                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEOPACITY_EDIT), bCheckedNickname && bCheckedNicknameOutline);
			                EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEOPACITY), bCheckedNickname && bCheckedNicknameOutline);
						}
						if(LOWORD(wParam) == IDC_USENICKNAMECOLOR)
						{
							BOOL bCheckedNickname = SendMessage(GetDlgItem(hwnd, IDC_USENICKNAME), BM_GETCHECK, 0, 0) == BST_CHECKED;
							BOOL bCheckedUseNicknameColor = bChecked;
			                BOOL bCheckedNicknameOutline = SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEUSEOUTLINE), BM_GETCHECK, 0, 0) == BST_CHECKED;
							EnableWindow(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEAUTOCOLOR), bCheckedNickname && bCheckedUseNicknameColor && bCheckedNicknameOutline);
						}

						if(LOWORD(wParam) == IDC_USEJUSTINFAN)
						{
                            EnableWindow(GetDlgItem(hwnd, IDC_NICKNAME), !bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_PASSWORD), !bChecked);
                        }
                    }
                    break;

				case IDC_IRCSERVER:
				case IDC_PORT:
                case IDC_NICKNAME:
                case IDC_PASSWORD:
                case IDC_CHANNEL:
                    break;

                case IDOK:
                    {
                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;
                        XElement *data = configInfo->data;
	
                        String strFont = GetFontFace(configInfo, GetDlgItem(hwnd, IDC_FONT));
                        String strFontDisplayName = GetEditText(GetDlgItem(hwnd, IDC_FONT));
                        if( strFont.IsEmpty() ) {
                            UINT id = FindFontName(configInfo, GetDlgItem(hwnd, IDC_FONT), strFontDisplayName);
                            if(id != INVALID)
                                strFont = configInfo->fontFaces[id];
                        }

                        if( strFont.IsEmpty() ) {
                            String strError = Str("Sources.TextSource.FontNotFound");
                            strError.FindReplace(TEXT("$1"), strFontDisplayName);
                            MessageBox(hwnd, strError, NULL, 0);
                            break;
                        }

                        String strNicknameFont = GetFontFace(configInfo, GetDlgItem(hwnd, IDC_NICKNAMEFONT));
                        String strNicknameFontDisplayName = GetEditText(GetDlgItem(hwnd, IDC_NICKNAMEFONT));
                        if( strNicknameFont.IsEmpty() ) {
                            UINT id = FindFontName(configInfo, GetDlgItem(hwnd, IDC_NICKNAMEFONT), strNicknameFontDisplayName);
                            if(id != INVALID)
                                strNicknameFont = configInfo->fontFaces[id];
                        }

                        if( strNicknameFont.IsEmpty() ) {
                            String strError = Str("Sources.TextSource.FontNotFound");
                            strError.FindReplace(TEXT("$1"), strNicknameFontDisplayName);
                            MessageBox(hwnd, strError, NULL, 0);
                            break;
                        }

						UINT extentWidth=(UINT)SendMessage(GetDlgItem(hwnd, IDC_EXTENTWIDTH),  UDM_GETPOS32, 0, 0);
						UINT extentHeight=(UINT)SendMessage(GetDlgItem(hwnd, IDC_EXTENTHEIGHT), UDM_GETPOS32, 0, 0);
						configInfo->cx = float(extentWidth);
						configInfo->cy = float(extentHeight);
                        data->SetFloat(TEXT("baseSizeCX"), configInfo->cx);
                        data->SetFloat(TEXT("baseSizeCY"), configInfo->cy);
                        data->SetInt(TEXT("extentWidth"), extentWidth);
                        data->SetInt(TEXT("extentHeight"), extentHeight);
						data->SetInt(TEXT("NumOfLines"),(int)SendMessage(GetDlgItem(hwnd, IDC_NUMOFLINES), UDM_GETPOS32, 0, 0));
                        data->SetInt(TEXT("scrollSpeed"), (int)SendMessage(GetDlgItem(hwnd, IDC_SCROLLSPEED), UDM_GETPOS32, 0, 0));
                        data->SetInt(TEXT("backgroundColor"), CCGetColor(GetDlgItem(hwnd, IDC_BACKGROUNDCOLOR)));
                        data->SetInt(TEXT("backgroundOpacity"), (UINT)SendMessage(GetDlgItem(hwnd, IDC_BACKGROUNDOPACITY), UDM_GETPOS32, 0, 0));
                        data->SetInt(TEXT("pointFiltering"), SendMessage(GetDlgItem(hwnd, IDC_POINTFILTERING), BM_GETCHECK, 0, 0) == BST_CHECKED);

                        data->SetString(TEXT("font"), strFont);
                        data->SetInt(TEXT("fontSize"), (UINT)SendMessage(GetDlgItem(hwnd, IDC_TEXTSIZE), UDM_GETPOS32, 0, 0));
                        data->SetInt(TEXT("color"), CCGetColor(GetDlgItem(hwnd, IDC_COLOR)));
                        data->SetInt(TEXT("textOpacity"), (UINT)SendMessage(GetDlgItem(hwnd, IDC_TEXTOPACITY), UDM_GETPOS32, 0, 0));
                        data->SetInt(TEXT("bold"), SendMessage(GetDlgItem(hwnd, IDC_BOLD), BM_GETCHECK, 0, 0) == BST_CHECKED);
                        data->SetInt(TEXT("italic"), SendMessage(GetDlgItem(hwnd, IDC_ITALIC), BM_GETCHECK, 0, 0) == BST_CHECKED);
                        data->SetInt(TEXT("underline"), SendMessage(GetDlgItem(hwnd, IDC_UNDERLINE), BM_GETCHECK, 0, 0) == BST_CHECKED);
                        data->SetInt(TEXT("useOutline"), SendMessage(GetDlgItem(hwnd, IDC_USEOUTLINE), BM_GETCHECK, 0, 0) == BST_CHECKED);
                        data->SetInt(TEXT("outlineColor"), CCGetColor(GetDlgItem(hwnd, IDC_OUTLINECOLOR)));
                        data->SetFloat(TEXT("outlineSize"), (float)SendMessage(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS), UDM_GETPOS32, 0, 0));
                        data->SetInt(TEXT("outlineOpacity"), (UINT)SendMessage(GetDlgItem(hwnd, IDC_OUTLINEOPACITY), UDM_GETPOS32, 0, 0));

						data->SetInt(TEXT("useNickname"), SendMessage(GetDlgItem(hwnd, IDC_USENICKNAME), BM_GETCHECK, 0, 0) == BST_CHECKED );
						data->SetInt(TEXT("NicknameFallbackColor"), CCGetColor(GetDlgItem(hwnd, IDC_NICKNAMEFALLBACKCOLOR)));
						data->SetInt(TEXT("useNicknameColor"), SendMessage(GetDlgItem(hwnd, IDC_USENICKNAMECOLOR), BM_GETCHECK, 0, 0) == BST_CHECKED );
						data->SetString(TEXT("nicknameFont"), strNicknameFont);
						data->SetInt(TEXT("nicknameSize"), (UINT)SendMessage(GetDlgItem(hwnd, IDC_NICKNAMESIZE), UDM_GETPOS32, 0, 0));
                        data->SetInt(TEXT("nicknameBold"), SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEBOLD), BM_GETCHECK, 0, 0) == BST_CHECKED);
                        data->SetInt(TEXT("nicknameItalic"), SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEITALIC), BM_GETCHECK, 0, 0) == BST_CHECKED);
                        data->SetInt(TEXT("nicknameUnderline"), SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEUNDERLINE), BM_GETCHECK, 0, 0) == BST_CHECKED);
                        data->SetInt(TEXT("nicknameOpacity"), (UINT)SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOPACITY), UDM_GETPOS32, 0, 0));
                        data->SetInt(TEXT("nicknameUseOutline"), SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEUSEOUTLINE), BM_GETCHECK, 0, 0) == BST_CHECKED);
                        data->SetInt(TEXT("nicknameOutlineColor"), CCGetColor(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINECOLOR)));
                        data->SetFloat(TEXT("nicknameOutlineSize"), (float)SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINETHICKNESS), UDM_GETPOS32, 0, 0));
                        data->SetInt(TEXT("nicknameOutlineOpacity"), (UINT)SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEOPACITY), UDM_GETPOS32, 0, 0));
                        data->SetInt(TEXT("nicknameOutlineAutoColor"), SendMessage(GetDlgItem(hwnd, IDC_NICKNAMEOUTLINEAUTOCOLOR), BM_GETCHECK, 0, 0) == BST_CHECKED);

						data->SetInt(TEXT("iServer"), (int)SendMessage(GetDlgItem(hwnd, IDC_IRCSERVER), CB_GETCURSEL, 0, 0));
						data->SetInt(TEXT("iPort"), (int)SendMessage(GetDlgItem(hwnd, IDC_PORT), CB_GETCURSEL, 0, 0));
						data->SetString(TEXT("Nickname"), GetEditText(GetDlgItem(hwnd, IDC_NICKNAME)));
                        data->SetString(TEXT("Password"), GetEditText(GetDlgItem(hwnd, IDC_PASSWORD)));
                        data->SetString(TEXT("Channel"), GetEditText(GetDlgItem(hwnd, IDC_CHANNEL)));

						data->SetInt(TEXT("useJustinfan"), SendMessage(GetDlgItem(hwnd, IDC_USEJUSTINFAN), BM_GETCHECK, 0, 0) == BST_CHECKED );

                    }

                case IDCANCEL:
                    if(LOWORD(wParam) == IDCANCEL)
                        DoCancelStuff(hwnd);

                    EndDialog(hwnd, LOWORD(wParam));
            }
            break;

        case WM_CLOSE:
            DoCancelStuff(hwnd);
            EndDialog(hwnd, IDCANCEL);
    }
    return 0;
}

bool STDCALL ConfigureTextSource(XElement *element, bool bCreating)
{
    if(!element)
    {
        AppWarning(TEXT("ConfigureTextSource: NULL element"));
        return false;
    }

    XElement *data = element->GetElement(TEXT("data"));
    if(!data)
        data = element->CreateElement(TEXT("data"));

    ConfigTextSourceInfo configInfo;
    configInfo.lpName = element->GetName();
    configInfo.data = data;

    if(DialogBoxParam(hinstMain, MAKEINTRESOURCE(IDD_CONFIG), API->GetMainWindow(), ConfigureTextProc, (LPARAM)&configInfo) == IDOK)
    {
        element->SetFloat(TEXT("cx"), configInfo.cx);
        element->SetFloat(TEXT("cy"), configInfo.cy);

        return true;
    }

    return false;
}


bool LoadPlugin()
{
	pluginLocale = new LocaleStringLookup;

    if(!pluginLocale->LoadStringFile(TEXT("plugins/NicoCommentPlugin/locale/en.txt")))
        AppWarning(TEXT("Could not open locale string file '%s'"), TEXT("plugins/NicoCommentPlugin/locale/en.txt"));

    if(scmpi(API->GetLanguage(), TEXT("en")) != 0)
    {
        String pluginStringFile;
        pluginStringFile << TEXT("plugins/NicoCommentPlugin/locale/") << API->GetLanguage() << TEXT(".txt");
        if(!pluginLocale->LoadStringFile(pluginStringFile))
            AppWarning(TEXT("Could not open locale string file '%s'"), pluginStringFile.Array());
    }

	InitColorControl(hinstMain);
    API->RegisterImageSourceClass(TEXT("NicoCommentPlugin"), PluginStr("ClassName"), (OBSCREATEPROC)CreateTextSource, (OBSCONFIGPROC)ConfigureTextSource);
	return true;
}

void UnloadPlugin()
{
	delete pluginLocale;
}

CTSTR GetPluginName()
{
	return PluginStr("Plugin.Name");
}

CTSTR GetPluginDescription()
{
    return PluginStr("Plugin.Description");
}

BOOL CALLBACK DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpBla)
{
	if(dwReason == DLL_PROCESS_ATTACH)
		hinstMain = hInst;

	return TRUE;
}

