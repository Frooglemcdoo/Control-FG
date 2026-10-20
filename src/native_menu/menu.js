// Runs inside Control's Coherent view. All visible controls are OptionsTab's own
// components; this file adds data and translations, never CSS or DOM overlays.
(function () {
    "use strict";
    var cfg = window.__controlFGConfig;
    var labels = Object.create(null), rows = null, mobx = null;
    function label(key, text) { labels[key] = text; return key; }
    function choice(key, value, text) {
        return {m_value: value, m_strLabelLocID: label(key + "_V" + value, text)};
    }
    var definitions = [
        ["FG", "Frame Generation", "Generate additional frames to improve motion smoothness.", 3, 4,
            [[0,"Off"],[1,"Dynamic"],[2,"2x"],[3,"3x"],[4,"4x"],[5,"5x"],[6,"6x"]], "FRAME GENERATION"],
        ["TARGET_MODE", "Dynamic Frame Rate", "Automatic uses your display refresh rate. Manual uses the target below.", 3, 0, [[0,"Automatic"],[1,"Manual"]]],
        ["TARGET", "Target Frame Rate", "Target frame rate for Dynamic Frame Generation.", 1, 144, [30,1000,1]],
        ["RR", "Ray Reconstruction", "Use DLSS Ray Reconstruction to reconstruct ray-traced lighting and reflections.", 3, 0, [[0,"Off"],[1,"On"]], "RAY RECONSTRUCTION"],
        ["PRESET", "Ray Reconstruction Model", "Select the Ray Reconstruction model. Changing models while playing may increase video memory use.", 3, cfg.presetF, [[cfg.presetE,"Model E"],[cfg.presetF,"Model F"]]],
        ["CLAMP", "Reflection Detail", "Adjust reflection detail with Model F. Lower values look smoother; higher values retain more detail and noise. 60% is the recommended balance.", 1, 60, [25,75,1]],
        ["GI", "Global Illumination Samples", "Increase indirect lighting samples. Higher values may reduce performance. Native uses the game's sample count.", 3, 4, [[0,"Native"],[2,"2"],[4,"4"],[6,"6"],[8,"8"]]],
        ["GLASS", "Enhanced Glass Reflections", "Enhance reflections on glass surfaces. Requires native ray-traced transparent reflections.", 3, 1, [[0,"Off"],[1,"On"]], "REFLECTION ENHANCEMENTS"],
        ["TEXTURE", "Texture Streaming Budget", "Allow more time for texture streaming. Higher budgets can reduce texture pop-in but may cause traversal stutter.", 3, 0, [[0,"Native"],[4,"4 ms"],[6,"6 ms"],[8,"8 ms"]], "ADVANCED"],
        ["VRAM", "Video Memory Monitor", "Show the mod's video memory usage monitor during gameplay.", 3, 1, [[0,"Off"],[1,"On"]]],
        ["MFG40", "Experimental Multi Frame Generation", "Enable experimental Multi Frame Generation on RTX 40-series GPUs. Restart the game to apply this setting.", 3, 0, [[0,"Off"],[1,"On"]]]
    ];
    function makeRows() {
        return definitions.map(function (d, index) {
            var key = "CFG_" + d[0];
            label(key, d[1]); label(key + "_INFO", d[2]);
            var row = {m_iPropertyID: 10000 + index, m_eDataType: d[3],
                m_strNameLocID: key, m_value: cfg.values[index], m_defaultValue: d[4],
                m_bIsDisabled: false, m_vecPossibleValues: [], m_minValue: 0,
                m_maxValue: 0, m_valueStep: 1, cfgHeader: ""};
            if (d[6]) row.cfgHeader = label(key + "_HEADER", d[6]);
            if (d[3] === 3) row.m_vecPossibleValues = d[5].filter(function (v) {
                return index !== 0 || cfg.fgChoices.indexOf(v[0]) !== -1;
            }).map(function (v) { return choice(key, v[0], v[1]); });
            else { row.m_minValue = d[5][0]; row.m_maxValue = d[5][1]; row.m_valueStep = d[5][2]; }
            return row;
        }).filter(function (r) { return r.m_iPropertyID !== 10010 || cfg.rtx40; });
    }
    function refresh() {
        rows.forEach(function (r) {
            var id = r.m_iPropertyID;
            r.m_bIsDisabled = (id === 10001 && rows[0].m_value !== 1) ||
                (id === 10002 && (rows[0].m_value !== 1 || rows[1].m_value !== 1)) ||
                (id === 10005 && (rows[3].m_value !== 1 || rows[4].m_value !== cfg.presetF));
        });
    }
    window.__controlFG = {
        rows: function (require) {
            if (rows) return rows;
            mobx = require("./node_modules/mobx/lib/mobx.module.js");
            rows = mobx.observable.array(makeRows());
            var map = require("./northlight/LinkComponents/OptionsTab/OptionToDescriptionModeMap.ts");
            rows.forEach(function (r) {
                map.default[r.m_strNameLocID] = 2; // Native text-only description panel.
                map.NO_WARNING_WHEN_DISABLED[r.m_strNameLocID] = true;
            });
            var engine = require("northlight/vendor/coherent"), original = engine.trigger;
            var pending = Object.create(null), confirmed = Object.create(null);
            rows.forEach(function (r) {
                var id = r.m_iPropertyID;
                pending[id] = []; confirmed[id] = r.m_value;
                [false, true].forEach(function (success) {
                    engine.on("CFG_" + (success ? "OK_" : "FAIL_") + id, function () {
                        if (!pending[id].length) return;
                        var requested = pending[id].shift();
                        if (success) confirmed[id] = requested;
                        mobx.runInAction(function () {
                            if (!pending[id].length) r.m_value = confirmed[id];
                            refresh();
                        });
                    });
                });
            });
            // The persistent resource contains a startup snapshot. A fresh view
            // must read live native settings before treating that snapshot as current.
            var incoming = null;
            engine.on("CFG_STATE_BEGIN", function () { incoming = definitions.map(function () { return 0; }); });
            definitions.forEach(function (_, index) {
                for (var bit = 0; bit < 10; ++bit) (function (b) {
                    engine.on("CFG_STATE_" + index + "_" + b, function () {
                        if (incoming) incoming[index] |= (1 << b);
                    });
                }(bit));
            });
            engine.on("CFG_STATE_END", function () {
                if (!incoming) return;
                mobx.runInAction(function () {
                    rows.forEach(function (r) {
                        var id = r.m_iPropertyID;
                        confirmed[id] = incoming[id - 10000];
                        if (!pending[id].length) r.m_value = confirmed[id];
                    });
                    refresh();
                });
                incoming = null;
            });
            engine.trigger = function (event, id, value) {
                if ((event === "OnEnumGraphicsPropertySet" || event === "OnIntGraphicsPropertySet") && id >= 10000 && id <= 10010) {
                    var row = rows.find(function (r) { return r.m_iPropertyID === id; });
                    if (!row || row.m_bIsDisabled || !Number.isInteger(value)) return;
                    if (row.m_eDataType === 3 ? !row.m_vecPossibleValues.some(function (v) { return v.m_value === value; }) : value < row.m_minValue || value > row.m_maxValue) return;
                    pending[id].push(value);
                    mobx.runInAction(function () { row.m_value = value; refresh(); });
                } else if (event === "OnGraphicsReset") {
                    mobx.runInAction(function () {
                        rows.forEach(function (r) {
                            r.m_value = cfg.defaults[r.m_iPropertyID - 10000];
                            pending[r.m_iPropertyID].push(r.m_value);
                        });
                        refresh();
                    });
                }
                return original.apply(engine, arguments);
            };
            mobx.runInAction(refresh);
            original.call(engine, "OnEnumGraphicsPropertySet", 10999, 41);
            return rows;
        },
        translate: function (key) {
            // Unknown CFG descriptions are intentionally blank, never hotpink.
            return Object.prototype.hasOwnProperty.call(labels, key) ? labels[key] : "";
        }
    };
}());
