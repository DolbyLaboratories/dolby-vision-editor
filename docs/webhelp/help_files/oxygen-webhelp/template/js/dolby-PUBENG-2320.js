define(["jquery"], function ($) {
    $(document).ready(function () {
        console.log("Loading dolby-PUBENG-2320");
        if ( ( $( "#wh_topic_body" ).hasClass("header-toc") ) ) {
            // Isolating the topic icon class
            var classList = $("#wh_topic_body").attr("class");
            var classArr = classList.split("icons:");
            if (classArr[1]) {
                var classArr = classArr[1].split(" ");
                var topicClass = classArr[0];
            }
            else {
                var topicClass = '';
            }

            // Adding the icon to the header, if it exists
            if ( !(topicClass == '') ) {
                if ( !( $( "#wh_topic_body" ).hasClass("release-notes") ) ) {
                    $( "article h1" ).prepend( "<i class='material-icons'>" + topicClass + "</i>" );
                } else {
                    $( "#release-notes h1" ).prepend( "<i class='material-icons'>" + topicClass + "</i>" );
                }
            }

            // Adding a parent for the publication TOC clone
            $( ".wh_topic_content main" ).append("<div class='header-toc-children'></div>");

            // Cloning the publication TOC
            $( "[role=treeitem].active ul" ).clone().appendTo( ".header-toc-children" );

            // Adding icons to the publication TOC clone
            var iconNodes = $( ".header-toc-children div" );
            iconNodes.each(function() {
                    // Isolating the topic icon class
                    var classList = $(this).attr("class");
                    var classArr = classList.split("icons:");
                    if (classArr[1]) {
                        var classArr = classArr[1].split(" ");
                        var iconClass = classArr[0];
                    }
                    else {
                        // If there is no icon, we still add an empty element for consistent layout
                        var iconClass = 'x';
                    }
                    // Grabbing the href
                    var href = $(this).find("a").attr("href");
                    // Adding the icon with link
                    $(this).children(".title").before("<a href='" + href + "'><i class='material-icons'>" + iconClass + "</i></a>");
            });
            
            // Removing the redundant children links
            $( " .wh_child_links " ).remove();
            }
        });
    });