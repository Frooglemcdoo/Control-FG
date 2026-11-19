#ifndef CONTROL_SCENE_CAPTURE_GRID
#define CONTROL_SCENE_CAPTURE_GRID
#ifdef __cplusplus
#include <cstdint>
namespace gi_reuse {using SceneUInt=std::uint32_t;
#define SCENE_INLINE inline
#else
#define SceneUInt uint
#define SCENE_INLINE
#endif
SCENE_INLINE SceneUInt GISceneCoordinate(SceneUInt cell,SceneUInt extent,SceneUInt count){return cell*extent/count;}
SCENE_INLINE SceneUInt GISceneCell(SceneUInt pixel,SceneUInt extent,SceneUInt count){
 if(!extent||!count||pixel>=extent||count>extent)return 0xffffffffu;
 SceneUInt cell=(pixel*count+extent-1)/extent;
 return cell<count&&GISceneCoordinate(cell,extent,count)==pixel?cell:0xffffffffu;
}
#ifdef __cplusplus
}
#else
#undef SceneUInt
#endif
#undef SCENE_INLINE
#endif
