#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define SEARCH_DEPTH_HINT 1024
#define BLOCK_WEIGHT_MAX  255

struct map_t {
	int width;
	int height;
	uint8_t data[0];
};

struct pathnode_t {
	int x;
	int y;
	int camefrom;
	int gscore;
	int fscore;
};

struct path_t {
	int depth;
	
	int *set;
	struct pathnode_t *pathnode;
};

struct route_coord_t {
	int x;
	int y;
};

struct route_queue_t {
	int head;
	int tail;
	int n;
	struct route_coord_t route_coord[0];
};

static inline int
map_set(struct map_t *m, int x, int y, int w) {
	int v;

	v = m->data[y * m->width + x];
	m->data[y * m->width + x] = w;
	return v;
}

static inline int
map_get(struct map_t *m, int x, int y) {
	if (x < 0 || x >= m->width || y < 0 || y >= m->height)
		return BLOCK_WEIGHT_MAX;

	return m->data[y * m->width + x];
}

static int
get_number(lua_State *L, const char *k) {
	int v;

	lua_getfield(L, -1, k);
	if (lua_type(L, -1) != LUA_TNUMBER) {
		return luaL_error(L, "invalid number -- %s", k);
	}

	v = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);
	return v;
}

static void
add_obstacle(lua_State *L, struct map_t *m, int line, const char *obstacle, size_t width) {
	int x, y, v;
	size_t i, weight;
	char c;

	y = line;
	if (y >= m->height) {
		luaL_error(L, "add obstacle (y = %d) fail", y);
	}

	for (i = 0; i < width; ++i) {
		x = i;
		if (x >= m->width) {
			luaL_error(L, "add obstacle (%d, %d) fail", x, y);
		}
		c = obstacle[i];
		if (c >= 'A' && c <= 'Z') {
			weight = (c - 'A' + 1);
			v = map_set(m, x, y, weight);
			if (v != 0) {
				luaL_error(L, "add obstacle (%d, %d) fail", x, y);
			}
		}
	}
}

static int
lnewmap(lua_State *L) {
	int width, height, i;
	size_t sz;
	const char *obstacle;
	struct map_t *m;

	luaL_checktype(L, 1, LUA_TTABLE);
	lua_settop(L, 1);
	width = get_number(L, "width");
	height = get_number(L, "height");
	m = lua_newuserdata(L, (size_t)(sizeof(struct map_t) + width * height * sizeof(m->data[0])));
	m->width = width;
	m->height = height;
	memset(m->data, 0, width * height * sizeof(m->data[0]));

	lua_getfield(L, 1, "obstacle");
	if (lua_type(L, -1) == LUA_TTABLE) {
		i = 1;
		lua_rawgeti(L, -1, i);
		while (lua_type(L, -1) == LUA_TSTRING) {
			obstacle = lua_tolstring(L, -1, &sz);
			add_obstacle(L, m, i - 1, obstacle, sz);
			lua_pop(L, 1);
			++i;
			lua_rawgeti(L, -1, i);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	return 1;
}

/*
    0  1  2
     \ | /
    7 - - 3
     / | \
    6  5  4
*/

static int
lweight(lua_State *L) {
	struct map_t *m;
	int x, y;

	luaL_checktype(L, 1, LUA_TUSERDATA);
	m = lua_touserdata(L, 1);
	x = (int)luaL_checkinteger(L, 2);
	y = (int)luaL_checkinteger(L, 3);
	if (x < 0 || x >= m->width ||
		y < 0 || y >= m->height) {
		luaL_error(L, "Position (%d,%d) is out of map", x, y);
	}
	lua_pushinteger(L, map_get(m, x, y));
	return 1;
}

static inline int
distance(int x1, int y1, int x2, int y2) {
	int dx, dy;

	dx = x1 - x2;
	dy = y1 - y2;
	if (dx < 0) {
		dx = -dx;
	}
	if (dy < 0) {
		dy = -dy;
	}
	if (dx < dy)
		return dx * 7 + (dy - dx) * 5;
	else
		return dy * 7 + (dx - dy) * 5;
}

struct context_t {
	struct path_t *P;
	int open;
	int closed;
	int n;
	int end_x;
	int end_y;
};

static struct pathnode_t *
add_open(struct context_t *ctx, int x, int y, int camefrom, int gscore) {
	struct path_t *P;
	struct pathnode_t *pn;

	P = ctx->P;
	if (ctx->n >= P->depth) {
		return NULL;
	}
	P->set[ctx->open++] = ctx->n;
	pn = &P->pathnode[ctx->n++];
	pn->x = x;
	pn->y = y;
	pn->camefrom = camefrom;
	pn->gscore = gscore;
	pn->fscore = gscore + distance(x, y, ctx->end_x, ctx->end_y);
	return pn;
};

static struct pathnode_t *
lowest_fscore(struct context_t *ctx) {
	int idx, fscore, i;
	struct pathnode_t *pn;

	idx = 0;
	pn = &ctx->P->pathnode[ctx->P->set[idx]];
	fscore = pn->fscore;
	for (i = 1; i < ctx->open; ++i) {
		struct pathnode_t *tmp = &ctx->P->pathnode[ctx->P->set[i]];
		if (tmp->fscore < fscore) {
			pn = tmp;
			idx = i;
			fscore = tmp->fscore;
		}
	}

	/* remove from open set */
	--ctx->open;
	if (idx != ctx->open) {
		ctx->P->set[idx] = ctx->P->set[ctx->open];
	}
	return pn;
}

static void
add_closed(struct context_t *ctx, int idx) {
	ctx->P->set[ctx->P->depth - 1 - ctx->closed++] = idx;
}

static int
in_closed(struct context_t *ctx, int x, int y) {
	int i, idx;
	struct pathnode_t *pn;

	for (i = 0; i < ctx->closed; ++i) {
		idx = ctx->P->set[ctx->P->depth - 1 - i];
		pn = &ctx->P->pathnode[idx];
		if (pn->x == x && pn->y == y)
			return 1;
	}
	return 0;
}

static struct pathnode_t *
find_open(struct context_t *ctx, int x, int y) {
	int i, idx;
	struct pathnode_t *pn;

	for (i = 0; i < ctx->open; ++i) {
		idx = ctx->P->set[i];
		pn = &ctx->P->pathnode[idx];
		if (pn->x == x && pn->y == y)
			return pn;
	}
	return NULL;
}

static int
nearest(struct path_t *P, int from, int to) {
	int ret, fscore, i;
	struct pathnode_t *pn;

	ret = P->set[from];
	pn = &P->pathnode[ret];
	fscore = pn->fscore;

	for (i = from + 1; i < to; ++i) {
		int idx = P->set[i];
		pn = &P->pathnode[idx];
		if (pn->fscore < fscore) {
			fscore = pn->fscore;
			ret = idx;
		}
	}
	return ret;
}

struct offset_t {
	int dx;
	int dy;
	int distance;
};

static struct offset_t OFFSET[8] = {
	{ -1, -1, 7 },	/* up-left */
	{ 0, -1, 5 },	/* up */
	{ 1, -1, 7 },	/* up-right */
	{ 1,  0, 5 },	/* right */
	{ 1,  1, 7 },	/* bottom-right */
	{ 0,  1, 5 },	/* bottom */
	{ -1, 1, 7 },	/* bottom-left */
	{ -1, 0, 5 },	/* left */
};

static int
path_finding(struct map_t *m, struct path_t *P, int start_x, int start_y, int end_x, int end_y) {
	struct context_t ctx;
	struct pathnode_t *pn, *neighbor;
	int current, i, x, y, weight, tentative_gscore;

	ctx.P = P;
	ctx.open = 0;
	ctx.closed = 0;
	ctx.n = 0;
	ctx.end_x = end_x;
	ctx.end_y = end_y;
	add_open(&ctx, start_x, start_y, -1, 0);
	while (ctx.open > 0) {
		pn = lowest_fscore(&ctx);
		current = pn - P->pathnode;
		if (pn->x == end_x && pn->y == end_y)
			return current;
		add_closed(&ctx, current);

		for (i = 0; i < 8; ++i) {
			x = pn->x + OFFSET[i].dx;
			y = pn->y + OFFSET[i].dy;
			weight = map_get(m, x, y);
			if (weight == BLOCK_WEIGHT_MAX)
				continue;
			if (in_closed(&ctx, x, y))
				continue;
			tentative_gscore = pn->gscore + OFFSET[i].distance + OFFSET[i].distance * weight;
			neighbor = find_open(&ctx, x, y);
			if (neighbor) {
				if (tentative_gscore < neighbor->gscore) {
					neighbor->camefrom = current;
					neighbor->gscore = tentative_gscore;
					neighbor->fscore = tentative_gscore + distance(x, y, end_x, end_y);
				}
			}
			else if (add_open(&ctx, x, y, current, tentative_gscore) == NULL) {
				break;
			}

		}
	}
	if (ctx.open > 0) {
		return nearest(P, 0, ctx.open);
	}
	else {
		return nearest(P, P->depth - ctx.closed, P->depth);
	}
}

static void
close_path(struct path_t *P) {
	if (P->depth > SEARCH_DEPTH_HINT) {
		free(P->set);
		free(P->pathnode);
	}
}

static void
check_position(lua_State *L, struct map_t *m, int x, int y) {
	if (x < 0 || x >= m->width ||
		y < 0 || y >= m->height) {
		luaL_error(L, "Invalid position (%d,%d)", x, y);
	}
}

struct pos_t {
	int x;
	int y;
};

static int
lpath(lua_State *L) {
	struct map_t *m;
	int start_x, start_y, end_x, end_y;
	struct path_t p;

	int set[SEARCH_DEPTH_HINT];
	struct pathnode_t pn[SEARCH_DEPTH_HINT];
	struct pos_t pos[SEARCH_DEPTH_HINT];

	int stack_size, node, n, idx, i;

	luaL_checktype(L, 1, LUA_TUSERDATA);
	m = lua_touserdata(L, 1);
	start_x = (int)luaL_checkinteger(L, 2);
	start_y = (int)luaL_checkinteger(L, 3);
	end_x = (int)luaL_checkinteger(L, 4);
	end_y = (int)luaL_checkinteger(L, 5);

	check_position(L, m, start_x, start_y);
	check_position(L, m, end_x, end_y);

	p.depth = (int)luaL_optinteger(L, 6, SEARCH_DEPTH_HINT);
	stack_size = p.depth > SEARCH_DEPTH_HINT ? 0 : p.depth;

	if (p.depth > SEARCH_DEPTH_HINT) {
		p.set = malloc(sizeof(int) * p.depth);
		p.pathnode = malloc(sizeof(struct pathnode_t) * p.depth);
	}
	else {
		p.set = set;
		p.pathnode = pn;
	}

	node = path_finding(m, &p, start_x, start_y, end_x, end_y);
	n = 1;
	idx = node;
	while (p.pathnode[idx].camefrom >= 0) {
		idx = p.pathnode[idx].camefrom;
		++n;
	}

	for (i = 0; i < n; ++i) {
		struct pathnode_t *pn = &p.pathnode[node];
		pos[i].x = pn->x;
		pos[i].y = pn->y;
		node = p.pathnode[node].camefrom;
	}

	close_path(&p);

	lua_settop(L, 0);
	luaL_checkstack(L, n * 2, NULL);
	for (i = n - 1;i >= 0;i--) {
		lua_pushinteger(L, pos[i].x);
		lua_pushinteger(L, pos[i].y);
	}
	return n * 2;
}

struct map_t *
new_flowgraph(lua_State *L, struct map_t *block, int index) {
	struct map_t * graph;

	if (lua_type(L, index) == LUA_TUSERDATA) {
		graph = lua_touserdata(L, index);
		if (block->width == graph->width && block->height == graph->height) {
			return graph;
		}
		lua_settop(L, index - 1);
	}
	graph = lua_newuserdata(L, sizeof(struct map_t) + block->width * block->height * sizeof(block->data[0]));
	graph->width = block->width;
	graph->height = block->height;
	memset(graph->data, 0, block->width * block->height * sizeof(graph->data[0]));
	return graph;
}

static void
add_target_mark(lua_State *L, struct map_t *m, const char *target, char mark) {
	int x, y, i;
	char c;

	x = y = 0;

	for (i = 0; (c = target[i]); ++i) {
		if (c == '\r')
			continue;
		else if (c == '\n') {
			++y;
			if (y >= m->height)
				break;
			x = 0;
			continue;
		}
		else if (c == mark && x < m->width) {
			m->data[y * m->width + x] = 1;
		}
		++x;
	}
}

static struct route_queue_t *
create_queue(int n) {
	struct route_queue_t *q;

	q = malloc(sizeof(struct route_queue_t) + sizeof(q->route_coord[0]) * n);
	q->head = 0;
	q->tail = 0;
	q->n = n;
	return q;
}

static void
enter_queue(struct route_queue_t *q, int x, int y) {
	struct route_coord_t *c;

	c = &q->route_coord[q->tail];
	c->x = x;
	c->y = y;
	++q->tail;
	if (q->tail >= q->n)
		q->tail = 0;
	assert(q->head != q->tail);
}

static struct route_coord_t *
leave_queue(struct route_queue_t *q) {
	struct route_coord_t *c;

	if (q->head == q->tail)
		return NULL;
	c = &q->route_coord[q->head];
	++q->head;
	if (q->head >= q->n)
		q->head = 0;
	return c;
}

static int
queue_exist(struct route_queue_t *q, int x, int y) {
	struct route_coord_t *c;
	int head;

	head = q->head;
	while (head != q->tail) {
		c = &q->route_coord[head];
		if (c->x == x && c->y == y)
			return 1;
		++head;
		if (head > q->n)
			head = 0;
	}
	return 0;
}

static void
init_route(struct map_t *block, struct map_t *graph, int *route, struct route_queue_t *q) {
	int width, height, i, j, wg, wb;

	width = graph->width;
	height = graph->height;

	for (i = 0; i < height; ++i) {
		for (j = 0; j < width; ++j) {
			wg = graph->data[i * width + j];
			wb = block->data[i * width + j];
			if (wg && wb != BLOCK_WEIGHT_MAX) {
				route[i * width + j] = wg + wb * 5;
				enter_queue(q, j, i);
			}
		}
	}
}

static void
gen_route(struct map_t *block, int *route, struct route_queue_t *q) {
	struct route_coord_t *c;
	int width, height, wb, odis, dis, i, x, y, w;

	c = NULL;
	width = block->width;
	height = block->height;

	while ((c = leave_queue(q))) {
		wb = block->data[c->y * width + c->x];
		if (wb == BLOCK_WEIGHT_MAX)
			continue;

		odis = route[c->y * width + c->x];
		for (i = 0; i < 8; ++i) {
			x = c->x + OFFSET[i].dx;
			y = c->y + OFFSET[i].dy;
			if (x >= 0 && x < width && y >= 0 && y < height) {
				dis = odis + OFFSET[i].distance + block->data[y * width + x] * 5;
				w = route[y * width + x];
				if (w == 0) {
					route[y * width + x] = dis;
					enter_queue(q, x, y);
				}
				else {
					if (w > dis) {
						route[y * width + x] = dis;
						if (!queue_exist(q, x, y)) {
							enter_queue(q, x, y);
						}
					}
				}
			}
		}
	}
}

static void
convert_route(int *route, struct map_t *graph) {
	int width = graph->width;
	int height = graph->height;
	int i, j, k, w, min_id, min_w, x, y, weight;

	for (i = 0; i < height; ++i) {
		for (j = 0; j < width; ++j) {
			w = route[i*width + j];
			min_id = 0;

			if (w > 1) {
				min_w = w + 7;
				for (k = 0; k < 8; ++k) {
					x = j + OFFSET[k].dx;
					y = i + OFFSET[k].dy;

					if (x >= 0 && x < width && y >= 0 && y < height) {
						weight = route[y*width + x] + OFFSET[k].distance;
						if (weight > 0 && weight < min_w) {
							min_w = weight;
							min_id = k + 1;
						}
					}
				}
			}
			graph->data[i*width + j] = min_id;
		}
	}
}

/*
userdata buildingmap
table target {
{ x = , y = , size = , radius = },
...
} or table target {
string map
string mark
}
userdata flowmap (optinal: graph)

return userdata flowmap
*/
static int
lflowgraph(lua_State *L) {
	struct map_t *block, *graph;
	int width, height, i;
	const char *target, *mark;
	int *route;
	struct route_queue_t *q;

	luaL_checktype(L, 1, LUA_TUSERDATA);
	block = lua_touserdata(L, 1);
	luaL_checktype(L, 2, LUA_TTABLE);
	graph = new_flowgraph(L, block, 3);
	width = block->width;
	height = block->height;

	i = 1;
	lua_rawgeti(L, 2, i);
	if (lua_type(L, -1) == LUA_TSTRING) {
		target = lua_tostring(L, -1);
		lua_pop(L, 1);	/* the string is in the table */

		lua_rawgeti(L, 2, i+1);
		if (lua_type(L, -1) != LUA_TSTRING) {
			return luaL_error(L, "Invalid target map");
		}
		mark = lua_tostring(L, -1);
		lua_pop(L, 1);

		add_target_mark(L, graph, target, mark[0]);
	}
	else {
		lua_pop(L, 1);
	}

	route = malloc(width * height * sizeof(int));
	memset(route, 0, width * height * sizeof(int));
	q = create_queue(width * height);
	init_route(block, graph, route, q);
	gen_route(block, route, q);
	convert_route(route, graph);
	free(route);
	free(q);

	return 1;
}

LUALIB_API int
luaopen_pathfinding(lua_State *L) {
	/*//luaL_checkversion(L); */
	luaL_Reg l[] = {
		{ "new", lnewmap },
		{ "weight", lweight },
		{ "path", lpath },
		{ "flowgraph", lflowgraph },
		{ NULL, NULL },
	};
#if LUA_VERSION_NUM < 502
	luaL_register(L, "pathfinding", l);
	return 1;
#else
	luaL_newlib(L, l);
	return 1;
#endif
}
