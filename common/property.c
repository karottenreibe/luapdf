/*
 * common/property.c - GObject property set/get lua functions
 *
 * Copyright © 2011 Mason Larobina <mason.larobina@gmail.com>
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

#include "common/property.h"
#include <stdlib.h>
#include <gtk/gtk.h>
#include "luah.h"

GHashTable*
hash_properties(property_t *properties_table)
{
    GHashTable *properties = g_hash_table_new(g_str_hash, g_str_equal);
    for (property_t *p = properties_table; p->name; p++) {
        /* pre-compile "property::name" signals for each property */
        if (!p->signame)
            p->signame = g_strdup_printf("property::%s", p->name);
        g_hash_table_insert(properties, (gpointer) p->name, (gpointer) p);
    }
    return properties;
}

/* sets a gobject property from lua */
gint
luaH_get_property(lua_State *L, GHashTable *properties, gpointer obj, gint nidx)
{
    GObject *so;
    property_t *p;
    property_tmp_value_t tmp;

    /* get property struct */
    const gchar *name = luaL_checkstring(L, nidx);
    if ((p = g_hash_table_lookup(properties, name))) {
        /* get scope object */
        so = G_OBJECT(obj);

        switch(p->type) {
          case BOOL:
            g_object_get(so, p->name, &tmp.b, NULL);
            lua_pushboolean(L, tmp.b);
            return 1;

          case INT:
            g_object_get(so, p->name, &tmp.i, NULL);
            lua_pushnumber(L, tmp.i);
            return 1;

          case FLOAT:
            g_object_get(so, p->name, &tmp.f, NULL);
            lua_pushnumber(L, tmp.f);
            return 1;

          case DOUBLE:
            g_object_get(so, p->name, &tmp.d, NULL);
            lua_pushnumber(L, tmp.d);
            return 1;

          case CHAR:
            g_object_get(so, p->name, &tmp.c, NULL);
            lua_pushstring(L, tmp.c);
            g_free(tmp.c);
            return 1;

          default:
            warn("unknown property type for: %s", p->name);
            break;
        }
    }
    warn("unknown property: %s", name);
    return 0;
}

/* gets a gobject property from lua */
gint
luaH_set_property(lua_State *L, GHashTable *properties, gpointer obj, gint nidx, gint vidx)
{
    GObject *so;
    property_t *p;
    property_tmp_value_t tmp;

    /* get property struct */
    const gchar *name = luaL_checkstring(L, nidx);
    if ((p = g_hash_table_lookup(properties, name))) {
        if (!p->writable) {
            warn("attempt to set read-only property: %s", p->name);
            return 0;
        }

        so = G_OBJECT(obj);
        switch(p->type) {
          case BOOL:
            tmp.b = luaH_checkboolean(L, vidx);
            g_object_set(so, p->name, tmp.b, NULL);
            return 0;

          case INT:
            tmp.i = (gint) luaL_checknumber(L, vidx);
            g_object_set(so, p->name, tmp.i, NULL);
            return 0;

          case FLOAT:
            tmp.f = (gfloat) luaL_checknumber(L, vidx);
            g_object_set(so, p->name, tmp.f, NULL);
            return 0;

          case DOUBLE:
            tmp.d = (gdouble) luaL_checknumber(L, vidx);
            g_object_set(so, p->name, tmp.d, NULL);
            return 0;

          case CHAR:
            tmp.c = (gchar*) luaL_checkstring(L, vidx);
            g_object_set(so, p->name, tmp.c, NULL);
            return 0;

          default:
            warn("unknown property type for: %s", p->name);
            break;
        }
    }
    warn("unknown property: %s", name);
    return 0;
}
