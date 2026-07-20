/*
 * extract_angular_routes.c — Angular Router navigation route extractor.
 *
 * Detects `Routes` arrays declared in TS/TSX/JS files and captures one
 * CBMNavRoute record per entry (path + component / loadChildren /
 * loadComponent / redirectTo), recursing into nested `children`. The
 * records are materialized later (in the definitions pipeline pass) as
 * `NavigationRoute` graph nodes plus `ROUTES_TO` / `REDIRECTS_TO` /
 * `LAZY_LOADS` edges — kept deliberately separate from the HTTP `Route`
 * namespace and the HTTP / cross_service matching machinery.
 *
 * Detection: a `lexical_declaration` / `variable_declaration` whose
 * declarator value is an array of route-shaped objects. A declarator is
 * considered route-shaped when it is annotated `: Routes` OR its first
 * array element is an object containing a route-defining key (`path`,
 * `children`, `redirectTo`, `loadChildren`, `loadComponent`). This
 * catches both typed (`const r: Routes = [...]`) and untyped forms
 * without relying on imported type names.
 *
 * Lazy loading: `loadChildren` / `loadComponent` are usually arrow
 * functions (`() => import("./mod")`, optionally `.then(m => m.X)`).
 * We statically resolve the first `import("...")` call inside the arrow
 * body and record its string argument; dynamic/conditional imports are
 * left unresolved (acceptable — those require real data-flow analysis).
 */
#include "cbm.h"
#include "arena.h"
#include "helpers.h"
#include "foundation/constants.h"
#include "extract_node_stack.h"
#include "tree_sitter/api.h"
#include <stdint.h>
#include <string.h>

enum {
    NAV_STACK_CAP = 8192,
    NAV_DEPTH_LIMIT = 16, /* nested `children` depth guard */
};

/* ── String helpers ──────────────────────────────────────────────── */

static const char *nav_unquote(CBMArena *a, const char *s) {
    if (!s) {
        return NULL;
    }
    size_t len = strlen(s);
    if (len < CBM_QUOTE_PAIR) {
        return NULL;
    }
    char first = s[0];
    char last = s[len - CBM_QUOTE_OFFSET];
    if ((first == '"' && last == '"') || (first == '\'' && last == '\'') ||
        (first == '`' && last == '`')) {
        return cbm_arena_strndup(a, s + CBM_QUOTE_OFFSET, len - CBM_QUOTE_PAIR);
    }
    return NULL;
}

/* Return the unquoted string content of a string-literal node, else NULL. */
static const char *nav_string_value(CBMExtractCtx *ctx, TSNode n) {
    const char *k = ts_node_type(n);
    if (strcmp(k, "string") != 0 && strcmp(k, "string_literal") != 0 &&
        strcmp(k, "template_string") != 0) {
        return NULL;
    }
    char *t = cbm_node_text(ctx->arena, n, ctx->source);
    if (!t) {
        return NULL;
    }
    return nav_unquote(ctx->arena, t);
}

/* ── Dynamic import() resolution ───────────────────────────────── */

/* Walk a subtree and return the unquoted argument of the first
 * `import("...")` call_expression found. Handles both the bare form
 * `import("./m")` and `.then(...)` chains by scanning the whole arrow
 * body. Returns NULL if no static import target is present. */
static const char *nav_find_import_target(CBMExtractCtx *ctx, TSNode root) {
    TSNodeStack stack;
    ts_nstack_init(&stack, ctx->arena, NAV_STACK_CAP);
    ts_nstack_push(&stack, ctx->arena, root);

    while (stack.count > 0) {
        TSNode node = ts_nstack_pop(&stack);
        const char *k = ts_node_type(node);
        if (strcmp(k, "call_expression") == 0 || strcmp(k, "import_expression") == 0) {
            TSNode func = ts_node_child_by_field_name(node, TS_FIELD("function"));
            bool is_import = false;
            if (!ts_node_is_null(func)) {
                char *ft = cbm_node_text(ctx->arena, func, ctx->source);
                is_import = ft && strcmp(ft, "import") == 0;
            } else if (strcmp(k, "import_expression") == 0) {
                /* Some grammars model `import(...)` as a dedicated node whose
                 * first named child is the argument. */
                is_import = true;
            }
            if (is_import) {
                TSNode args = ts_node_child_by_field_name(node, TS_FIELD("arguments"));
                TSNode arg = ts_node_is_null(args) ? (TSNode){0} : ts_node_named_child(args, 0);
                if (ts_node_is_null(arg) && strcmp(k, "import_expression") == 0) {
                    arg = ts_node_named_child(node, 0);
                }
                if (!ts_node_is_null(arg)) {
                    const char *target = nav_string_value(ctx, arg);
                    if (target) {
                        return target;
                    }
                }
            }
        }
        ts_nstack_push_children(&stack, ctx->arena, node);
    }
    return NULL;
}

/* ── Route object parsing ───────────────────────────────────────── */

/* True if `key_text` names a field that identifies a Routes entry. */
static bool nav_is_route_key(const char *key_text) {
    return key_text &&
           (strcmp(key_text, "path") == 0 || strcmp(key_text, "children") == 0 ||
            strcmp(key_text, "redirectTo") == 0 || strcmp(key_text, "loadChildren") == 0 ||
            strcmp(key_text, "loadComponent") == 0);
}

/* Does this object node look like a Routes entry? Checks for any
 * route-defining key among its `pair` children. */
static bool nav_object_is_route(CBMExtractCtx *ctx, TSNode obj) {
    if (strcmp(ts_node_type(obj), "object") != 0) {
        return false;
    }
    uint32_t n = ts_node_named_child_count(obj);
    for (uint32_t i = 0; i < n; i++) {
        TSNode pair = ts_node_named_child(obj, i);
        if (strcmp(ts_node_type(pair), "pair") != 0) {
            continue;
        }
        TSNode key = ts_node_child_by_field_name(pair, TS_FIELD("key"));
        if (ts_node_is_null(key)) {
            continue;
        }
        char *kt = cbm_node_text(ctx->arena, key, ctx->source);
        if (nav_is_route_key(kt)) {
            return true;
        }
    }
    (void)ctx;
    return false;
}

/* Join a parent route path with a child segment Angular-style (no
 * leading slash; empty segments collapse). Wildcards and params are
 * preserved verbatim. */
static const char *nav_join_path(CBMArena *a, const char *parent, const char *child) {
    if (!child || child[0] == '\0') {
        return parent ? parent : "";
    }
    if (!parent || parent[0] == '\0') {
        return cbm_arena_strdup(a, child);
    }
    size_t plen = strlen(parent);
    size_t clen = strlen(child);
    char *out = (char *)cbm_arena_alloc(a, plen + clen + 2);
    if (!out) {
        return parent;
    }
    memcpy(out, parent, plen);
    out[plen] = '/';
    memcpy(out + plen + 1, child, clen);
    out[plen + 1 + clen] = '\0';
    return out;
}

/* Parse a single route object into CBMNavRoute fields and push it,
 * then recurse into `children`. `parent_path` is the canonical path of
 * the enclosing route ("" at the top level). */
static void nav_parse_route_object(CBMExtractCtx *ctx, TSNode obj, const char *parent_path,
                                   const char *const_name, int depth) {
    if (depth > NAV_DEPTH_LIMIT || strcmp(ts_node_type(obj), "object") != 0) {
        return;
    }
    const char *path = NULL;
    const char *component = NULL;
    const char *load_children = NULL;
    const char *load_component = NULL;
    const char *redirect_to = NULL;
    TSNode children_node = {0};
    bool have_children = false;

    uint32_t n = ts_node_named_child_count(obj);
    for (uint32_t i = 0; i < n; i++) {
        TSNode pair = ts_node_named_child(obj, i);
        if (strcmp(ts_node_type(pair), "pair") != 0) {
            continue;
        }
        TSNode key = ts_node_child_by_field_name(pair, TS_FIELD("key"));
        TSNode value = ts_node_child_by_field_name(pair, TS_FIELD("value"));
        if (ts_node_is_null(key) || ts_node_is_null(value)) {
            continue;
        }
        char *kt = cbm_node_text(ctx->arena, key, ctx->source);
        if (!kt) {
            continue;
        }
        if (strcmp(kt, "path") == 0) {
            path = nav_string_value(ctx, value);
            if (!path) {
                /* `path` may be a bare identifier or template; fall back to
                 * the raw text so the route is still recorded. */
                char *raw = cbm_node_text(ctx->arena, value, ctx->source);
                path = raw ? cbm_arena_strdup(ctx->arena, raw) : NULL;
            }
        } else if (strcmp(kt, "component") == 0) {
            if (strcmp(ts_node_type(value), "identifier") == 0) {
                char *nm = cbm_node_text(ctx->arena, value, ctx->source);
                component = nm;
            }
        } else if (strcmp(kt, "loadChildren") == 0) {
            load_children = nav_find_import_target(ctx, value);
        } else if (strcmp(kt, "loadComponent") == 0) {
            load_component = nav_find_import_target(ctx, value);
        } else if (strcmp(kt, "redirectTo") == 0) {
            redirect_to = nav_string_value(ctx, value);
        } else if (strcmp(kt, "children") == 0) {
            children_node = value;
            have_children = true;
        }
    }

    const char *joined = nav_join_path(ctx->arena, parent_path, path ? path : "");
    TSPoint start = ts_node_start_point(obj);

    /* Only emit a record if the entry carries at least one routing
     * signal — a path alone with children is still meaningful (pathless
     * grouping routes), but a bare `{}` should not produce a node. */
    bool any = (path && path[0]) || component || load_children || load_component || redirect_to ||
               have_children;
    if (any) {
        CBMNavRoute nr = {
            .path = joined,
            .component = component,
            .load_children_module = load_children,
            .load_component_module = load_component,
            .redirect_to = redirect_to,
            .const_name = const_name,
            .file_path = ctx->rel_path,
            .start_line = start.row + 1,
        };
        cbm_nav_routes_push(&ctx->result->nav_routes, ctx->arena, nr);
    }

    if (have_children && !ts_node_is_null(children_node) &&
        strcmp(ts_node_type(children_node), "array") == 0) {
        uint32_t cn = ts_node_named_child_count(children_node);
        for (uint32_t i = 0; i < cn; i++) {
            TSNode child_obj = ts_node_named_child(children_node, i);
            if (strcmp(ts_node_type(child_obj), "object") == 0) {
                nav_parse_route_object(ctx, child_obj, joined, const_name, depth + 1);
            }
        }
    }
}

/* ── Routes-array detection ─────────────────────────────────────── */

/* Read the type annotation of a declarator (`: Routes`) and return
 * true when it resolves to the identifier `Routes`. */
static bool nav_declarator_is_routes_typed(CBMExtractCtx *ctx, TSNode declarator) {
    TSNode type_node = ts_node_child_by_field_name(declarator, TS_FIELD("type"));
    if (ts_node_is_null(type_node)) {
        return false;
    }
    /* type_annotation wraps the actual type; descend one level if present. */
    if (strcmp(ts_node_type(type_node), "type_annotation") == 0) {
        TSNode inner = ts_node_named_child(type_node, 0);
        if (ts_node_is_null(inner)) {
            return false;
        }
        type_node = inner;
    }
    if (strcmp(ts_node_type(type_node), "type_identifier") != 0 &&
        strcmp(ts_node_type(type_node), "generic_type") != 0) {
        return false;
    }
    char *tname = cbm_node_text(ctx->arena, type_node, ctx->source);
    return tname && strcmp(tname, "Routes") == 0;
}

/* Is `value` an array whose first element is a route-shaped object? */
static bool nav_value_is_routes_array(CBMExtractCtx *ctx, TSNode value) {
    if (ts_node_is_null(value) || strcmp(ts_node_type(value), "array") != 0) {
        return false;
    }
    uint32_t n = ts_node_named_child_count(value);
    for (uint32_t i = 0; i < n; i++) {
        TSNode elem = ts_node_named_child(value, i);
        if (nav_object_is_route(ctx, elem)) {
            return true;
        }
    }
    return false;
}

/* Process one declarator: if it holds a Routes array, parse every
 * route object it contains. */
static void nav_process_declarator(CBMExtractCtx *ctx, TSNode declarator) {
    TSNode name_node = ts_node_child_by_field_name(declarator, TS_FIELD("name"));
    TSNode value = ts_node_child_by_field_name(declarator, TS_FIELD("value"));
    if (ts_node_is_null(name_node) || ts_node_is_null(value)) {
        return;
    }
    char *const_name = cbm_node_text(ctx->arena, name_node, ctx->source);
    if (!const_name) {
        return;
    }

    bool typed = nav_declarator_is_routes_typed(ctx, declarator);
    bool shaped = nav_value_is_routes_array(ctx, value);
    if (!typed && !shaped) {
        return;
    }
    /* When the array is empty but typed Routes, there is nothing to emit. */
    if (strcmp(ts_node_type(value), "array") != 0) {
        return;
    }

    uint32_t n = ts_node_named_child_count(value);
    for (uint32_t i = 0; i < n; i++) {
        TSNode elem = ts_node_named_child(value, i);
        if (strcmp(ts_node_type(elem), "object") == 0 && nav_object_is_route(ctx, elem)) {
            nav_parse_route_object(ctx, elem, "", const_name, 0);
        }
    }
}

/* ── AST walk ───────────────────────────────────────────────────── */

static void nav_walk(CBMExtractCtx *ctx) {
    TSNodeStack stack;
    ts_nstack_init(&stack, ctx->arena, NAV_STACK_CAP);
    ts_nstack_push(&stack, ctx->arena, ctx->root);

    while (stack.count > 0) {
        TSNode node = ts_nstack_pop(&stack);
        const char *k = ts_node_type(node);
        if (strcmp(k, "lexical_declaration") == 0 || strcmp(k, "variable_declaration") == 0) {
            uint32_t n = ts_node_named_child_count(node);
            for (uint32_t i = 0; i < n; i++) {
                TSNode child = ts_node_named_child(node, i);
                const char *ck = ts_node_type(child);
                if (strcmp(ck, "variable_declarator") == 0) {
                    nav_process_declarator(ctx, child);
                }
            }
        }
        ts_nstack_push_children(&stack, ctx->arena, node);
    }
}

/* ── Entry point ────────────────────────────────────────────────── */

void cbm_extract_angular_routes(CBMExtractCtx *ctx) {
    if (!ctx) {
        return;
    }
    switch (ctx->language) {
    case CBM_LANG_JAVASCRIPT:
    case CBM_LANG_TYPESCRIPT:
    case CBM_LANG_TSX:
        nav_walk(ctx);
        break;
    default:
        break; /* Angular Router is TS-only; no extraction for other langs. */
    }
}
