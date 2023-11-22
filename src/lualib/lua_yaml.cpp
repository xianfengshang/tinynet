// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_yaml.h"
#include "lua_types.h"
#include "yaml.h"
#include "base/io_buffer_stream.h"
#include "util/string_utils.h"

static int lua_yaml_encode_value(lua_State* L, yaml_emitter_t* out);

static int lua_yaml_encode_seq(lua_State* L, yaml_emitter_t* out) {
    yaml_event_t event;
    if (!yaml_sequence_start_event_initialize(&event, NULL, (const yaml_char_t*)YAML_SEQ_TAG, 1, YAML_ANY_SEQUENCE_STYLE)) {
        yaml_event_delete(&event);
        return 0;
    }
    if (!yaml_emitter_emit(out, &event)) {
        yaml_event_delete(&event);
        return 0;
    }
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_yaml_encode_value(L, out);
        lua_pop(L, 1);
    }
    if (!yaml_sequence_end_event_initialize(&event)) {
        yaml_event_delete(&event);
        return 0;
    }
    if (!yaml_emitter_emit(out, &event)) {
        yaml_event_delete(&event);
        return 0;
    }
    return 1;
}

static int lua_yaml_encode_map(lua_State* L, yaml_emitter_t* out) {
    yaml_event_t event;
    if (!yaml_mapping_start_event_initialize(&event, NULL, (const yaml_char_t*)YAML_MAP_TAG, 1, YAML_ANY_MAPPING_STYLE)) {
        yaml_event_delete(&event);
        return 0;
    }
    if (!yaml_emitter_emit(out, &event)) {
        yaml_event_delete(&event);
        return 0;
    }
    lua_pushnil(L);
    while (lua_next(L, -2)) {
        lua_pushvalue(L, -2);
        lua_yaml_encode_value(L, out);
        lua_pop(L, 1);
        lua_yaml_encode_value(L, out);
        lua_pop(L, 1);
    }
    if (!yaml_mapping_end_event_initialize(&event)) {
        yaml_event_delete(&event);
        return 0;
    }
    if (!yaml_emitter_emit(out, &event)) {
        yaml_event_delete(&event);
        return 0;
    }
    return 1;
}

static int lua_yaml_encode_value(lua_State* L, yaml_emitter_t* out) {
    yaml_event_t event;
    int type = lua_type(L, -1);
    switch (type) {
    case LUA_TNIL: {
        if (!yaml_scalar_event_initialize(&event, NULL, (const yaml_char_t*)YAML_NULL_TAG, (const yaml_char_t*)"~", -1, 1, 1, YAML_PLAIN_SCALAR_STYLE)) {
            yaml_event_delete(&event);
            return 0;
        }
        if (!yaml_emitter_emit(out, &event)) {
            yaml_event_delete(&event);
            return 0;
        }
        break;
    }
    case LUA_TBOOLEAN: {
        int value = lua_toboolean(L, -1);
        if (!yaml_scalar_event_initialize(&event, NULL, (const yaml_char_t*)YAML_BOOL_TAG, (const yaml_char_t*)(value ? "true" : "false"), -1, 1, 1, YAML_PLAIN_SCALAR_STYLE)) {
            yaml_event_delete(&event);
            return 0;
        }
        if (!yaml_emitter_emit(out, &event)) {
            yaml_event_delete(&event);
            return 0;
        }
        break;
    }
    case LUA_TNUMBER:
    case LUA_TSTRING: {
        size_t len;
        const char* value = lua_tolstring(L, -1, &len);
        if (!yaml_scalar_event_initialize(&event, NULL, (const yaml_char_t*)YAML_STR_TAG, (const yaml_char_t*)value, (int)len, 1, 1, YAML_PLAIN_SCALAR_STYLE)) {
            yaml_event_delete(&event);
            return 0;
        }
        if (!yaml_emitter_emit(out, &event)) {
            yaml_event_delete(&event);
            return 0;
        }
        break;
    }

    case LUA_TTABLE: {
#if (LUA_VERSION_NUM >= 503)
        size_t len = lua_rawlen(L, -1);
#else
        size_t len = lua_objlen(L, -1);
#endif
        if (len > 0) {
            return lua_yaml_encode_seq(L, out);
        } else {
            return lua_yaml_encode_map(L, out);
        }
        break;
    }
    default:
        break;
    }
    return 1;
}

static int lua_yaml_writer(void *data, unsigned char *buffer, size_t size) {
    auto io_buf = static_cast<tinynet::IOBuffer*>(data);
    if (!io_buf) {
        return 0;
    }
    io_buf->append(buffer, size);
    return 1;
}

static int lua_yaml_encode(lua_State* L) {
    yaml_emitter_t out;
    yaml_event_t event;
    tinynet::IOBuffer io_buf;
    std::string msg;
    if (!yaml_emitter_initialize(&out)) {
        lua_pushnil(L);
        lua_pushstring(L, "yaml_emitter_initialize failed");
        return 2;
    }
    yaml_emitter_set_output(&out, lua_yaml_writer, &io_buf);

    if (!yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING)) {
        goto event_error;
    }
    if (!yaml_emitter_emit(&out, &event)) {
        goto emitter_error;
    }

    if (!yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1)) {
        goto event_error;
    }
    if (!yaml_emitter_emit(&out, &event)) {
        goto emitter_error;
    }

    if (!lua_yaml_encode_value(L, &out)) {
        goto emitter_error;
    }

    if (!yaml_document_end_event_initialize(&event, 1)) {
        goto event_error;
    }
    if (!yaml_emitter_emit(&out, &event)) {
        goto emitter_error;
    }

    if (!yaml_stream_end_event_initialize(&event)) {
        goto event_error;
    }
    if (!yaml_emitter_emit(&out, &event)) {
        goto emitter_error;
    }

    yaml_emitter_delete(&out);

    lua_pushlstring(L, io_buf.begin(), io_buf.size());
    return 1;

emitter_error:

    switch (out.error) {
    case YAML_MEMORY_ERROR:
        msg = "Memory error: Not enough memory for emitting";
        break;

    case YAML_WRITER_ERROR:
        StringUtils::Format(msg, "Writer error: %s", out.problem);
        break;

    case YAML_EMITTER_ERROR:
        StringUtils::Format(msg, "Emitter error: %s", out.problem);
        break;

    default:
        /* Couldn't happen. */
        msg = "Internal error";
        break;
    }

    yaml_event_delete(&event);
    yaml_emitter_delete(&out);
    lua_pushnil(L);
    lua_pushstring(L, msg.c_str());
    return 2;

event_error:

    yaml_event_delete(&event);
    yaml_emitter_delete(&out);
    lua_pushnil(L);
    lua_pushstring(L, "Memory error: Not enough memory for creating an event");
    return 2;
}

static void luaL_pushyamlnode(lua_State*L, yaml_document_t *doc, yaml_node_t *node);
static void luaL_pushyamlmappair(lua_State*L, yaml_document_t *doc, yaml_node_t *key, yaml_node_t* value);

static void luaL_pushyamlmap(lua_State* L, yaml_document_t* doc, yaml_node_t* node, bool merge) {
    yaml_node_pair_t    *pair;
    yaml_node_t         *key;
    yaml_node_t         *value;
    if (!merge) {
        lua_newtable(L);
    }
    for (pair = node->data.mapping.pairs.start; pair < node->data.mapping.pairs.top; pair++) {
        key = yaml_document_get_node(doc, pair->key);
        value = yaml_document_get_node(doc, pair->value);
        luaL_pushyamlmappair(L, doc, key, value);
    }
}

static void luaL_pushyamlmappair(lua_State*L, yaml_document_t *doc, yaml_node_t *key, yaml_node_t* value) {
    if (key->type == YAML_SCALAR_NODE
            && value->type == YAML_MAPPING_NODE
            && key->data.scalar.length == 2
            && key->data.scalar.value[0] == '<'
            && key->data.scalar.value[1] == '<') {
        luaL_pushyamlmap(L, doc, value, true);
    } else {
        luaL_pushyamlnode(L, doc, key);
        luaL_pushyamlnode(L, doc, value);
        lua_rawset(L, -3);
    }
}

static void luaL_pushyamlsequence(lua_State* L, yaml_document_t* doc, yaml_node_t* node) {
    yaml_node_item_t    *item;
    yaml_node_t         *value;
    int i;
    lua_newtable(L);
    for (item = node->data.sequence.items.start, i = 0; item < node->data.sequence.items.top; item++, i++) {
        value = yaml_document_get_node(doc, *item);
        luaL_pushyamlnode(L, doc, value);
        lua_rawseti(L, -2, i + 1);
    }
}

static void luaL_pushyamlscalar(lua_State* L, yaml_document_t* doc, yaml_node_t* node) {
    lua_pushlstring(L, (const char*)node->data.scalar.value, node->data.scalar.length);
}

static void luaL_pushyamlnode(lua_State*L, yaml_document_t *doc, yaml_node_t *node) {
    switch (node->type) {
    case YAML_NO_NODE: {
        lua_pushnil(L);
        break;
    }
    case YAML_SCALAR_NODE: {
        luaL_pushyamlscalar(L, doc, node);
        break;
    }
    case YAML_SEQUENCE_NODE: {
        luaL_pushyamlsequence(L, doc, node);
        break;
    }
    case YAML_MAPPING_NODE: {
        luaL_pushyamlmap(L, doc, node, false);
        break;
    }
    default:
        break;
    }
}

static int lua_yaml_decode(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    yaml_parser_t       parser;
    yaml_document_t     doc;
    yaml_node_t*			root;
    if (!yaml_parser_initialize(&parser)) {
        lua_pushnil(L);
        lua_pushstring(L, "yaml_parser_initialize failed");
        return 2;
    }
    yaml_parser_set_input_string(&parser, (const unsigned char*)data, len);
    if (!yaml_parser_load(&parser, &doc)) {
        yaml_parser_delete(&parser);
        lua_pushnil(L);
        lua_pushstring(L, "yaml_parser_load failed");
        return 2;
    }
    root = yaml_document_get_root_node(&doc);
    if (root == NULL) {
        yaml_parser_delete(&parser);
        yaml_document_delete(&doc);
        lua_pushnil(L);
        lua_pushstring(L, "yaml_document_get_root_node failed");
        return 2;
    }
    luaL_pushyamlnode(L, &doc, root);
    yaml_parser_delete(&parser);
    yaml_document_delete(&doc);
    return 1;
}

static const luaL_Reg yaml_methods[] = {
    {"encode", lua_yaml_encode},
    {"decode", lua_yaml_decode},
    {0, 0}
};


LUALIB_API int luaopen_yaml(lua_State *L) {
    luaL_newlib(L, yaml_methods);
    return 1;
}
