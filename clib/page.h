/*
 * page.h - Poppler page class header
 *
 * Copyright © 2010 Fabian Streitel <karottenreibe@gmail.com>
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

#ifndef LUAKIT_CLIB_PAGE_H
#define LUAKIT_CLIB_PAGE_H

#include <lua.h>
#include <poppler.h>

void page_class_setup(lua_State *);
static int luaH_page_new(lua_State *, PopplerDocument *, int);

#endif

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
