#ifndef SQL_H_INCLUDED
#define SQL_H_INCLUDED

#include <sqlite3.h>
#include <glib.h>

gint    sql_count_rows      (sqlite3 *sqlite, gchar *table);
gint    sql_fetch_row_pos   (sqlite3 *sqlite, gchar *table, gchar *oid);
GPtrArray* sql_fetch_next      (sqlite3 *sqlite, gchar *table, gchar *last_oid);
GPtrArray* sql_fetch_row       (sqlite3 *sqlite, gchar *table, gchar *oid);
GPtrArray* sql_fetch_nth_row   (sqlite3 *sqlite, gchar *table, gint index);
gint    sql_fetch_n_columns (sqlite3 *sqlite, gchar *table);

#endif // SQL_H_INCLUDED
