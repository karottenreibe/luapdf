/*
 * widgets/image.h - image widget functions
 *
 * Copyright © 2010 Mason Larobina <mason.larobina@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef LUAKIT_WIDGETS_IMAGE_H
#define LUAKIT_WIDGETS_IMAGE_H

#include "clib/widget.h"

widget_t * widget_image(widget_t *, luapdf_token_t);
void luaH_image_set_pixbuf(lua_State *, GdkPixbuf *);

#endif
// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
