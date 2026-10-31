// Runs inside Control's Coherent view. All visible controls are OptionsTab's own
// components; this file adds data and translations, with the game's fonts and controls.
(function () {
    "use strict";
    var cfg = window.__controlFGConfig;
    var labels = Object.create(null), rows = null, mobx = null;
    // Native slider labels need explicit vertical alignment; retain native typography.
    var sliderValueStyle = {display: "flex", alignItems: "center", justifyContent: "center", padding: 0};
    function label(key, text) { labels[key] = text; return key; }
    function choice(key, value, text) {
        return {m_value: value, m_strLabelLocID: label(key + "_V" + value, text)};
    }
    var schema = window.__controlFGSchema;
    var definitions = schema.options, protocol = schema.protocol;
    var nativeModel = null, nativeRequire = null;
    var rowsByKey = Object.create(null), rowsById = Object.create(null);
    var storageBusy = false, storageRestartIssued = false;
    label("CFG_STORAGE_ON", "DirectStorage: On");
    label("CFG_STORAGE_OFF", "DirectStorage: Off");
    label("CFG_STORAGE_UNAVAILABLE", "DirectStorage: Unavailable");
    label("CFG_STORAGE_ENABLE", "Enabling DirectStorage requires restarting Control. Restart now? Progress since the last checkpoint will be lost.");
    label("CFG_STORAGE_DISABLE", "Disabling DirectStorage requires restarting Control. Restart now? Progress since the last checkpoint will be lost.");
    function makeRows() {
        return definitions.map(function (definition, slot) {
            var key = "CFG_" + definition.localizationKey;
            label(key, definition.label);
            label(key + "_INFO", definition.description);
            var row = {
                m_iPropertyID: definition.id,
                m_eDataType: definition.presentation === "checkbox" ? 0 : definition.type === "choice" ? 3 : 1,
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
        }).filter(function (row) { return (row.cfgDefinition.visibility !== "rtx40" || cfg.rtx40); });
    }
    function nativeOptionEnabled(name) {
        if (!nativeModel) return false;
        var groups = ["m_vecBoolProperties", "m_vecEnumProperties", "m_vecIntProperties"];
        return groups.some(function (group) {
            return Array.from(nativeModel[group] || []).some(function (property) {
                return property.m_strNameLocID === name && !property.m_bIsDisabled && Number(property.m_value) > 0;
            });
        });
    }
    function refresh() {
        rows.forEach(function (row) {
            row.m_bIsDisabled = (row.cfgDefinition.key === "DirectStorage" && (!cfg.storageSupported || storageBusy)) || (row.cfgDefinition.requires || []).some(function (dependency) {
                var source = rowsByKey[dependency.key];
                return !source || source.m_value !== dependency.value;
            }) || (row.cfgDefinition.nativeRequires || []).some(function (name) { return !nativeOptionEnabled(name); });
        });
    }
    window.__controlFG = {
        rows: function (require, model) {
            nativeModel = model || null; nativeRequire = require;
            if (rows) { mobx.runInAction(refresh); return rows; }
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
            engine.on("CFG_STORAGE_RESTART_READY", function () {
                if (!storageBusy || storageRestartIssued) return;
                storageRestartIssued = true;
                original.call(engine, "OnExitClicked");
            });
            engine.on("CFG_STORAGE_RESTART_FAILED", function () {
                storageBusy = false;
                label("CFG_STORAGE_INFO", "Could not save the setting or prepare the restart. DirectStorage remains unchanged. Check that the complete mod package, including ControlFG-Restart.exe, is installed.");
                mobx.runInAction(refresh);
            });
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
                if ((event === "OnEnumGraphicsPropertySet" || event === "OnIntGraphicsPropertySet" || event === "OnBoolGraphicsPropertySet") && id >= protocol.firstId && id < protocol.endId) {
                    var row = rowsById[id];
                    if (row && row.cfgDefinition.presentation === "checkbox" && typeof value === "boolean") value = Number(value);
                    if (!row || row.m_bIsDisabled || !Number.isInteger(value)) return;
                    if (row.cfgDefinition.presentation === "slider") {
                        var selected = row.cfgDefinition.choices[value];
                        if (!selected) return;
                        value = selected.value;
                    }
                    if (row.cfgDefinition.type === "choice" ? !row.m_vecPossibleValues.some(function (v) { return v.m_value === value; }) : value < row.m_minValue || value > row.m_maxValue || (value - row.m_minValue) % row.m_valueStep !== 0) return;
                    if (row.cfgDefinition.presentation === "restartButton") {
                        if (value === row.m_value) return;
                        storageBusy = true;
                    }
                    pending[id].push(value);
                    mobx.runInAction(function () { row.m_value = value; refresh(); });
                } else if (event === "OnGraphicsReset") {
                    mobx.runInAction(function () {
                        rows.forEach(function (r) {
                            if (r.cfgDefinition.presentation === "restartButton") return;
                            r.m_value = cfg.defaults[r.cfgSlot];
                            pending[r.m_iPropertyID].push(r.m_value);
                        });
                        refresh();
                    });
                }
                if (rowsById[id] && (event === "OnEnumGraphicsPropertySet" || event === "OnIntGraphicsPropertySet" || event === "OnBoolGraphicsPropertySet"))
                    return original.call(engine, event === "OnBoolGraphicsPropertySet" ? "OnEnumGraphicsPropertySet" : event, id, value);
                return original.apply(engine, arguments);
            };
            mobx.runInAction(refresh);
            original.call(engine, "OnEnumGraphicsPropertySet", protocol.handshakeId, protocol.version);
            return rows;
        },
        order: function (allRows) {
            var nativeRows = allRows.filter(function (row) { return !row.cfgDefinition; });
            var customRows = allRows.filter(function (row) { return !!row.cfgDefinition; });
            var result = [], inserted = Object.create(null);
            nativeRows.forEach(function (row, index) {
                result.push(row);
                Object.keys(schema.layout.groups).forEach(function (group) {
                    var anchor = nativeRows.findIndex(function (item) {
                        return item.m_strNameLocID === schema.layout.groups[group];
                    });
                    var next = nativeRows[index + 1];
                    var boundary = !next || schema.layout.boundaries.indexOf(next.m_strNameLocID) !== -1;
                    if (anchor >= 0 && index >= anchor && boundary && !inserted[group]) {
                        customRows.forEach(function (custom) { if (custom.cfgDefinition.group === group) result.push(custom); });
                        inserted[group] = true;
                    }
                });
            });
            // Keep controls usable if a native group is absent; never discard rows.
            customRows.forEach(function (row) { if (!inserted[row.cfgDefinition.group]) result.push(row); });
            return result;
        },
        restartButton: function (row) {
            var require = nativeRequire;
            var react = require("./node_modules/react/index.js");
            var EventButton = require("./northlight/LinkComponents/EventButton/EventButton.tsx").default;
            var Text = require("./northlight/components/Text/Text.tsx").default;
            return react.createElement(EventButton, {
                key: "Graphics" + row.m_iPropertyID,
                event: "OnEnumGraphicsPropertySet", eventArgs: [row.m_iPropertyID, row.m_value ? 0 : 1],
                label: row.m_strNameLocID, descriptionMode: 2,
                className: "options-tile", width: 3, autoScroll: true,
                needsConfirm: true, useHold: false, isDisabled: row.m_bIsDisabled,
                confirmLocId: row.m_value ? "CFG_STORAGE_DISABLE" : "CFG_STORAGE_ENABLE"
            }, react.createElement(Text, {locId: !cfg.storageSupported ? "CFG_STORAGE_UNAVAILABLE" : row.m_value ? "CFG_STORAGE_ON" : "CFG_STORAGE_OFF", autoUpdate: true}));
        },
        slider: function (row) {
            var values = row.cfgDefinition.choices;
            var index = values.findIndex(function (item) { return item.value === row.m_value; });
            return Object.assign({}, row, {m_eDataType: 1, m_value: row.m_bIsDisabled ? 0 : Math.max(0, index),
                m_minValue: 0, m_maxValue: values.length - 1, m_valueStep: 1});
        },
        sliderValueStyle: function (row) {
            return row && row.cfgDefinition ? sliderValueStyle : undefined;
        },
        sliderLabels: function (row) {
            return row.cfgDefinition && row.cfgDefinition.presentation === "slider" ?
                row.cfgDefinition.choices.map(function (item) { return item.label; }) : [];
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
