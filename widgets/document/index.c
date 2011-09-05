/*
 * widgets/document/index.c - Poppler document index functions
 *
 * Copyright © 2010 Fabian Streitel <luapdf@rottenrei.be>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

static gint
luaH_document_push_index(lua_State *L, PopplerIndexIter *iter)
{
    lua_newtable(L);
    if (!iter) return 1;

    int i = 1;
    do {
        PopplerAction *action = poppler_index_iter_get_action(iter);
        lua_createtable(L, 0, 2);

        lua_pushstring(L, "title");
        lua_pushstring(L, action->any.title);
        lua_rawset(L, -3);

        lua_pushstring(L, "children");
        PopplerIndexIter *child = poppler_index_iter_get_child(iter);
        luaH_document_push_index(L, child);
        lua_rawset(L, -3);

        lua_rawseti(L, -2, i);
        i += 1;
    } while (poppler_index_iter_next(iter));
    poppler_index_iter_free(iter);
    return 1;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
