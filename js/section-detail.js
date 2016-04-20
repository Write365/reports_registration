/**
 * Created by: Nathan Healea
 * Project: report_registration
 * File:
 * Date: 4/19/16
 * Time: 9:47 AM
 */
$(document).ready(function () {
    
    var classInfo = JSON.parse(Drupal.settings.reportsregistration.classinfo);
    var start = new Date(Drupal.settings.reportsregistration.startdate);
    var end = new Date(Drupal.settings.reportsregistration.enddate);
    var classKeys = Object.keys(classInfo);
    var calendars = [];

    for(var key in classKeys){
        var options ={
            id: classKeys[key],
            event: classInfo[classKeys[key]]
        };
        var cal = new TACAL(options);
        cal.linearMonth(start, end);
        calendars[classKeys[key]] = cal;
    }

});