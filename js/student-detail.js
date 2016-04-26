/**
 * Created by: Nathan Healea
 * Project: report_registration
 * File: render-calendar.js
 * Date: 4/13/16
 * Time: 11:42 AM
 */

(function ($) {
    
    $(document).ready(function() {
        var option = {
            id: Drupal.settings.reportsregistration.studentid,
            event: JSON.parse(Drupal.settings.reportsregistration.studentpost),
            show: ['date','wordcount', 'is_wordsalad']
        };
        var cal = new TACAL(option);
        cal.fullMonth();
    });
    
}) (jQuery);