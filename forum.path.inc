<?php
/**
 * @file
 * Path alias patterns.
 */

/**
 * Implements hook_pathauto() for forum module.
 */
function forum_path_info() {
  $vocabulary_name = config_get('forum.settings', 'forum_nav_vocabulary');

  $info['forum'] = array(
    'entity type' => 'taxonomy_term',
    'label' => t('Forum category'),
    'pattern label' => t('URL alias pattern for forum categories'),
    'pattern default' => 'forum/[term:name]',
    'type delete callback' => 'forum_path_delete_callback',
    'batch update callback' => 'forum_pathauto_bulk_update_batch_process',
    'source prefix' => 'forum/',
    'batch file' => 'forum.path.inc',
    'batch file path' => backdrop_get_path('module', 'forum'),
  );

  $languages = array();
  if (module_exists('language')) {
    $languages = array(LANGUAGE_NONE => (object) array(
      'name' => t('Language neutral'),
      'langcode' => LANGUAGE_NONE,
    )) + language_list();

    // Update the pattern info for multiple options.
    $info['forum']['pattern label'] = t('Default URL alias pattern for forum categories');
    $info['forum']['pattern description'] = t('Fallback pattern for all forum categories without a specific URL alias pattern below.');
    $info['forum']['pattern items'] = array();
    $info['forum']['pattern items label'] = t('URL alias patterns for specific languages');

    $vocabulary = taxonomy_vocabulary_load($vocabulary_name);
    if (count($languages) && !empty($vocabulary->language)) {
      $info['forum']['pattern items'][$vocabulary_name] = t('Default URL alias pattern for all forum categories (any language)');
      foreach ($languages as $langcode => $language) {
        $info['forum']['pattern items'][$vocabulary_name . '_' . $langcode] = t('Forum categories in %language', array('%language' => $language->name));
      }
    }
  }

  return $info;
}


/**
 * Callback to delete aliases for forum terms
 */
function forum_path_delete_callback() {
 $vocabulary = config_get('forum.settings', 'forum_nav_vocabulary');

  $query = db_select('forum_data', 'td');
  $query->addField('td', 'tid');
  $query->condition('td.vocabulary', $vocabulary);
  $tids = $query->execute()->fetchCol();

  $sources = array();
  foreach ($tids as $tid) {
    $sources[] = 'forum/' . $tid;
  }

  $query = db_delete('url_alias');
  $query->condition('source', $sources, 'IN');
  $query->execute();
}

/**
 * Batch processing callback; Generate aliases for forum categories.
 */
function forum_pathauto_bulk_update_batch_process(&$context) {
   if (!isset($context['sandbox']['current'])) {
     $context['sandbox']['count'] = 0;
     $context['sandbox']['current'] = 0;
   }

   $query = db_select('forum_data', 'td');
   $query->leftJoin('url_alias', 'ua', "CONCAT('forum/', td.tid) = ua.source");
   $query->addField('td', 'tid');
   $query->isNull('ua.source');
   $query->condition('td.tid', $context['sandbox']['current'], '>');
   $query->condition('td.vid', variable_get('forum_nav_vocabulary', ''));
   $query->orderBy('td.tid');
   $query->addTag('pathauto_bulk_update');
   $query->addMetaData('entity', 'forum');

   // Get the total amount of items to process.
   if (!isset($context['sandbox']['total'])) {
     $context['sandbox']['total'] = $query->countQuery()->execute()->fetchField();

     // If there are no nodes to update, the stop immediately.
     if (!$context['sandbox']['total']) {
       $context['finished'] = 1;
       return;
     }
   }

   $query->range(0, 25);
   $tids = $query->execute()->fetchCol();

   pathauto_forum_update_alias_multiple($tids, 'bulkupdate');
   $context['sandbox']['count'] += count($tids);
   $context['sandbox']['current'] = max($tids);
   $context['message'] = t('Updated alias for forum @tid.', array('@tid' => end($tids)));

   if ($context['sandbox']['count'] != $context['sandbox']['total']) {
     $context['finished'] = $context['sandbox']['count'] / $context['sandbox']['total'];
   }
}

