/*
 * widgets/image.c - gtk text area widget
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

#include "luah.h"
#include "widgets/common.h"
#include "widgets/image.h"

static gint
luaH_image_index(lua_State *L, luapdf_token_t token)
{
    switch(token)
    {
      LUAKIT_WIDGET_INDEX_COMMON

      default:
        break;
    }
    return 0;
}

void
luaH_image_set_pixbuf(lua_State *L, GdkPixbuf *buf)
{
    widget_t *w = luaH_checkwidget(L, 1);
    gtk_image_set_from_pixbuf(GTK_IMAGE(w->widget), buf);
}

widget_t *
widget_image(widget_t *w, luapdf_token_t UNUSED(token))
{
    w->index = luaH_image_index;
    w->newindex = NULL;
    w->destructor = widget_destructor;

    /* create gtk image widget as main widget */
    w->widget = gtk_image_new_from_stock(GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON);
    g_object_set_data(G_OBJECT(w->widget), "lua_widget", (gpointer) w);

    gtk_widget_show(w->widget);
    return w;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
