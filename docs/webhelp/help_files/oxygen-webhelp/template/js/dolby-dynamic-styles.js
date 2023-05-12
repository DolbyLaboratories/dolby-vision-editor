/* Add Javascript to this file to apply custom Javascript styling to the Topic page templates. */

// define(["options", 'util', 'jquery', 'jquery.highlight'], function(options, util, $) {
//     // Add some Bootstrap classes when document is ready
//     var highlighted = false;

//     $(document).ready(function () {
//         var scrollPosition = $(window).scrollTop();
//         dolby_handleSideTocPosition(scrollPosition);
//         // handlePageTocPosition(scrollPosition);
//         console.log("dolby-dynamic-styles is loaded"); 
//     });
// });

/**
 * @description Handle the vertical position of the side toc
 */

//  function dolby_handleSideTocPosition(scrollPosition) {
//    var scrollPosition = scrollPosition !== undefined ? scrollPosition : 0;
//    var $sideToc = $(".wh_publication_toc");
//    var $sideTocID = $("#wh_publication_toc");
//    var $navSection = $(".wh_tools");
//    var bottomNavOffset = 0;
//    var $slideSection = $('#wh_topic_body');
//    var topOffset = 20;
//    var visibleAreaHeight = parseInt($(window).height()) - parseInt($(".wh_footer").outerHeight());

//    if ($sideToc.length > 0 && $slideSection.length > 0) {
//        var minVisibleOffset = $(window).scrollTop();
//        var tocHeight = parseInt($sideToc.height()) + parseInt($sideToc.css("padding-top")) + parseInt($sideToc.css("padding-bottom")) + parseInt($sideToc.css("margin-top")) + parseInt($sideToc.css("margin-bottom"));
//        var tocWidth = parseInt($sideTocID.outerWidth()) - parseInt($sideTocID.css("padding-left")) - parseInt($sideTocID.css("padding-right"));
//        var tocXNav = parseInt($slideSection.offset().left) - tocWidth;
   
//        if (scrollPosition > $(window).scrollTop()) {
//            if ($sideToc.offset().top < $sideToc.parent().offset().top) {
//                $sideToc.css('position', 'inherit');
//                console.log("sideToc.offset.top < sideToc.parent.offset.top");
//            }
//        } else {
//            if (tocHeight > $(window).height()) {
//                $sideToc.css('position', 'inherit');
//                console.log("tocHeight > window.height");

//            }
//        }

//        if ($navSection.length > 0) {
//            bottomNavOffset = parseInt($navSection.offset().top) + parseInt($navSection.height()) + parseInt($navSection.css("padding-top")) + parseInt($navSection.css("padding-bottom")) + parseInt($navSection.css("margin-top")) + parseInt($navSection.css("margin-bottom"));
//        }
//        if (bottomNavOffset > minVisibleOffset) {
//            minVisibleOffset = bottomNavOffset;
//        }
//        console.log("outside if"); 
//        if (tocHeight  <=   visibleAreaHeight) {
//             console.log("inside if");    
//             var cHeight = (parseInt($('.wh_content_area').height())+600);
//            if (parseInt(minVisibleOffset - topOffset) <=  $(window).scrollTop() && parseInt($(window).width()) > 767) {
//                $('.wh_content_area').css('min-height', cHeight+'px !important');
//                $sideToc.css("top", topOffset + "px").css("width", tocWidth + "px").css("position", "fixed").css("z-index", "999");
//                console.log("Dolby_handleSideTocPosition was here adding a min-height" + cHeight); 
//            } else {
//                $sideToc.removeAttr('style');
//            }
//        } else {
//            $sideToc.removeAttr('style');
//        }
//        scrollPosition = $(window).scrollTop();
//    }
//   console.log("Running dolby_handleSideTocPosition");
//   return $(window).scrollTop();
// }

/**
 * @description Handle the vertical position of the page toc
 */
// function handlePageTocPosition(scrollPosition) {
//    scrollPosition = scrollPosition !== undefined ? scrollPosition : 0;
//    var $pageTOCID = $("#wh_topic_toc");
//    var $pageTOC = $(".wh_topic_toc");
//    var $navSection = $(".wh_tools");
//    var bottomNavOffset = 0;
//    var topOffset = 33;
//    var $contentBody = $(".wh_topic_content");

//    if ($pageTOC.length > 0) {
//        pageTocHighlightNode(scrollPosition);
       
//        var visibleAreaHeight = parseInt($(window).height()) - parseInt($(".wh_footer").outerHeight());

//        var tocHeight = parseInt($pageTOC.height()) + parseInt($pageTOC.css("padding-top")) + parseInt($pageTOC.css("padding-bottom")) + parseInt($pageTOC.css("margin-top")) + parseInt($pageTOC.css("margin-bottom"));
//        var tocWidth =  parseInt($pageTOCID.outerWidth()) - parseInt($pageTOCID.css("padding-left")) - parseInt($pageTOCID.css("padding-right"));

//        var minVisibleOffset = $(window).scrollTop();
//        if ($navSection.length > 0) {
//            bottomNavOffset = parseInt($navSection.offset().top) + parseInt($navSection.height()) + parseInt($navSection.css("padding-top")) + parseInt($navSection.css("padding-bottom")) + parseInt($navSection.css("margin-top")) + parseInt($navSection.css("margin-bottom"));
//        }
//        if (bottomNavOffset > minVisibleOffset) {
//            minVisibleOffset = bottomNavOffset;
//        }

//        if ((tocHeight+topOffset) < visibleAreaHeight && (bottomNavOffset-topOffset) < $(window).scrollTop() && (tocHeight+topOffset) < $contentBody.height()) {
//            if (parseInt(minVisibleOffset - topOffset) <=  $(window).scrollTop() && parseInt($(window).width()) > 767) {
//                $pageTOC.css("top", "20px").css("position", "fixed").css("width", tocWidth + "px").css("height", tocHeight + "px");
//            } else {
//                $pageTOC.removeAttr('style');
//            }
//        } else {
//            $pageTOC.removeAttr('style');
//        }
//    }
// }

/**
 * @description Highlight the current node in the page toc section on page scroll or clicking on Topic TOC items 
 */
// function pageTocHighlightNode(scrollPosition) {
//    var scrollPosition = scrollPosition !== undefined ? Math.round(scrollPosition) : 0;
//    var topOffset = 150;
//    var hash = location.hash != undefined ? location.hash : "";
//    var hashOffTop = $(hash).offset() != undefined ? $(hash).offset().top : 0;
//    var elemHashTop =  hash != "" ? Math.round(hashOffTop) : 0;
   
//    if( hash.substr(1) != '' && elemHashTop >= scrollPosition && (elemHashTop <= (scrollPosition + topOffset)) ){
//        $('#wh_topic_toc a').removeClass('current_node');
//        $('#wh_topic_toc a[data-tocid = "'+ hash.substr(1) + '"]').addClass('current_node');
//    } else {
//        $.each( $('.wh_topic_content .title'), function (e) {
//            var currentId = $(this).parent().attr('id');
//            var elemTop = Math.round($(this).offset().top);
   
//            if( elemTop >= scrollPosition && (elemTop <= (scrollPosition + topOffset)) ){
//                $('#wh_topic_toc a').removeClass('current_node');
//                $('#wh_topic_toc a[data-tocid = "'+ currentId + '"]').addClass('current_node');
//            }  
//        });
//    }
//    return $(window).scrollTop();
// }

define(["jquery"], function ($) {
    $(document).ready(function () {
        // var scrollPosition = $(window).scrollTop();
        // Debugging whether this loads
        console.log("dolby-dynamic-styles was loaded");
        // dolby_handleSideTocPosition(scrollPosition);

    // PUBENG-1310, removing border from collapsed tables

        // var DolbyExpandTables = $(document).find('.wh_expand_btn');
        // DolbyExpandTables.click(function (event) {
            // var IsTable = ($(this).parentNode == caption);
            // toggleSubtopics.call(this);
            // if (IsTable) {
            // console.log("A .wh_expand_btn in a caption has been clicked");
            // };
        });
    });