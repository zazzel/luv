/*
 *  Copyright 2014 The Luvit Authors. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */
#include "luv.h"

static int luv_new_udp(lua_State* L) {
  uv_loop_t* loop = luv_check_loop(L, 1);
  uv_udp_t* handle = luv_create_udp(L);
  int ret = uv_udp_init(loop, handle);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }
  return 1;
}

static int luv_udp_open(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  uv_os_sock_t sock = luaL_checkinteger(L, 2);
  int ret = uv_udp_open(handle, sock);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_udp_bind(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  const char* host = luaL_checkstring(L, 2);
  int port = luaL_checkinteger(L, 3);
  unsigned int flags = 0;
  struct sockaddr_storage addr;
  int ret;
  if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
      uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
    return luaL_argerror(L, 2, "Invalid IP address or port");
  }
  if (lua_type(L, 4) == LUA_TTABLE) {
    luaL_checktype(L, 4, LUA_TTABLE);
    lua_getfield(L, 4, "reuseaddr");
    if (lua_toboolean(L, -1)) flags |= UV_UDP_REUSEADDR;
    lua_pop(L, 1);
    lua_getfield(L, 4, "ipv6only");
    if (lua_toboolean(L, -1)) flags |= UV_UDP_IPV6ONLY;
    lua_pop(L, 1);
  }
  ret = uv_udp_bind(handle, (struct sockaddr*)&addr, flags);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_udp_getsockname(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  struct sockaddr_storage address;
  int addrlen = sizeof(address);
  int ret = uv_udp_getsockname(handle, (struct sockaddr*)&address, &addrlen);
  if (ret < 0) return luv_error(L, ret);
  parse_sockaddr(L, &address, addrlen);
  return 1;
}

// These are the same order as uv_membership which also starts at 0
static const char *const udp_membership_opts[] = {
  "leave", "join", NULL
};

static int luv_udp_set_membership(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  const char* multicast_addr = luaL_checkstring(L, 2);
  const char* interface_addr = luaL_checkstring(L, 3);
  uv_membership membership = luaL_checkoption(L, 2, NULL, udp_membership_opts);
  int ret = uv_udp_set_membership(handle, multicast_addr, interface_addr, membership);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_udp_set_multicast_loop(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int on, ret;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  on = lua_toboolean(L, 2);
  ret = uv_udp_set_multicast_loop(handle, on);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_udp_set_multicast_ttl(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int ttl, ret;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  ttl = lua_toboolean(L, 2);
  ret = uv_udp_set_multicast_ttl(handle, ttl);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_udp_set_multicast_interface(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  const char* interface_addr = luaL_checkstring(L, 2);
  int ret = uv_udp_set_multicast_interface(handle, interface_addr);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_udp_set_broadcast(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int on, ret;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  on = lua_toboolean(L, 2);
  ret =uv_udp_set_broadcast(handle, on);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_udp_set_ttl(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int ttl, ret;
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  ttl = lua_toboolean(L, 2);
  ret = uv_udp_set_ttl(handle, ttl);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static void luv_udp_send_cb(uv_udp_send_t* req, int status) {
  lua_State* L = luv_find(req->data);
  lua_pop(L, 1);
  luv_resume_with_status(L, status, 0);
}

static int luv_udp_send(lua_State* L) {
  uv_udp_send_t* req = luv_check_udp_send(L, 1);
  uv_udp_t* handle = luv_check_udp(L, 2);
  uv_buf_t buf;
  int ret, port;
  const char* host;
  struct sockaddr_storage addr;
  buf.base = (char*) luaL_checklstring(L, 3, &buf.len);
  host = luaL_checkstring(L, 4);
  port = luaL_checkinteger(L, 5);
  if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
      uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
    return luaL_argerror(L, 4, "Invalid IP address or port");
  }
  ret = uv_udp_send(req, handle, &buf, 1, (struct sockaddr*)&addr, luv_udp_send_cb);
  return luv_wait(L, req->data, ret);
}

static int luv_udp_try_send(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  uv_buf_t buf;
  int ret, port;
  const char* host;
  struct sockaddr_storage addr;
  buf.base = (char*) luaL_checklstring(L, 2, &buf.len);
  host = luaL_checkstring(L, 3);
  port = luaL_checkinteger(L, 4);
  if (uv_ip4_addr(host, port, (struct sockaddr_in*)&addr) &&
      uv_ip6_addr(host, port, (struct sockaddr_in6*)&addr)) {
    return luaL_argerror(L, 3, "Invalid IP address or port");
  }
  ret = uv_udp_try_send(handle, &buf, 1, (struct sockaddr*)&addr);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static void luv_udp_recv_cb(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf, const struct sockaddr* addr, unsigned flags) {
  // self
  lua_State* L = luv_find(handle->data);

  // err
  if (nread < 0) {
    fprintf(stderr, "%s: %s\n", uv_err_name(nread), uv_strerror(nread));
    lua_pushstring(L, uv_err_name(nread));
  }
  else {
    lua_pushnil(L);
  }

  // data
  if (nread == 0) {
    if (addr) {
      lua_pushstring(L, "");
    }
    else {
      lua_pushnil(L);
    }
  }
  else if (nread > 0) {
    lua_pushlstring(L, buf->base, nread);
  }
  if (buf) free(buf->base);

  // address
  if (addr) {
    parse_sockaddr(L, (struct sockaddr_storage*)addr, sizeof *addr);
  }
  else {
    lua_pushnil(L);
  }

  // flags
  lua_newtable(L);
  if (flags & UV_UDP_PARTIAL) {
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "partial");
  }

  luv_emit_event(L, handle->data, "onrecv", 5);
}

static int luv_udp_recv_start(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int ret;
  luv_ref_state(handle->data, L);
  ret = uv_udp_recv_start(handle, luv_alloc_cb, luv_udp_recv_cb);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}

static int luv_udp_recv_stop(lua_State* L) {
  uv_udp_t* handle = luv_check_udp(L, 1);
  int ret = uv_udp_recv_stop(handle);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, ret);
  return 1;
}