// Capture tail: existing pixel summaries, producer packets (rounded to 32B),
// then two training-sized records per pixel for its strongest red contribution.
uint GIRedIndex(uint auditIndex,uint rays){
 uint pixels=min(trainingWidth,512u)*min(trainingHeight,512u);
 return 2*trainingCapacity+2*pixels+(pixels*(rays+1)+1)/2+auditIndex-2*trainingCapacity;
}
void GIRedWrite(uint index,GIAutoTraining a,GIAutoTraining b,float scale,uint rays){
 if(index==0xffffffff)return;b.reserved1=asuint(scale);
 uint base=GIRedIndex(index,rays);autoTraining[base]=a;autoTraining[base+1]=b;
}
