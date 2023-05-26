/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright 2010-2023, Tarantool AUTHORS, please see AUTHORS file.
 */

#include "box/lua/tuple_format.h"

#include "box/tuple.h"
#include "box/tuple_format.h"

#include "lua/utils.h"

static const char *tuple_format_typename = "box.tuple.format";

struct tuple_format *
luaT_check_tuple_format(struct lua_State *L, int narg)
{
	return *(struct tuple_format **)luaL_checkudata(L, narg,
							tuple_format_typename);
}

static int
lbox_tuple_format_gc(struct lua_State *L)
{
	struct tuple_format *format = luaT_check_tuple_format(L, 1);
	tuple_format_unref(format);
	return 0;
}

/*
 * Creates a cdata object for tuple format and pushes it onto Lua stack.
 */
static int
luaT_push_tuple_format(struct lua_State *L, struct tuple_format *format)
{
	struct tuple_format **ptr = lua_newuserdata(L, sizeof(*ptr));
	*ptr = format;
	tuple_format_ref(format);
	luaL_getmetatable(L, tuple_format_typename);
	lua_setmetatable(L, -2);
	return 1;
}

/*
 * Create a tuple format using a format clause with an external source (IPROTO):
 * all format clause fields except for 'name' are ignored.
 */
static int
lbox_tuple_format_new(struct lua_State *L)
{
	int top = lua_gettop(L);
	if (top == 0)
		return luaT_push_tuple_format(L, tuple_format_runtime);
	assert(top == 1 && lua_istable(L, 1));
	uint32_t count = lua_objlen(L, 1);
	struct region *region = &fiber()->gc;
	size_t region_svp = region_used(region);
	struct field_def *fields = xregion_alloc_array(region,
						       struct field_def, count);
	for (uint32_t i = 0; i < count; ++i) {
		size_t len;
		fields[i] = field_def_default;
		lua_pushinteger(L, i + 1);
		lua_gettable(L, 1);
		lua_pushstring(L, "name");
		lua_gettable(L, -2);
		assert(!lua_isnil(L, -1));
		const char *name = lua_tolstring(L, -1, &len);
		fields[i].name = (char *)xregion_alloc(region, len + 1);
		memcpy(fields[i].name, name, len);
		fields[i].name[len] = '\0';
		lua_pop(L, 1);
		lua_pop(L, 1);
	}
	struct tuple_dictionary *dict = tuple_dictionary_new(fields, count);
	region_truncate(region, region_svp);
	if (dict == NULL)
		return luaT_error(L);
	struct tuple_format *format = runtime_tuple_format_new(dict);
	/*
	 * Since dictionary reference counter is 1 from the
	 * beginning and after creation of the tuple_format
	 * increases by one, we must decrease it once.
	 */
	tuple_dictionary_unref(dict);
	if (format == NULL)
		return luaT_error(L);
	return luaT_push_tuple_format(L, format);
}

void
box_lua_tuple_format_init(struct lua_State *L)
{
	const struct luaL_Reg lbox_tuple_format_meta[] = {
		{"__gc", lbox_tuple_format_gc},
		{NULL, NULL}
	};
	luaL_register_type(L, tuple_format_typename, lbox_tuple_format_meta);

	const struct luaL_Reg box_tuple_formatlib_internal[] = {
		{"new", lbox_tuple_format_new},
		{NULL, NULL}
	};
	luaL_findtable(L, LUA_GLOBALSINDEX, "box.internal", 1);
	lua_createtable(L, 0, 1);
	luaL_setfuncs(L, box_tuple_formatlib_internal, 0);
	lua_setfield(L, -2, "tuple_format");
	lua_pop(L, 1);
}