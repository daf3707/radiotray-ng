// Copyright 2017 Michael A. Burns <michael.burns.oss@gmail.com>
//
// This file is part of Radiotray-NG.
//
// Radiotray-NG is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Radiotray-NG is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Radiotray-NG.  If not, see <http://www.gnu.org/licenses/>.


#ifdef WX_PRECOMP
#include "wx_pch.hpp"
#else
    #include <wx/splitter.h>
	#include <wx/listctrl.h>

	#include <radiotray-ng/common.hpp>
	#include <rtng_user_agent.hpp>
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include <wx/aboutdlg.h>

#include "editor_frame.hpp"
#include "editor_app.hpp"
#include "group_list.hpp"
#include "station_list.hpp"

#include <radiotray-ng/config/config.hpp>


// Set to 1 to show the wxWidgets build
// information in the About message box
#define SHOW_WXBUILD_INFO	0


namespace
{
#if SHOW_WXBUILD_INFO
    //helper functions
    enum wxbuildinfoformat
    {
        short_f,
        long_f
    };

    wxString wxbuildinfo(wxbuildinfoformat format)
    {
        wxString wxbuild(wxVERSION_STRING);

        if (format == long_f )
        {
 #if defined(__WXMSW__)
            wxbuild << _T("-Windows");
 #elif defined(__WXMAC__)
            wxbuild << _T("-Mac");
 #elif defined(__UNIX__)
            wxbuild << _T("-Linux");
 #endif

 #if wxUSE_UNICODE
            wxbuild << _T("-Unicode build");
 #else
            wxbuild << _T("-ANSI build");
 #endif // wxUSE_UNICODE
        }

        return wxbuild;
    }
#endif
}

// custom event definition
wxDEFINE_EVENT(SET_BOOKMARKS_DIRTY, wxCommandEvent);

BEGIN_EVENT_TABLE(EditorFrame, wxFrame)
    EVT_CLOSE(EditorFrame::onClose)
    EVT_MENU(idMenuNew, EditorFrame::onNew)
    EVT_MENU(idMenuOpen, EditorFrame::onOpen)
    EVT_MENU(idMenuSave, EditorFrame::onSave)
    EVT_MENU(idMenuSaveAs, EditorFrame::onSaveAs)
    EVT_MENU(idMenuQuit, EditorFrame::onQuit)
    EVT_MENU(idMenuAddGroup, EditorFrame::onAddGroup)
    EVT_MENU(idMenuEditGroup, EditorFrame::onEditGroup)
    EVT_MENU(idMenuCopyGroup, EditorFrame::onCopyGroup)
    EVT_MENU(idMenuDeleteGroup, EditorFrame::onDeleteGroup)
    EVT_MENU(idMenuAddStation, EditorFrame::onAddStation)
    EVT_MENU(idMenuEditStation, EditorFrame::onEditStation)
    EVT_MENU(idMenuCopyStation, EditorFrame::onCopyStation)
    EVT_MENU(idMenuCutStation, EditorFrame::onCutStation)
    EVT_MENU(idMenuPasteStation, EditorFrame::onPasteStation)
    EVT_MENU(idMenuDeleteStation, EditorFrame::onDeleteStation)
    EVT_MENU(idMenuAbout, EditorFrame::onAbout)
    EVT_COMMAND(MAIN_WINDOW_ID, SET_BOOKMARKS_DIRTY, EditorFrame::onSetBookmarksDirty)
	EVT_LIST_ITEM_ACTIVATED(GROUP_LIST_ID, EditorFrame::onGroupActivated)
	EVT_LIST_ITEM_ACTIVATED(STATION_LIST_ID, EditorFrame::onStationActivated)
END_EVENT_TABLE()


EditorFrame::EditorFrame(wxFrame* frame, const wxString& title, const wxString& file_to_load)
    : wxFrame(frame, MAIN_WINDOW_ID, title),
	  splitter(nullptr),
	  group_list(nullptr),
	  station_list(nullptr)
{
	this->createMenus();
	this->createStatusBar();
	this->createAccelerators();
	this->createPanels();
	this->restoreConfiguration();

	if (file_to_load.size() == 0)
	{
		std::string config_path = radiotray_ng::get_data_dir(APP_NAME);
		std::unique_ptr<IConfig> config{std::make_unique<Config>(config_path + RTNG_CONFIG_FILE)};
		if (config->load())
		{
			std::string filename = config->get_string(BOOKMARKS_KEY, RTNG_DEFAULT_BOOKMARK_FILE);
			this->loadBookmarks(filename);
			SetStatusText(filename, 1);
		}
	}
	else
	{
		this->loadBookmarks(file_to_load.ToStdString());
		SetStatusText(file_to_load, 1);
	}
}

EditorFrame::~EditorFrame()
{
	this->saveConfiguration();
}

void
EditorFrame::createMenus()
{
	// create a menu bar
	wxMenuBar* mbar = new wxMenuBar();

	wxMenu* fileMenu = new wxMenu(_T(""));
	fileMenu->Append(idMenuNew, wxString::FromUTF8("新建 (&N)"), wxString::FromUTF8("建立新的书签文件"));
	fileMenu->Append(idMenuOpen, wxString::FromUTF8("打开 (&O)\tCtrl-O"), wxString::FromUTF8("打开书签文件"));
	fileMenu->Append(idMenuSave, wxString::FromUTF8("保存 (&S)\tCtrl-S"), wxString::FromUTF8("保存书签文件"));
	fileMenu->Append(idMenuSaveAs, wxString::FromUTF8("另存为..."), wxString::FromUTF8("保存书签为新文件"));
	fileMenu->AppendSeparator();
	fileMenu->Append(idMenuQuit, wxString::FromUTF8("退出 (&Q)\tCtrl-Q"), wxString::FromUTF8("退出软件"));
	mbar->Append(fileMenu, wxString::FromUTF8("文件 (&F)"));

	wxMenu* groupMenu = new wxMenu(_T(""));
	groupMenu->Append(idMenuAddGroup, wxString::FromUTF8("新增 (&A)"));
	groupMenu->Append(idMenuEditGroup, wxString::FromUTF8("编辑 (&E)"));
	groupMenu->Append(idMenuCopyGroup, wxString::FromUTF8("复制 (&C)"));
	groupMenu->Append(idMenuDeleteGroup, wxString::FromUTF8("删除 (&D)"));
	mbar->Append(groupMenu, wxString::FromUTF8("分组 (&G)"));

	wxMenu* stationMenu = new wxMenu(_T(""));
	stationMenu->Append(idMenuAddStation, wxString::FromUTF8("新增 (&A)\tCtrl-A"));
	stationMenu->Append(idMenuEditStation, wxString::FromUTF8("编辑 (&E)\tCtrl-E"));
	stationMenu->Append(idMenuCopyStation, wxString::FromUTF8("复制 (&C)\tCtrl-C"));
	stationMenu->Append(idMenuCutStation, wxString::FromUTF8("剪切 (&T)\tCtrl-X"));
	stationMenu->Append(idMenuPasteStation, wxString::FromUTF8("粘贴 (&P)\tCtrl-V"));
	stationMenu->Append(idMenuDeleteStation, wxString::FromUTF8("删除 (&D)\tCtrl-D"));
	mbar->Append(stationMenu, wxString::FromUTF8("电台 (&S)"));

	wxMenu* helpMenu = new wxMenu(_T(""));
	helpMenu->Append(idMenuAbout, wxString::FromUTF8("关于 (&A)\tF1"), wxString::FromUTF8("关于这个软件"));
	mbar->Append(helpMenu, wxString::FromUTF8("帮助 (&H)"));

	this->SetMenuBar(mbar);

	// disable various menu items
	this->GetMenuBar()->Enable(idMenuSave, false);
	this->GetMenuBar()->Enable(idMenuSaveAs, false);
	this->GetMenuBar()->Enable(idMenuAddGroup, false);
	this->GetMenuBar()->Enable(idMenuEditGroup, false);
	this->GetMenuBar()->Enable(idMenuCopyGroup, false);
	this->GetMenuBar()->Enable(idMenuDeleteGroup, false);
	this->GetMenuBar()->Enable(idMenuAddStation, false);
	this->GetMenuBar()->Enable(idMenuEditStation, false);
	this->GetMenuBar()->Enable(idMenuCopyStation, false);
	this->GetMenuBar()->Enable(idMenuCutStation, false);
	this->GetMenuBar()->Enable(idMenuPasteStation, false);
	this->GetMenuBar()->Enable(idMenuDeleteStation, false);
}

void
EditorFrame::createStatusBar()
{
	// create a status bar with some information about the used wxWidgets version
	CreateStatusBar(2);
	SetStatusText(APPLICATION_NAME, 0);
	SetStatusText(wxT("{no file}"), 1);
}

void
EditorFrame::createAccelerators()
{
#define ACCELERATOR_COUNT	9

	// set up accelerator keys
	wxAcceleratorEntry entries[ACCELERATOR_COUNT];

	int count = 0;
	entries[count++].Set(wxACCEL_CTRL, (int) 'O', idMenuOpen);
	entries[count++].Set(wxACCEL_CTRL, (int) 'S', idMenuSave);
	entries[count++].Set(wxACCEL_CTRL, (int) 'Q', idMenuQuit);
	entries[count++].Set(wxACCEL_CTRL, (int) 'A', idMenuAddStation);
	entries[count++].Set(wxACCEL_CTRL, (int) 'E', idMenuEditStation);
	entries[count++].Set(wxACCEL_CTRL, (int) 'C', idMenuCopyStation);
	entries[count++].Set(wxACCEL_CTRL, (int) 'X', idMenuCutStation);
	entries[count++].Set(wxACCEL_CTRL, (int) 'V', idMenuPasteStation);
	entries[count++].Set(wxACCEL_CTRL, (int) 'D', idMenuDeleteStation);

	assert(count == ACCELERATOR_COUNT);

	wxAcceleratorTable accel(ACCELERATOR_COUNT, entries);
	SetAcceleratorTable(accel);
}

void
EditorFrame::createPanels()
{
	this->splitter = new wxSplitterWindow(this);
	this->splitter->SetSashGravity(0.2);

	this->station_list = new StationList(this->splitter);
	this->group_list = new GroupList(this->splitter, this->station_list);

	this->splitter->Initialize(this->group_list);
	this->splitter->SplitVertically(this->group_list, this->station_list);
}

void
EditorFrame::restoreConfiguration()
{
	std::shared_ptr<wxConfig> config = static_cast<EditorApp*>(wxTheApp)->getConfig();

    config->SetPath(wxT("/main"));

    // restore frame position and size
    int x = config->Read(wxT("x"), 50),
        y = config->Read(wxT("y"), 100),
        w = config->Read(wxT("w"), 710),
        h = config->Read(wxT("h"), 420);
    this->Move(x, y);
    this->SetClientSize(w, h);

    // restore split position
    int split = config->Read(wxT("split"), 180);
    this->splitter->SetSashPosition(split);

    // restore window configurations
    this->group_list->restoreConfiguration();
    this->station_list->restoreConfiguration();
}

void
EditorFrame::saveConfiguration()
{
	std::shared_ptr<wxConfig> config = static_cast<EditorApp*>(wxTheApp)->getConfig();

	config->SetPath(wxT("/main"));

	// save the split position
	int split = this->splitter->GetSashPosition();
	config->Write(wxT("split"), (long) split);

	// save the frame position
	int x, y, w, h;
	this->GetClientSize(&w, &h);
	this->GetPosition(&x, &y);
	config->Write(wxT("x"), (long) x);
	config->Write(wxT("y"), (long) y);
	config->Write(wxT("w"), (long) w);
	config->Write(wxT("h"), (long) h);

	// save window configurations
	this->group_list->saveConfiguration();
	this->station_list->saveConfiguration();
}

void
EditorFrame::onNew(wxCommandEvent& /* event */)
{
	if (this->editor_bookmarks.get())
	{
		if (this->editor_bookmarks->isDirty())
		{
			if (this->saveBookmarks(true) == wxCANCEL)
			{
				return;
			}
		}

		this->group_list->clearGroups();
		this->editor_bookmarks.reset();
	}

	this->editor_bookmarks = std::make_shared<EditorBookmarks>("");
	this->group_list->doNew(this->editor_bookmarks);

	this->GetMenuBar()->Enable(idMenuSaveAs, true);
	this->GetMenuBar()->Enable(idMenuAddGroup, true);
	this->GetMenuBar()->Enable(idMenuEditGroup, true);
	this->GetMenuBar()->Enable(idMenuCopyGroup, true);
	this->GetMenuBar()->Enable(idMenuDeleteGroup, true);
	this->GetMenuBar()->Enable(idMenuAddStation, true);
	this->GetMenuBar()->Enable(idMenuEditStation, true);
	this->GetMenuBar()->Enable(idMenuCopyStation, true);
	this->GetMenuBar()->Enable(idMenuCutStation, true);
	this->GetMenuBar()->Enable(idMenuPasteStation, true);
	this->GetMenuBar()->Enable(idMenuDeleteStation, true);
}

void
EditorFrame::onOpen(wxCommandEvent& /* event */)
{
	wxFileDialog dialog(this,
						("打开书签文件"),
						"",
						"",
						"书签文件 (*.json)|*.json",
						wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dialog.ShowModal() != wxID_OK)
	{
		return;
	}

	this->loadBookmarks(dialog.GetPath().ToStdString());
	SetStatusText(dialog.GetPath(), 1);
}

void
EditorFrame::onSave(wxCommandEvent& event)
{
	if (this->editor_bookmarks->getBookmarks()->get_file_name().empty())
	{
		this->onSaveAs(event);
		return;
	}

	this->saveBookmarks();
}

void
EditorFrame::onSaveAs(wxCommandEvent& /* event */)
{
	if (this->editor_bookmarks.get() == nullptr)
	{
		// should never reach here
		wxMessageBox(wxT("未检测到打开的书签"), wxT("错误"));
		this->GetMenuBar()->Enable(idMenuSave, false);
		this->GetMenuBar()->Enable(idMenuSaveAs, false);
		return;
	}

	wxFileDialog dialog(this,
						wxT("保存书签文件"),
						"",
						"",
						"书签文件 (*.json)|*.json",
						wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dialog.ShowModal() != wxID_OK)
	{
		return;
	}

	std::string filename = dialog.GetPath().ToStdString();
	if (filename.find(".json") == std::string::npos)
	{
		filename.append(".json");
	}

	this->saveBookmarks(false, filename);

	this->GetMenuBar()->Enable(idMenuSave, false);
	SetStatusText(filename, 1);
}

void
EditorFrame::onClose(wxCloseEvent& event)
{
	if (this->editor_bookmarks.get() != nullptr && this->editor_bookmarks->isDirty())
	{
		if (this->saveBookmarks(true) == wxCANCEL)
		{
			if (event.CanVeto())
			{
				event.Veto();
				return;
			}
			else
			{
				wxMessageBox(wxT("发生致命冲突，程序必须退出！"), wxT("警告"));
			}
		}
	}

	this->saveConfiguration();
	this->group_list->clearGroups();
    this->Destroy();
}

void
EditorFrame::onQuit(wxCommandEvent& /* event */)
{
	if (this->editor_bookmarks.get() != nullptr && this->editor_bookmarks->isDirty())
	{
		if (this->saveBookmarks(true) == wxCANCEL)
		{
			return;
		}
	}

	this->saveConfiguration();
	this->group_list->clearGroups();
    this->Destroy();
}

void
EditorFrame::onAddGroup(wxCommandEvent& /* event */)
{
	this->group_list->addGroup();
}

void
EditorFrame::onEditGroup(wxCommandEvent& /* event */)
{
	this->group_list->editGroup();
}

void
EditorFrame::onCopyGroup(wxCommandEvent& /* event */)
{
	this->group_list->copyGroup();
}

void
EditorFrame::onDeleteGroup(wxCommandEvent& /* event */)
{
	this->group_list->deleteGroup();
}

void
EditorFrame::onAddStation(wxCommandEvent& /* event */)
{
	this->station_list->addStation();
}

void
EditorFrame::onEditStation(wxCommandEvent& /* event */)
{
	this->station_list->editStation();
}

void
EditorFrame::onCopyStation(wxCommandEvent& /* event */)
{
	this->station_list->copyStation();
}

void
EditorFrame::onCutStation(wxCommandEvent& /* event */)
{
	this->station_list->cutStation();
}

void
EditorFrame::onPasteStation(wxCommandEvent& /* event */)
{
	this->station_list->pasteStation();
}

void
EditorFrame::onDeleteStation(wxCommandEvent& /* event */)
{
	this->station_list->deleteStation();
}

void
EditorFrame::onAbout(wxCommandEvent& /* event */)
{
    wxAboutDialogInfo aboutInfo;

    std::string image_name(static_cast<EditorApp*>(wxTheApp)->getResourcePath() + RADIOTRAY_NG_LOGO_ICON);
    wxImage image(image_name, wxBITMAP_TYPE_PNG);
    wxBitmap bitmap(image);
	wxIcon icon;
	icon.CopyFromBitmap(bitmap);
	aboutInfo.SetIcon(icon);

	std::string version{APP_NAME_DISPLAY "\nv" RTNG_VERSION};

	// if git version differs, then append hash...
	if (std::string("v" RTNG_VERSION) != std::string(RTNG_GIT_VERSION))
	{
		version += "\n(" RTNG_GIT_VERSION ")";
	}

    aboutInfo.SetName(APPLICATION_NAME);
	aboutInfo.SetDescription(version);

	std::string license = "版权所有 (C) 2017-2021  Michael A. Burns\n\n本程序不提供任何担保。\n"
		"详情请参阅 GNU 通用公共许可证第 3 版或更高版本。";

	aboutInfo.SetCopyright(license);
    aboutInfo.SetWebSite(APP_WEBSITE);

    wxAboutBox(aboutInfo);
}

void
EditorFrame::onSetBookmarksDirty(wxCommandEvent& /* event */)
{
	this->GetMenuBar()->Enable(idMenuSave, true);
}

void
EditorFrame::onGroupActivated(wxListEvent& /* event */)
{
	this->group_list->editGroup();
}

void
EditorFrame::onStationActivated(wxListEvent& /* event */)
{
	this->station_list->editStation();
}

void
EditorFrame::loadBookmarks(const std::string& filename)
{
	if (this->editor_bookmarks.get())
	{
		if (this->editor_bookmarks->isDirty())
		{
			this->saveBookmarks(true);
		}

		this->group_list->clearGroups();
		this->editor_bookmarks.reset();
	}

	LOG(debug) << "loadBookmarks -- " << filename;

	this->editor_bookmarks = std::make_shared<EditorBookmarks>(filename);
	if (this->group_list->loadBookmarks(this->editor_bookmarks) == false)
	{
		wxString msg("加载书签时失败。");
		wxMessageBox(msg, wxString::FromUTF8("错误"));

		this->editor_bookmarks.reset();
		return;
	}

	this->GetMenuBar()->Enable(idMenuSaveAs, true);
	this->GetMenuBar()->Enable(idMenuAddGroup, true);
	this->GetMenuBar()->Enable(idMenuEditGroup, true);
	this->GetMenuBar()->Enable(idMenuCopyGroup, true);
	this->GetMenuBar()->Enable(idMenuDeleteGroup, true);
	this->GetMenuBar()->Enable(idMenuAddStation, true);
	this->GetMenuBar()->Enable(idMenuEditStation, true);
	this->GetMenuBar()->Enable(idMenuCopyStation, true);
	this->GetMenuBar()->Enable(idMenuPasteStation, true);
	this->GetMenuBar()->Enable(idMenuDeleteStation, true);
}

int
EditorFrame::saveBookmarks(bool ask_to_save, const std::string& file_to_save)
{
	if (this->editor_bookmarks.get() == nullptr)
	{
		// should never reach here
		wxMessageBox(wxT("未检测到任何打开的书签！"), wxT("错误"));
		this->GetMenuBar()->Enable(idMenuSave, false);
		this->GetMenuBar()->Enable(idMenuSaveAs, false);
		return wxCANCEL;
	}

	if (file_to_save.size() == 0 && this->editor_bookmarks->isDirty() == false)
	{
		// likewise, should never reach here
		this->GetMenuBar()->Enable(idMenuSave, false);
		return wxCANCEL;
	}

	if (ask_to_save)
	{
		int status = wxMessageBox(wxT("保存书签吗?"), wxT("通知"), wxYES_NO | wxCANCEL, this);
		if (status != wxYES)
		{
			return status;
		}
	}

	if (!this->editor_bookmarks->getBookmarks()->save_as(file_to_save))
	{
		wxMessageBox("书签保存失败!", wxString::FromUTF8("错误"));
		return wxCANCEL;
	}

	this->editor_bookmarks->setDirty(false);

	this->GetMenuBar()->Enable(idMenuSave, false);

	return wxYES;
}
