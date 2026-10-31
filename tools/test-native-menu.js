const fs = require('fs'), vm = require('vm'), assert = require('assert');
const schema = JSON.parse(fs.readFileSync('src/settings_schema.json', 'utf8'));
const script = fs.readFileSync('src/native_menu/menu.js', 'utf8');
if (process.argv[2] === '--config') {
    const native = JSON.parse(fs.readFileSync(process.argv[3], 'utf8'));
    assert.equal(native.values.length, schema.options.length);
    assert.deepEqual(native.defaults, schema.options.map(option => option.default));
    assert.deepEqual(native.fgChoices, schema.options.find(option => option.key === 'FrameGeneration').choices.map(choice => choice.value));
}
function exercise(rtx40) {
    const config = {rtx40,storageSupported:true,
        values:schema.options.map(o=>o.default),defaults:schema.options.map(o=>o.default),
        fgChoices:rtx40?[0,2]:[0,1,2,3,4,5,6]};
    if(rtx40) config.values[0]=config.defaults[0]=2;
    const liveValues=config.values.slice();
    config.fgChoices=[0]; // Startup resource can predate GPU capability discovery.
    config.values[0]=0; // Reopened view still receives the cached startup snapshot.
    const context={window:{__controlFGConfig:config,__controlFGSchema:schema},Number};
    vm.createContext(context);vm.runInContext(script,context);
    const events=[], handlers={};
    const engine={on(name,fn){handlers[name]=fn;},trigger(...args){
        events.push(args);
        if(args[1]===10999) {
            handlers.CFG_CHOICES_BEGIN();
            (rtx40?[0,2]:[0,1,2,3,4,5,6]).forEach(v=>handlers['CFG_CHOICE_FG_'+v]());
            handlers.CFG_CHOICES_END();
            handlers.CFG_DEFAULT_BEGIN();
            config.defaults.forEach((v,i)=>{for(let b=0;b<10;++b)if(v&(1<<b))handlers['CFG_DEFAULT_'+i+'_'+b]();});
            handlers.CFG_DEFAULT_END();
            handlers.CFG_STATE_BEGIN();
            liveValues.forEach((v,i)=>{for(let b=0;b<10;++b)if(v&(1<<b))handlers['CFG_STATE_'+i+'_'+b]();});
            handlers.CFG_STATE_END();
        }
        else if(args[0]==='OnGraphicsReset')Object.keys(handlers).filter(k=>k.startsWith('CFG_OK_')).forEach(k=>handlers[k]());
        else if(args[1]>=10000&&args[1]<=10011) {
            const key=(engine.reject?'CFG_FAIL_':'CFG_OK_')+args[1];
            if(handlers[key])handlers[key]();
        }
    }};
    const maps={default:{},NO_WARNING_WHEN_DISABLED:{}};
    const requireModule=name=>name.includes('mobx/lib')?{observable:{array:x=>x},runInAction:f=>f()}:
        name.includes('OptionToDescriptionModeMap')?maps:
        name.includes('react/index.js')?{createElement:(type,props,...children)=>({type,props,children})}:
        name.includes('EventButton.tsx')||name.includes('/Text.tsx')?{default:name}:engine;
    const nativeModel={m_vecBoolProperties:[
        {m_strNameLocID:'MENU_OPTIONS_RT_INDIRECT_DIFFUSE_QUALITY',m_value:true},
        {m_strNameLocID:'MENU_OPTIONS_RT_SPOT_CONTACT_SHADOW_QUALITY',m_value:false}
    ]};
    const menu=context.window.__controlFG, rows=menu.rows(requireModule,nativeModel);
    const byId=Object.fromEntries(rows.map(row=>[row.m_iPropertyID,row]));
    assert.equal(rows.length,rtx40?10:9);
    assert(!byId[10004] && !byId[10005]);
    [10003,10007,10009].concat(rtx40?[10010]:[]).forEach(id=>{
        assert.equal(byId[id].m_eDataType,0);
        const next=!byId[id].m_value;
        engine.trigger('OnBoolGraphicsPropertySet',id,next);
        assert.equal(byId[id].m_value,Number(next));
        assert.deepEqual(events.at(-1),['OnEnumGraphicsPropertySet',id,Number(next)]);
        engine.trigger('OnBoolGraphicsPropertySet',id,!next);
    });
    const style=menu.sliderValueStyle(byId[10006]);
    assert.equal(style.alignItems,'center');assert.equal(style.justifyContent,'center');
    assert.strictEqual(style,menu.sliderValueStyle(byId[10002]));
    assert.strictEqual(style,menu.sliderValueStyle(byId[10008]));
    assert.equal(menu.sliderValueStyle({}),undefined);
    events.splice(1); // Discard checkbox round-trip events; retain the initial handshake.
    assert.equal(byId[10000].m_vecPossibleValues.length,rtx40?2:7);
    assert.equal(byId[10000].m_value,liveValues[0]); // Live FG survives view recreation.
    assert.equal(byId[10000].cfgHeader,'');
    assert.equal(menu.translate('CFG_GLASS'),'Enhanced Glass Reflections');
    assert(!rows.some(r=>r.m_strNameLocID.includes('OPAQUE')));
    assert.equal(menu.rows(requireModule,nativeModel),rows);
    assert.equal(events.length,1); // One handshake, no duplicate event wrapping.
    rows.forEach(r=>{
        assert(menu.translate(r.m_strNameLocID));
        assert(menu.translate(r.m_strNameLocID+'_INFO'));
        assert.equal(maps.default[r.m_strNameLocID],2);
        r.m_vecPossibleValues.forEach(v=>assert(menu.translate(v.m_strLabelLocID)));
    });
    engine.trigger('OnEnumGraphicsPropertySet',10000,99);
    assert.equal(events.length,1);
    engine.trigger('OnBoolGraphicsPropertySet',10003,true);
    assert.deepEqual(events.at(-1),['OnEnumGraphicsPropertySet',10003,1]);
    assert.equal(byId[10003].m_eDataType,0);
    assert.equal(byId[10003].m_value,1);
    engine.trigger('OnIntGraphicsPropertySet',10005,75);
    assert(!byId[10005]);
    engine.reject=true;
    engine.trigger('OnIntGraphicsPropertySet',10008,2);
    assert.deepEqual(events.at(-1),['OnIntGraphicsPropertySet',10008,6]);
    assert.equal(byId[10008].m_value,0); // Rejected native writes roll back the displayed value.
    engine.reject=false;
    assert.equal(byId[10006].m_bIsDisabled,false); // Contact Shadows off does not disable GI.
    engine.trigger('OnIntGraphicsPropertySet',10006,3);
    assert.equal(byId[10006].m_value,6);
    assert.equal(menu.slider(byId[10006]).m_value,3);
    assert.deepEqual(Array.from(menu.sliderLabels(byId[10006])),['Native','2','4','6','8','12','16']);
    engine.trigger('OnIntGraphicsPropertySet',10006,6);
    assert.deepEqual(events.at(-1),['OnIntGraphicsPropertySet',10006,16]);
    assert.equal(byId[10006].m_value,16);
    assert.equal(menu.slider(byId[10006]).m_value,6);
    engine.trigger('OnIntGraphicsPropertySet',10006,3);
    nativeModel.m_vecBoolProperties[0].m_bIsDisabled=true;
    menu.rows(requireModule,nativeModel);
    assert.equal(byId[10006].m_bIsDisabled,true); // Global RT capability/settings can disable the native control.
    nativeModel.m_vecBoolProperties[0].m_bIsDisabled=false;
    nativeModel.m_vecBoolProperties[0].m_value=false;
    menu.rows(requireModule,nativeModel);
    assert.equal(byId[10006].m_bIsDisabled,true);
    assert.equal(menu.slider(byId[10006]).m_value,0);
    assert.equal(byId[10006].m_value,6); // Keep the requested count while inactive.
    nativeModel.m_vecBoolProperties[0].m_value=true;
    menu.rows(requireModule,nativeModel);
    assert.equal(menu.slider(byId[10006]).m_value,3);
    engine.trigger('OnIntGraphicsPropertySet',10005,76);
    assert(!byId[10005]);
    if(!rtx40){
        engine.trigger('OnEnumGraphicsPropertySet',10000,1);
        engine.trigger('OnEnumGraphicsPropertySet',10001,1);
        engine.trigger('OnIntGraphicsPropertySet',10002,240);
        assert.equal(byId[10002].m_value,240);
    }
    engine.trigger('OnGraphicsReset');
    assert.equal(byId[10003].m_value,0);
    assert.equal(byId[10007].m_value,1);assert.equal(byId[10001].m_value,0);
    engine.trigger('OnEnumGraphicsPropertySet',14,3);
    assert.deepEqual(events.at(-1),['OnEnumGraphicsPropertySet',14,3]);
    engine.trigger('OnBoolGraphicsPropertySet',14,true);
    assert.deepEqual(events.at(-1),['OnBoolGraphicsPropertySet',14,true]); // Native checkboxes pass through unchanged.
    const names=['MENU_OPTIONS_WINDOW_MODE','VSYNC','MENU_OPTIONS_HDR','MENU_OPTIONS_FILM_GRAIN',
        'MENU_OPTIONS_QUALITY_PRESET','QUALITY_LAST','MENU_OPTIONS_RT_QUALITY_PRESET','RT_LAST'];
    const native=names.map((name,i)=>({m_iPropertyID:i,m_strNameLocID:name}));
    const ordered=menu.order(native.concat(rows));
    const position=name=>ordered.findIndex(row=>row.m_strNameLocID===name);
    assert(position('CFG_FG')>position('VSYNC') && position('CFG_FG')<position('MENU_OPTIONS_HDR'));
    assert(position('CFG_TEXTURE')>position('QUALITY_LAST') && position('CFG_TEXTURE')<position('MENU_OPTIONS_RT_QUALITY_PRESET'));
    ['CFG_RR','CFG_GI','CFG_GLASS'].forEach(name=>assert(position(name)>position('RT_LAST')));
    assert.equal(new Set(ordered).size,ordered.length);
    assert.deepEqual(ordered.filter(row=>!row.cfgDefinition),native);
    const storage=byId[10011];
    const button=menu.restartButton(storage);
    assert.equal(button.type,'./northlight/LinkComponents/EventButton/EventButton.tsx');
    assert.equal(button.props.needsConfirm,true);assert.equal(button.props.useHold,false);
    assert.equal(button.props.confirmLocId,'CFG_STORAGE_DISABLE');
    assert.equal(storage.m_value,1); // Creating/opening a native button cannot change the preference.
    const beforeRestart=events.length;
    handlers.CFG_STORAGE_RESTART_READY();assert.equal(events.length,beforeRestart); // Ignore unsolicited/duplicate ready.
    engine.reject=true;
    engine.trigger(button.props.event,...button.props.eventArgs);
    handlers.CFG_STORAGE_RESTART_FAILED();assert.equal(storage.m_value,1);
    assert.equal(storage.m_bIsDisabled,false);
    engine.reject=false;
    engine.trigger(button.props.event,...button.props.eventArgs);
    assert.equal(storage.m_value,0);assert.equal(storage.m_bIsDisabled,true);
    handlers.CFG_STORAGE_RESTART_READY();assert.equal(events.at(-1)[0],'OnExitClicked');
    const afterRestart=events.length;handlers.CFG_STORAGE_RESTART_READY();assert.equal(events.length,afterRestart);
    handlers.CFG_STORAGE_RESTART_FAILED(); // Simulate next view's non-busy state for rendering checks.
    const enable=menu.restartButton(storage);assert.equal(enable.props.confirmLocId,'CFG_STORAGE_ENABLE');
    engine.trigger('OnGraphicsReset');assert.equal(storage.m_value,0); // Reset cannot silently change restart-only preference.
    config.storageSupported=false;menu.rows(requireModule,nativeModel);assert.equal(storage.m_bIsDisabled,true);
    config.storageSupported=true;menu.rows(requireModule,nativeModel);
    engine.trigger('OnSaveOptions');assert.equal(events.at(-1)[0],'OnSaveOptions');
    return {context, requireModule, rows};
}
exercise(false);exercise(true);
// Optional local check against user-supplied assets; proprietary fixtures are not shipped.
if(process.argv[2] && process.argv[2] !== '--config') {
    const {context,requireModule,rows}=exercise(false);
    vm.runInContext(fs.readFileSync(process.argv[2],'utf8'),context);
    const modules=context.window.webpackJsonp[0][1];
    class Component {constructor(props){this.props=props;}}
    const react={Component,Fragment:'Fragment',createElement:(type,props,...children)=>({type,props,children})};
    const types={DataType:{BOOLEAN:0,INTEGER:1,FLOAT:2,ENUM:3,STRING:4,DISPLAY_MODE:5,KEY_BIND:6},OptionsEventType:{Bool:'Bool',Enum:'Enum',Int:'Int',Float:'Float',DisplayMode:'DisplayMode'}};
    const exports={};
    function req(name) {
        if(name.includes('react/index.js'))return react;
        if(name.includes('mobx-react'))return {observer:x=>x};
        if(name.includes('mobx/lib')||name.includes('OptionToDescriptionModeMap')||name==='northlight/vendor/coherent')return requireModule(name);
        if(name.includes('classnames'))return (...args)=>args.join(' ');
        if(name.endsWith('/types.ts'))return types;
        if(name.includes('ControlsHacks'))return {BannedPauseMenuOptions:[],DropdownSelectOnHover:[],BannedBindings:[]};
        if(name.includes('/Description.ts'))return {DescriptionMode:{TITLE_ONLY:2,NONE:23}};
        return {default:name};
    }
    modules['./northlight/LinkComponents/OptionsTab/OptionsTab.tsx']({},exports,req);
    const model={};['Bool','DisplayMode','Enum','Float','Int','Str','KeyBind'].forEach(t=>model['m_vec'+t+'Properties']=[]);
    const graphics=new exports.default({sectionType:'Graphics',modelProperties:model});
    assert.equal(graphics.renderOptions().length,9);
    const audio=new exports.default({sectionType:'Audio',modelProperties:model});
    assert.equal(audio.renderOptions().length,0);
    const rendered=graphics.renderOptions();
    assert.equal(rendered[0].props.label,'CFG_FG');
    assert.equal(rendered[0].props.valueDataType,3);
    const toggle=rendered.find(element=>element.props.eventType==='Bool');
    assert.equal(toggle.props.property.m_strNameLocID,'CFG_RR');
    assert.equal(rendered[4].props.max,6); // GI choice slider.
    const storageButton=rendered.find(e=>e.props.eventArgs&&e.props.eventArgs[0]===10011);
    assert.equal(storageButton.props.needsConfirm,true);
    const textureSlider=rendered.find(e=>e.props.propertyId===10008);
    assert.equal(textureSlider.props.max,5);
    assert.deepEqual(Array.from(textureSlider.props.strValues),['Automatic','4 ms','6 ms','8 ms','10 ms','12 ms']);

}
console.log('PASS native menu labels, choices, events, reset, capability filtering and native component rendering');
