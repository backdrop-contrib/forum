<?php

/**
 * @file
 * Contains a Views filter handler to take care of filtering content by status.
 */
class taxonomy_term_data_handler_filter_is_container extends views_handler_filter {
  function admin_summary() { }
  function operator_form(&$form, &$form_state) { }
  function can_expose() { return FALSE; }

  function query() {
    $containers = implode(',', forum_get_containers());
    $table = $this->ensure_my_table();
    $column = $this->table_alias . '.tid';
    $this->query->add_where_expression(0, "$column NOT IN($containers) OR $column IS NULL");
  }
}
