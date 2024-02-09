/*
 * Copyright 2015 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define __STDBOOL_H__	1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include "utils/log.h"
#include "netsurf/mouse.h"
#include "netsurf/plotters.h"
#include "netsurf/cookie_db.h"
#include "netsurf/keypress.h"
#include "desktop/cookie_manager.h"
}
#include "beos/cookies.h"

#include <Application.h>
#include <InterfaceKit.h>
#include <String.h>
#include <Button.h>
#include <Catalog.h>
#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>
#include <GroupLayoutBuilder.h>
#include <NetworkCookieJar.h>
#include <OutlineListView.h>
#include <ScrollView.h>
#include <StringView.h>

#include <vector>

static std::vector<struct cookie_data*> cookieJar;

class CookieWindow : public BWindow {
public:
								CookieWindow(BRect frame);
	virtual	void				MessageReceived(BMessage* message);
	virtual void				Show();
	virtual	bool				QuitRequested();

private:
			void				_BuildDomainList();
			BStringItem*		_AddDomain(BString domain, bool fake);
			void				_ShowCookiesForDomain(BString domain);
			void				_DeleteCookies();

private:
	BOutlineListView*			fDomains;
	BColumnListView*			fCookies;
	BStringView*				fHeaderView;
};

enum {
	COOKIE_IMPORT = 'cimp',
	COOKIE_EXPORT = 'cexp',
	COOKIE_DELETE = 'cdel',
	COOKIE_REFRESH = 'rfsh',

	DOMAIN_SELECTED = 'dmsl'
};


class CookieDateColumn: public BDateColumn
{
public:
	CookieDateColumn(const char* title, float width)
		:
		BDateColumn(title, width, width / 2, width * 2)
	{
	}

	void DrawField(BField* field, BRect rect, BView* parent) {
		BDateField* dateField = (BDateField*)field;
		if (dateField->UnixTime() == -1) {
			DrawString("Session cookie", parent, rect);
		} else {
			BDateColumn::DrawField(field, rect, parent);
		}
	}
};


class CookieRow: public BRow
{
public:
	CookieRow(BColumnListView* list, struct cookie_data& cookie)
		:
		BRow(),
		fCookie(cookie)
	{
		list->AddRow(this);
		SetField(new BStringField(cookie.name), 0);
		SetField(new BStringField(cookie.path), 1);
		time_t expiration = cookie.expires;
		SetField(new BDateField(&expiration), 2);
		SetField(new BStringField(cookie.value), 3);

		BString flags;
		if (cookie.secure)
			flags = "https ";
		if (cookie.http_only)
			flags = "http ";

		SetField(new BStringField(flags.String()), 4);
	}

public:
	struct cookie_data	fCookie;
};


class DomainItem: public BStringItem
{
public:
	DomainItem(BString text, bool empty)
		:
		BStringItem(text),
		fEmpty(empty)
	{
	}

public:
	bool	fEmpty;
};


CookieWindow::CookieWindow(BRect frame)
	:
	BWindow(frame,"Cookie manager", B_TITLED_WINDOW,
		B_NORMAL_WINDOW_FEEL,
		B_AUTO_UPDATE_SIZE_LIMITS | B_ASYNCHRONOUS_CONTROLS)
{
	BGroupLayout* root = new BGroupLayout(B_HORIZONTAL, 0.0);
	SetLayout(root);

	fDomains = new BOutlineListView("domain list");
	root->AddView(new BScrollView("scroll", fDomains, 0, false, true), 1);

	fHeaderView = new BStringView("label","The cookie jar is empty!");
	fCookies = new BColumnListView("cookie list", B_WILL_DRAW, B_FANCY_BORDER,
		false);

	float em = fCookies->StringWidth("M");
	float flagsLength = fCookies->StringWidth("Mhttps hostOnly" B_UTF8_ELLIPSIS);

	fCookies->AddColumn(new BStringColumn("Name",
		20 * em, 10 * em, 50 * em, 0), 0);
	fCookies->AddColumn(new BStringColumn("Path",
		10 * em, 10 * em, 50 * em, 0), 1);
	fCookies->AddColumn(new CookieDateColumn("Expiration",
		fCookies->StringWidth("88/88/8888 88:88:88 AM")), 2);
	fCookies->AddColumn(new BStringColumn("Value",
		20 * em, 10 * em, 50 * em, 0), 3);
	fCookies->AddColumn(new BStringColumn("Flags",
		flagsLength, flagsLength, flagsLength, 0), 4);

	root->AddItem(BGroupLayoutBuilder(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(5, 5, 5, 5)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fHeaderView)
			.AddGlue()
		.End()
		.Add(fCookies)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.SetInsets(5, 5, 5, 5)
			.AddGlue()
			.Add(new BButton("delete", "Delete",
				new BMessage(COOKIE_DELETE))), 3);

	fDomains->SetSelectionMessage(new BMessage(DOMAIN_SELECTED));
}


void
CookieWindow::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case DOMAIN_SELECTED:
		{
			int32 index = message->FindInt32("index");
			BStringItem* item = (BStringItem*)fDomains->ItemAt(index);
			if (item != NULL) {
				BString domain = item->Text();
				_ShowCookiesForDomain(domain);
			}
			return;
		}

		case COOKIE_REFRESH:
			_BuildDomainList();
			return;

		case COOKIE_DELETE:
			_DeleteCookies();
			return;
	}
	BWindow::MessageReceived(message);
}


void
CookieWindow::Show()
{
	BWindow::Show();
	if (IsHidden())
		return;

	PostMessage(COOKIE_REFRESH);
}


bool
CookieWindow::QuitRequested()
{
	if (!IsHidden())
		Hide();
	cookieJar.clear();
	return false;
}


void
CookieWindow::_BuildDomainList()
{
	// Empty the domain list (TODO should we do this when hiding instead?)
	for (int i = fDomains->FullListCountItems() - 1; i >= 1; i--) {
		delete fDomains->FullListItemAt(i);
	}
	fDomains->MakeEmpty();

	// BOutlineListView does not handle parent = NULL in many methods, so let's
	// make sure everything always has a parent.
	DomainItem* rootItem = new DomainItem("", true);
	fDomains->AddItem(rootItem);

	// Populate the domain list - TODO USE STL VECTOR


	for(std::vector<struct cookie_data*>::iterator it = cookieJar.begin(); it != cookieJar.end(); ++it) {
		_AddDomain((*it)->domain, false);
	}

	int i = 1;
	while (i < fDomains->FullListCountItems())
	{
		DomainItem* item = (DomainItem*)fDomains->FullListItemAt(i);
		// Detach items from the fake root
		item->SetOutlineLevel(item->OutlineLevel() - 1);
		i++;
	}
	fDomains->RemoveItem(rootItem);
	delete rootItem;

	i = 0;
	int firstNotEmpty = i;
	// Collapse empty items to keep the list short
	while (i < fDomains->FullListCountItems())
	{
		DomainItem* item = (DomainItem*)fDomains->FullListItemAt(i);
		if (item->fEmpty == true) {
			if (fDomains->CountItemsUnder(item, true) == 1) {
				// The item has no cookies, and only a single child. We can
				// remove it and move its child one level up in the tree.

				int count = fDomains->CountItemsUnder(item, false);
				int index = fDomains->FullListIndexOf(item) + 1;
				for (int j = 0; j < count; j++) {
					BListItem* child = fDomains->FullListItemAt(index + j);
					child->SetOutlineLevel(child->OutlineLevel() - 1);
				}

				fDomains->RemoveItem(item);
				delete item;

				// The moved child is at the same index the removed item was.
				// We continue the loop without incrementing i to process it.
				continue;
			} else {
				// The item has no cookies, but has multiple children. Mark it
				// as disabled so it is not selectable.
				item->SetEnabled(false);
				if (i == firstNotEmpty)
					firstNotEmpty++;
			}
		}

		i++;
	}

	fDomains->Select(firstNotEmpty);
}


BStringItem*
CookieWindow::_AddDomain(BString domain, bool fake)
{
	BStringItem* parent = NULL;
	int firstDot = domain.FindFirst('.');
	if (firstDot >= 0) {
		BString parentDomain(domain);
		parentDomain.Remove(0, firstDot + 1);
		parent = _AddDomain(parentDomain, true);
	} else {
		parent = (BStringItem*)fDomains->FullListItemAt(0);
	}

	BListItem* existing;
	int i = 0;
	// check that we aren't already there
	while ((existing = fDomains->ItemUnderAt(parent, true, i++)) != NULL) {
		DomainItem* stringItem = (DomainItem*)existing;
		if (stringItem->Text() == domain) {
			if (fake == false)
				stringItem->fEmpty = false;
			return stringItem;
		}
	}

	// Insert the new item, keeping the list alphabetically sorted
	BStringItem* domainItem = new DomainItem(domain, fake);
	domainItem->SetOutlineLevel(parent->OutlineLevel() + 1);
	BStringItem* sibling = NULL;
	int siblingCount = fDomains->CountItemsUnder(parent, true);
	for (i = 0; i < siblingCount; i++) {
		sibling = (BStringItem*)fDomains->ItemUnderAt(parent, true, i);
		if (strcmp(sibling->Text(), domainItem->Text()) > 0) {
			fDomains->AddItem(domainItem, fDomains->FullListIndexOf(sibling));
			return domainItem;
		}
	}

	if (sibling) {
		// There were siblings, but all smaller than what we try to insert.
		// Insert after the last one (and its subitems)
		fDomains->AddItem(domainItem, fDomains->FullListIndexOf(sibling)
			+ fDomains->CountItemsUnder(sibling, false) + 1);
	} else {
		// There were no siblings, insert right after the parent
		fDomains->AddItem(domainItem, fDomains->FullListIndexOf(parent) + 1);
	}

	return domainItem;
}


void
CookieWindow::_ShowCookiesForDomain(BString domain)
{
	BString label;
	label.SetToFormat("Cookies for %s", domain.String());
	fHeaderView->SetText(label);

	// Empty the cookie list
	fCookies->Clear();

	// Populate the domain list

	for(std::vector<struct cookie_data*>::iterator it = cookieJar.begin(); it != cookieJar.end(); ++it) {
		if((*it)->domain == domain) {
			new CookieRow(fCookies,**it);
		}
	}
}

static bool nsbeos_cookie_parser(const struct cookie_data* data)
{
	cookieJar.push_back((struct cookie_data*)data);
	return true;
}

void
CookieWindow::_DeleteCookies()
{
	// TODO shall we handle multiple selection here?
	CookieRow* row = (CookieRow*)fCookies->CurrentSelection();
	if (row == NULL) {
		// TODO see if a domain is selected in the domain list, and delete all
		// cookies for that domain
		return;
	}

	fCookies->RemoveRow(row);

	urldb_delete_cookie(row->fCookie.domain, row->fCookie.path, row->fCookie.name);
	cookieJar.clear();
	urldb_iterate_cookies(&nsbeos_cookie_parser);

	delete row;
}

/**
 * Creates the Cookie Manager
 */
void nsbeos_cookies_init(void)
{
	CookieWindow* cookWin=new CookieWindow(BRect(100,100,700,500));
	cookWin->Show();
	cookWin->Activate();
	urldb_iterate_cookies(&nsbeos_cookie_parser);
}
