define(["options", "localization", "jquery"], function (options, i18n, $) {

// Based on expand.js from com.oxygenxml.webhelp.responsive
    
    var expandClass = "wh_expand_btn";

    $(document).ready(function () {
        /* Select every expand buttons and move it after its next sibling */
        var matchedNodes = Array.from(document.getElementsByClassName(expandClass));
        console.log("matchedNodes is " + matchedNodes); 
        matchedNodes.forEach(
            function (matchedNode) {
                /* First we remove any permalink spans */
                var permalinkNodes = Array.from(matchedNode.parentNode.getElementsByClassName('permalink'));
                console.log("permalinkNodes is " + permalinkNodes);
                permalinkNodes.forEach(
                    function (permalinkNode) {
                        permalinkNode.remove();
                    });
                
                /* Then we move the expand buttons */
                console.log("matchedNode is " + matchedNode);
                console.log("matchedNode.parentNode is " + matchedNode.parentNode);
                if (matchedNode.parentNode.tagName == "H1") {
                    /*  We don't want collapse buttons on H1 elements */
                    matchedNode.remove();
                } else if (matchedNode.parentNode.tagName == "CAPTION") {
                    /*  We want the collapse button immediately after the table title */
                    matchedNode.nextSibling.append(matchedNode);
                } else {
                    /*  for everything that is not an H1 we move the button */
                    matchedNode.parentNode.appendChild(matchedNode);
                }
        });
        console.log("Loading dolby-PUBENG-1824-expandbuttons");
    });
});