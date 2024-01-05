/*
 * Copyright (C) 2023 by Andreas Theofilu <andreas@theosys.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include "tbuttonstates.h"
#include "tresources.h"
#include "terror.h"

TButtonStates::TButtonStates(const TButtonStates& bs)
{
    DECL_TRACER("TButtonStates::TButtonStates(const TButtonStates& bs)");

    type = bs.type;
    ap = bs.ap;
    ad = bs.ad;
    ch = bs.ch;
    cp = bs.cp;
    lp = bs.lp;
    lv = bs.lv;

    init();
}

TButtonStates::TButtonStates(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv)
    : type(t),
      ap(rap),
      ad(rad),
      ch(rch),
      cp(rcp),
      lp(rlp),
      lv(rlv)
{
    DECL_TRACER("TButtonStates::TButtonStates(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv)");

    init();
}

TButtonStates::~TButtonStates()
{
    DECL_TRACER("TButtonStates::~TButtonStates()");
}

void TButtonStates::init()
{
    DECL_TRACER("TButtonStates::init()");

    switch(type)
    {
        case GENERAL:               mID = createButtonID(type, ap, ad, cp, ch, lp, lv); break;
        case MULTISTATE_GENERAL:    mID = createButtonID(type, ap, ad, cp, ch); break;
        case BARGRAPH:              mID = createButtonID(type, -1, -1, -1, -1, lp, lv); break;
        case MULTISTATE_BARGRAPH:   mID = createButtonID(type, -1, -1, cp, ch, lp, lv); break;
        case JOYSTICK:              mID = createButtonID(type, -1, -1, -1, -1, lp, lv); break;
        case TEXT_INPUT:            mID = createButtonID(type, ap, ad, cp, ch); break;
        case LISTBOX:               mID = createButtonID(type, ap, ad, cp, ch); break;
        case SUBPAGE_VIEW:          mID = createButtonID(type, ap, ad, cp, ch); break;
        default:
            mID = createButtonID(type, ap, ad, cp, ch, lp, lv);
    }
}

bool TButtonStates::isButton(BUTTONTYPE t, uint32_t ID)
{
//    DECL_TRACER("TButtonStates::isButton(BUTTONTYPE t, uint32_t ID)");

    if (type == t && ID == mID)
        return true;

    return false;
}

bool TButtonStates::isButton(BUTTONTYPE t, int rap, int rad, int rch, int rcp, int rlp, int rlv)
{
//    DECL_TRACER("TButtonStates::isButton(int rap, int rad, int rch, int rcp, int rlp, int rlv)");

    if (type == t && rap == ap && rad == ad && rch == ch && rcp == cp && rlp == lp && rlv == lv)
        return true;

    return false;
}

bool TButtonStates::isButton(const TButtonStates& bs)
{
//    DECL_TRACER("TButtonStates::isButton(const TButtonStates& bs)");

    if (bs.type == type && bs.ad == ad && bs.ap == ap && bs.ch == ch && bs.cp == cp && bs.lp == lp && bs.lv == lv)
        return true;

    return false;
}
