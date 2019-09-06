'use strict';

var fs = require('fs');
var pozyx = {
    pozyxpy: undefined,
    pozyxMode: false,
    pozyxWorker: {
        positioning: undefined,
        startPositioning: undefined,
        stopPositioning: undefined,
        errDialogueOpen: false
    }
};

TABS.pozyx = {};
TABS.pozyx.isYmapLoad = false;
TABS.pozyx.initialize = function (callback) {
    if (GUI.active_tab != 'pozyx') {
        GUI.active_tab = 'pozyx';
        googleAnalytics.sendAppView('Pozyx');
    }

    var startPositioning = function () {
        pozyx.pozyxWorker.positioning = setInterval(function () {
            pozyx.pozyxpy
                .getPosition()
                .then(data => {
                    //console.log(data);
                    if (data.error) {
                        stopPositioning(() => {
                        });
                        if (!pozyx.pozyxWorker.errDialogueOpen) {
                            confirm(data.error);
                        }
                        pozyx.pozyxWorker.errDialogueOpen = true;
                    } else {
                        GPS_DATA.lat = parseFloat(POZYX.anchors[0].lat) + data.y/POZYX.earthRadius * 180.0/Math.PI;
                        GPS_DATA.lon = parseFloat(POZYX.anchors[0].lon) + data.x/POZYX.earthRadius * 180.0/Math.PI / Math.cos(GPS_DATA.lat * 180.0/Math.PI);
                    }
                })
                .catch(err => GUI.log(err + ''));
        }, 20);
    };

    var stopPositioning = function (callback) {
        if (pozyx.pozyxWorker.positioning) {
            clearInterval(pozyx.pozyxWorker.positioning);
            GUI.log('[uniks] Waiting for positioning to stop...');
            // wait 2 seconds so that all running positionings can stop
            setTimeout(() => {
                GUI.log('[uniks] Ready to communicate with FC.');
                delete pozyx.pozyxWorker.positioning;
                callback();
            }, 2000);
        } else {
            GUI.log('[uniks] Postioning already stopped.');
            callback();
        }
    };
    //
    // var updateTagIds = function () {
    //     pozyx.pozyxpy
    //         .getTagIds()
    //         .then(tagIds => {
    //             const tagList = $('#tag_select');
    //             tagList.empty();
    //             tagList.append('<option value="1">None</option>');
    //             tagIds.forEach(tId => {
    //                 tagList.append(
    //                     `<option value="${tId}">0x${tId.toString(16)}</option>`
    //                 );
    //             });
    //         })
    //         .catch(err => GUI.log(err + ''));
    // };

    pozyx.pozyxpy = new PozyxPy();
    pozyx.pozyxWorker.startPositioning = startPositioning;
    pozyx.pozyxWorker.stopPositioning = stopPositioning;

    //updateTagIds();

    if (!pozyx.pozyxWorker.positioning) {
        startPositioning();
    }

    var loadChainer = new MSPChainerClass();
    loadChainer.setChain([mspHelper.getMissionInfo]);
    loadChainer.setExitPoint(loadHtml);
    loadChainer.execute();

    function updateTotalInfo() {
        $('#availablePoints').text(
            MISSION_PLANER.countBusyPoints + '/' + MISSION_PLANER.maxWaypoints
        );
        $('#missionValid').html(
            MISSION_PLANER.isValidMission
                ? chrome.i18n.getMessage('armingCheckPass')
                : chrome.i18n.getMessage('armingCheckFail')
        );
    }

    function loadHtml() {
        $('#content').load('./tabs/pozyx.html', process_html);
    }

    function process_html() {
        if (typeof require !== 'undefined') {
            chrome.storage.local.get('missionPlanerSettings', function (result) {
                if (result.missionPlanerSettings) {
                    $('#MPdefaultPointAlt').val(result.missionPlanerSettings.alt);
                    $('#MPdefaultPointSpeed').val(result.missionPlanerSettings.speed);
                } else {
                    chrome.storage.local.set({
                        missionPlanerSettings: {speed: 0, alt: 5000}
                    });
                    $('#MPdefaultPointAlt').val(5000);
                    $('#MPdefaultPointSpeed').val(0);
                }
            });

            initMap();
        } else {
            $('#missionMap, #missionControls').hide();
            $('#notLoadMap').show();
        }
        localize();

        function get_raw_gps_pozyx_data() {
            MSP.send_message(MSPCodes.MSP_RAW_GPS, false, false, get_comp_gps_data);
        }

        function get_comp_gps_data() {
            MSP.send_message(
                MSPCodes.MSP_COMP_GPS,
                false,
                false,
                get_gpsstatistics_data
            );
        }

        function get_gpsstatistics_data() {
            MSP.send_message(MSPCodes.MSP_GPSSTATISTICS, false, false, update_ui);
        }

        function update_ui() {
            let showPosition = function () {
                let iconFeature = new ol.Feature({
                    geometry: new ol.geom.Point(newCenter)
                });

                iconFeature.setStyle(getPointIcon(false, 1));

                let vectorSource = new ol.source.Vector({
                    features: [iconFeature]
                });

                let vectorLayer = new ol.layer.Vector({
                    source: vectorSource
                });

                // TODO[uniks] make position layer not clickable
                map.removeLayer(positionLayer);
                positionLayer = vectorLayer;
                map.addLayer(positionLayer);
            };

            let showAnchors = function () {
                let anchorsFeatures = new Array(5);
                let anchorDefaultLat = parseFloat(POZYX.anchors[0].lat);
                let anchorDefaultLon = parseFloat(POZYX.anchors[0].lon);
                for (var i = 0; i < 5; ++i) {
                    // TODO uniks scale of anchors' coordinates looks wrong
                    let anchor_x = POZYX.anchors[i].Coordinates[0];
                    let anchor_y = POZYX.anchors[i].Coordinates[1];
                    let anchor_z = POZYX.anchors[i].Coordinates[2] / 200;
                    
                    console.log(lat + "   " + lon);
                    let anchorLat = anchorDefaultLat + (anchor_y/POZYX.earthRadius) * 180.0/Math.PI;
	                let anchorLon = anchorDefaultLon + (anchor_x/POZYX.earthRadius) * 180.0/Math.PI / Math.cos(anchorLat * 180.0/Math.PI);

                    anchorsFeatures[i] = new ol.Feature({
                        geometry: new ol.geom.Point(
                            ol.proj.fromLonLat([
                                anchorLon,
                                anchorLat
                            ])
                        ),
                        //'geometry': new ol.geom.Point([newCenter[0] + anchor_x*factor, newCenter[1] + anchor_y*factor]),
                        i: i,
                        id: POZYX.anchors[i].id,
                        size: Math.max(4, Math.abs(anchor_z))
                    });
                }

                let vectorSource = new ol.source.Vector({
                    features: anchorsFeatures,
                    wrapX: false
                });

                let vectorLayer = new ol.layer.Vector({
                    source: vectorSource,
                    style: function (feature) {
                        let style = new ol.style.Style({
                            image: new ol.style.Circle({
                                radius: feature.get('size'),
                                fill: new ol.style.Fill({color: 'red'}),
                                stroke: new ol.style.Stroke({color: 'black', width: 1})
                            })
                        });
                        return style;
                    }
                });

                map.removeLayer(anchorLayer);
                anchorLayer = vectorLayer;
                map.addLayer(anchorLayer);
            };

            if (GPS_DATA.fix > 0) {
                $('#loadmap').show();
                $('#waiting').hide();
            } else {
                $('#loadmap').hide();
                $('#waiting').show();
            }

            var gpsFixType = chrome.i18n.getMessage('gpsFixNone');
            if (GPS_DATA.fix >= 2) gpsFixType = chrome.i18n.getMessage('gpsFix3D');
            else if (GPS_DATA.fix >= 1)
                gpsFixType = chrome.i18n.getMessage('gpsFix2D');

            var lat = GPS_DATA.lat;
            var lon = GPS_DATA.lon;

            $('.GPS_info td.fix').html(gpsFixType);
            $('.GPS_info td.alt').text(GPS_DATA.alt + ' m');
            $('.GPS_info td.lat').text(lat.toFixed(7) + ' deg');
            $('.GPS_info td.lon').text(lon.toFixed(7) + ' deg');
            $('.GPS_info td.speed').text(GPS_DATA.speed + ' cm/s');
            $('.GPS_info td.distToHome').text(GPS_DATA.distanceToHome + ' m');

            var gpsRate = 0;
            if (GPS_DATA.messageDt > 0) {
                gpsRate = 1000 / GPS_DATA.messageDt;
            }

            $('.GPS_stat td.messages').text(GPS_DATA.packetCount);
            $('.GPS_stat td.rate').text(gpsRate.toFixed(1) + ' Hz');
            $('.GPS_stat td.errors').text(GPS_DATA.errors);
            $('.GPS_stat td.timeouts').text(GPS_DATA.timeouts);
            $('.GPS_stat td.eph').text((GPS_DATA.eph / 100).toFixed(2) + ' m');
            $('.GPS_stat td.epv').text((GPS_DATA.epv / 100).toFixed(2) + ' m');
            $('.GPS_stat td.hdop').text((GPS_DATA.hdop / 100).toFixed(2));

            let oldCenter = map.getView().getCenter();
            let newCenter = ol.proj.fromLonLat([lon, lat]);

            let mapFollowDrone = $('#followdrone').is(':checked');

            if (
                oldCenter[0] !== newCenter[0] &&
                oldCenter[1] !== newCenter[1] &&
                mapFollowDrone
            ) {
                map.getView().setCenter(newCenter);
            }

            // update position every time the map is updated
            showPosition();

            if (!anchorLayer) {
                showAnchors();
            }
        }

        $('#followdrone').on('change', function () {
            update_ui();
        });

        /*
         * enable data pulling
         * GPS is usually refreshed at 200Hz on fc
         */
        // TODO uniks change update frequency for gps
        helper.mspBalancedInterval.add('gps_pull', 200, 3, function gps_update() {
            // avoid usage of the GPS commands until a GPS sensor is detected for targets that are compiled without GPS support.
            if (!have_sensor(CONFIG.activeSensors, 'gps')) {
                update_ui();
                return;
            }

            if (helper.mspQueue.shouldDrop()) {
                return;
            }

            get_raw_gps_pozyx_data();
        });
        GUI.content_ready(callback);
    }

    var markers = [];
    var lines = [];
    var map;
    var selectedMarker = null;
    var pointForSend = 0;
    var positionLayer = null;
    var anchorLayer = null;

    function clearEditForm() {
        $('#pointLat').val('');
        $('#pointLon').val('');
        $('#pointAlt').val('');
        $('#pointSpeed').val('');
        $('[name=pointNumber]').val('');
        $('#MPeditPoint').fadeOut(300);
    }

    function repaint() {
        var oldPos;
        for (var i in lines) {
            map.removeLayer(lines[i]);
        }
        lines = [];
        $('#missionDistance').text(0);

        map.getLayers().forEach(function (t) {
            //feature.getGeometry().getType()
            if (t instanceof ol.layer.Vector && typeof t.alt !== 'undefined') {
                var geometry = t
                    .getSource()
                    .getFeatures()[0]
                    .getGeometry();
                if (typeof oldPos !== 'undefined') {
                    paintLine(oldPos, geometry.getCoordinates());
                }

                oldPos = geometry.getCoordinates();
            }
        });
    }

    function paintLine(pos1, pos2) {
        var line = new ol.geom.LineString([pos1, pos2]);

        var feature = new ol.Feature({
            geometry: line
        });
        feature.setStyle(
            new ol.style.Style({
                stroke: new ol.style.Stroke({
                    color: '#1497f1',
                    width: 3
                })
            })
        );

        var vectorSource = new ol.source.Vector({
            features: [feature]
        });

        var vectorLayer = new ol.layer.Vector({
            source: vectorSource
        });

        lines.push(vectorLayer);

        var length =
            ol.Sphere.getLength(line) + parseFloat($('#missionDistance').text());
        $('#missionDistance').text(length.toFixed(3));

        map.addLayer(vectorLayer);
    }

    function getPointIcon(isEdit, id) {
        let offsetX = 0;
        if (!id) id = '';
        else {
            offsetX = id >= 10 ? -1 : 0;
            id = id.toString();
        }

        return new ol.style.Style({
            image: new ol.style.Icon({
                anchor: [0.5, 1],
                opacity: 1,
                scale: 0.5,
                src:
                    '../images/icons/cf_icon_position' + (isEdit ? '_edit' : '') + '.png'
            }),
            text: new ol.style.Text({
                text: id,
                offsetX: offsetX,
                offsetY: -12,
                overflow: true,
                scale: 1,
                fill: new ol.style.Fill({
                    color: 'black'
                })
            })
        });
    }

    function addMarker(_pos, _alt, _action, _speed) {
        var iconFeature = new ol.Feature({
            geometry: new ol.geom.Point(_pos),
            name: 'Null Island',
            population: 4000,
            rainfall: 500
        });

        iconFeature.setStyle(getPointIcon());

        var vectorSource = new ol.source.Vector({
            features: [iconFeature]
        });

        var vectorLayer = new ol.layer.Vector({
            source: vectorSource
        });

        vectorLayer.alt = _alt;
        vectorLayer.number = markers.length;
        vectorLayer.action = _action;
        vectorLayer.speedValue = _speed;

        markers.push(vectorLayer);

        return vectorLayer;
    }

    function initMap() {
        var app = {};

        /**
         * @constructor
         * @extends {ol.interaction.Pointer}
         */
        app.Drag = function () {
            ol.interaction.Pointer.call(this, {
                handleDownEvent: app.Drag.prototype.handleDownEvent,
                handleDragEvent: app.Drag.prototype.handleDragEvent,
                handleMoveEvent: app.Drag.prototype.handleMoveEvent,
                handleUpEvent: app.Drag.prototype.handleUpEvent
            });

            /**
             * @type {ol.Pixel}
             * @private
             */
            this.coordinate_ = null;

            /**
             * @type {string|undefined}
             * @private
             */
            this.cursor_ = 'pointer';

            /**
             * @type {ol.Feature}
             * @private
             */
            this.feature_ = null;

            /**
             * @type {string|undefined}
             * @private
             */
            this.previousCursor_ = undefined;
        };
        ol.inherits(app.Drag, ol.interaction.Pointer);

        /**
         * @constructor
         * @extends {ol.control.Control}
         * @param {Object=} opt_options Control options.
         */
        app.PlannerSettingsControl = function (opt_options) {
            var options = opt_options || {};
            var button = document.createElement('button');

            button.innerHTML = ' ';
            button.style =
                "background: url('../images/CF_settings_white.svg') no-repeat 1px -1px;background-color: rgba(0,60,136,.5);";

            var handleShowSettings = function () {
                $('#MPeditPoint, #missionPlanerTotalInfo').hide();
                $('#missionPlanerSettings').fadeIn(300);
            };

            var handleChangePozyxSettings = function () {
                $('#savePozyxAnchorSettings').removeClass('is-hidden');
            };

            $('#savePozyxAnchorSettings').on('click', function () {
                // save anchor settings in resource/pozyx-settings.json
                $('#savePozyxAnchorSettings').addClass('is-hidden');
                let data = $('.pozyxAnchorSettingData');
                for (i = 0; i < data.length; i++) {
                    //write values to POZYX json object
                    let newData = data[i].textContent;
                    let nrData = parseFloat(newData);
                    if (newData.length === parseFloat(nrData).length) newData = nrData;
                    POZYX.anchors[0][
                        data[i].className
                            .split(' ')[0]
                            .replace('pozyxAnchor', '')
                            .toLowerCase()
                        ] = newData;
                }

                let newPozyxSettings = JSON.stringify(POZYX);
                fs.writeFile(
                    './resources/pozyx-settings.json',
                    newPozyxSettings,
                    err => {
                        if (err) {
                            console.log(err);
                            throw err;
                        }
                        console.log('Data written to file');
                    }
                );
            });

            // add change listener on pozyx settings
            let data = document.getElementsByClassName('pozyxAnchorSettingData');
            for (i = 0; i < data.length; i++) {
                data[i].addEventListener('input', handleChangePozyxSettings);
            }

            button.addEventListener('click', handleShowSettings, false);
            button.addEventListener('touchstart', handleShowSettings, false);

            var element = document.createElement('div');
            element.className = 'mission-control-settings ol-unselectable ol-control';
            element.appendChild(button);
            element.title = 'MP Settings';

            ol.control.Control.call(this, {
                element: element,
                target: options.target
            });
        };
        ol.inherits(app.PlannerSettingsControl, ol.control.Control);

        /**
         * @param {ol.MapBrowserEvent} evt Map browser event.
         * @return {boolean} `true` to start the drag sequence.
         */
        app.Drag.prototype.handleDownEvent = function (evt) {
            var map = evt.map;

            var feature = map.forEachFeatureAtPixel(evt.pixel, function (
                feature,
                layer
            ) {
                return feature;
            });

            if (feature) {
                this.coordinate_ = evt.coordinate;
                this.feature_ = feature;
            }

            return !!feature;
        };

        /**
         * @param {ol.MapBrowserEvent} evt Map browser event.
         */
        app.Drag.prototype.handleDragEvent = function (evt) {
            var map = evt.map;

            var feature = map.forEachFeatureAtPixel(evt.pixel, function (
                feature,
                layer
            ) {
                return feature;
            });

            var deltaX = evt.coordinate[0] - this.coordinate_[0];
            var deltaY = evt.coordinate[1] - this.coordinate_[1];

            var geometry /** @type {ol.geom.SimpleGeometry} */ = this.feature_.getGeometry();
            geometry.translate(deltaX, deltaY);

            this.coordinate_[0] = evt.coordinate[0];
            this.coordinate_[1] = evt.coordinate[1];
            repaint();
        };

        /**
         * @param {ol.MapBrowserEvent} evt Event.
         */
        app.Drag.prototype.handleMoveEvent = function (evt) {
            if (this.cursor_) {
                var map = evt.map;
                var feature = map.forEachFeatureAtPixel(evt.pixel, function (
                    feature,
                    layer
                ) {
                    return feature;
                });
                var element = evt.map.getTargetElement();
                if (feature) {
                    if (element.style.cursor != this.cursor_) {
                        this.previousCursor_ = element.style.cursor;
                        element.style.cursor = this.cursor_;
                    }
                } else if (this.previousCursor_ !== undefined) {
                    element.style.cursor = this.previousCursor_;
                    this.previousCursor_ = undefined;
                }
            }
        };

        /**
         * @param {ol.MapBrowserEvent} evt Map browser event.
         * @return {boolean} `false` to stop the drag sequence.
         */
        app.Drag.prototype.handleUpEvent = function (evt) {
            this.coordinate_ = null;
            this.feature_ = null;
            return false;
        };

        if (!GPS_DATA) {
            // prepare data for map view without fc plugedd in
            GUI.log('FC not connected, returning static gps data...');
            FC.resetState();
        }

        map = new ol.Map({
            controls: ol.control
                .defaults({
                    attributionOptions: {
                        collapsible: false
                    }
                })
                .extend([new app.PlannerSettingsControl()]),
            interactions: ol.interaction.defaults().extend([new app.Drag()]),
            layers: [
                new ol.layer.Tile({
                    source: new ol.source.OSM()
                })
            ],
            target: document.getElementById('missionMap'),
            view: new ol.View({
                center: ol.proj.fromLonLat([GPS_DATA.lon, GPS_DATA.lat]),
                zoom: 20
            })
        });

        // Set the attribute link to open on an external browser window, so
        // it doesn't interfere with the configurator.
        var interval;
        interval = setInterval(function () {
            var anchor = $('.ol-attribution a');
            if (anchor.length) {
                anchor.attr('target', '_blank');
                clearInterval(interval);
            }
        }, 100);

        map.on('click', function (evt) {
            if (selectedMarker != null) {
                try {
                    selectedMarker
                        .getSource()
                        .getFeatures()[0]
                        .setStyle(getPointIcon());
                    selectedMarker = null;
                    clearEditForm();
                } catch (e) {
                    GUI.log(e + '');
                }
            }

            var selectedFeature = map.forEachFeatureAtPixel(evt.pixel, function (
                feature,
                layer
            ) {
                return feature;
            });
            selectedMarker = map.forEachFeatureAtPixel(evt.pixel, function (
                feature,
                layer
            ) {
                return layer;
            });
            if (selectedFeature) {
                var geometry = selectedFeature.getGeometry();
                var coord = ol.proj.toLonLat(geometry.getCoordinates());

                selectedFeature.setStyle(getPointIcon(true));

                $('#pointLon').val(coord[0]);
                $('#pointLat').val(coord[1]);
                $('#pointAlt').val(selectedMarker.alt);
                $('#pointType').val(selectedMarker.action);
                $('#pointSpeed').val(selectedMarker.speedValue);
                $('#MPeditPoint').fadeIn(300);
            } else {
                map.addLayer(
                    addMarker(
                        evt.coordinate,
                        $('#MPdefaultPointAlt').val(),
                        1,
                        $('#MPdefaultPointSpeed').val()
                    )
                );
                repaint();
            }
        });

        // change mouse cursor when over marker
        $(map.getViewport()).on('mousemove', function (e) {
            var pixel = map.getEventPixel(e.originalEvent);
            var hit = map.forEachFeatureAtPixel(pixel, function (feature, layer) {
                return true;
            });
            if (hit) {
                map.getTarget().style.cursor = 'pointer';
            } else {
                map.getTarget().style.cursor = '';
            }
        });

        $('#removeAllPoints').on('click', function () {
            if (confirm(chrome.i18n.getMessage('confirm_delete_all_points'))) {
                removeAllPoints();
            }
        });

        $('#removePoint').on('click', function () {
            if (selectedMarker) {
                var tmp = [];
                for (var i in markers) {
                    if (
                        markers[i] !== selectedMarker &&
                        typeof markers[i].action !== 'undefined'
                    ) {
                        tmp.push(markers[i]);
                    }
                }
                map.removeLayer(selectedMarker);
                markers = tmp;
                selectedMarker = null;

                clearEditForm();
                repaint();
            }
        });

        $('#savePoint').on('click', function () {
            if (selectedMarker) {
                map.getLayers().forEach(function (t) {
                    if (t === selectedMarker) {
                        var geometry = t
                            .getSource()
                            .getFeatures()[0]
                            .getGeometry();
                        geometry.setCoordinates(
                            ol.proj.fromLonLat([
                                parseFloat($('#pointLon').val()),
                                parseFloat($('#pointLat').val())
                            ])
                        );
                        t.alt = $('#pointAlt').val();
                        t.action = $('#pointType').val();
                        t.speedValue = $('#pointSpeed').val();
                    }
                });

                selectedMarker
                    .getSource()
                    .getFeatures()[0]
                    .setStyle(getPointIcon());
                selectedMarker = null;
                clearEditForm();
                repaint();
            }
        });

        // const tags = $('#tag_select');
        // tags.on('change', () => {
        //     stopPositioning(() => {
        //         GUI.log(tags.val());
        //         pozyx.pozyxpy.setRemoteId(tags.val())
        //             .then(data => {
        //                 if (data.error) {
        //                     console.log(data.error);
        //                 } else {
        //                     GUI.log(data.success);
        //                 }
        //             })
        //             .catch(err => GUI.log(err + ''));
        //         setTimeout(() => {
        //             console.log("XD");
        //             startPositioning();
        //         }, 1000);
        //     });
        // });

        $('#loadPOZYXMissionButton').on('click', function () {
            if (markers.length) {
                if (!confirm(chrome.i18n.getMessage('confirm_delete_all_points'))) {
                    return;
                }
                removeAllPoints();
            }
            GUI.log(chrome.i18n.getMessage('eeprom_load_ok'));

            loadPoints();
        });

        $('#savePOZYXMissionButton').on('click', function () {
            GUI.log(chrome.i18n.getMessage('pozyx_saved_ok'));
            $(this).addClass('disabled');
            GUI.log('Start send point');

            pointForSend = 0;
            sendNextPoint();
        });

        $('#showPozyxSettings').on('click', function () {
            var visible = $(this).data('visible');

            if (visible) {
                $('#pozyxAnchorSettingTable').addClass('is-hidden');

                visible = false;
            } else {
                // show pozyx anchor data
                $('.GPS_info td.pozyxAnchorId').text(POZYX.anchors[0].id);
                $('.GPS_info td.pozyxAnchorAlt').text(
                    parseInt(POZYX.anchors[0].alt) + ' m'
                );
                $('.GPS_info td.pozyxAnchorLat').text(
                    parseFloat(POZYX.anchors[0].lat).toFixed(7) + ' deg'
                );
                $('.GPS_info td.pozyxAnchorLon').text(
                    parseFloat(POZYX.anchors[0].lon).toFixed(7) + ' deg'
                );

                $('#pozyxAnchorSettingTable').removeClass('is-hidden');
                visible = true;
            }
            $(this).text(visible ? 'Hide' : 'Show');
            $(this).data('visible', visible);
        });

        $('#runPyScriptButton').on('click', function () {
            // TODO uniks
            GUI.log('button clicked');
        });

        $('#rthEndMission').on('change', function () {
            if ($(this).is(':checked')) {
                $('#rthSettings').fadeIn(300);
            } else {
                $('#rthSettings').fadeOut(300);
            }
        });

        $('#saveSettings').on('click', function () {
            chrome.storage.local.set({
                missionPlanerSettings: {
                    speed: $('#MPdefaultPointSpeed').val(),
                    alt: $('#MPdefaultPointAlt').val()
                }
            });
            $('#missionPlanerSettings').hide();
            $('#missionPlanerTotalInfo').fadeIn(300);
            if (selectedMarker !== null) {
                $('#MPeditPoint').fadeIn(300);
            }
        });

        updateTotalInfo();
    }

    function removeAllPoints() {
        for (var i in markers) {
            map.removeLayer(markers[i]);
        }
        markers = [];
        clearEditForm();
        repaint();
    }

    function loadPoints() {
        stopPositioning(() => {
            pointForSend = 0;
            MSP.send_message(MSPCodes.MSP_WP_MISSION_LOAD, [0], () => {
                MSP.send_message(MSPCodes.MSP_WP_GETINFO, false, false, getNextPoint);
            });
        });
    }

    function endGetPoint() {
        GUI.log('End get point');
        $('#loadMissionButton').removeClass('disabled');
        startPositioning();
        repaint();
        updateTotalInfo();
    }

    function getNextPoint() {
        if (MISSION_PLANER.countBusyPoints == 0) {
            endGetPoint();
            return;
        }

        if (pointForSend > 0) {
            if (MISSION_PLANER.bufferPoint.action == 4) {
                $('#rthEndMission').attr('checked', true);
                $('#rthSettings').fadeIn(300);
                if (MISSION_PLANER.bufferPoint.p1 > 0) {
                    $('#rthLanding').attr('checked', true);
                }
            } else {
                var coord = ol.proj.fromLonLat([
                    MISSION_PLANER.bufferPoint.lon,
                    MISSION_PLANER.bufferPoint.lat
                ]);
                map.addLayer(
                    addMarker(
                        coord,
                        MISSION_PLANER.bufferPoint.alt,
                        MISSION_PLANER.bufferPoint.action,
                        MISSION_PLANER.bufferPoint.p1
                    )
                );
                if (pointForSend === 1) {
                    map.getView().setCenter(coord);
                }
            }
        }

        if (pointForSend >= MISSION_PLANER.countBusyPoints) {
            endGetPoint();
            return;
        }

        MISSION_PLANER.bufferPoint.number = pointForSend;

        pointForSend++;

        MSP.send_message(
            MSPCodes.MSP_WP,
            mspHelper.crunch(MSPCodes.MSP_WP),
            false,
            getNextPoint
        );
    }

    function sendNextPoint() {
        stopPositioning(() => {
            GUI.log('[uniks] Sending WP to FC.');
            var isRTH = $('#rthEndMission').is(':checked');

            if (pointForSend >= markers.length) {
                if (isRTH) {
                    MISSION_PLANER.bufferPoint.number = pointForSend + 1;
                    MISSION_PLANER.bufferPoint.action = 4;
                    MISSION_PLANER.bufferPoint.lon = 0;
                    MISSION_PLANER.bufferPoint.lat = 0;
                    MISSION_PLANER.bufferPoint.alt = 0;
                    MISSION_PLANER.bufferPoint.endMission = 0xa5;
                    MISSION_PLANER.bufferPoint.p1 = $('#rthLanding').is(':checked')
                        ? 1
                        : 0;
                    MSP.send_message(
                        MSPCodes.MSP_SET_WP,
                        mspHelper.crunch(MSPCodes.MSP_SET_WP),
                        false,
                        endSendPoint
                    );
                } else {
                    endSendPoint();
                }

                return;
            }

            var geometry = markers[pointForSend]
                .getSource()
                .getFeatures()[0]
                .getGeometry();
            var coordinate = ol.proj.toLonLat(geometry.getCoordinates());

            // TODO uniks increase factor by factor of 10? Therefore increasing gps precision
            MISSION_PLANER.bufferPoint.number = pointForSend + 1;
            MISSION_PLANER.bufferPoint.action = markers[pointForSend].action;
            MISSION_PLANER.bufferPoint.lon = parseInt(coordinate[0] * 10000000);
            MISSION_PLANER.bufferPoint.lat = parseInt(coordinate[1] * 10000000);
            MISSION_PLANER.bufferPoint.alt = markers[pointForSend].alt;
            MISSION_PLANER.bufferPoint.p1 = markers[pointForSend].speedValue;
            pointForSend++;
            if (pointForSend >= markers.length && !isRTH) {
                MISSION_PLANER.bufferPoint.endMission = 0xa5;
            } else {
                MISSION_PLANER.bufferPoint.endMission = 0;
            }

            MSP.send_message(
                MSPCodes.MSP_SET_WP,
                mspHelper.crunch(MSPCodes.MSP_SET_WP),
                false,
                sendNextPoint
            );
        });
    }

    function endSendPoint() {
        GUI.log('End send point');

        MSP.send_message(MSPCodes.MSP_WP_GETINFO, false, false, () => {
            startPositioning();
            updateTotalInfo();
        });

        $('#savePOZYXMissionButton').removeClass('disabled');
    }
};

TABS.pozyx.cleanup = function (callback) {
    console.log(pozyx);

    pozyx.pozyxWorker.stopPositioning(() => {
        pozyx.pozyxpy.exit();
        if (callback) callback();
    });
};
