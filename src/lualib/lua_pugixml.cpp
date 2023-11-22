// Copyright (C), Xianfeng Shang.  All rights reserved.
// Author: Xianfeng Shang (shangxianfeng@outlook.com)
#include "lua_pugixml.h"
#include "pugixml.hpp"
#include "lua_types.h"
#include "base/io_buffer_stream.h"
#include <sstream>

class xml_writer_lua : public pugi::xml_writer {
  public:
    void write(const void* data, size_t size) override {
        buffer_.append(data, size);
    }
    const char* data() { return buffer_.begin(); }
    size_t size() { return buffer_.size(); }
  private:
    tinynet::IOBuffer buffer_;
};

#define PUGIXML_DOCUMENT_META_TABLE "pugixml_document_meta_table"
#define PUGIXML_NODE_META_TABLE "pugixml_node_meta_table"
#define PUGIXML_ATTRIBUTE_META_TABLE "pugixml_attribute_meta_table"

static pugi::xml_document* luaL_checkpugixml(lua_State *L, int idx) {
    return (pugi::xml_document *)luaL_checkudata(L, idx, PUGIXML_DOCUMENT_META_TABLE);
}

static pugi::xml_node_struct *luaL_checkxmlnode(lua_State *L, int idx) {
    pugi::xml_node_struct **ud = (pugi::xml_node_struct **)luaL_checkudata(L, idx, PUGIXML_NODE_META_TABLE);
    luaL_argcheck(L, *ud != NULL, idx, "Invalid pugixml node");
    return *ud;
}

static pugi::xml_attribute_struct* luaL_checkxmlattribute(lua_State *L, int idx) {
    pugi::xml_attribute_struct **ud = (pugi::xml_attribute_struct **)luaL_checkudata(L, idx, PUGIXML_ATTRIBUTE_META_TABLE);
    luaL_argcheck(L, *ud != NULL, idx, "Invalid pugixml attribute");
    return *ud;
}

static int luaL_pushxmlnode(lua_State *L, pugi::xml_node_struct *node) {
    if (!node) {
        lua_pushnil(L);
        return 1;
    }
    pugi::xml_node_struct **ud = (pugi::xml_node_struct **)lua_newuserdata(L, sizeof(pugi::xml_node_struct *));
    *ud = node;
    luaL_getmetatable(L, PUGIXML_NODE_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int luaL_pushxmlattribute(lua_State *L, pugi::xml_attribute_struct *attrib) {
    if (!attrib) {
        lua_pushnil(L);
        return 1;
    }
    pugi::xml_attribute_struct **ud = (pugi::xml_attribute_struct **)lua_newuserdata(L, sizeof(pugi::xml_attribute_struct *));
    *ud = attrib;
    luaL_getmetatable(L, PUGIXML_ATTRIBUTE_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_pugixml_new(lua_State *L) {
    pugi::xml_document *doc = (pugi::xml_document *)lua_newuserdata(L, sizeof(pugi::xml_document));
    new(doc) pugi::xml_document();
    luaL_getmetatable(L, PUGIXML_DOCUMENT_META_TABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int lua_pugixml_delete(lua_State *L) {
    pugi::xml_document *doc = luaL_checkpugixml(L, 1);
    doc->~xml_document();
    return 0;
}

static int lua_pugixml_reset(lua_State *L) {
    pugi::xml_document *doc = luaL_checkpugixml(L, 1);
    doc->reset();
    return 0;
}

static int lua_pugixml_load_file(lua_State *L) {
    pugi::xml_document *doc = luaL_checkpugixml(L, 1);
    const char *filename = luaL_checkstring(L, 2);
    auto result = doc->load_file(filename, pugi::parse_full);
    if (result.status == pugi::status_ok) {
        return 0;
    }
    lua_pushstring(L, result.description());
    return 1;
}

static int lua_pugixml_load_buffer(lua_State *L) {
    size_t len;
    pugi::xml_document *doc = luaL_checkpugixml(L, 1);
    const char *buf = luaL_checklstring(L, 2, &len);
    auto result = doc->load_buffer(buf, len, pugi::parse_full);
    if (result.status == pugi::status_ok) {
        return 0;
    }
    lua_pushstring(L, result.description());
    return 1;
}

static int lua_pugixml_document_element(lua_State *L) {
    pugi::xml_document *doc = luaL_checkpugixml(L, 1);
    luaL_pushxmlnode(L, doc->document_element().internal_object());
    return 1;
}

static int lua_pugixml_as_node(lua_State *L) {
    pugi::xml_document *doc = luaL_checkpugixml(L, 1);
    luaL_pushxmlnode(L, doc->internal_object());
    return 1;
}

static int lua_pugixml_save(lua_State *L) {
    pugi::xml_document *doc = luaL_checkpugixml(L, 1);
    xml_writer_lua writer;
    const char* indent =   luaL_optstring(L, 2, PUGIXML_TEXT("\t"));
    unsigned int flag = (unsigned int)luaL_optint(L, 3, pugi::format_default);
    pugi::xml_encoding encoding = (pugi::xml_encoding)luaL_optint(L, 4, pugi::encoding_auto);
    doc->save(writer, indent, flag, encoding);
    lua_pushlstring(L, writer.data(), writer.size());
    return 1;
}

static int lua_pugixml_save_stream(lua_State *L) {
    pugi::xml_document *doc = luaL_checkpugixml(L, 1);
    std::stringstream stream;
    const char* indent = luaL_optstring(L, 2, PUGIXML_TEXT("\t"));
    unsigned int flag = (unsigned int)luaL_optint(L, 3, pugi::format_default);
    pugi::xml_encoding encoding = (pugi::xml_encoding)luaL_optint(L, 4, pugi::encoding_auto);
    doc->save(stream, indent, flag, encoding);
    lua_pushstring(L, stream.str().c_str());
    return 1;
}

static int lua_pugixml_save_file(lua_State *L) {
    pugi::xml_document *doc = luaL_checkpugixml(L, 1);
    const char *path = luaL_checkstring(L, 2);
    const char* indent = luaL_optstring(L, 3, PUGIXML_TEXT("\t"));
    unsigned int flag = (unsigned int)luaL_optint(L, 4, pugi::format_default);
    pugi::xml_encoding encoding = (pugi::xml_encoding)luaL_optint(L, 5, pugi::encoding_auto);
    lua_pushboolean(L, doc->save_file(path, indent, flag, encoding));
    return 1;
}

static int lua_pugixml_node_type(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    lua_pushnumber(L, nodeobj.type());
    return 1;
}

static int lua_pugixml_node_name(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    lua_pushstring(L, nodeobj.name());
    return 1;
}

static int lua_pugixml_node_value(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    lua_pushstring(L, nodeobj.value());
    return 1;
}

static int lua_pugixml_node_first_attribute(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    luaL_pushxmlattribute(L, nodeobj.first_attribute().internal_object());
    return 1;
}

static int lua_pugixml_node_last_attribute(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    luaL_pushxmlattribute(L, nodeobj.last_attribute().internal_object());
    return 1;
}

static int lua_pugixml_node_first_child(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    luaL_pushxmlnode(L, nodeobj.first_child().internal_object());
    return 1;
}

static int lua_pugixml_node_last_child(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    luaL_pushxmlnode(L, nodeobj.last_child().internal_object());
    return 1;
}

static int lua_pugixml_node_next_sibling(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char* name = luaL_optstring(L, 2, NULL);
    if (name) {
        luaL_pushxmlnode(L, nodeobj.next_sibling(name).internal_object());
    } else {
        luaL_pushxmlnode(L, nodeobj.next_sibling().internal_object());
    }
    return 1;
}

static int lua_pugixml_node_previous_sibling(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char* name = luaL_optstring(L, 2, NULL);
    if (name) {
        luaL_pushxmlnode(L, nodeobj.previous_sibling(name).internal_object());
    } else {
        luaL_pushxmlnode(L, nodeobj.previous_sibling().internal_object());
    }
    return 1;
}

static int lua_pugixml_node_child(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char *name = luaL_checkstring(L, 2);
    luaL_pushxmlnode(L, nodeobj.child(name).internal_object());
    return 1;
}

static int lua_pugixml_node_has_child(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char *name = luaL_checkstring(L, 2);
    lua_pushboolean(L, nodeobj.child(name).internal_object() != nullptr);
    return 1;
}

static int lua_pugixml_node_attribute(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char *name = luaL_checkstring(L, 2);
    luaL_pushxmlattribute(L, nodeobj.attribute(name).internal_object());
    return 1;
}

static int lua_pugixml_node_child_value(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char *name = luaL_optstring(L, 2, NULL);
    if (name) {
        lua_pushstring(L, nodeobj.child_value(name));
    } else {
        lua_pushstring(L, nodeobj.child_value());
    }
    return 1;
}

static int lua_pugixml_node_set_name(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char *name = luaL_checkstring(L, 2);
    nodeobj.set_name(name);
    return 0;
}

static int lua_pugixml_node_set_value(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char *value = luaL_checkstring(L, 2);
    nodeobj.set_value(value);
    return 0;
}

static int lua_pugixml_node_append_attribute(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char *name = luaL_checkstring(L, 2);
    luaL_pushxmlattribute(L, nodeobj.append_attribute(name).internal_object());
    return 1;
}

static int lua_pugixml_node_prepend_attribute(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char *name = luaL_checkstring(L, 2);
    luaL_pushxmlattribute(L, nodeobj.prepend_attribute(name).internal_object());
    return 1;
}

static int lua_pugixml_node_append_child(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    int type = lua_type(L, 2);
    if (type == LUA_TSTRING) {
        const char *name = lua_tostring(L, 2);
        luaL_pushxmlnode(L, nodeobj.append_child(name).internal_object());
    } else if (type == LUA_TNUMBER) {
        auto node_type = (pugi::xml_node_type)lua_tointeger(L, 2);
        luaL_pushxmlnode(L, nodeobj.append_child(node_type).internal_object());
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int lua_pugixml_node_prepend_child(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    int type = lua_type(L, 2);
    if (type == LUA_TSTRING) {
        const char *name = lua_tostring(L, 2);
        luaL_pushxmlnode(L, nodeobj.prepend_child(name).internal_object());
    } else if (type == LUA_TNUMBER) {
        auto node_type = (pugi::xml_node_type)lua_tointeger(L, 2);
        luaL_pushxmlnode(L, nodeobj.prepend_child(node_type).internal_object());
    } else {
        lua_pushnil(L);
    }
    return 1;
}


static int lua_pugixml_node_remove_attribute(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char *name = luaL_checkstring(L, 2);
    lua_pushboolean(L, nodeobj.remove_attribute(name));
    return 1;
}

static int lua_pugixml_node_remove_child(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    const char *name = luaL_checkstring(L, 2);
    lua_pushboolean(L, nodeobj.remove_child(name));
    return 0;
}

static int lua_pugixml_node_find_child_by_attribute(lua_State *L) {
    pugi::xml_node_struct *node = luaL_checkxmlnode(L, 1);
    pugi::xml_node nodeobj(node);
    int top = lua_gettop(L);
    if (top == 3) {
        const char* attr_name = luaL_checkstring(L, 2);
        const char* attr_value = luaL_checkstring(L, 3);
        luaL_pushxmlnode(L, nodeobj.find_child_by_attribute(attr_name, attr_value).internal_object());
    } else if (top == 4) {
        const char* name = luaL_checkstring(L, 2);
        const char* attr_name = luaL_checkstring(L, 3);
        const char* attr_value = luaL_checkstring(L, 4);
        luaL_pushxmlnode(L, nodeobj.find_child_by_attribute(name, attr_name, attr_value).internal_object());
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int lua_pugixml_attribute_name(lua_State *L) {
    pugi::xml_attribute_struct *attrib = luaL_checkxmlattribute(L, 1);
    pugi::xml_attribute attribobj(attrib);
    lua_pushstring(L, attribobj.name());
    return 1;
}

static int lua_pugixml_attribute_value(lua_State *L) {
    pugi::xml_attribute_struct *attrib = luaL_checkxmlattribute(L, 1);
    pugi::xml_attribute attribobj(attrib);
    lua_pushstring(L, attribobj.value());
    return 1;
}

static int lua_pugixml_attribute_as_string(lua_State *L) {
    pugi::xml_attribute_struct *attrib = luaL_checkxmlattribute(L, 1);
    pugi::xml_attribute attribobj(attrib);
    lua_pushstring(L, attribobj.as_string());
    return 1;
}

static int lua_pugixml_attribute_as_int(lua_State *L) {
    pugi::xml_attribute_struct *attrib = luaL_checkxmlattribute(L, 1);
    pugi::xml_attribute attribobj(attrib);
    lua_pushnumber(L, attribobj.as_int());
    return 1;
}

static int lua_pugixml_attribute_as_double(lua_State *L) {
    pugi::xml_attribute_struct *attrib = luaL_checkxmlattribute(L, 1);
    pugi::xml_attribute attribobj(attrib);
    lua_pushnumber(L, attribobj.as_double());
    return 1;
}

static int lua_pugixml_attribute_as_float(lua_State *L) {
    pugi::xml_attribute_struct *attrib = luaL_checkxmlattribute(L, 1);
    pugi::xml_attribute attribobj(attrib);
    lua_pushnumber(L, attribobj.as_float());
    return 1;
}

static int lua_pugixml_attribute_as_bool(lua_State *L) {
    pugi::xml_attribute_struct *attrib = luaL_checkxmlattribute(L, 1);
    pugi::xml_attribute attribobj(attrib);
    lua_pushnumber(L, attribobj.as_bool());
    return 1;
}

static int lua_pugixml_attribute_set_value(lua_State *L) {
    pugi::xml_attribute_struct *attrib = luaL_checkxmlattribute(L, 1);
    pugi::xml_attribute attribobj(attrib);
    int type = lua_type(L, 2);
    switch (type) {
    case LUA_TSTRING: {
        const char* value = lua_tostring(L, 2);
        attribobj.set_value(value);
        break;
    }
    case LUA_TNUMBER: {
        lua_Number value = lua_tonumber(L, 2);
        attribobj.set_value(value);
        break;
    }
    case LUA_TBOOLEAN: {
        int value = lua_toboolean(L, 2);
        attribobj.set_value((bool)value);
        break;
    }
    default:
        break;
    }
    return 0;
}

static int lua_pugixml_attribute_next_attribute(lua_State *L) {
    pugi::xml_attribute_struct *attrib = luaL_checkxmlattribute(L, 1);
    pugi::xml_attribute attribobj(attrib);
    luaL_pushxmlattribute(L, attribobj.next_attribute().internal_object());
    return 1;
}

static int lua_pugixml_attribute_previous_attribute(lua_State *L) {
    pugi::xml_attribute_struct *attrib = luaL_checkxmlattribute(L, 1);
    pugi::xml_attribute attribobj(attrib);
    luaL_pushxmlattribute(L, attribobj.previous_attribute().internal_object());
    return 1;
}


static const luaL_Reg pugixml_attribute_meta_methods[] = {
    {"name",lua_pugixml_attribute_name},
    {"value",lua_pugixml_attribute_value},
    {"as_string",lua_pugixml_attribute_as_string},
    {"as_int",lua_pugixml_attribute_as_int},
    {"as_double",lua_pugixml_attribute_as_double},
    {"as_float",lua_pugixml_attribute_as_float},
    {"as_bool",lua_pugixml_attribute_as_bool},
    {"set_value",lua_pugixml_attribute_set_value},
    {"next_attribute",lua_pugixml_attribute_next_attribute},
    {"previous_attribute",lua_pugixml_attribute_previous_attribute},
    {0, 0}
};

static const luaL_Reg pugixml_node_meta_methods[] = {
    {"type", lua_pugixml_node_type},
    {"name", lua_pugixml_node_name},
    {"value", lua_pugixml_node_value},
    {"first_attribute", lua_pugixml_node_first_attribute},
    {"last_attribute", lua_pugixml_node_last_attribute},
    {"first_child", lua_pugixml_node_first_child},
    {"last_child", lua_pugixml_node_last_child},
    {"next_sibling", lua_pugixml_node_next_sibling},
    {"previous_sibling", lua_pugixml_node_previous_sibling},
    {"child", lua_pugixml_node_child},
    {"has_child", lua_pugixml_node_has_child},
    {"attribute", lua_pugixml_node_attribute},
    {"child_value", lua_pugixml_node_child_value},
    {"set_name", lua_pugixml_node_set_name},
    {"set_value", lua_pugixml_node_set_value},
    {"append_attribute", lua_pugixml_node_append_attribute},
    {"prepend_attribute", lua_pugixml_node_prepend_attribute},
    {"append_child", lua_pugixml_node_append_child},
    {"prepend_child", lua_pugixml_node_prepend_child},
    {"remove_attribute", lua_pugixml_node_remove_attribute},
    {"remove_child", lua_pugixml_node_remove_child},
    {"find_child_by_attribute", lua_pugixml_node_find_child_by_attribute},
    {0, 0}
};

static const luaL_Reg pugixml_meta_methos[] = {
    {"reset", lua_pugixml_reset},
    {"load_file", lua_pugixml_load_file},
    {"load_buffer", lua_pugixml_load_buffer},
    {"save", lua_pugixml_save},
    {"save_stream", lua_pugixml_save_stream},
    {"save_file", lua_pugixml_save_file},
    {"document_element", lua_pugixml_document_element},
    {"as_node", lua_pugixml_as_node},
    {"__gc", lua_pugixml_delete},
    {0, 0}
};

static const luaL_Reg pugixml_methods[] = {
    {"new", lua_pugixml_new},
    {0, 0}
};


LUALIB_API int luaopen_pugixml(lua_State *L) {
    using namespace pugi;
    luaL_newmetatable(L, PUGIXML_DOCUMENT_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, pugixml_meta_methos, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, PUGIXML_NODE_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, pugixml_node_meta_methods, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, PUGIXML_ATTRIBUTE_META_TABLE);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, pugixml_attribute_meta_methods, 0);
    lua_pop(L, 1);

    luaL_newlib(L, pugixml_methods);

    //push xml_node_type enum
    lua_pushstring(L, "xml_node_type");
    lua_newtable(L);
    LUA_WRITE_ENUM(node_null);
    LUA_WRITE_ENUM(node_document);
    LUA_WRITE_ENUM(node_element);
    LUA_WRITE_ENUM(node_pcdata);
    LUA_WRITE_ENUM(node_cdata);
    LUA_WRITE_ENUM(node_comment);
    LUA_WRITE_ENUM(node_pi);
    LUA_WRITE_ENUM(node_declaration);
    LUA_WRITE_ENUM(node_doctype);
    lua_rawset(L, -3);

    //push constant
    LUA_WRITE_CONST(format_indent);
    LUA_WRITE_CONST(format_write_bom);
    LUA_WRITE_CONST(format_raw);
    LUA_WRITE_CONST(format_no_declaration);
    LUA_WRITE_CONST(format_no_escapes);
    LUA_WRITE_CONST(format_save_file_text);

    //push xml_encoding enum
    lua_pushstring(L, "xml_encoding");
    lua_newtable(L);
    LUA_WRITE_ENUM(encoding_auto);
    LUA_WRITE_ENUM(encoding_utf8);
    LUA_WRITE_ENUM(encoding_utf16_le);
    LUA_WRITE_ENUM(encoding_utf16_be);
    LUA_WRITE_ENUM(encoding_utf16);
    LUA_WRITE_ENUM(encoding_utf32_le);
    LUA_WRITE_ENUM(encoding_utf32_be);
    LUA_WRITE_ENUM(encoding_utf32);
    LUA_WRITE_ENUM(encoding_wchar);
    LUA_WRITE_ENUM(encoding_latin1);
    lua_rawset(L, -3);
    return 1;
}
