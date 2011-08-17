/*
 * widgets/window.c - gtk window widget wrapper
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

#include <gdk/gdkx.h>
#include "luah.h"
#include "widgets/common.h"
#include "clib/soup/auth.h"

static void
destroy_cb(GtkObject* UNUSED(win), widget_t *w)
{
    /* remove window from global windows list */
    g_ptr_array_remove(globalconf.windows, w);

    lua_State *L = globalconf.L;
    luaH_object_push(L, w->ref);
    luaH_object_emit_signal(L, -1, "destroy", 0, 0);
    lua_pop(L, 1);
}

static gint
luaH_window_set_default_size(lua_State *L)
{
    widget_t *w = luaH_checkwidget(L, 1);
    gint width = (gint) luaL_checknumber(L, 2);
    gint height = (gint) luaL_checknumber(L, 3);
    gtk_window_set_default_size(GTK_WINDOW(w->widget), width, height);
    return 0;
}

static gint
luaH_window_show(lua_State *L)
{
    widget_t *w = luaH_checkwidget(L, 1);
    gtk_widget_show(w->widget);
    gdk_window_set_events(gtk_widget_get_window(w->widget), GDK_ALL_EVENTS_MASK);
    return 0;
}

static gint
luaH_window_set_screen(lua_State *L)
{
    widget_t *w = luaH_checkwidget(L, 1);
    GdkScreen *screen = NULL;

    if (lua_islightuserdata(L, 2))
        screen = (GdkScreen*)lua_touserdata(L, 2);
    else
        luaL_argerror(L, 2, "expected GdkScreen lightuserdata");

    gtk_window_set_screen(GTK_WINDOW(w->widget), screen);
    gtk_window_present(GTK_WINDOW(w->widget));
    return 0;
}


static gint
luaH_window_fullscreen(lua_State *L)
{
    widget_t *w = luaH_checkwidget(L, 1);
    gtk_window_fullscreen(GTK_WINDOW(w->widget));
    return 0;
}

static gint
luaH_window_unfullscreen(lua_State *L)
{
    widget_t *w = luaH_checkwidget(L, 1);
    gtk_window_unfullscreen(GTK_WINDOW(w->widget));
    return 0;
}

static gint
luaH_window_maximize(lua_State *L)
{
    widget_t *w = luaH_checkwidget(L, 1);
    gtk_window_maximize(GTK_WINDOW(w->widget));
    return 0;
}

static gint
luaH_window_unmaximize(lua_State *L)
{
    widget_t *w = luaH_checkwidget(L, 1);
    gtk_window_unmaximize(GTK_WINDOW(w->widget));
    return 0;
}

static gint
luaH_window_index(lua_State *L, luapdf_token_t token)
{
    widget_t *w = luaH_checkwidget(L, 1);

    switch(token)
    {
      LUAKIT_WIDGET_BIN_INDEX_COMMON(w)
      LUAKIT_WIDGET_CONTAINER_INDEX_COMMON(w)

      /* push widget class methods */
      PF_CASE(DESTROY, luaH_widget_destroy)
      PF_CASE(FOCUS,   luaH_widget_focus)
      PF_CASE(HIDE,    luaH_widget_hide)

      /* push window class methods */
      PF_CASE(SET_DEFAULT_SIZE, luaH_window_set_default_size)
      PF_CASE(SHOW,             luaH_window_show)
      PF_CASE(SET_SCREEN,       luaH_window_set_screen)
      PF_CASE(FULLSCREEN,       luaH_window_fullscreen)
      PF_CASE(UNFULLSCREEN,     luaH_window_unfullscreen)
      PF_CASE(MAXIMIZE,         luaH_window_maximize)
      PF_CASE(UNMAXIMIZE,       luaH_window_unmaximize)

      /* push string methods */
      PS_CASE(TITLE, gtk_window_get_title(GTK_WINDOW(w->widget)))

      /* push boolean properties */
      PB_CASE(DECORATED, gtk_window_get_decorated(GTK_WINDOW(w->widget)))

      case L_TK_XID:
        lua_pushnumber(L, GDK_WINDOW_XID(GTK_WIDGET(w->widget)->window));
        return 1;

      default:
        break;
    }
    return 0;
}

static gint
luaH_window_newindex(lua_State *L, luapdf_token_t token)
{
    widget_t *w = luaH_checkwidget(L, 1);

    switch(token)
    {
      LUAKIT_WIDGET_BIN_NEWINDEX_COMMON(w)

      case L_TK_DECORATED:
        gtk_window_set_decorated(GTK_WINDOW(w->widget),
                luaH_checkboolean(L, 3));
        break;

      case L_TK_TITLE:
        gtk_window_set_title(GTK_WINDOW(w->widget),
            luaL_checkstring(L, 3));
        break;

      case L_TK_ICON:
        gtk_window_set_icon_from_file(GTK_WINDOW(w->widget),
            luaL_checkstring(L, 3), NULL);
        break;

      default:
        return 0;
    }

    return luaH_object_emit_property_signal(L, 1);
}

widget_t *
widget_window(widget_t *w, luapdf_token_t UNUSED(token))
{
    w->index = luaH_window_index;
    w->newindex = luaH_window_newindex;
    w->destructor = widget_destructor;

    /* create and setup window widget */
    w->widget = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_object_set_data(G_OBJECT(w->widget), "lua_widget", (gpointer) w);
    gtk_window_set_wmclass(GTK_WINDOW(w->widget), "luapdf", "luapdf");
    gtk_window_set_default_size(GTK_WINDOW(w->widget), 800, 600);
    gtk_window_set_title(GTK_WINDOW(w->widget), "luapdf");
    GdkGeometry hints;
    hints.min_width = 1;
    hints.min_height = 1;
    gtk_window_set_geometry_hints(GTK_WINDOW(w->widget), NULL, &hints, GDK_HINT_MIN_SIZE);

    g_object_connect(G_OBJECT(w->widget),
      "signal::add",             G_CALLBACK(add_cb),       w,
      "signal::destroy",         G_CALLBACK(destroy_cb),   w,
      "signal::key-press-event", G_CALLBACK(key_press_cb), w,
      "signal::remove",          G_CALLBACK(remove_cb),    w,
      NULL);

    /* add to global windows list */
    g_ptr_array_add(globalconf.windows, w);

    return w;
}

// vim: ft=c:et:sw=4:ts=8:sts=4:tw=80
