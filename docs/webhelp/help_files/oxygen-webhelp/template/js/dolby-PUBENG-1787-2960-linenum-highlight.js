// depends on highlight.js and hljs-linenums.js

define(["jquery"], function ($) {
    $(document).ready(function () {
        console.log("Loading dolby-PUBENG-1787-2960-linenum-highlight");
        
        // Grabbing all pre elements with .show-line-numbers
        var linedcode = document.querySelectorAll('pre.show-line-numbers');
        
        // Splits lines and adding the necessary classes 
        Array.prototype.forEach.call(linedcode, function(el, i) {
            
            // Isolating the language class from the pre
            var langclass = el.classList.value;
            langclass = langclass.replace("+ topic/pre pr-d/codeblock pre codeblock", "");
            langclass = langclass.replace("show-line-numbers", "");
            console.log(langclass);
            
            // Unwrapping the code lines
            el.innerHTML = el.innerHTML.replaceAll("<code>", "");
            el.innerHTML = el.innerHTML.replaceAll("</code>", "");
            
            // Adding a space before each line break so that we can keep empty lines after the innerHTML split
            el.innerHTML = el.innerHTML.replaceAll("\n\n", "\n \n");
            
            // Splitting code into lines
            var lines = el.innerHTML.split("\n");
            var content = '';
            
            // Wrapping lines and applying language class and class for line number pseudo-elements
            lines.forEach(function(item, i, array) {
                if (i === array.length - 1){ 
                    // This does not add a new line for the last line
                    content += '<code class="code__line ' + langclass + '">' + item + '</code>';
                    } else {
                    // The new line is needed so that the codeblock can be copied into the clipboard
                    content += '<code class="code__line ' + langclass + '">' + item + "\n" + '</code>';
                };
            });
            
            el.innerHTML = content;
        });
        
        // This calls highlight.js to apply code highlighting
        hljs.highlightAll();
        console.log("Calling highlight.js");
        
        // This adds linenums only to the codeblocks with .show-line-numbers
/*        $('pre.show-line-numbers > code.hljs').each(function(i, block) {
            hljs.lineNumbersBlock(block);
            console.log("Applying linenumbers");
        });*/
    });
});