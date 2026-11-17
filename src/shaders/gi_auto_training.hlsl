#ifndef CONTROL_GI_AUTO_TRAINING_H
#define CONTROL_GI_AUTO_TRAINING_H
// One randomly chosen receiver per tile, strongest of all ten rays. Fixed-address records avoid
// atomics/counter overflow. Every tile writes an invalid record before shading,
// so invalid receivers and misses cannot preserve a previous frame's training.
RWStructuredBuffer<GIAutoTraining> autoTraining:register(u0,space19);
cbuffer TrainingLimits:register(b1,space19) {uint trainingWidth,trainingHeight,trainingStep,trainingCapacity;}
uint GIAutoTrainingIndex(uint2 pixel,uint frame) {
 if(trainingStep<8||trainingStep>256||trainingWidth==0||trainingHeight==0||
    trainingCapacity==0||trainingCapacity>65535||any(pixel>=uint2(trainingWidth,trainingHeight)))return 0xffffffff;
 uint2 tile=pixel/trainingStep,start=tile*trainingStep;
 uint2 size=min(uint2(trainingStep,trainingStep),uint2(trainingWidth,trainingHeight)-start);
 uint hash=GIProducerHash(tile.x*73856093u^tile.y*19349663u^frame*83492791u^0xe473a9c5u);
 uint2 chosen=start+uint2(hash%size.x,(hash>>16)%size.y);
 uint index=tile.y*((trainingWidth+trainingStep-1)/trainingStep)+tile.x;
 return all(pixel==chosen)&&index<trainingCapacity?index:0xffffffff;
}
// Proposal discovery scores raw brightness, not weighted final radiance. This
// avoids making a learned bright region disappear merely because p/q falls.
GIAutoTraining GIAutoCandidate(uint version,uint material,float3 position,float3 rgb,float weight) {
 GIAutoTraining record=(GIAutoTraining)0;record.version=version;
 float energy=dot(rgb,float3(.2126,.7152,.0722));
 if(material<65534&&version!=0&&all(isfinite(position))&&all(isfinite(rgb))&&all(rgb>=0)&&
    isfinite(weight)&&weight>0&&isfinite(energy)&&energy>0){
  record.position.x=position.x;record.position.y=position.y;record.position.z=position.z;
  record.energy=energy;record.valid=1;
 }
 return record;
}
GIAutoTraining GIAutoKeepBrightest(GIAutoTraining best,GIAutoTraining candidate) {
 if(candidate.valid&&candidate.energy>best.energy)return candidate;return best;
}
#endif
