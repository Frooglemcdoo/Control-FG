// Exercise the live combination present in the R504 logs: the private Both
// texture is bound on every frame, and unfiltered character history is valid.
static void TestBoundSource(bool r504){
    Device gpu(false);
    ComPtr<ID3DBlob> bytes,error;Check(RRCharacterSerializeRoot(&bytes,&error));
    ComPtr<ID3D12RootSignature> root;
    Check(gpu.device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&root)));
    auto code=Read(r504?"build/gi-character-r504.dxil":"build/gi-character-blend.dxil");
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};
    ComPtr<ID3D12PipelineState> pso;Check(gpu.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
    constexpr UINT w=40,h=12;
    constexpr DXGI_FORMAT f=DXGI_FORMAT_R32G32B32A32_FLOAT;
    const Pixel baseGI={.000468f,.000434f,.000351f,1},baseScene={.000102f,.000041f,.000024f,.75f};
    const Pixel flashGI={.153931f,.120056f,.074219f,1},flashScene={.036713f,.015060f,.004642f,.75f};
    auto a=Texture(gpu,w,h,DXGI_FORMAT_R32G32B32A32_UINT,nullptr,true);
    auto b=Texture(gpu,w,h,DXGI_FORMAT_R32G32B32A32_UINT,nullptr,true);
    constexpr auto read=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,write=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    gpu.Transition(b.Get(),write,read);
    Readback outputs[3];std::vector<Pixel> expected[3];
    for(UINT frame=0;frame<3;++frame){
        std::vector<Pixel> gi(w*h,baseGI),scene(w*h,baseScene),filtered(w*h,baseGI),
            depth(w*h,{.5f,0,0,0}),norm(w*h,{.5f,.5f,0,0}),mat(w*h),mask(w*h),
            motion(w*h,{frame==1?-2.f/w:0,0,0,0});
        const UINT dx=frame?2:0;
        for(UINT y=2;y<=8;++y)for(UINT x=2+dx;x<=30+dx;++x)mask[y*w+x]=TestMask(.5f,1);
        auto lit=[&](UINT x,UINT y){auto i=y*w+x+dx;gi[i]=flashGI;scene[i]=flashScene;};
        // Fixed surface features translate with the character.
        for(UINT y=4;y<=5;++y)for(UINT x=4;x<=5;++x)lit(x,y);
        lit(10,5);
        // Seed the private signal with the native value for the isolated feature.
        if(frame==0)filtered[5*w+10]=flashGI;
        auto orange=5*w+16+dx, green=5*w+22+dx, mixed=5*w+28+dx;
        if(frame==1) {
            lit(16,5);filtered[orange]=flashGI; // The private source retained the flash.
            gi[green][1]=.14f;scene[green][1]=.032f;filtered[green]=gi[green];
            // Green is direct light, uncorrelated with the red GI spike.
            gi[mixed][0]=.15f;scene[mixed][0]=.037f;scene[mixed][1]=.040f;
        }
        lit(34,5); // Outside the character; lighting, mask and source are independent.
        expected[frame]=scene;
        std::vector<UINT> selectors((w/2)*(h/2),0x03020100u);
        auto map=Texture(gpu,w/2,h/2,DXGI_FORMAT_R32_UINT,selectors.data());
        const UINT off=12*frame;TextureView(gpu,map.Get(),off+10);
        auto source=Texture(gpu,w,h,f,filtered.data());TextureView(gpu,source.Get(),off+11);
        std::vector<Pixel>* inputs[]={&gi,&mask,&depth,&norm,&mat,&scene,&scene,&motion};
        ComPtr<ID3D12Resource> target;
        for(UINT i=0;i<8;++i){auto t=Texture(gpu,w,h,f,inputs[i]->data(),i==6);TextureView(gpu,t.Get(),off+i,i==6);if(i==6)target=t;}
        auto* prev=frame==1?a.Get():b.Get();auto* next=frame==1?b.Get():a.Get();
        if(frame)gpu.Transition(next,read,write);
        TextureView(gpu,prev,off+8);TextureView(gpu,next,off+9,true);
        gpu.list->SetComputeRootSignature(root.Get());gpu.list->SetPipelineState(pso.Get());
        auto table=gpu.views->GetGPUDescriptorHandleForHeapStart();table.ptr+=UINT64(off)*gpu.descriptorSize;
        gpu.list->SetComputeRootDescriptorTable(0,table);
        RRCharacterHistoryParams params{float(w),float(h),0,0,frame?2u:1u,1};
        gpu.list->SetComputeRoot32BitConstants(2,6,&params,0);
        gpu.list->Dispatch((w+7)/8,(h+7)/8,1);
        gpu.Transition(next,write,read);outputs[frame]=CopyOutput(gpu,target.Get());
    }
    gpu.Submit();
    for(UINT frame=0;frame<3;++frame){
        unsigned char* p=nullptr;Check(outputs[frame].buffer->Map(0,nullptr,reinterpret_cast<void**>(&p)));
        UINT dx=frame?2:0;
        for(UINT i=0;i<w*h;++i){
            Pixel actual{};std::memcpy(&actual,p+(i/w)*outputs[frame].rowPitch+(i%w)*sizeof(Pixel),sizeof(Pixel));
            const UINT x=i%w,y=i/w;
            const bool patch=x>=4+dx && x<=5+dx && y>=4 && y<=5;
            const bool repeated=x==10+dx && y==5;
            const bool flash=frame==1 && x==16+dx && y==5;
            const bool green=frame==1 && x==22+dx && y==5;
            if(frame==1 && ((x==4+dx && y==5) || repeated || flash))
                std::cout<<(r504?"R504":"R505")<<" source-bound "<<(flash?"flash":repeated?"persistent point":"patch")
                         <<" red "<<expected[frame][i][0]<<" -> "<<actual[0]<<"\n";
            if(r504 && (patch || (frame && repeated))){
                Require(actual[0]<.01f,"R504 fixture must reproduce darkening of persistent detail");
            } else if(!r504 && (flash || green)){
                for(UINT k=0;k<3;++k) {
                    if(green && k!=1)Require(actual[k]==baseScene[k],"Independent color changed");
                    else Require(actual[k]>=0 && actual[k]<expected[frame][i][k]*.03f,"Retained GI flash escaped temporal correction");
                }
            }else Require(actual==expected[frame][i],"Unexpected edit to stable color, independent light, environment, or trailing frame");
            Require(actual[3]==expected[frame][i][3],"Character alpha");
        }
        D3D12_RANGE empty{};outputs[frame].buffer->Unmap(0,&empty);
    }
}
