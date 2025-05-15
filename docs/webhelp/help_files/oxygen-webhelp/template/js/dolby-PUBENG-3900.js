define(["jquery"], function ($) {
        $(document).ready(function () {
        $('.sort-table').DataTable({
            paging: false,
            searching: false,
            info: false
        });
    });
});