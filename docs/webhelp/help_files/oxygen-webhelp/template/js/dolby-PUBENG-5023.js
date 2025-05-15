define(["jquery"], function ($) {
    $(document).ready(function () {
        console.log("Loading dolby-PUBENG-5023");
        const zoomableSVGs = document.querySelectorAll('figure object[type="image/svg+xml"]');
        // const zoomableIMGs = document.querySelectorAll('figure img');
        var zoomableImages = [];
        zoomableImages.push.apply(zoomableImages, zoomableSVGs);
        // zoomableImages.push.apply(zoomableImages, zoomableIMGs);
        console.log(zoomableImages);
        zoomableImages.forEach(e => {
            const anchor = document.createElement('a');
            const classes = 'material-icons image-zoom';
            anchor.textContent = 'fit_screen';
            anchor.classList.add(...classes.split(' '))
            anchor.setAttribute('title', 'Enlarge image to fit window');
            anchor.setAttribute('onclick', 'createLightbox(this);')
            e.parentNode.insertBefore(anchor, e);
            });
    });
});

function createLightbox(element) {
    var zoomedelement = element.nextElementSibling;

    var lightbox = document.createElement("div");
    lightbox.classList.add("lightbox");
    lightbox.classList.add("fade-in");

    var lightboxContent = document.createElement("div");
    lightboxContent.classList.add("lightbox-content");
    
    var lightboxClose = document.createElement("a");
    lightboxClose.classList.add("lightbox-close", "material-icons");
    lightboxClose.innerHTML += "close";
    lightboxClose.title += "Close image";
    lightboxClose.onclick = function(){this.parentNode.parentNode.classList.add("fade-out");setTimeout(() => {this.parentNode.parentNode.remove()}, 275)};
    
    lightboxContent.appendChild(lightboxClose);
    lightboxContent.appendChild(zoomedelement.cloneNode(true));
    lightbox.appendChild(lightboxContent);

    var mainElement = document.querySelector("html");
    mainElement.appendChild(lightbox);
};