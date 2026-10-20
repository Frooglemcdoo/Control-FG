// Runs inside Control's Coherent view. All visible controls are OptionsTab's own
// components; this file adds data and translations, with the game's fonts and controls.
(function () {
    "use strict";
    var cfg = window.__controlFGConfig;
    var labels = Object.create(null), rows = null, mobx = null;
    function label(key, text) { labels[key] = text; return key; }
    function choice(key, value, text) {
        return {m_value: value, m_strLabelLocID: label(key + "_V" + value, text)};
    }
    var schema = window.__controlFGSchema;
    var definitions = schema.options, protocol = schema.protocol;
    var rowsByKey = Object.create(null), rowsById = Object.create(null);
    function makeRows() {
        return definitions.map(function (definition, slot) {
            var key = "CFG_" + definition.localizationKey;
            label(key, definition.label);
            label(key + "_INFO", definition.description);
            var row = {
                m_iPropertyID: definition.id,
                m_eDataType: definition.type === "choice" ? 3 : 1,
                m_strNameLocID: key,
                m_value: cfg.values[slot],
                m_defaultValue: cfg.defaults[slot],
                m_bIsDisabled: false,
                m_vecPossibleValues: [],
                m_minValue: definition.min || 0,
                m_maxValue: definition.max || 0,
                m_valueStep: definition.step || 1,
                cfgHeader: "",
                cfgDefinition: definition,
                cfgSlot: slot
            };
            if (definition.header) row.cfgHeader = label(key + "_HEADER", definition.header);
            row.m_vecPossibleValues = (definition.choices || []).filter(function (item) {
                return definition.capability !== "frameGeneration" || cfg.fgChoices.indexOf(item.value) !== -1;
            }).map(function (item) { return choice(key, item.value, item.label); });
            return row;
        }).filter(function (row) { return row.cfgDefinition.visibility !== "rtx40" || cfg.rtx40; });
    }
    function refresh() {
        rows.forEach(function (row) {
            row.m_bIsDisabled = (row.cfgDefinition.requires || []).some(function (dependency) {
                var source = rowsByKey[dependency.key];
                return !source || source.m_value !== dependency.value;
            });
        });
    }
    window.__controlFG = {
        rows: function (require) {
            if (rows) return rows;
            mobx = require("./node_modules/mobx/lib/mobx.module.js");
            rows = mobx.observable.array(makeRows());
            rows.forEach(function (row) {
                rowsByKey[row.cfgDefinition.key] = row;
                rowsById[row.m_iPropertyID] = row;
            });
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
            var incomingChoices = null, incomingDefaults = null;
            engine.on("CFG_CHOICES_BEGIN", function () { incomingChoices = []; });
            var fgRow = rowsByKey.FrameGeneration;
            fgRow.cfgDefinition.choices.forEach(function (item) {
                engine.on("CFG_CHOICE_FG_" + item.value, function () {
                    if (incomingChoices) incomingChoices.push(item.value);
                });
            });
            engine.on("CFG_CHOICES_END", function () {
                if (!incomingChoices) return;
                cfg.fgChoices = incomingChoices;
                mobx.runInAction(function () {
                    fgRow.m_vecPossibleValues = fgRow.cfgDefinition.choices.filter(function (item) {
                        return incomingChoices.indexOf(item.value) !== -1;
                    }).map(function (item) { return choice(fgRow.m_strNameLocID, item.value, item.label); });
                });
                incomingChoices = null;
            });
            engine.on("CFG_DEFAULT_BEGIN", function () { incomingDefaults = definitions.map(function () { return 0; }); });
            definitions.forEach(function (_, slot) {
                for (var bit = 0; bit < protocol.stateBits; ++bit) (function (b) {
                    engine.on("CFG_DEFAULT_" + slot + "_" + b, function () {
                        if (incomingDefaults) incomingDefaults[slot] |= (1 << b);
                    });
                }(bit));
            });
            engine.on("CFG_DEFAULT_END", function () {
                if (!incomingDefaults) return;
                cfg.defaults = incomingDefaults;
                mobx.runInAction(function () {
                    rows.forEach(function (row) { row.m_defaultValue = cfg.defaults[row.cfgSlot]; });
                });
                incomingDefaults = null;
            });
            var incoming = null;
            engine.on("CFG_STATE_BEGIN", function () { incoming = definitions.map(function () { return 0; }); });
            definitions.forEach(function (_, index) {
                for (var bit = 0; bit < protocol.stateBits; ++bit) (function (b) {
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
                        confirmed[id] = incoming[r.cfgSlot];
                        if (!pending[id].length) r.m_value = confirmed[id];
                    });
                    refresh();
                });
                incoming = null;
            });
            engine.trigger = function (event, id, value) {
                if ((event === "OnEnumGraphicsPropertySet" || event === "OnIntGraphicsPropertySet") && id >= protocol.firstId && id < protocol.endId) {
                    var row = rowsById[id];
                    if (!row || row.m_bIsDisabled || !Number.isInteger(value)) return;
                    if (row.m_eDataType === 3 ? !row.m_vecPossibleValues.some(function (v) { return v.m_value === value; }) : value < row.m_minValue || value > row.m_maxValue || (value - row.m_minValue) % row.m_valueStep !== 0) return;
                    pending[id].push(value);
                    mobx.runInAction(function () { row.m_value = value; refresh(); });
                } else if (event === "OnGraphicsReset") {
                    mobx.runInAction(function () {
                        rows.forEach(function (r) {
                            r.m_value = cfg.defaults[r.cfgSlot];
                            pending[r.m_iPropertyID].push(r.m_value);
                        });
                        refresh();
                    });
                }
                return original.apply(engine, arguments);
            };
            mobx.runInAction(refresh);
            original.call(engine, "OnEnumGraphicsPropertySet", protocol.handshakeId, protocol.version);
            return rows;
        },
        suffix: function (key) {
            var definition = definitions.find(function (option) { return "CFG_" + option.localizationKey === key; });
            return definition && definition.suffix || "";
        },
        translate: function (key) {
            // Unknown CFG descriptions are intentionally blank, never hotpink.
            return Object.prototype.hasOwnProperty.call(labels, key) ? labels[key] : "";
        }
    };
}());
