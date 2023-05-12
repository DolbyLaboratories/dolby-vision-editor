define(["jquery"], function ($) {
    $(document).ready(function () {
        /* This works together with a script added to the wt_topic templates which adds the fouc class to the wh_content_area div */
        /* The class needs to be added by javascript before the document is ready, and removed once it is ready */
        /* The class is added via JS instead of in the topic template as a progressive enhancement, so that the mechanism only fires if JS is enabled in the browser */
        /* The .fouc class has @visibility: hidden, hiding the FOUC (Flash Of Unstyled Content) when the browser uses JS */
        console.log("Loading dolby-PUBENG-2376-FOUC");
        $("#wh_topic_body").removeClass("fouc");
        });
    });