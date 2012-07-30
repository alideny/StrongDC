/* 
 * Copyright (C) 2003-2004 "Opera", <opera at home dot se>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __OMENU_H
#define __OMENU_H

namespace dcpp {

class OMenu;

struct OMenuItem {
	typedef vector<OMenuItem*> List;
	typedef List::iterator Iter;

	OMenuItem() : ownerdrawn(true), text(), parent(NULL), data(NULL) {}

	tstring text;
	OMenu* parent;
	void* data;
	bool ownerdrawn;
};

/*
 * Wouldn't it be Wonderful if WTL made their functions virtual? Yes it would...
 */
class OMenu : public CMenu {
public:
	OMenu() : CMenu() { }
	~OMenu();

	BOOL CreatePopupMenu();

	void InsertSeparator(UINT uItem, BOOL byPosition, const tstring& caption, bool accels = false);
	inline void InsertSeparatorFirst(const tstring& caption/*, bool accels = false*/) {
		InsertSeparator(0, TRUE, caption);
	}
	inline void InsertSeparatorLast(const tstring& caption/*, bool accels = false*/) {
		InsertSeparator(GetMenuItemCount(), TRUE, caption);
	}

	void CheckOwnerDrawn(UINT uItem, BOOL byPosition);

	inline void RemoveFirstItem() {
		RemoveMenu(0, MF_BYPOSITION);
	}
	inline void RemoveFirstItem(int amount) {
		for (int i = 0; i < amount; ++i) {
			RemoveMenu(0, MF_BYPOSITION);
		}
	}
	BOOL DeleteMenu(UINT nPosition, UINT nFlags) {
		CheckOwnerDrawn(nPosition, nFlags & MF_BYPOSITION);
		return CMenu::DeleteMenu(nPosition, nFlags);
	}
	BOOL RemoveMenu(UINT nPosition, UINT nFlags) {
		CheckOwnerDrawn(nPosition, nFlags & MF_BYPOSITION);
		return CMenu::RemoveMenu(nPosition, nFlags);
	}

	inline void ClearMenu() {
		RemoveFirstItem(GetMenuItemCount());
	}

	BOOL InsertMenuItem(UINT uItem, BOOL bByPosition, LPMENUITEMINFO lpmii);

	static LRESULT onInitMenuPopup(HWND hWnd, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	static LRESULT onMeasureItem(HWND hWnd, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	static LRESULT onDrawItem(HWND hWnd, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);

private:
	OMenuItem::List items;

	static void CalcTextSize(const tstring& text, HFONT font, LPSIZE size) {
		HDC dc = CreateCompatibleDC(NULL);
		HGDIOBJ old = SelectObject(dc, font);
		::GetTextExtentPoint32(dc, text.c_str(), min((int)_tcslen(text.c_str()), (int)8192), size);
		SelectObject(dc, old);
		DeleteDC(dc);
	}
};

#define MESSAGE_HANDLER_HWND(msg, func) \
	if(uMsg == msg) \
	{ \
		bHandled = TRUE; \
		lResult = func(hWnd, uMsg, wParam, lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

} // namespace dcpp

#endif // __OMENU_H
