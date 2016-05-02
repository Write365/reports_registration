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

        $form = "<form name=\"courses\" action=\"course-detail\" method=\"post\">" .
            "<input type=\"hidden\" name=\"crs_crn\" value=\"" . $row->crn . "\">" .
            "<input type=\"hidden\" name=\"crs_name\" value=\"" . $row->course_name . "\">" .
            "<input type=\"hidden\" name=\"crs_section\" value=\"" . $row->section_number . "\">" .
            "<input type=\"hidden\" name=\"crs_instructor\" value=\"" . $row->instructor_name . "\">" .
            "<input type=\"hidden\" name=\"crs_term\" value=\"$row->term\">" .
            "<input type=\"hidden\" name=\"start_date\" value=\"$row->start_date\">" .
            "<input type=\"hidden\" name=\"end_date\" value=\"$row->end_date\">" .
            "<input type=\"submit\" value=\"View " . $row->crn . "\">" .
            "</form>";
        $rows[$row->crn]['crn'] = $form;
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

function course_detail()
{
    global $user;
// First the $_POST vars
    $crs_crn = htmlentities($_POST['crs_crn']);
    $crs_name = htmlentities($_POST['crs_name']);
    $crs_section = htmlentities($_POST['crs_section']);
    $crs_instructor = htmlentities($_POST['crs_instructor']);
    $term = htmlentities($_POST['crs_term']);
    $start = htmlentities($_POST['start_date']);
    $end = htmlentities($_POST['end_date']);

    // Converting DateTime to UnixTime
    $startUnix = new DateTime($start);
    $startUnix = $startUnix->getTimestamp();
    $endUnix = new DateTime($end);
    $endUnix = $endUnix->getTimestamp();

    $build = '';

    // Displaying class details
    $build .= '<h2 class="report-title">Course Details: ' . $crs_name . '</h2>';

    $build .= '<div class="row-fluid">'
        . '<div class="span2"><p><strong>Term</strong></p><p>' . $term . '</p></div>'
        . '<div class="span2"><p><strong>Section</strong></p><p>' . $crs_section . '</p></div>'
        . '<div class="span2"><p><strong>CRN</strong></p><p>' . $crs_crn . '</p></div>'
        . '<div class="span2"><p><strong>Instructor</strong></p><p>' . $crs_instructor . '</p></div>'
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
    $result = db_select('OSU_Registration_Student_Input', 's')/*->extend('PagerDefault')->limit(30)*/
    ;
    $result->fields('s', array('student_id', 'onid', 'last_name', 'first_name', 'email', 'registration_status'))
        ->fields('u', array('uid', 'name'))
        ->condition('term', $term)
        ->condition('crn', $crs_crn)
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

        // Creates a form for each student
        $form = "<form name=\"student\" action=\"student-detail\" method=\"post\">" .
            "<input type=\"hidden\" name=\"student_id\" value=\"" . $row->student_id . "\">" .
            "<input type=\"hidden\" name=\"uid\" value=\"" . $row->uid . "\">" .
            "<input type=\"hidden\" name=\"crs_term\" value=\"$term\">" .
            "<input type=\"hidden\" name=\"start_date\" value=\"$start\">" .
            "<input type=\"hidden\" name=\"end_date\" value=\"$end\">" .
            "<input class=\"btn\" type=\"submit\" value=\"View " . $row->student_id . "\">" .
            "</form>";

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
        $build .= '<p class="report-title">Student: ' . $row->last_name . ', ' . $row->first_name . '</p>';

        $build .= '<div class="row-fluid">'
            . '<div class="span2"><p><strong>Student Id</strong></p><p>' . $row->student_id . '</p></div>'
            . '<div class="span1"><p><strong>Onid</strong></p><p>' . $row->onid . '</p></div>'
            . '<div class="span3"><p><strong>Status</strong></p><p>' . $row->registration_status . '</p></div>'
            . '<div class="span3"><p><strong>Email</strong></p><p>' . $row->email . '</p></div>'
            . '<div class="span3"><p><strong>Details</strong></p><p>' . $form . '</p></div>'
            . '</div>';

        $build .= '<div id="' . $row->student_id . '"></div>';
        $build .= '</div>';
    }

    // TACAL resources
    // Loaded from Write365
    // If not installed add js load for TACAL here.


    // Custom js
    drupal_add_js(drupal_get_path('module', 'reports_registration') . '/js/section-detail.js', 'file');
    drupal_add_js(array('reportsregistration' => array('startdate' => $start, 'enddate' => $end, 'classinfo' => json_encode($classInfo))), array('type' => 'setting'));

    return $build;
}

function student_detail()
{
    /*
     * This is the report for an individual student.
     * Inputs: uid, first_name, email, onid
     */
    global $user;

    $term = htmlentities($_POST['crs_term']);
    $student_id = htmlentities($_POST['student_id']);
    $uid = htmlentities($_POST['uid']);
    $start = htmlentities($_POST['start_date']);
    $end = htmlentities($_POST['end_date']);

    // Converting DateTime to UnixTime
    $start = new DateTime($start);
    $start = $start->getTimestamp();
    $end = new DateTime($end);
    $end = $end->getTimestamp();

    $build = "";

    // Getting student Information
    $result = db_select('OSU_Registration_Student_Input', 's');
    $result->fields('s', array('last_name', 'first_name', 'student_id', 'email', 'registration_status', 'onid'))
        ->condition('student_id', $student_id);

    // Executing query
    $student = $result->execute()->fetch();

    // Display student school information
    $build .= '<p class="report-title">Student Information</p>';

    $build .= '<div class="row-fluid">'
        . '<div class="span2"><p><strong>Last Name</strong></p><p>' . $student->last_name . '</p></div>'
        . '<div class="span2"><p><strong>First Name</strong></p><p>' . $student->first_name . '</p></div>'
        . '<div class="span1"><p><strong>Onid</strong></p><p>' . $student->onid . '</p></div>'
        . '<div class="span3"><p><strong>Status</strong></p><p>' . $student->registration_status . '</p></div>'
        . '<div class="span3"><p><strong>Email</strong></p><p>' . $student->email . '</p></div>'
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
    $studentPosts = $result->execute()->fetch();

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

    // Gets average words per entry
    if ($entryTotal != 0) {
        $wordPerEntry = round($wordTotal / $entryTotal, 3);
    } else { // Defaults to 0.0, if $entryTotal is 0
        $wordPerEntry = 0.0;
    }


    // Displaying Post Statistics
    $build .= '<p class="report-title">Student Post Statistics</p>';

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

        $form = "<form name=\"courses\" action=\"adm-course-detail\" method=\"post\">" .
            "<input type=\"hidden\" name=\"crs_crn\" value=\"" . $row->crn . "\">" .
            "<input type=\"hidden\" name=\"crs_name\" value=\"" . $row->course_name . "\">" .
            "<input type=\"hidden\" name=\"crs_section\" value=\"" . $row->section_number . "\">" .
            "<input type=\"hidden\" name=\"crs_instructor\" value=\"" . $row->instructor_name . "\">" .
            "<input type=\"hidden\" name=\"crs_term\" value=\"$row->term\">" .
            "<input type=\"hidden\" name=\"start_date\" value=\"$row->start_date\">" .
            "<input type=\"hidden\" name=\"end_date\" value=\"$row->end_date\">" .
            "<input type=\"submit\" value=\"View " . $row->crn . "\">" .
            "</form>";
        $rows[$row->crn]['crn'] = $form;
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

function adm_course_detail()
{

    global $user;

    // First the $_POST vars
    $crs_crn = htmlentities($_POST['crs_crn']);
    $crs_name = htmlentities($_POST['crs_name']);
    $crs_section = htmlentities($_POST['crs_section']);
    $crs_instructor = htmlentities($_POST['crs_instructor']);
    $term = htmlentities($_POST['crs_term']);
    $start = htmlentities($_POST['start_date']);
    $end = htmlentities($_POST['end_date']);

    $title = $crs_name . "-" . $crs_section . "-" . $crs_crn . "-" . $term . "-" . $crs_instructor . "-" . date('Y-m-d H:i:s');

    $build = "<p>Course Detail: " . $title . "</p>";
    $build .= "<p>Term: " . $term . "</p>";
    $build .= "<p>CRN: " . $crs_crn . "</p>";

    $header = array(
        array('data' => 'Student ID', 'field' => 'student_id', 'sort' => 'desc'),
        array('data' => 'ONID', 'field' => 'onid', 'sort' => 'desc',),
        array('data' => 'Last Name', 'field' => 'last_name', 'sort' => 'desc',),
        array('data' => 'First Name', 'field' => 'first_name', 'sort' => 'desc',),
        array('data' => 'Email', 'field' => 'email', 'sort' => 'desc',),
        array('data' => 'Status', 'field' => 'registration_status', 'sort' => 'desc',),
    );

    $result = db_select('OSU_Registration_Student_Input', 's')->extend('PagerDefault')->limit(30);
    $result->fields('s', array('student_id', 'onid', 'last_name', 'first_name', 'email', 'registration_status'))
        ->fields('u', array('uid', 'name'))
        ->condition('term', $term)
        ->condition('crn', $crs_crn)
        ->addTag('node_access');
    $result->join('users', 'u', 's.onid = u.name');
    $result->extend('TableSort')
        ->orderByHeader($header); //then use an extender to create the sorted table

    $results = $result->execute();

    $rows = array();

    foreach ($results as $row) {

        $form = "<form name=\"student\" action=\"adm-student-detail\" method=\"post\">" .
            "<input type=\"hidden\" name=\"student_id\" value=\"" . $row->student_id . "\">" .
            "<input type=\"hidden\" name=\"uid\" value=\"" . $row->uid . "\">" .
            "<input type=\"hidden\" name=\"crs_term\" value=\"$term\">" .
            "<input type=\"hidden\" name=\"start_date\" value=\"$start\">" .
            "<input type=\"hidden\" name=\"end_date\" value=\"$end\">" .
            "<input type=\"submit\" value=\"View " . $row->student_id . "\">" .
            "</form>";

        $rows[] = array(
            "student_id" => $form,
            "onid" => $row->onid,
            "last_name" => $row->last_name,
            "first_name" => $row->first_name,
            "email" => $row->email,
            "registration_status" => $row->registration_status,
            //"uid" => $uid,
            //"name" => $name,
        );
    }

    $build .= theme('table', array('header' => $header, 'rows' => $rows));
    $build .= theme('pager');
    return $build;
}

function adm_student_detail()
{
    /*
     * This is the report for an individual student.
     * Inputs: uid, first_name, email, onid
     */
    global $user;

    $term = htmlentities($_POST['crs_term']);
    $student_id = htmlentities($_POST['student_id']);
    $uid = htmlentities($_POST['uid']);
    $start = htmlentities($_POST['start_date']);
    $end = htmlentities($_POST['end_date']);

    // Converting DateTime to UnixTime
    $start = new DateTime($start);
    $start = $start->getTimestamp();
    $end = new DateTime($end);
    $end = $end->getTimestamp();

    $build = "";

    // Getting student Information
    $result = db_select('OSU_Registration_Student_Input', 's');
    $result->fields('s', array('last_name', 'first_name', 'student_id', 'email', 'registration_status', 'onid'))
        ->condition('student_id', $student_id);

    // Executing query
    $student = $result->execute()->fetch();

    // Display student school information
    $build .= '<p class="report-title">Student Information</p>';

    $build .= '<div class="row-fluid">'
        . '<div class="span2"><p><strong>Last Name</strong></p><p>' . $student->last_name . '</p></div>'
        . '<div class="span2"><p><strong>First Name</strong></p><p>' . $student->first_name . '</p></div>'
        . '<div class="span1"><p><strong>Onid</strong></p><p>' . $student->onid . '</p></div>'
        . '<div class="span3"><p><strong>Status</strong></p><p>' . $student->registration_status . '</p></div>'
        . '<div class="span3"><p><strong>Email</strong></p><p>' . $student->email . '</p></div>'
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
    $build .= '<p class="report-title">Student Post Statistics</p>';

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
    $build .= '<p class="report-title">Term Calendar</p>';
    $build .= '<div id="' . $uid . '"></div>';

    // Displaying student post
    $build .= '<p class="report-title">Student Post</p>';
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

