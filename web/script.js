function initMap() {
    $.get('./info.json', function (info) {
        var mids = $.map(info.types, function (mtype) {
            return mtype.id;
        });

        var map = new google.maps.Map(document.getElementById('map'), {
            zoom: 0,
            center: new google.maps.LatLng(0, 0, false),
            backgroundColor: 'black',
            mapTypeId: mids[0],
            mapTypeControlOptions: {
                mapTypeIds: mids
            },
            streetViewControl: false
        });

        $.each(info.types, function (i, mtype) {
            console.log(mtype);
            map.mapTypes.set(mtype.id, new google.maps.ImageMapType({
                name: mtype.name,
                alt: mtype.name,
                isPng: true,
                opacity: 1,
                maxZoom: mtype.maxZoom,
                minZoom: mtype.minZoom,
                tileSize: new google.maps.Size(info.tileSize, info.tileSize),
                getTileUrl: function (point, zoom) {
                    return mtype.id + '/zoom' + zoom + '/' + point.x + '.' + point.y + '.png'
                }
            }));
        });

        $('.date').text(info.time);
        $('#note').show();
    });
}
