/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* games-individual-groups.c: This class tracks server-saved and local 
 * user-defined groups of individuals as provided by folks. It makes the
 * expanded state of groups persistent in a configuration file so the last
 * state is retrievable by any game displaying groups.
 *
 * Copyright © 2012 Chandni Verma
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Authors: Chandni Verma <chandniverma2112@gmail.com>
 */

#include "config.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "games-ui-utils.h"
#include "games-individual-groups.h"

#define INDIVIDUAL_GROUPS_XML_FILENAME "individual-groups.xml"
#define INDIVIDUAL_GROUPS_DTD_FILENAME "games-individual-groups.dtd"

typedef struct {
	gchar    *name;
	gboolean  expanded;
} IndividualGroup;

static void             individual_groups_file_parse (const gchar  *filename);
static gboolean         individual_groups_file_save  (void);
static IndividualGroup *individual_group_new         (const gchar  *name,
                                          					  gboolean      expanded);
static void             individual_group_free        (IndividualGroup *group);

static GList           *groups = NULL;

void
games_individual_groups_get_all (void)
{
	gchar *dir;
	gchar *file_with_path;

	/* If already set up clean up first */
	if (groups) {
		g_list_foreach (groups, (GFunc)individual_group_free, NULL);
		g_list_free (groups);
		groups = NULL;
	}

	dir = g_build_filename (g_get_user_config_dir (), PACKAGE, NULL);
	file_with_path = g_build_filename (dir, INDIVIDUAL_GROUPS_XML_FILENAME, NULL);
	g_free (dir);

	if (g_file_test (file_with_path, G_FILE_TEST_EXISTS)) {
		individual_groups_file_parse (file_with_path);
	}

	g_free (file_with_path);
}

static void
individual_groups_file_parse (const gchar *filename)
{
	xmlParserCtxtPtr ctxt;
	xmlDocPtr        doc;
	xmlNodePtr       contacts;
	xmlNodePtr       account;
	xmlNodePtr       node;

	g_debug ("Attempting to parse file:'%s'...", filename);

	ctxt = xmlNewParserCtxt ();

	/* Parse and validate the file. */
	doc = xmlCtxtReadFile (ctxt, filename, NULL, 0);
	if (!doc) {
		g_warning ("Failed to parse file:'%s'", filename);
		xmlFreeParserCtxt (ctxt);
		return;
	}

	if (!games_xml_validate (doc, INDIVIDUAL_GROUPS_DTD_FILENAME)) {
		g_warning ("Failed to validate file:'%s'", filename);
		xmlFreeDoc (doc);
		xmlFreeParserCtxt (ctxt);
		return;
	}

	/* The root node, contacts. */
	contacts = xmlDocGetRootElement (doc);

	account = NULL;
	node = contacts->children;
	while (node) {
		if (strcmp ((gchar *) node->name, "account") == 0) {
			account = node;
			break;
		}
		node = node->next;
	}

	node = NULL;
	if (account) {
		node = account->children;
	}

	while (node) {
		if (strcmp ((gchar *) node->name, "group") == 0) {
			gchar        *name;
			gchar        *expanded_str;
			gboolean      expanded;
			IndividualGroup *individual_group;

			name = (gchar *) xmlGetProp (node, (const xmlChar *) "name");
			expanded_str = (gchar *) xmlGetProp (node, (const xmlChar *) "expanded");

			if (expanded_str && strcmp (expanded_str, "yes") == 0) {
				expanded = TRUE;
			} else {
				expanded = FALSE;
			}

			individual_group = individual_group_new (name, expanded);
			groups = g_list_append (groups, individual_group);

			xmlFree (name);
			xmlFree (expanded_str);
		}

		node = node->next;
	}

	g_debug ("Parsed %d contact groups", g_list_length (groups));

	xmlFreeDoc (doc);
	xmlFreeParserCtxt (ctxt);
}

static IndividualGroup *
individual_group_new (const gchar *name,
		   gboolean     expanded)
{
	IndividualGroup *group;

	group = g_new0 (IndividualGroup, 1);

	group->name = g_strdup (name);
	group->expanded = expanded;

	return group;
}

static void
individual_group_free (IndividualGroup *group)
{
	g_return_if_fail (group != NULL);

	g_free (group->name);

	g_free (group);
}

static gboolean
individual_groups_file_save (void)
{
	xmlDocPtr   doc;
	xmlNodePtr  root;
	xmlNodePtr  node;
	GList      *l;
	gchar      *dir;
	gchar      *file;

	dir = g_build_filename (g_get_user_config_dir (), PACKAGE_NAME, NULL);
	g_mkdir_with_parents (dir, S_IRUSR | S_IWUSR | S_IXUSR);
	file = g_build_filename (dir, INDIVIDUAL_GROUPS_XML_FILENAME, NULL);
	g_free (dir);

	doc = xmlNewDoc ((const xmlChar *) "1.0");
	root = xmlNewNode (NULL, (const xmlChar *) "individuals");
	xmlDocSetRootElement (doc, root);

	node = xmlNewChild (root, NULL, (const xmlChar *) "account", NULL);
	xmlNewProp (node, (const xmlChar *) "name", (const xmlChar *) "Default");

	for (l = groups; l; l = l->next) {
		IndividualGroup *cg;
		xmlNodePtr    subnode;

		cg = l->data;

		subnode = xmlNewChild (node, NULL, (const xmlChar *) "group", NULL);
		xmlNewProp (subnode, (const xmlChar *) "expanded", cg->expanded ?
				(const xmlChar *) "yes" : (const xmlChar *) "no");
		xmlNewProp (subnode, (const xmlChar *) "name", (const xmlChar *) cg->name);
	}

	/* Make sure the XML is indented properly */
	xmlIndentTreeOutput = 1;

	g_debug ("Saving file:'%s'", file);
	xmlSaveFormatFileEnc (file, doc, "utf-8", 1);
	xmlFreeDoc (doc);

	xmlMemoryDump ();

	g_free (file);

	return TRUE;
}

gboolean
games_individual_group_get_expanded (const gchar *group)
{
	GList    *l;
	gboolean  default_val = TRUE;

	g_return_val_if_fail (group != NULL, default_val);

	for (l = groups; l; l = l->next) {
		IndividualGroup *cg = l->data;

		if (!cg || !cg->name) {
			continue;
		}

		if (strcmp (cg->name, group) == 0) {
			return cg->expanded;
		}
	}

	return default_val;
}

void
games_individual_group_set_expanded (const gchar *group,
				   gboolean     expanded)
{
	GList        *l;
	IndividualGroup *cg;
	gboolean      changed = FALSE;

	g_return_if_fail (group != NULL);

	for (l = groups; l; l = l->next) {
		cg = l->data;

		if (!cg || !cg->name) {
			continue;
		}

		if (strcmp (cg->name, group) == 0) {
			cg->expanded = expanded;
			changed = TRUE;
			break;
		}
	}

	/* if here... we don't have an IndividualGroup for the group. */
	if (!changed) {
		cg = individual_group_new (group, expanded);
		groups = g_list_append (groups, cg);
	}

	individual_groups_file_save ();
}
