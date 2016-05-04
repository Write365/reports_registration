<?php
/**
 * Created by PhpStorm.
 * User: markdinsmore, dennis bennett
 * Date: 3/17/16
 * Time: 7:26 PM
 *
 * @return string Course listing for the instructor reports
 */

function course_list()
{
    global $user;

    $header = array(
        array('data' => t('Report by CRN'), 'field' => 'i.crn', 'sort' => 'desc'),
        array('data' => t('Instructor'), 'field' => 'i.instructor_name', 'sort' => 'desc',),
        array('data' => t('Course Name'), 'field' => 'i.course_name', 'sort' => 'desc',),
        array('data' => t('Section Number'), 'field' => 'i.section_number', 'sort' => 'desc',),
        array('data' => t('Term'), 'field' => 'i.term', 'sort' => 'desc',),
    );

    $courses = db_select('OSU_Registration_Instructor_Input', 'i');
    $courses->fields('i', array('crn', 'instructor_name', 'course_name', 'section_number', 'term'));
    $courses->fields('s', array('term_description', 'start_date', 'end_date'));
    $courses->addTag('node access');
    $courses->condition('i.onid', $user->name, '=');

    $courses->innerJoin('OSU_Registration_Term_Validation', 's', 's.term_code = i.term');

    $table_sort = $courses->extend('TableSort')
        ->orderByHeader($header); //then use an extender to create the sorted table
    $pager = $table_sort->extend('PagerDefault')
        ->limit(20);    //take the sorted table and extend that with a pager
    $result = $pager->execute(); // execute the result
    $rows[] = array();

    foreach ($result as $row) { // add the data

        $rows[$row->crn]['crn'] = '<a href="'
            . $GLOBALS['base_url']
            . '/reports_registration/instructor/course-detail/'
            . $row->crn . '/' . $row->term . '">
            <div class="btn btn-default">' . $row->crn . '</div>
        </a>';
        $rows[$row->crn]['instructor_name'] = $row->instructor_name;
        $rows[$row->crn]['course_name'] = $row->course_name;
        $rows[$row->crn]['section_number'] = $row->section_number;
        $rows[$row->crn]['term'] = $row->term;
    }

    $build = "<p>The following are your courses registered with the Write 365 application (across all terms).</p>
                <p>Note: Columns are sortable. Click on the column heading to change the sort order.</p>";
    $build .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'sort-table')));
    $build .= theme('pager');

    return $build; // done

}

function course_detail($crn = null, $term = null)
{
    if ($crn == null || $term == null) {
        $build = '<h3>CRN or Term could not be found.</h3>';
        return $build;
    }
    global $user;

    // Getting Term Information
    $result = db_select('OSU_Registration_Instructor_Input', 'i');
    $result->fields('i', array('instructor_name', 'crn', 'course_name', 'section_number', 'term'))
        ->fields('t', array('term_description', 'start_date', 'end_date'))
        ->condition('crn', $crn)
        ->condition('term', $term);
    $result->join('OSU_Registration_Term_Validation', 't', 't.term_code = i.term');

    $course = $result->execute()->fetchObject();

    // Converting DateTime to UnixTime
    $startUnix = new DateTime($course->start_date);
    $startUnix = $startUnix->getTimestamp();
    $endUnix = new DateTime($course->end_date);
    $endUnix = $endUnix->getTimestamp();

    $build = '';

    // Displaying class details
    $build .= '<h3 class="section-header">Course Details: ' . $course->crn . '</h3>';

    $build .= '<div class="row-fluid">'
        . '<div class="span2"><p><strong>Term</strong></p><p>' . $course->term . '</p></div>'
        . '<div class="span2"><p><strong>Section</strong></p><p>' . $course->section_number . '</p></div>'
        . '<div class="span2"><p><strong>CRN</strong></p><p>' . $course->crn . '</p></div>'
        . '<div class="span2"><p><strong>Instructor</strong></p><p>' . $course->instructor_name . '</p></div>'
        . '</div>';

    // Building header for table.
    /*$header = array(
        array('data' => 'Student ID', 'field' => 'student_id', 'sort' => 'desc'),
        array('data' => 'ONID', 'field' => 'onid', 'sort' => 'desc',),
        array('data' => 'Last Name', 'field' => 'last_name', 'sort' => 'desc',),
        array('data' => 'First Name', 'field' => 'first_name', 'sort' => 'desc',),
        array('data' => 'Email', 'field' => 'email', 'sort' => 'desc',),
        array('data' => 'Status', 'field' => 'registration_status', 'sort' => 'desc',),
        //array('data' => 'UID', 'field' => 'uid', 'sort' => 'desc',),
        //array('data' => 'Name', 'field' => 'name', 'sort' => 'desc',),
    );*/

    // Selecting all the students information that have regiesters for the class.
    // Getting class roster.
    $result = db_select('OSU_Registration_Student_Input', 's');
    $result->fields('s', array('student_id', 'onid', 'last_name', 'first_name', 'email', 'registration_status'))
        ->fields('u', array('uid', 'name'))
        ->condition('term', $course->term)
        ->condition('crn', $course->crn)
        ->addTag('node_access')
        ->orderBy('s.last_name');
    $result->join('users', 'u', 's.onid = u.name');


    // storing results of query in results
    $results = $result->execute();

    // Creating rows for table
    $rows = array();

    // Creating variable that will all student inforamion for js to use
    $classInfo = array();

    // Creating form for each student
    // Formatting information for table
    foreach ($results as $row) {

        // clearing the result;
        $result = null;

        /* ---- Begin: Selects the current students post --- */
        // Selecting post for students
        $result = db_select('node', 'n');

        $result->fields('n', array('nid', 'title', 'type'))
            ->fields('w', array('is_wordsalad', 'percent', 'score'))
            ->fields('wc', array('wordcount', 'timewriting', 'wordsperminute'))
            ->condition('type', 'writing365')
            ->condition('status', 1)
            ->condition('uid', $row->uid)
            ->condition('created', array($startUnix, $endUnix), 'BETWEEN')
            ->addTag('node_access')
            ->addField('n', 'created', 'date');

        $result->join('wordsalad_score', 'w', 'w.nid = n.nid');
        $result->join('W365_JournalNodeData', 'wc', 'wc.nid = n.nid');


        // Execute the query
        $studentPosts = $result->execute()->fetchAll();
        /* ---- End: Selects the current students post --- */

        // Building Student Detail button
        $button = '<a href="' . $GLOBALS['base_url']
            . '/reports_registration/instructor/student-detail/'
            . $course->crn . '/' . $course->term . '/' . $row->uid
            . '"><div class="btn btn-default">View Student Detail</div></a>';


        // Formatting Date for student post
        foreach ($studentPosts as $key => $post) {
            if (property_exists($post, 'date')) {
                // January = 0 not 1
                $day = date('d', $post->date);
                $month = date('m', $post->date);
                $year = date('Y', $post->date);
                // Puts months in javasript date format
                // January = 0 != 1
                if ($month - 1 < 0) {
                    $month = 11;
                } else {
                    $month--;
                }
                // Add a 0 for months less then 10
                if ($month < 10) {
                    $month = '0' . $month;
                }
                $studentPosts[$key]->date = $year . '-' . $month . '-' . $day;
            }

        }

        // Adds the students post to the class information container
        $classInfo[$row->student_id] = $studentPosts;

        // Adding display information for calender
        $build .= '<div>';
        $build .= '<h4 class="section-header">Student: ' . $row->last_name . ', ' . $row->first_name . '<span class="status">' . $row->registration_status . '</span></h4>';

        $build .= '<div class="row-fluid">'
            . '<div class="span2"><p><strong>Student Id</strong></p><p>' . $row->student_id . '</p></div>'
            . '<div class="span2"><p><strong>Onid</strong></p><p>' . $row->onid . '</p></div>'
            /*. '<div class="span3"><p><strong>Status</strong></p><p>' . $row->registration_status . '</p></div>'*/
            . '<div class="span5"><p><strong>Email</strong></p><p>' . $row->email . '</p></div>'
            . '<div class="span3"><p><strong>Details</strong></p>' . $button . '</div>'
            . '</div>';


        $build .= '<div id="' . $row->student_id . '"></div>';
        $build .= '</div>';
    }

    // TACAL resources
    // Loaded from Write365
    // If not installed add js load for TACAL here.

    // Custom js
    drupal_add_js(drupal_get_path('module', 'reports_registration') . '/js/section-detail.js', 'file');
    drupal_add_js(array('reportsregistration' => array('classinfo' => json_encode($classInfo))), array('type' => 'setting'));

    return $build;
}

function student_detail($crn = null, $term = null, $uid = null)
{
    /*
     * This is the report for an individual student.
     * Inputs: uid, first_name, email, onid
     */
    global $user;

    // Get Student course information
    $result = db_select('OSU_Registration_Student_Input', 's');
    $result->fields('s', array('student_id', 'onid', 'last_name', 'first_name', 'email', 'registration_status'))
        ->fields('t', array('start_date', 'end_date'))
        ->fields('u', array('uid'))
        ->condition('term', $term)
        ->condition('crn', $crn)
        ->condition('uid', $uid)
        ->addTag('node_access');

    $result->join('OSU_Registration_Term_Validation', 't', 't.term_code = s.term');
    $result->join('users', 'u', 'u.name = s.onid');

    $studentInfo = $result->execute()->fetchObject();

    // Converting DateTime to UnixTime
    $start = new DateTime($studentInfo->start_date);
    $start = $start->getTimestamp();
    $end = new DateTime($studentInfo->end_date);
    $end = $end->getTimestamp();

    // Start Page build
    $build = "";

    // Display student school information
    $build .= '<h2 class="section-header">Student Information</h2>';

    $build .= '<div class="row-fluid">'
        . '<div class="span3"><p><strong>Name</strong></p><p>' . $studentInfo->last_name . ', '. $studentInfo->first_name .'</p></div>'
        /*. '<div class="span2"><p><strong>First Name</strong></p><p>' . $studentInfo->first_name . '</p></div>'*/
        . '<div class="span2"><p><strong>Onid</strong></p><p>' . $studentInfo->onid . '</p></div>'
        . '<div class="span3"><p><strong>Status</strong></p><p>' . $studentInfo->registration_status . '</p></div>'
        . '<div class="span3"><p><strong>Email</strong></p><p>' . $studentInfo->email . '</p></div>'
        . '</div>';


    // Selecting post for students
    $result = db_select('node', 'n');

    $result->fields('n', array('nid', 'title', 'type'))
        ->fields('w', array('is_wordsalad', 'percent', 'score'))
        ->fields('wc', array('wordcount', 'timewriting', 'wordsperminute'))
        ->condition('type', 'writing365')
        ->condition('status', 1)
        ->condition('uid', $studentInfo->uid)
        ->condition('created', array($start, $end), 'BETWEEN')
        ->addTag('node_access')
        ->addField('n', 'created', 'date');

    $result->join('wordsalad_score', 'w', 'w.nid = n.nid');
    $result->join('W365_JournalNodeData', 'wc', 'wc.nid = n.nid');

    // Execute the query
    $studentPost = $result->execute()->fetchAll();

    // Student Post Stats
    $wordTotal = 0;  // total number of words for the entire term.
    $entryTotal = 0; // total number of entries for the entire term.
    $wordPerEntry = 0; // average words per entry.
    $wordSalad = 0; // number of WordSalad post

    // Gathering Information on student post
    foreach ($studentPost as $key => $post) {
        // Formatting Date
        if (property_exists($post, 'date')) {
            // January = 0 not 1
            $day = date('d', $post->date);
            $month = date('m', $post->date);
            $year = date('Y', $post->date);
            // Puts months in javascript date format
            // January = 0 != 1
            if ($month - 1 < 0) {
                $month = 11;
            } else {
                $month--;
            }
            // Add a 0 for months less then 10
            if ($month < 10) {
                $month = '0' . $month;
            }
            $studentPost[$key]->date = $year . '-' . $month . '-' . $day;
        }

        // Count number of WordSalad post
        if ($post->is_wordsalad) {
            $wordSalad++;
        }

        // Tallies number of entries
        $entryTotal++;

        // Totaling word count
        $wordTotal += $post->wordcount;

    }

    // Gets average words per entry
    if ($entryTotal != 0) {
        $wordPerEntry = round($wordTotal / $entryTotal, 3);
    } else { // Defaults to 0.0, if $entryTotal is 0
        $wordPerEntry = 0.0;
    }


    // Displaying Post Statistics
    $build .= '<h3 class="section-header">Student Post Statistics</h3>';

    $build .= '<div class="row-fluid">'
        . '<div class="span3"><p><strong>Word Total</strong></p><p>' . $wordTotal . '</p></div>'
        . '<div class="span3"><p><strong>Entries Total</strong></p><p>' . $entryTotal . '</p></div>'
        . '<div class="span3"><p><strong>Words/Entries</strong></p><p>' . $wordPerEntry . '</p></div>'
        . '<div class="span3"><p><strong>WorldSalad Count</strong></p><p>' . $wordSalad . '</p></div>'
        . '</div>';


    // TACAL resources
    // Loaded from Write365
    // If not installed add js load for TACAL here.

    // Custom TACAL js
    drupal_add_js(drupal_get_path('module', 'reports_registration') . '/js/student-detail.js', 'file');
    drupal_add_js(array('reportsregistration' => array('studentid' => $studentInfo->uid, 'studentpost' => json_encode($studentPost))), array('type' => 'setting'));

    // Display Div for TACAL
    $build .= '<div id="' . $uid . '"></div>';

    return $build;

}

/**
 * @return string Course listing for the instructor reports
 */
function adm_courses_list()
{
    global $user;

    $header = array(
        array('data' => t('Report by CRN'), 'field' => 'i.crn', 'sort' => 'desc'),
        array('data' => t('Instructor'), 'field' => 'i.instructor_name', 'sort' => 'desc',),
        array('data' => t('Course Name'), 'field' => 'i.course_name', 'sort' => 'desc',),
        array('data' => t('Section Number'), 'field' => 'i.section_number', 'sort' => 'desc',),
        array('data' => t('Term'), 'field' => 'i.term', 'sort' => 'desc',),
    );

    $courses = db_select('OSU_Registration_Instructor_Input', 'i');
    $courses->fields('i', array('crn', 'instructor_name', 'course_name', 'section_number', 'term'));
    $courses->fields('s', array('term_description', 'start_date', 'end_date'));
    $courses->addTag('node access');

    $courses->innerJoin('OSU_Registration_Term_Validation', 's', 's.term_code = i.term');

    $table_sort = $courses->extend('TableSort')
        ->orderByHeader($header); //then use an extender to create the sorted table
    $pager = $table_sort->extend('PagerDefault')
        ->limit(20);    //take the sorted table and extend that with a pager
    $result = $pager->execute(); // execute the result
    $rows[] = array();

    foreach ($result as $row) { // add the data

        $rows[$row->crn]['crn'] = '<a href="'
            . $GLOBALS['base_url']
            . '/reports_registration/administrator/adm-course-detail/'
            . $row->crn . '/' . $row->term . '">
            <div class="btn btn-default">' . $row->crn . '</div>
        </a>';
        $rows[$row->crn]['instructor_name'] = $row->instructor_name;
        $rows[$row->crn]['course_name'] = $row->course_name;
        $rows[$row->crn]['section_number'] = $row->section_number;
        $rows[$row->crn]['term'] = $row->term;
    }

    $build = "<p>The following are your courses registered with the Write 365 application (across all terms).</p>
                <p>Note: Columns are sortable. Click on the column heading to change the sort order.</p>";
    $build .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'sort-table')));
    $build .= theme('pager');

    return $build; // done

}

function adm_course_detail($crn = null, $term = null)
{
    if ($crn == null || $term == null) {
        $build = '<h3>CRN or Term could not be found.</h3>';
        return $build;
    }
    global $user;

    // Getting Term Information
    $result = db_select('OSU_Registration_Instructor_Input', 'i');
    $result->fields('i', array('instructor_name', 'crn', 'course_name', 'section_number', 'term'))
        ->fields('t', array('term_description', 'start_date', 'end_date'))
        ->condition('crn', $crn)
        ->condition('term', $term);
    $result->join('OSU_Registration_Term_Validation', 't', 't.term_code = i.term');

    $course = $result->execute()->fetchObject();

    // Converting DateTime to UnixTime
    $startUnix = new DateTime($course->start_date);
    $startUnix = $startUnix->getTimestamp();
    $endUnix = new DateTime($course->end_date);
    $endUnix = $endUnix->getTimestamp();

    $build = '';

    // Displaying class details
    $build .= '<h3 class="section-header">Course Details: ' . $course->crn . '</h3>';

    $build .= '<div class="row-fluid">'
        . '<div class="span2"><p><strong>Term</strong></p><p>' . $course->term . '</p></div>'
        . '<div class="span2"><p><strong>Section</strong></p><p>' . $course->section_number . '</p></div>'
        . '<div class="span2"><p><strong>CRN</strong></p><p>' . $course->crn . '</p></div>'
        . '<div class="span2"><p><strong>Instructor</strong></p><p>' . $course->instructor_name . '</p></div>'
        . '</div>';

    // Building header for table.
    /*$header = array(
        array('data' => 'Student ID', 'field' => 'student_id', 'sort' => 'desc'),
        array('data' => 'ONID', 'field' => 'onid', 'sort' => 'desc',),
        array('data' => 'Last Name', 'field' => 'last_name', 'sort' => 'desc',),
        array('data' => 'First Name', 'field' => 'first_name', 'sort' => 'desc',),
        array('data' => 'Email', 'field' => 'email', 'sort' => 'desc',),
        array('data' => 'Status', 'field' => 'registration_status', 'sort' => 'desc',),
        //array('data' => 'UID', 'field' => 'uid', 'sort' => 'desc',),
        //array('data' => 'Name', 'field' => 'name', 'sort' => 'desc',),
    );*/

    // Selecting all the students information that have regiesters for the class.
    // Getting class roster.
    $result = db_select('OSU_Registration_Student_Input', 's');
    $result->fields('s', array('student_id', 'onid', 'last_name', 'first_name', 'email', 'registration_status'))
        ->fields('u', array('uid', 'name'))
        ->condition('term', $course->term)
        ->condition('crn', $course->crn)
        ->addTag('node_access')
        ->orderBy('s.last_name');
    $result->join('users', 'u', 's.onid = u.name');


    // storing results of query in results
    $results = $result->execute();

    // Creating rows for table
    $rows = array();

    // Creating variable that will all student inforamion for js to use
    $classInfo = array();

    // Creating form for each student
    // Formatting information for table
    foreach ($results as $row) {

        // clearing the result;
        $result = null;

        /* ---- Begin: Selects the current students post --- */
        // Selecting post for students
        $result = db_select('node', 'n');

        $result->fields('n', array('nid', 'title', 'type'))
            ->fields('w', array('is_wordsalad', 'percent', 'score'))
            ->fields('wc', array('wordcount', 'timewriting', 'wordsperminute'))
            ->condition('type', 'writing365')
            ->condition('status', 1)
            ->condition('uid', $row->uid)
            ->condition('created', array($startUnix, $endUnix), 'BETWEEN')
            ->addTag('node_access')
            ->addField('n', 'created', 'date');

        $result->join('wordsalad_score', 'w', 'w.nid = n.nid');
        $result->join('W365_JournalNodeData', 'wc', 'wc.nid = n.nid');


        // Execute the query
        $studentPosts = $result->execute()->fetchAll();
        /* ---- End: Selects the current students post --- */

        // Building Student Detail button
        $button = '<a href="'
            . $GLOBALS['base_url']
            . '/reports_registration/administrator/adm-student-detail/'
            . $course->crn . '/'
            . $course->term . '/'
            . $row->uid
            . '"><div class="btn btn-default">View Student Detail</div></a>';


        // Formatting Date for student post
        foreach ($studentPosts as $key => $post) {
            if (property_exists($post, 'date')) {
                // January = 0 not 1
                $day = date('d', $post->date);
                $month = date('m', $post->date);
                $year = date('Y', $post->date);
                // Puts months in javasript date format
                // January = 0 != 1
                if ($month - 1 < 0) {
                    $month = 11;
                } else {
                    $month--;
                }
                // Add a 0 for months less then 10
                if ($month < 10) {
                    $month = '0' . $month;
                }
                $studentPosts[$key]->date = $year . '-' . $month . '-' . $day;
            }

        }

        // Adds the students post to the class information container
        $classInfo[$row->student_id] = $studentPosts;

        // Adding display information for calender
        $build .= '<div>';
        $build .= '<h4 class="section-header">Student: ' . $row->last_name . ', ' . $row->first_name . '<span class="status">' . $row->registration_status . '</span></h4>';

        $build .= '<div class="row-fluid">'
            . '<div class="span2"><p><strong>Student Id</strong></p><p>' . $row->student_id . '</p></div>'
            . '<div class="span2"><p><strong>Onid</strong></p><p>' . $row->onid . '</p></div>'
            /*. '<div class="span3"><p><strong>Status</strong></p><p>' . $row->registration_status . '</p></div>'*/
            . '<div class="span5"><p><strong>Email</strong></p><p>' . $row->email . '</p></div>'
            . '<div class="span3"><p><strong>Details</strong></p>' . $button . '</div>'
            . '</div>';

        $build .= '<div id="' . $row->student_id . '"></div>';
        $build .= '</div>';
    }

    // TACAL resources
    // Loaded from Write365
    // If not installed add js load for TACAL here.

    // Custom js
    drupal_add_js(drupal_get_path('module', 'reports_registration') . '/js/section-detail.js', 'file');
    drupal_add_js(array('reportsregistration' => array('classinfo' => json_encode($classInfo))), array('type' => 'setting'));

    return $build;
}

function adm_student_detail($crn = null, $term = null, $uid = null)
{
    /*
     * This is the report for an individual student.
     * Inputs: uid, first_name, email, onid
     */
    global $user;

    // Get Student course information
    $result = db_select('OSU_Registration_Student_Input', 's');
    $result->fields('s', array('student_id', 'onid', 'last_name', 'first_name', 'email', 'registration_status'))
        ->fields('t', array('start_date', 'end_date'))
        ->fields('u', array('uid'))
        ->condition('term', $term)
        ->condition('crn', $crn)
        ->condition('uid', $uid)
        ->addTag('node_access');

    $result->join('OSU_Registration_Term_Validation', 't', 't.term_code = s.term');
    $result->join('users', 'u', 'u.name = s.onid');

    $studentInfo = $result->execute()->fetchObject();

    // Converting DateTime to UnixTime
    $start = new DateTime($studentInfo->start_date);
    $start = $start->getTimestamp();
    $end = new DateTime($studentInfo->end_date);
    $end = $end->getTimestamp();

    // Start Page build
    $build = "";

    // Display student school information
    $build .= '<h2 class="section-header">Student Information</h2>';

    $build .= '<div class="row-fluid">'
        . '<div class="span3"><p><strong>Name</strong></p><p>' . $studentInfo->last_name . ', '. $studentInfo->first_name .'</p></div>'
        /*. '<div class="span2"><p><strong>First Name</strong></p><p>' . $studentInfo->first_name . '</p></div>'*/
        . '<div class="span2"><p><strong>Onid</strong></p><p>' . $studentInfo->onid . '</p></div>'
        . '<div class="span3"><p><strong>Status</strong></p><p>' . $studentInfo->registration_status . '</p></div>'
        . '<div class="span3"><p><strong>Email</strong></p><p>' . $studentInfo->email . '</p></div>'
        . '</div>';


    // Selecting post for students
    $result = db_select('node', 'n');

    $result->fields('n', array('nid', 'title', 'type'))
        ->fields('w', array('is_wordsalad', 'percent', 'score'))
        ->fields('wc', array('wordcount', 'timewriting', 'wordsperminute'))
        ->condition('type', 'writing365')
        ->condition('status', 1)
        ->condition('uid', $uid)
        ->condition('created', array($start, $end), 'BETWEEN')
        ->addTag('node_access')
        ->addField('n', 'created', 'date');

    $result->join('wordsalad_score', 'w', 'w.nid = n.nid');
    $result->join('W365_JournalNodeData', 'wc', 'wc.nid = n.nid');

    // Execute the query
    $studentPosts = $result->execute()->fetchAll();

    // Student Post Stats
    $wordTotal = 0;  // total number of words for the entire term.
    $entryTotal = 0; // total number of entries for the entire term.
    $wordPerEntry = 0; // average words per entry.
    $wordSalad = 0; // number of WordSalad post

    // Gathering Information on student post
    foreach ($studentPosts as $key => $post) {
        // Formatting Date
        if (property_exists($post, 'date')) {
            // January = 0 not 1
            $day = date('d', $post->date);
            $month = date('m', $post->date);
            $year = date('Y', $post->date);
            // Puts months in javascript date format
            // January = 0 != 1
            if ($month - 1 < 0) {
                $month = 11;
            } else {
                $month--;
            }
            // Add a 0 for months less then 10
            if ($month < 10) {
                $month = '0' . $month;
            }
            $studentPosts[$key]->date = $year . '-' . $month . '-' . $day;
        }

        // Count number of WordSalad post
        if ($post->is_wordsalad) {
            $wordSalad++;
        }

        // Tallies number of entries
        $entryTotal++;

        // Totaling word count
        $wordTotal += $post->wordcount;

    }

    // Gets total works per entry
    if ($entryTotal != 0) {
        $wordPerEntry = round($wordTotal / $entryTotal, 3);
    } else { // Defaults to 0.0, if $entryTotal is 0
        $wordPerEntry = 0.0;
    }


    // Displaying Post Statistics
    $build .= '<h4 class="section-header">Student Post Statistics</h4>';

    $build .= '<div class="row-fluid">'
        . '<div class="span3"><p><strong>Word Total</strong></p><p>' . $wordTotal . '</p></div>'
        . '<div class="span3"><p><strong>Entries Total</strong></p><p>' . $entryTotal . '</p></div>'
        . '<div class="span3"><p><strong>Words/Entries</strong></p><p>' . $wordPerEntry . '</p></div>'
        . '<div class="span3"><p><strong>WorldSalad Count</strong></p><p>' . $wordSalad . '</p></div>'
        . '</div>';


    // TACAL resources
    // Loaded from Write365
    // If not installed add js load for TACAL here.

    // Custom TACAL js
    drupal_add_js(drupal_get_path('module', 'reports_registration') . '/js/student-detail.js', 'file');
    drupal_add_js(array('reportsregistration' => array('studentid' => $uid, 'studentpost' => json_encode($studentPosts))), array('type' => 'setting'));

    // Displaying  TACAL
    $build .= '<h4 class="section-header">Term Calendar</h4>';
    $build .= '<div id="' . $uid . '"></div>';

    // Displaying student post
    $build .= '<h4 class="section-header">Student Post</h4>';
    $build .= "<p><i class=\" fa fa-exclamation-triangle fa-2x\" style=\"color:#f08080;\" title=\"Exclamation Triangle\"></i>&nbsp;&nbsp;A WordSalad value indicating that the journal entry is <u>probably not</u> structured English.<br />";
    $build .= "<i class=\" fa fa-check fa-2x\" style=\"color:#90ee90;\" title=\"Check Mark\"></i>&nbsp;&nbsp;A WordSalad value indicating that the journal entry is <u>probably</u> structured English.</p>";


    // Configure the table header columns
    $header = array(
        array('data' => 'Node Id', 'field' => 'nid'),
        array('data' => 'Title', 'field' => 'title', 'sort' => 'ASC'),
        array('data' => 'Created', 'field' => 'created', 'sort' => 'ASC'),
        array('data' => 'Status', 'field' => 'is_wordsalad'),
        array('data' => 'Wordcount', 'field' => 'wordcount'),
    );

    $result->extend('TableSort')
        ->orderByHeader($header); //then use an extender to create the sorted table
    $results = $result->execute();

    // Creating rows for table
    $rows = array();

    foreach ($results as $row) {
        if ($row->is_wordsalad) {
            $wordsalad = '<i class=" fa fa-exclamation-triangle fa-2x" style="color:#f08080;" title="Exclamation Triangle"></i>';
        } else {
            $wordsalad = '<i class=" fa fa-check fa-2x" style="color:#90ee90;" title="Check Mark"></i>';
        }
        $rows[] = array(
            "nid" => $row->nid,
            "title" => "<a href='/node/" . $row->nid . "''/>" . $row->title . "</a>",
            "created" => date('F j, Y, g:i a', $row->date),
            "is_wordsalad" => $wordsalad,
            "wordcount" => $row->wordcount,
        );
    }

    $build .= theme('table', array('header' => $header, 'rows' => $rows));

    // Add the pager
    $build .= theme('pager');

    return $build;

}

