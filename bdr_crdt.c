#include "postgres.h"

#include <ctype.h>
#include <limits.h>

#include "access/hash.h"
#include "access/xlog.h"
#include "catalog/pg_type.h"
#include "libpq/pqformat.h"
#include "miscadmin.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "bdr_crdt.h"


PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(crdt_counter_in);
PG_FUNCTION_INFO_V1(crdt_counter_out);
PG_FUNCTION_INFO_V1(crdt_counter_recv);
PG_FUNCTION_INFO_V1(crdt_counter_send);

PG_FUNCTION_INFO_V1(int4_to_crdt_counter);
PG_FUNCTION_INFO_V1(int8_to_crdt_counter);

PG_FUNCTION_INFO_V1(crdt_counter_plus_int4);
PG_FUNCTION_INFO_V1(crdt_counter_plus_int8);

PG_FUNCTION_INFO_V1(crdt_counter_to_int8);
PG_FUNCTION_INFO_V1(crdt_counter_to_text);
PG_FUNCTION_INFO_V1(crdt_counter_merge);

extern Datum byteain(PG_FUNCTION_ARGS);
extern Datum byteaout(PG_FUNCTION_ARGS);
extern Datum bytearecv(PG_FUNCTION_ARGS);
extern Datum byteasend(PG_FUNCTION_ARGS);

static char *crdt_counter_to_cstring(crdt_counter *counter);

/*****************************************************************************
 *	 USER I/O ROUTINES														 *
 *****************************************************************************/

/*
 *	crdt_counter_in		- converts "(node,value)" to crdt_counter struct
 */
Datum
crdt_counter_in(PG_FUNCTION_ARGS)
{
	return DirectFunctionCall1(byteain, PG_GETARG_DATUM(0));
}

/*
 *	crdt_counter_in		- converts crdt_counter to string representation
 */
Datum
crdt_counter_out(PG_FUNCTION_ARGS)
{
	return DirectFunctionCall1(byteaout, PG_GETARG_DATUM(0));
}

/*
 *	crdt_counter_recv		- converts external binary format to crdt_counter
 */
Datum
crdt_counter_recv(PG_FUNCTION_ARGS)
{
	return DirectFunctionCall1(bytearecv, PG_GETARG_DATUM(0));
}

/*
 * crdt_counter_send		- converts crdt_counter to external binary format
 */
Datum
crdt_counter_send(PG_FUNCTION_ARGS)
{
	return DirectFunctionCall1(byteasend, PG_GETARG_DATUM(0));
}

/*
 * int4 as counter
 */
Datum
int4_to_crdt_counter(PG_FUNCTION_ARGS)
{
	Size			len;
	crdt_counter   *counter = NULL;
	int32			value = PG_GETARG_INT32(0);

	if (value < 0)
		elog(ERROR, "invalid counter value (only non-negative values allowed)");

	len = offsetof(crdt_counter, nodes) + sizeof(crdt_counter_node);
	counter = palloc0(len);
	SET_VARSIZE(counter, len);

	counter->nnodes = 1;
	counter->value = value;

	counter->nodes[0].dboid = MyDatabaseId;
	counter->nodes[0].sysid = GetSystemIdentifier();
	counter->nodes[0].timeline = ThisTimeLineID;
	counter->nodes[0].value = value;
	counter->nodes[0].version = 1;

	PG_RETURN_BYTEA_P(counter);
}

/*
 * int8 as counter
 */
Datum
int8_to_crdt_counter(PG_FUNCTION_ARGS)
{
	Size			len;
	crdt_counter   *counter = NULL;
	int64			value = PG_GETARG_INT64(0);

	if (value < 0)
		elog(ERROR, "invalid counter value (only non-negative values allowed)");

	len = offsetof(crdt_counter, nodes) + sizeof(crdt_counter_node);
	counter = palloc0(len);
	SET_VARSIZE(counter, len);

	counter->nnodes = 1;
	counter->value = value;

	counter->nodes[0].dboid = MyDatabaseId;
	counter->nodes[0].sysid = GetSystemIdentifier();
	counter->nodes[0].timeline = ThisTimeLineID;
	counter->nodes[0].value = value;
	counter->nodes[0].version = 1;

	PG_RETURN_BYTEA_P(counter);
}

/*
 * counter + int4
 *
 * Find record for the current node and increment the value. If the record
 * does not exist, add it to the end.
 */
Datum
crdt_counter_plus_int4(PG_FUNCTION_ARGS)
{
	int				i, idx;
	bool			found;
	Size			len;
	crdt_counter   *tmp;
	crdt_counter   *counter = PG_GETARG_CRDT_COUNTER_P(0);
	int32			value = PG_GETARG_INT32(1);

	if (value < 0)
		elog(ERROR, "invalid counter value (only non-negative values allowed)");

	elog(WARNING, "START: node %lu adding %d to %s", GetSystemIdentifier(), value, crdt_counter_to_cstring(counter));

	found = false;
	idx = -1;

	for (i = 0; i < counter->nnodes; i++)
	{
		crdt_counter_node	node = counter->nodes[i];

		if ((node.dboid == MyDatabaseId) &&
			(node.sysid == GetSystemIdentifier()) &&
			(node.timeline == ThisTimeLineID))
		{
			found = true;
			idx = i;
			break;
		}
	}

	/* add new node */
	len = offsetof(crdt_counter, nodes) +
			counter->nnodes * sizeof(crdt_counter_node);

	if (!found)
		len += sizeof(crdt_counter_node);

	tmp = palloc0(len);

	memcpy(tmp, counter, offsetof(crdt_counter, nodes) +
			counter->nnodes * sizeof(crdt_counter_node));

	SET_VARSIZE(tmp, len);

	tmp->value += value;

	if (!found)
	{
		tmp->nodes[tmp->nnodes].dboid    = MyDatabaseId;
		tmp->nodes[tmp->nnodes].sysid    = GetSystemIdentifier();
		tmp->nodes[tmp->nnodes].timeline = ThisTimeLineID;
		tmp->nodes[tmp->nnodes].value    = value;
		tmp->nodes[tmp->nnodes].version  = 1;

		tmp->nnodes++;
	}
	else
	{
		tmp->nodes[idx].value += value;
		tmp->nodes[idx].version++;
	}

	elog(WARNING, "STOP: node %lu adding %d to %s", GetSystemIdentifier(), value, crdt_counter_to_cstring(tmp));

	PG_RETURN_BYTEA_P(tmp);
}

/*
 * counter + int8
 *
 * Find record for the current node and increment the value. If the record
 * does not exist, add it to the end.
 */
Datum
crdt_counter_plus_int8(PG_FUNCTION_ARGS)
{
	int				i, idx;
	bool			found;
	Size			len;
	crdt_counter   *tmp;
	crdt_counter   *counter = PG_GETARG_CRDT_COUNTER_P(0);
	int64			value = PG_GETARG_INT64(1);

	if (value < 0)
		elog(ERROR, "invalid counter value (only non-negative values allowed)");

	elog(WARNING, "START: node %lu adding %ld to %s", GetSystemIdentifier(), value, crdt_counter_to_cstring(counter));

	found = false;
	idx = -1;

	for (i = 0; i < counter->nnodes; i++)
	{
		crdt_counter_node	node = counter->nodes[i];

		if ((node.dboid == MyDatabaseId) &&
			(node.sysid == GetSystemIdentifier()) &&
			(node.timeline == ThisTimeLineID))
		{
			found = true;
			idx = i;
			break;
		}
	}

	/* add new node */
	len = offsetof(crdt_counter, nodes) +
			counter->nnodes * sizeof(crdt_counter_node);

	if (!found)
		len += sizeof(crdt_counter_node);

	tmp = palloc0(len);

	memcpy(tmp, counter, offsetof(crdt_counter, nodes) +
			counter->nnodes * sizeof(crdt_counter_node));

	SET_VARSIZE(tmp, len);

	tmp->value += value;

	if (!found)
	{
		tmp->nodes[tmp->nnodes].dboid    = MyDatabaseId;
		tmp->nodes[tmp->nnodes].sysid    = GetSystemIdentifier();
		tmp->nodes[tmp->nnodes].timeline = ThisTimeLineID;
		tmp->nodes[tmp->nnodes].value    = value;
		tmp->nodes[tmp->nnodes].version  = 1;

		tmp->nnodes++;
	}
	else
	{
		tmp->nodes[idx].value += value;
		tmp->nodes[idx].version++;
	}

	elog(WARNING, "STOP: node %lu adding %ld to %s", GetSystemIdentifier(), value, crdt_counter_to_cstring(tmp));

	PG_RETURN_BYTEA_P(tmp);
}

/*
 * counter + counter => counter
 *
 * Merge two counter records to resolve conflict. If a node has records
 * in both counters, use the maximum value.
 */
Datum
crdt_counter_merge(PG_FUNCTION_ARGS)
{
	crdt_counter   *result;
	crdt_counter   *counter1 = PG_GETARG_CRDT_COUNTER_P(0);
	crdt_counter   *counter2 = PG_GETARG_CRDT_COUNTER_P(1);

	int		i, j;
	Size	len;

	/* includes varlena header, so no SET_VARSIZE here */
	len = offsetof(crdt_counter, nodes) +
				   counter1->nnodes * sizeof(crdt_counter_node);
	result = palloc0(len);
	memcpy(result, counter1, len);

	for (i = 0; i < counter2->nnodes; i++)
	{
		bool				found = false;
		crdt_counter_node  *node1 = &counter2->nodes[i];

		for (j = 0; j < result->nnodes; j++)
		{
			crdt_counter_node  *node2 = &result->nodes[j];

			if ((node1->dboid == node2->dboid) &&
				(node1->sysid == node2->sysid) &&
				(node1->timeline == node2->timeline))
			{
				result->value -= node2->value;

				node2->version = (node1->version > node2->version) ? node1->version : node2->version;
				node2->value = (node1->value > node2->value) ? node1->value : node2->value;

				result->value += node2->value;

				found = true;
				break;
			}
		}

		/* not found, add it at the end */
		if (!found)
		{
			Size len = offsetof(crdt_counter, nodes) +
					   (result->nnodes + 1) * sizeof(crdt_counter_node);
			result = repalloc(result, len);
			SET_VARSIZE(result, len);
			result->value += node1->value;
			result->nodes[result->nnodes].dboid = counter2->nodes[i].dboid;
			result->nodes[result->nnodes].timeline = counter2->nodes[i].timeline;
			result->nodes[result->nnodes].sysid = counter2->nodes[i].sysid;
			result->nodes[result->nnodes].version = counter2->nodes[i].version;
			result->nnodes++;
		}
	}

	elog(WARNING, "merge %s + %s => %s", crdt_counter_to_cstring(counter1), crdt_counter_to_cstring(counter2), crdt_counter_to_cstring(result));

	PG_RETURN_BYTEA_P(result);
}

Datum
crdt_counter_to_int8(PG_FUNCTION_ARGS)
{
	crdt_counter   *counter = PG_GETARG_CRDT_COUNTER_P(0);

	PG_RETURN_INT64(counter->value);
}

static char *
crdt_counter_to_cstring(crdt_counter *counter)
{
	int i;
	StringInfoData	str;

	initStringInfo(&str);

	appendStringInfo(&str, "[%lu,%d", counter->value, counter->nnodes);

	for (i = 0; i < counter->nnodes; i++)
		appendStringInfo(&str, ",(%lu,%u,%u,%u,%lu)",
							counter->nodes[i].sysid,
							counter->nodes[i].timeline,
							counter->nodes[i].dboid,
							counter->nodes[i].version,
							counter->nodes[i].value);

	appendStringInfoChar(&str, ']');

	return str.data;
}


Datum
crdt_counter_to_text(PG_FUNCTION_ARGS)
{
	crdt_counter   *counter = PG_GETARG_CRDT_COUNTER_P(0);

	PG_RETURN_TEXT_P(cstring_to_text(crdt_counter_to_cstring(counter)));
}
