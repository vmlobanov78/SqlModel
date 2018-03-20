#include <string.h>
#include <stdlib.h>

#include <glib-object.h>
#include <gtk/gtk.h>

#include "Sql.h"
#include "SqlStore.h"
#include "SqlStore-private.h"

static GObjectClass *parent_class = NULL;

static void
g_ptr_array_free_full (gpointer data)
{
	g_ptr_array_free ((GPtrArray*)data, TRUE);
}

static gint
g_int_cmp (gpointer x, gpointer y)
{
	gint x_int, y_int;

	x_int = GPOINTER_TO_INT (x);
	y_int = GPOINTER_TO_INT (y);

	if (x > y) return 1;
	else if (x < y) return -1;
	else return 0;
}

GType
sql_store_get_type (void)
{
	static GType my_type = 0;
	if (!my_type) {
		static const GTypeInfo my_info = {
			sizeof(SqlStoreClass),
			NULL,										 /* base init	  */
			NULL,										 /* base finalize  */
			(GClassInitFunc) sql_store_class_init,
			NULL,										 /* class finalize */
			NULL,										 /* class data	 */
			sizeof (SqlStore),
			1,											/* n_preallocs	*/
			(GInstanceInitFunc) sql_store_init,
			NULL
		};
		my_type = g_type_register_static (G_TYPE_OBJECT, "SqlStore",
										  &my_info, (GTypeFlags) 0);

		static const GInterfaceInfo tree_model_info = {
			(GInterfaceInitFunc) sql_store_tree_model_init,
			NULL, NULL};
		g_type_add_interface_static (my_type, GTK_TYPE_TREE_MODEL,
									 &tree_model_info);
	}
	return my_type;
}

static void
sql_store_class_init (SqlStoreClass *klass)
{
	GObjectClass *gobject_class;
	gobject_class = (GObjectClass*) klass;

	parent_class = (_GObjectClass*) g_type_class_peek_parent (klass);
	gobject_class->finalize = sql_store_finalize;

	g_type_class_add_private (gobject_class, sizeof (SqlStorePrivate));
}

static void
sql_store_tree_model_init (GtkTreeModelIface *iface)
{
	iface->get_flags	   = sql_store_get_flags;
	iface->get_n_columns   = sql_store_get_n_columns;
	iface->get_column_type = sql_store_get_column_type;
	iface->get_iter        = sql_store_get_iter;
	iface->get_path        = sql_store_get_path;
	iface->get_value	   = sql_store_get_value;
	iface->iter_next	   = sql_store_iter_next;
	iface->iter_children   = sql_store_iter_children;
	iface->iter_has_child  = sql_store_iter_has_child;
	iface->iter_n_children = sql_store_iter_n_children;
	iface->iter_nth_child  = sql_store_iter_nth_child;
	iface->iter_parent     = sql_store_iter_parent;
}

static void
sql_store_init (SqlStore *self)
{
	SqlStorePrivate *priv;

	self->n_columns = 0;
	self->stamp = g_random_int ();

	priv = SQL_STORE_GET_PRIVATE (self);
	g_assert (priv);

	priv->cache = g_tree_new_full ((GCompareDataFunc) strcmp,
								   NULL, NULL, g_ptr_array_free_full);
	g_assert (priv->cache);

	priv->rcache = g_tree_new ((GCompareFunc) g_int_cmp);
	g_assert (priv->rcache);
}

static void
sql_store_finalize (GObject *self)
{
	SqlStorePrivate *priv;

	g_return_if_fail (IS_SQL_STORE (self));

	priv = SQL_STORE_GET_PRIVATE (self);
	g_assert (priv);

	if (priv->dbh)
		sqlite3_close (priv->dbh);

	if (priv->table)
		g_free (priv->table);

	if (priv->cache)
		g_tree_destroy (priv->cache);

	G_OBJECT_CLASS (parent_class)->finalize (self);
}

static GtkTreeModelFlags
sql_store_get_flags (GtkTreeModel *tree_model)
{
	g_return_val_if_fail (IS_SQL_STORE (tree_model),
						  (GtkTreeModelFlags) 0);

	return (GtkTreeModelFlags)(GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint
sql_store_get_n_columns (GtkTreeModel *tree_model)
{
	g_return_val_if_fail (IS_SQL_STORE (tree_model), 0);
	return SQL_STORE (tree_model)->n_columns;
}

static GType
sql_store_get_column_type (GtkTreeModel *tree_model,
								  gint		  index)
{
	g_return_val_if_fail (IS_SQL_STORE (tree_model),
						  G_TYPE_INVALID);
	g_return_val_if_fail (index < SQL_STORE (tree_model)->n_columns
						  && index >= 0, G_TYPE_INVALID);
	return G_TYPE_STRING;
}

static gboolean
sql_store_get_iter (GtkTreeModel *tree_model,
						   GtkTreeIter  *iter,
						   GtkTreePath  *path)
{
	SqlStore		*self;
	SqlStorePrivate *priv;
	GPtrArray				*data;
	gint				  *indices, depth;

	g_assert (IS_SQL_STORE (tree_model));
	g_assert (path != NULL);

	self = SQL_STORE (tree_model);
	priv = SQL_STORE_GET_PRIVATE (self);
	g_assert (priv);

	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);

	g_assert (depth == 1);

	/* lookup the row position b-tree cache */
	data = (GPtrArray*)g_tree_lookup (priv->rcache, GINT_TO_POINTER (indices[0]));

	if (!data) {
		data = sql_fetch_nth_row (priv->dbh, priv->table, indices[0]);
	}

	/* DON'T FREE THE KEY! */
	gchar *key = (gchar*) g_ptr_array_index (data, 0);

	if (data && key) {
		if (g_tree_lookup (priv->cache, key) == NULL)
			g_tree_insert (priv->cache, key, data);
		if (g_tree_lookup (priv->rcache, GINT_TO_POINTER (indices[0])) == NULL)
			g_tree_insert (priv->rcache, GINT_TO_POINTER (indices[0]), data);
	}

	if (!data || data->len < 1)
		return FALSE;

	iter->stamp = self->stamp;
	iter->user_data = key;
	iter->user_data2 = NULL;
	iter->user_data3 = NULL;

	return TRUE;
}


static GtkTreePath*
sql_store_get_path (GtkTreeModel *tree_model,
						   GtkTreeIter  *iter)
{
	SqlStore        *self;
	SqlStorePrivate *priv;
	GtkTreePath           *path;
	gint				   pos;

	g_return_val_if_fail (IS_SQL_STORE (tree_model), NULL);
	g_return_val_if_fail (iter != NULL, NULL);
	g_return_val_if_fail (iter->user_data != NULL, NULL);

	self = SQL_STORE (tree_model);
	priv = SQL_STORE_GET_PRIVATE (self);
	g_assert (priv);

	pos = sql_fetch_row_pos (priv->dbh, priv->table, (gchar*)iter->user_data);

	path = gtk_tree_path_new ();
	gtk_tree_path_append_index (path, pos);

	return path;
}

static void
sql_store_get_value (GtkTreeModel *tree_model,
							GtkTreeIter  *iter,
							gint		  column,
							GValue	   *value)
{
	SqlStore		*self;
	SqlStorePrivate *priv;
	GPtrArray				*data;

	g_return_if_fail (IS_SQL_STORE (tree_model));
	g_return_if_fail (iter != NULL || iter->user_data != NULL);
	g_return_if_fail (column < SQL_STORE (tree_model)->n_columns);

	self = SQL_STORE (tree_model);
	priv = SQL_STORE_GET_PRIVATE (self);
	g_assert (priv);

	g_value_init (value, G_TYPE_STRING);

	data = (GPtrArray*)g_tree_lookup (priv->cache, iter->user_data);

	if (!data) {
		data = (GPtrArray*) sql_fetch_row (priv->dbh, priv->table, (gchar*)iter->user_data);

		/* Cache the results if they were retrieved */
		if (data) g_tree_insert (priv->cache, iter->user_data, data);
	}

	if (data && column < data->len)
		g_value_set_string (value, (const gchar*)g_ptr_array_index (data, column));
}

static gboolean
sql_store_iter_next (GtkTreeModel *tree_model,
							GtkTreeIter  *iter)
{
	SqlStore		  *self;
	SqlStorePrivate *priv;
	GPtrArray             *data = NULL;

	g_return_val_if_fail (IS_SQL_STORE (tree_model), FALSE);

	if (iter == NULL || iter->user_data == NULL)
	  return FALSE;

	self = SQL_STORE (tree_model);
	priv = SQL_STORE_GET_PRIVATE (self);

	/* if a oid is set in iter->user_data, lets convert it to an integer
	 * and increment it by one. if that value is found in the cache, we
	 * use it instead of doing a sqlite query. Note that this still isn't
	 * a good way to do this since cache would never hit if a row was
	 * removed from the table.
	 */
	if (iter->user_data) {
		gchar *key = g_strdup_printf ("%d", atoi ((const gchar*)(iter->user_data)) + 1);
		data = (GPtrArray*)g_tree_lookup (priv->cache, key);
		g_free (key);
	}

	if (!data) {
		data = sql_fetch_next (priv->dbh, priv->table, (gchar*)iter->user_data);
		if (data != NULL && data->len > 0) {
			g_tree_insert (priv->cache, g_ptr_array_index (data, 0), data);
		} else return FALSE;
	}

	iter->user_data = g_ptr_array_index (data, 0);
	iter->user_data2 = NULL;
	iter->user_data3 = NULL;

	return TRUE;
}

static gboolean
sql_store_iter_children (GtkTreeModel *tree_model,
								GtkTreeIter  *iter,
								GtkTreeIter  *parent)
{
	SqlStore		*self;
	SqlStorePrivate *priv;
	GPtrArray				*data;
	gchar				 *key;

	g_return_val_if_fail (IS_SQL_STORE (tree_model), FALSE);

	self = SQL_STORE (tree_model);
	priv = SQL_STORE_GET_PRIVATE (self);
	g_assert (priv);

	key = g_strdup ("1");
	data = (GPtrArray*)g_tree_lookup (priv->cache, key);
	g_free (key);

	if (!data) {
		data = sql_fetch_next (priv->dbh, priv->table, NULL);
		if (data) {
			g_tree_insert (priv->cache, g_ptr_array_index (data, 0), data);
		}
	}

	if (data) {
		iter->stamp = self->stamp;
		iter->user_data = g_ptr_array_index (data, 0);
		iter->user_data2 = NULL;
		iter->user_data3 = NULL;
		return TRUE;
	}

	return FALSE;
}

static gboolean
sql_store_iter_has_child (GtkTreeModel *tree_model,
							GtkTreeIter  *iter)
{
	return FALSE;
}

static gint
sql_store_iter_n_children (GtkTreeModel *tree_model,
								  GtkTreeIter  *iter)
{
	SqlStore		*self;
	SqlStorePrivate *priv;

	g_return_val_if_fail (IS_SQL_STORE (tree_model), -1);
	g_return_val_if_fail (iter == NULL || iter->user_data != NULL, -1);

	self = SQL_STORE (tree_model);
	priv = SQL_STORE_GET_PRIVATE (self);
	g_assert (priv);

	if (!iter) {
		return sql_count_rows (priv->dbh, priv->table);
	}

	return 0;
}

static gboolean
sql_store_iter_nth_child (GtkTreeModel *tree_model,
								 GtkTreeIter  *iter,
								 GtkTreeIter  *parent,
								 gint		  n)
{
	SqlStore		*self;
	SqlStorePrivate *priv;
	GPtrArray				*data;

	g_return_val_if_fail (IS_SQL_STORE (tree_model), FALSE);

	self = SQL_STORE (tree_model);
	priv = SQL_STORE_GET_PRIVATE (self);
	g_assert (priv);

	if (parent)
		return FALSE;

	iter->stamp = self->stamp;
	iter->user_data = NULL;
	iter->user_data2 = NULL;
	iter->user_data3 = NULL;

	data = sql_fetch_nth_row (priv->dbh, priv->table, n);

	if (data && !g_tree_lookup (priv->cache, g_ptr_array_index (data, 0)))
		g_tree_insert (priv->cache, g_ptr_array_index (data, 0), data);

	if (data) {
		iter->user_data = g_ptr_array_index (data, 0);
		return TRUE;
	}

	return FALSE;
}

static gboolean
sql_store_iter_parent (GtkTreeModel *tree_model,
							  GtkTreeIter  *iter,
							  GtkTreeIter  *child)
{
	return FALSE;
}

GtkTreeModel*
sql_store_new (void)
{
	return GTK_TREE_MODEL (g_object_new (TYPE_SQL_STORE, NULL));
}

void
sql_store_set_filename (SqlStore  *self,
							   const gchar	 *filename,
							   GError		 **error)
{
	SqlStorePrivate *priv;

	g_return_if_fail (IS_SQL_STORE (self));

	priv = SQL_STORE_GET_PRIVATE (self);
	g_assert (priv);

	if (priv->dbh != NULL)
	{
		if (error == NULL) return;
		*error = g_error_new (SQL_STORE_ERROR, 1,
							  "Database cannont be changed!");
		return;
	}
	else if (!g_file_test (filename, (GFileTest)(G_FILE_TEST_EXISTS
                                        | G_FILE_TEST_IS_REGULAR)))
	{
		if (error == NULL) return;
		*error = g_error_new (SQL_STORE_ERROR, 2,
							  "Database does not exists!");
		return;
	}
	else if (SQLITE_OK != sqlite3_open (filename, &priv->dbh))
	{
		if (error == NULL) return;
		*error = g_error_new (SQL_STORE_ERROR, 3,
							  "Error opening database!");
		return;
	}
}

void
sql_store_set_table (SqlStore  *self,
							const gchar	 *table,
							GError		 **error)
{
	SqlStorePrivate *priv;

	g_return_if_fail (IS_SQL_STORE (self));

	priv = SQL_STORE_GET_PRIVATE (self);
	g_assert (priv);

	if (!priv->dbh)
	{
		*error = g_error_new (SQL_STORE_ERROR, 4,
							  "No database connection set!");
		return;
	}
	else if (priv->dbh && priv->table)
	{
		*error = g_error_new (SQL_STORE_ERROR, 4,
							  "Cannot change table after connection");
		return;
	}

	if (priv->table)
		g_free (priv->table);
	priv->table = g_strdup (table);
	self->n_columns = sql_fetch_n_columns (priv->dbh, priv->table);
}

const gchar*
sql_store_get_table (SqlStore *self)
{
	g_return_val_if_fail (IS_SQL_STORE (self), NULL);
	return SQL_STORE_GET_PRIVATE (self)->table;
}

void
sql_store_set (SqlStore *self,
					  GtkTreeIter	*iter,
					  ...)
{
#warning sql_store_set not implemented
}

void
sql_store_clear (SqlStore  *self)
{
#warning sql_store_clear not implemented
}

gboolean
sql_store_iter_is_valid (SqlStore *self,
								GtkTreeIter	*iter)
{
#warning sql_store_iter_is_valid not Implemented
	return TRUE;
}

void
sql_store_remove (SqlStore *self,
						 GtkTreeIter	*iter)
{
#warning sql_store_remove not Implemented.
}

void
sql_store_append (SqlStore *self,
						 GtkTreeIter	*iter)
{
#warning sql_store_append not Implemented.
}
