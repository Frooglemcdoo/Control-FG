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
    const config = {presetE:5,presetF:6,rtx40,
        values:[4,0,144,0,6,60,4,1,0,1,0],defaults:[4,0,144,0,6,60,4,1,0,1,0],
        fgChoices:rtx40?[0,2]:[0,1,2,3,4,5,6]};
    if(rtx40) config.values[0]=config.defaults[0]=2;
    const liveValues=config.values.slice();
    config.values[0]=0; // Reopened view still receives the cached startup snapshot.
    const context={window:{__controlFGConfig:config,__controlFGSchema:schema},Number};
    vm.createContext(context);vm.runInContext(script,context);
    const events=[], handlers={};
    const engine={on(name,fn){handlers[name]=fn;},trigger(...args){
        events.push(args);
        if(args[1]===10999) {
            handlers.CFG_STATE_BEGIN();
            liveValues.forEach((v,i)=>{for(let b=0;b<10;++b)if(v&(1<<b))handlers['CFG_STATE_'+i+'_'+b]();});
            handlers.CFG_STATE_END();
        }
        else if(args[0]==='OnGraphicsReset')Object.keys(handlers).filter(k=>k.startsWith('CFG_OK_')).forEach(k=>handlers[k]());
        else if(args[1]>=10000&&args[1]<=10010) {
            const key=(engine.reject?'CFG_FAIL_':'CFG_OK_')+args[1];
            if(handlers[key])handlers[key]();
        }
    }};
    const maps={default:{},NO_WARNING_WHEN_DISABLED:{}};
    const requireModule=name=>name.includes('mobx/lib')?{observable:{array:x=>x},runInAction:f=>f()}:
        name.includes('OptionToDescriptionModeMap')?maps:engine;
    const menu=context.window.__controlFG, rows=menu.rows(requireModule);
    assert.equal(rows.length,rtx40?11:10);
    assert.equal(rows[0].m_value,liveValues[0]); // Live FG survives view recreation.
    assert.equal(rows[0].cfgHeader,'CFG_FG_HEADER');
    assert.equal(menu.translate('CFG_GLASS'),'Enhanced Glass Reflections');
    assert(!rows.some(r=>r.m_strNameLocID.includes('OPAQUE')));
    assert.equal(menu.rows(requireModule),rows);
    assert.equal(events.length,1); // One handshake, no duplicate event wrapping.
    rows.forEach(r=>{
        assert(menu.translate(r.m_strNameLocID));
        assert(menu.translate(r.m_strNameLocID+'_INFO'));
        assert.equal(maps.default[r.m_strNameLocID],2);
        r.m_vecPossibleValues.forEach(v=>assert(menu.translate(v.m_strLabelLocID)));
    });
    engine.trigger('OnEnumGraphicsPropertySet',10000,99);
    assert.equal(events.length,1);
    engine.trigger('OnEnumGraphicsPropertySet',10003,1);
    assert.equal(rows[3].m_value,1);assert.equal(rows[5].m_bIsDisabled,false);
    engine.trigger('OnIntGraphicsPropertySet',10005,75);
    assert.equal(rows[5].m_value,75);
    engine.reject=true;
    engine.trigger('OnEnumGraphicsPropertySet',10008,6);
    assert.equal(rows[8].m_value,0); // Rejected native writes roll back the displayed value.
    engine.reject=false;
    engine.trigger('OnIntGraphicsPropertySet',10005,76);
    assert.equal(rows[5].m_value,75);
    if(!rtx40){
        engine.trigger('OnEnumGraphicsPropertySet',10000,1);
        engine.trigger('OnEnumGraphicsPropertySet',10001,1);
        engine.trigger('OnIntGraphicsPropertySet',10002,240);
        assert.equal(rows[2].m_value,240);
    }
    engine.trigger('OnGraphicsReset');
    assert.equal(rows[3].m_value,0);assert.equal(rows[5].m_value,60);
    assert.equal(rows[7].m_value,1);assert.equal(rows[1].m_value,0);
    engine.trigger('OnEnumGraphicsPropertySet',14,3);
    assert.deepEqual(events.at(-1),['OnEnumGraphicsPropertySet',14,3]);
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
    const types={DataType:{BOOLEAN:0,INTEGER:1,FLOAT:2,ENUM:3,STRING:4,DISPLAY_MODE:5,KEY_BIND:6},OptionsEventType:{Enum:'Enum',Int:'Int',Float:'Float',DisplayMode:'DisplayMode'}};
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
    assert.equal(graphics.renderOptions().length,10);
    const audio=new exports.default({sectionType:'Audio',modelProperties:model});
    assert.equal(audio.renderOptions().length,0);
    const rendered=graphics.renderOptions();
    assert.equal(rendered[0].children[0].children[0].props.className,'options-tab__group-header');
    assert.equal(rendered[0].children[1].props.label,'CFG_FG');
    assert.equal(rendered[0].children[0].props.style.position,'relative');
    assert.equal(rendered[0].children[0].props.style.zIndex,0); // Contain native header text's z-index:999 below dropdowns.
}
console.log('PASS native menu labels, choices, events, reset, capability filtering and native component rendering');
