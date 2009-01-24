/*
 * liblcfg - lightweight configuration file library
 * Copyright (c) 2007--2009  Paul Baecher
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *    $Id$
 *
 */

#include <string.h>
#include <stdio.h>

#include <lcfgx/lcfgx_tree.h>

static struct lcfgx_tree_node *lcfgx_tree_node_new(enum lcfgx_type type, const char *key)
{
	struct lcfgx_tree_node *node = malloc(sizeof(struct lcfgx_tree_node));

	node->type = type;

	if( key != NULL )
		node->key = strdup(key);
	else
		node->key = NULL;

	node->next = NULL;

	return node;
}


void lcfgx_tree_dump(struct lcfgx_tree_node *node, int depth)
{
//	printf("%s node %p node->key %s depth %i\n", __PRETTY_FUNCTION__, node, node->key, depth);
	void sp(int n)
	{
		int i;

		for( i = 0; i < n; i++ )
			printf("%c", ' ');
	}

	sp(depth);
	if( node->key != NULL )
		printf("%s", node->key);
	else
		printf("%s", "(none)");

	struct lcfgx_tree_node *n;

	switch( node->type )
	{
		case lcfgx_string:
			printf(" = \"%s\"\n", (char *)node->value.string.data);
			break;

		case lcfgx_list:
		case lcfgx_map:
			printf("%c", '\n');
			n = node->value.elements;
			for( ; n != NULL; n = n->next )
				lcfgx_tree_dump(n, depth + 2);
			break;
	}
}

static void lcfgx_tree_insert(int pathc, char **pathv, void *data, size_t len, struct lcfgx_tree_node *node)
{
	struct lcfgx_tree_node *n;

	for( n = node->value.elements; n != NULL; n = n->next )
		if( !strcmp(pathv[0], n->key) )
			break;

	if( pathc == 1 )
	{
		/* leaf node --> string value */
		if( n == NULL )
		{
			/* not found, insert */
			n = lcfgx_tree_node_new(lcfgx_string, pathv[0]);
			n->value.string.len = len;
			n->value.string.data = malloc(len);
			memcpy(n->value.string.data, data, len);
			n->next = node->value.elements;
			node->value.elements = n;
		}
	}
	else
	{
		/* inner node --> (map/list) */
		if( n == NULL )
		{
			/* not found, insert it */
			n = lcfgx_tree_node_new(lcfgx_map, pathv[0]);
			n->value.elements = NULL;
			n->next = node->value.elements;
			node->value.elements = n;
		}

		/* recurse into map/list */
		lcfgx_tree_insert(pathc - 1, &pathv[1], data, len, n);
	}
}

enum lcfg_status lcfgx_tree_visitor(const char *key, void *data, size_t len, void *user_data)
{
	struct lcfgx_tree_node *root = user_data;
	char path[strlen(key) + 1];
	int path_components = 1;

	strncpy(path, key, strlen(key) + 1);

	while( *key != 0 )
		if( *key++ == '.' )
			path_components++;

	char *pathv[path_components];
	char *token;
	char *saveptr = NULL;
	int pathc = 0;

	while( (token = strtok_r(pathc == 0 ? path : NULL, ".", &saveptr)) != NULL )
		pathv[pathc++] = token;

	lcfgx_tree_insert(pathc, pathv, data, len, root);

	return lcfg_status_ok;
}



void lcfgx_correct_type(struct lcfgx_tree_node *node)
{
	if( node->type == lcfgx_string )
		return;

	struct lcfgx_tree_node *n = NULL;
	if( node->key == NULL )	/* root node */
		n = node;

	if( node->type == lcfgx_map || node->type == lcfgx_list )
		n = node->value.elements;

	/* child key is integer, we have a list */
	char *end;
	if( strtol(n->key, &end, 10) >= 0 && n->key != end )
		node->type = lcfgx_list;

	struct lcfgx_tree_node *it;
	for( it = n; it != NULL; it = it->next )
		lcfgx_correct_type(it);
}

struct lcfgx_tree_node *lcfgx_tree_new(struct lcfg *c)
{
	struct lcfgx_tree_node *root = lcfgx_tree_node_new(lcfgx_map, NULL);

	root->value.elements = NULL;

	lcfg_accept(c, lcfgx_tree_visitor, root);
	lcfgx_correct_type(root);
	return root;
}

void lcfgx_tree_delete(struct lcfgx_tree_node *n)
{

	if( n->type != lcfgx_string )
	{
		struct lcfgx_tree_node *m, *next;

		for( m = n->value.elements; m != NULL; )
		{
			next = m->next;
			lcfgx_tree_delete(m);
			m = next;
		}
	}
	else
	{
		free(n->value.string.data);
	}

	if( n->key != NULL )
		free(n->key);

	free(n);
}

const char *lcfgx_path_access_strings[] =
{
	"LCFGX_PATH_NOT_FOUND",
	"LCFGX_PATH_FOUND_WRONG_TYPE_BAD",
	"LCFGX_PATH_FOUND_TYPE_OK",
};

struct lcfgx_tree_node *cfg_get_recursive(struct lcfgx_tree_node *node, int pathc, char **pathv)
{
	struct lcfgx_tree_node *it;// = node;

	for( it = node->value.elements; it != NULL; it = it->next )
	{
		if( strcmp(pathv[0], it->key) == 0 )
			break;
	}

	if( it != NULL )
	{
		if( pathc == 1 )
			return it;
		else
			return cfg_get_recursive(it, pathc - 1, &pathv[1]);
	}
	else
		return NULL;
}


enum lcfgx_path_access lcfgx_get(struct lcfgx_tree_node *root, struct lcfgx_tree_node **n, const char *key, enum lcfgx_type type)
{
	char path[strlen(key) + 1];
	int path_components = 1;

	strncpy(path, key, strlen(key) + 1);

	while( *key != 0 )
		if( *key++ == '.' )
			path_components++;

	char *pathv[path_components];
	char *token;
	char *saveptr = NULL;
	int pathc = 0;

	while( (token = strtok_r(pathc == 0 ? path : NULL, ".", &saveptr)) != NULL )
		pathv[pathc++] = token;


	struct lcfgx_tree_node *node;

	if( pathc == 0 )
		node = root;
	else
		node = cfg_get_recursive(root, pathc, pathv);

	if( node == NULL )
		return LCFGX_PATH_NOT_FOUND;

	if( node->type != type )
		return LCFGX_PATH_FOUND_WRONG_TYPE_BAD;

	*n = node;

	return LCFGX_PATH_FOUND_TYPE_OK;
}

enum lcfgx_path_access lcfgx_get_list(struct lcfgx_tree_node *root, struct lcfgx_tree_node **n, const char *key)
{
	return lcfgx_get(root, n, key, lcfgx_list);
}

enum lcfgx_path_access lcfgx_get_map(struct lcfgx_tree_node *root, struct lcfgx_tree_node **n, const char *key)
{
	return lcfgx_get(root, n, key, lcfgx_map);
}

enum lcfgx_path_access lcfgx_get_string(struct lcfgx_tree_node *root, struct lcfgx_tree_node **n, const char *key)
{
	return lcfgx_get(root, n, key, lcfgx_string);
}

