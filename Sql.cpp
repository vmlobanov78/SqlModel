#include <stdlib.h>

#include <glib.h>
#include <sqlite3.h>

#include "Sql.h"

static gint
sql_count_rows_cb (gpointer   user_data,
                          gint       n_columns,
                          gchar    **values,
                          gchar	   **columns)
{
	gint *count = (gint*)user_data;

	if (n_columns > 0)
		(*count) = atoi (values[0]);

	return SQLITE_OK;
}

static gint
sql_fetch_row_pos_cb (gpointer   user_data,
                             gint	    n_columns,
                             gchar    **values,
                             gchar    **columns)
{
	gint *pos = (gint*)user_data;

	if (n_columns > 0)
		(*pos) = atoi (values[0]);

	return SQLITE_OK;
}

static gint
sql_fetch_n_columns_cb (gpointer   user_data,
                               gint	      n_columns,
                               gchar    **values,
                               gchar    **columns)
{
	gint *n = (gint*)user_data;
	(*n) = (*n) + 1;
	return SQLITE_OK;
}

static gint
sql_fetch_next_cb (gpointer   user_data,
                          gint       n_columns,
                          gchar    **values,
                          gchar    **columns)
{
	GPtrArray *result = (GPtrArray*)user_data;
	gchar  *value;
	gint	i;

	g_return_val_if_fail (result != NULL, SQLITE_ERROR);

	for (i = 0; i < n_columns; i++) {
		value = g_strdup (values[i]);
		g_ptr_array_add (result, value);
	}

	return SQLITE_OK;
}

static gint
sql_fetch_row_cb (gpointer   user_data,
                         gint	    n_columns,
                         gchar    **values,
                         gchar    **columns)
{
	GPtrArray *result = (GPtrArray*)user_data;
	gchar  *value;
	gint	i;

	g_return_val_if_fail (result != NULL, SQLITE_ERROR);

	for (i = 0; i < n_columns; i++) {
		value = g_strdup (values[i]);
		g_ptr_array_add (result, value);
	}

	return SQLITE_OK;
}

static gint
sql_fetch_nth_row_cb (gpointer   user_data,
                             gint       n_columns,
                             gchar    **values,
                             gchar    **columns)
{
	GPtrArray *result = (GPtrArray*)user_data;
	gint       i;

	g_return_val_if_fail (result != NULL, SQLITE_ERROR);

	for (i = 0; i < n_columns; i++)
		g_ptr_array_add (result, g_strdup (values[i]));

	return SQLITE_OK;
}

/**
 * sql_count_rows:
 * @sqlite: A sqlite3 handle.
 * @table: The table name to fetch from.
 *
 * Returns the number of rows found in the table or -1 if there was an error.
 **/
gint
sql_count_rows (sqlite3 *sqlite, gchar *table)
{
	gchar *query = NULL;
	gint   count = -1;

	g_return_val_if_fail (sqlite != NULL, -1);
	g_return_val_if_fail (table != NULL,  -1);

	query = g_strdup_printf ("SELECT COUNT(oid) FROM %s", table);
	sqlite3_exec (sqlite, query, sql_count_rows_cb, &count, NULL);
	g_free (query);

	return count;
}

/**
 * sql_fetch_next:
 * @sqlite: A sqlite3 handle.
 * @table: Name of the table to select from.
 * @last_oid: The oid previous to the row desired, or NULL for the first row.
 *
 * Returns a GPtrArray* of column values, with oid as the first column. The
 * array should be freed with g_array_free().
 **/
GPtrArray*
sql_fetch_next (sqlite3 *sqlite, gchar *table, gchar *last_oid)
{
	GPtrArray *result;
	gchar  *query;

	g_return_val_if_fail (sqlite != NULL, NULL);
	g_return_val_if_fail (table != NULL, NULL);

	result = g_ptr_array_sized_new (2);

	if (last_oid == NULL)
		query = g_strdup_printf (
			"SELECT oid, * FROM %s ORDER BY oid LIMIT 1", table);
	else
	  	query = g_strdup_printf (
			"SELECT oid, * FROM %s WHERE oid > %s ORDER BY oid LIMIT 1",
			table, last_oid);

	if (SQLITE_OK !=
		sqlite3_exec (sqlite, query, sql_fetch_next_cb, result, NULL))
	{
		g_ptr_array_free (result, FALSE);
		result = NULL;
	}

	g_free (query);
	return result;
}

/**
 * sql_fetch_row:
 * @sqlite: A sqlite3 handle.
 * @table: Name of the table to select from.
 * @oid: The oid used to reference the row in SQLite.
 *
 * Returns a GPtrArray* of column values, with oid as the first column. The
 * array should be freed with g_array_free().
 **/
GPtrArray*
sql_fetch_row  (sqlite3 *sqlite, gchar *table, gchar *oid)
{
	GPtrArray *result;
	gchar  *query;

	g_return_val_if_fail (sqlite != NULL, NULL);
	g_return_val_if_fail (table != NULL, NULL);
	g_return_val_if_fail (oid != NULL, NULL);

	query = g_strdup_printf ("SELECT oid, * FROM %s WHERE oid = %s", table, oid);
	result = g_ptr_array_sized_new (2);

	if (SQLITE_OK !=
		sqlite3_exec (sqlite, query, sql_fetch_row_cb, result, NULL))
		result = NULL;

	g_free (query);
	return result;
}

/**
 * sql_fetch_nth_row:
 * @sqlite: A sqlite3 handle.
 * @table: Name of table to select from.
 * @index: nth row to return, 0-based. therefore, to get the first row,
 *		 you would pass 0.
 **/
GPtrArray*
sql_fetch_nth_row (sqlite3 *sqlite, gchar *table, gint index)
{
	GPtrArray *result;
	gchar  *query;

	query = g_strdup_printf ("SELECT oid, * FROM %s LIMIT 1 OFFSET %d",
							 table, index);
	result = g_ptr_array_sized_new (2);

	if (SQLITE_OK !=
		sqlite3_exec (sqlite, query, sql_fetch_nth_row_cb, result, NULL))
		result = NULL;

	g_free (query);
	return result;
}

/**
 * sql_fetch_row_pos:
 * @sqlite: A sqlite3 handle.
 * @table: Name of table to select from.
 * @oid: oid of row to find position of.
 *
 * Retuns the rows offset from 0, or -1 if the row was not found.
 **/
gint
sql_fetch_row_pos (sqlite3 *sqlite, gchar *table, gchar *oid)
{
	gint   pos = 0;
	gchar *query;

	query = g_strdup_printf ("SELECT COUNT(oid) FROM %s WHERE oid < %s",
							 table, oid);
	if (SQLITE_OK !=
		sqlite3_exec (sqlite, query, sql_fetch_row_pos_cb, &pos, NULL))
		pos = -1;
	g_free (query);

	return pos;
}

/**
 * sql_fetch_n_columns:
 * @sqlite: A sqlite3 handle.
 * @table: Name of table to select from.
 *
 * Retuns the number of columns found in table, including oid.
 **/
gint
sql_fetch_n_columns (sqlite3 *sqlite, gchar *table)
{
	gint   n = 0;
	gchar *query;

	query = g_strdup_printf ("PRAGMA table_info('%s')", table);
	sqlite3_exec (sqlite, query, sql_fetch_n_columns_cb, &n, NULL);
	g_free (query);

	n += 1; /* oid */
	return n;
}

