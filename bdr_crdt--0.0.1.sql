
CREATE FUNCTION crdt_counter_in(cstring) RETURNS crdt_counter AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE FUNCTION crdt_counter_out(crdt_counter) RETURNS cstring AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE FUNCTION crdt_counter_recv(internal) RETURNS crdt_counter AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE FUNCTION crdt_counter_send(crdt_counter) RETURNS bytea AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE TYPE crdt_counter (
	INPUT = crdt_counter_in,
	OUTPUT = crdt_counter_out,
	RECEIVE = crdt_counter_recv,
	SEND = crdt_counter_send,
	INTERNALLENGTH = -1
);


CREATE FUNCTION crdt_counter_plus_int4(crdt_counter, int) RETURNS crdt_counter AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE OPERATOR + (
	LEFTARG = crdt_counter,
	RIGHTARG = int,
	PROCEDURE = crdt_counter_plus_int4,
	COMMUTATOR = +
);


CREATE FUNCTION crdt_counter_plus_int8(crdt_counter, bigint) RETURNS crdt_counter AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE OPERATOR + (
	LEFTARG = crdt_counter,
	RIGHTARG = bigint,
	PROCEDURE = crdt_counter_plus_int8,
	COMMUTATOR = +
);


CREATE FUNCTION crdt_counter_merge(crdt_counter, crdt_counter) RETURNS crdt_counter AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE OPERATOR + (
	LEFTARG = crdt_counter,
	RIGHTARG = crdt_counter,
	PROCEDURE = crdt_counter_merge,
	COMMUTATOR = +
);


CREATE FUNCTION crdt_counter_to_int8(crdt_counter) RETURNS bigint AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE CAST (crdt_counter AS bigint)
	WITH FUNCTION crdt_counter_to_int8 (crdt_counter) AS IMPLICIT;

CREATE OPERATOR # (
	RIGHTARG = crdt_counter,
	PROCEDURE = crdt_counter_to_int8
);


CREATE FUNCTION crdt_counter_to_text(crdt_counter) RETURNS text AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE CAST (crdt_counter AS text)
	WITH FUNCTION crdt_counter_to_text (crdt_counter) AS IMPLICIT;


CREATE FUNCTION int4_to_crdt_counter(int) RETURNS crdt_counter AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE CAST (int AS crdt_counter)
	WITH FUNCTION int4_to_crdt_counter (int) AS IMPLICIT;


CREATE FUNCTION int8_to_crdt_counter(bigint) RETURNS crdt_counter AS
'MODULE_PATHNAME'
LANGUAGE c IMMUTABLE STRICT;

CREATE CAST (bigint AS crdt_counter)
	WITH FUNCTION int8_to_crdt_counter (bigint) AS IMPLICIT;

