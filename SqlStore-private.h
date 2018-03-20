#ifndef SQLSTORE_PRIVATE_H_INCLUDED
#define SQLSTORE_PRIVATE_H_INCLUDED

#include <sqlite3.h>

#define SQL_STORE_ERROR g_quark_from_string("SqlStore")

#define SQL_STORE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE((o), \
                                         TYPE_SQL_STORE,           \
                                         SqlStorePrivate))

typedef struct _SqlStorePrivate SqlStorePrivate;
struct _SqlStorePrivate {
    gchar   *table;
    sqlite3 *dbh;
    GTree   *cache;  /* data cache indexed by oid (gchar*)      */
	GTree   *rcache; /* data cache indexed by row offset (gint) */
};

/* GObject implementations */
static void              sql_store_init            (SqlStore       *self);
static void              sql_store_class_init      (SqlStoreClass  *klass);
static void              sql_store_finalize        (GObject              *obj);

/* GtkTreeModelIface implementation */
static void              sql_store_tree_model_init (GtkTreeModelIface *iface);
static GtkTreeModelFlags sql_store_get_flags       (GtkTreeModel      *tree_model);
static gint              sql_store_get_n_columns   (GtkTreeModel      *tree_model);
static GType             sql_store_get_column_type (GtkTreeModel      *tree_model,
                                                           gint               index);
static gboolean          sql_store_get_iter        (GtkTreeModel      *tree_model,
                                                           GtkTreeIter       *iter,
                                                           GtkTreePath       *path);
static GtkTreePath*      sql_store_get_path        (GtkTreeModel      *tree_model,
                                                           GtkTreeIter       *iter);
static void              sql_store_get_value       (GtkTreeModel      *tree_model,
                                                           GtkTreeIter       *iter,
                                                           gint               column,
                                                           GValue            *value);
static gboolean          sql_store_iter_next       (GtkTreeModel      *tree_model,
                                                           GtkTreeIter       *iter);
static gboolean          sql_store_iter_children   (GtkTreeModel      *tree_model,
                                                           GtkTreeIter       *iter,
                                                           GtkTreeIter       *parent);
static gboolean          sql_store_iter_has_child  (GtkTreeModel      *tree_model,
                                                           GtkTreeIter       *iter);
static gint              sql_store_iter_n_children (GtkTreeModel      *tree_model,
                                                           GtkTreeIter       *iter);
static gboolean          sql_store_iter_nth_child  (GtkTreeModel      *tree_model,
                                                           GtkTreeIter       *iter,
                                                           GtkTreeIter       *parent,
                                                           gint               n);
static gboolean          sql_store_iter_parent     (GtkTreeModel      *tree_model,
                                                           GtkTreeIter       *iter,
                                                           GtkTreeIter       *child);

#endif // SQLSTORE_PRIVATE_H_INCLUDED
