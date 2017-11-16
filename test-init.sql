CREATE EXTENSION bdr CASCADE;
CREATE EXTENSION bdr_crdt;

\c "host=localhost port=6001 dbname=test"

SELECT bdr.bdr_group_create(
      local_node_name := 'node1',
      node_external_dsn := 'host=localhost port=6001 dbname=test'
);

SELECT bdr.bdr_node_join_wait_for_ready();    


\c "host=localhost port=6002 dbname=test"

CREATE EXTENSION bdr CASCADE;
CREATE EXTENSION bdr_crdt;

SELECT bdr.bdr_group_join(
      local_node_name := 'node2',
      node_external_dsn := 'host=localhost port=6002 dbname=test',
      join_using_dsn := 'host=localhost port=6001 dbname=test'
);

SELECT bdr.bdr_node_join_wait_for_ready();


\c "host=localhost port=6001 dbname=test"

select bdr.bdr_replicate_ddl_command('create table public.counters (id int primary key, counter public.crdt_counter)');

select bdr.bdr_replicate_ddl_command('
CREATE OR REPLACE FUNCTION public.counters_conflict_resolution(
    IN  rec1 public.counters,
    IN  rec2 public.counters,
    IN  conflict_info text,
    IN  table_name regclass,
    IN  conflict_type bdr.bdr_conflict_type, 
    OUT rec_resolution public.counters,
    OUT conflict_action bdr.bdr_conflict_handler_action)
 RETURNS record AS
$BODY$
BEGIN
  rec_resolution.id = rec1.id;
  rec_resolution.counter = public.crdt_counter_merge(rec1.counter, rec2.counter);
  conflict_action := ''ROW'';
END
$BODY$
 LANGUAGE plpgsql VOLATILE');

SELECT bdr.bdr_create_conflict_handler(
  ch_rel := 'public.counters',
  ch_name := 'counters_update_csh',
  ch_proc := 'public.counters_conflict_resolution(public.counters, public.counters, text, regclass, bdr.bdr_conflict_type)',
  ch_type := 'update_update'
);

-- initial value
INSERT INTO counters VALUES (1, 0);
