#ifndef BDR_CRDT_H
#define BDR_CRDT_H

#include <ctype.h>

#include "fmgr.h"

typedef struct crdt_counter_node
{
	uint64		sysid;
	TimeLineID	timeline;
	Oid			dboid;
	uint64		value;
	uint32		version;
} crdt_counter_node;

typedef struct crdt_counter
{
	/* varlena header (do not touch directly!) */
	int32				vl_len_;

	/* current total counter value */
	int64				value;

	/* list of per-node counters */
	int32				nnodes;
	crdt_counter_node	nodes[FLEXIBLE_ARRAY_MEMBER];

} crdt_counter;

#define DatumGetCrdtCounter(x)	((crdt_counter *) PG_DETOAST_DATUM(x))

#define PG_GETARG_CRDT_COUNTER_P(x)	DatumGetCrdtCounter(PG_GETARG_DATUM(x))
#define PG_RETURN_CRDT_COUNTER_P(x)	PG_RETURN_POINTER(x)

extern Datum crdt_counter_in(PG_FUNCTION_ARGS);
extern Datum crdt_counter_out(PG_FUNCTION_ARGS);
extern Datum crdt_counter_recv(PG_FUNCTION_ARGS);
extern Datum crdt_counter_send(PG_FUNCTION_ARGS);

extern Datum int8_to_crdt_counter(PG_FUNCTION_ARGS);

extern Datum crdt_counter_plus_int4(PG_FUNCTION_ARGS);
extern Datum crdt_counter_plus_int8(PG_FUNCTION_ARGS);

extern Datum crdt_counter_merge(PG_FUNCTION_ARGS);
extern Datum crdt_counter_to_int8(PG_FUNCTION_ARGS);
extern Datum crdt_counter_to_text(PG_FUNCTION_ARGS);

#endif   /* BDR_CRDT_H */
