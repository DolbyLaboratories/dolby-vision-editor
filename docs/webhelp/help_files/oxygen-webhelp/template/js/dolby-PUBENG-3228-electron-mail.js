define(["jquery"], function ($) {
    $(document).ready(function () {
        /* PUBENG-3228 detects when the browser is electron and removes @target="_blank" from mailto: links */
        
        var userAgent = navigator.userAgent.toLowerCase();
        if (userAgent.indexOf(' electron/') > -1) { // change electron to safari to test on a mac
           console.log("Loading dolby=PUBENG-3228");
           $( "a[href^='mailto']" ).removeAttr( "target" )
        }
    });
});