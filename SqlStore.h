#ifndef SQLSTORE_H_INCLUDED
#define SQLSTORE_H_INCLUDED

#include <gtk/gtk.h>

#define TYPE_SQL_STORE             (sql_store_get_type())
#define SQL_STORE(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                                           TYPE_SQL_STORE,            \
                                           SqlStore))
#define SQL_STORE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),  \
                                           TYPE_SQL_STORE,GObject))
#define IS_SQL_STORE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                           TYPE_SQL_STORE))
#define IS_SQL_STORE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),  \
                                           TYPE_SQL_STORE))
#define SQL_STORE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),  \
                                           TYPE_SQL_STORE,     \
                                           SqlStoreClass))

typedef struct _SqlStore SqlStore;
typedef struct _SqlStoreClass SqlStoreClass;

struct _SqlStore {
	 GObject parent;
	 gint    n_columns;
	 gint    stamp;
};

struct _SqlStoreClass {
    GObjectClass parent_class;
};

GType           sql_store_get_type    (void) G_GNUC_CONST;
GtkTreeModel*   sql_store_new         (void);

void            sql_store_set_filename  (SqlStore  *self,
                                                const gchar     *filename,
                                                GError         **error);
void            sql_store_set_table     (SqlStore  *self,
                                                const gchar     *table,
                                                GError         **error);
const gchar*    sql_store_get_table     (SqlStore  *self);
void            sql_store_append        (SqlStore  *self,
                                                GtkTreeIter     *iter);
void            sql_store_set           (SqlStore  *self,
                                                GtkTreeIter     *iter,
                                                ...);
void            sql_store_remove        (SqlStore  *self,
                                                GtkTreeIter     *iter);
void            sql_store_clear         (SqlStore  *self);
gboolean        sql_store_iter_is_valid (SqlStore  *self,
                                                GtkTreeIter     *iter);

#endif // SQLSTORE_H_INCLUDED
