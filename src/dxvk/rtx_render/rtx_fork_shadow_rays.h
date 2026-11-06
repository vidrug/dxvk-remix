#pragma once

// Cheaper visibility (shadow/occlusion) rays through alpha-tested geometry.
//
// Two independent switches, both read by rtx/algorithm/visibility.slangh:
// - firstHitPrimaryNee: the primary-surface NEE shadow ray (evalNEEPrimary) stops at the first confirmed
//   opaque hit instead of searching for the closest one, as every other visibility query does.
// - coarseAlphaMip: cutout (alpha-test) reads on any visibility ray use a fixed coarse mip instead of the
//   ray-cone footprint (which is zero, i.e. mip 0, on RTXDI's visibility rays).

#include <cstdint>

#include "rtx_option.h"

struct RaytraceArgs;

namespace dxvk {

  class ShadowRays {
  public:
    RTX_OPTION("rtx.shadowRays", bool, firstHitPrimaryNee, false,
               "Primary-surface shadow rays (direct lighting, with RTXDI or LightBVH) stop at the first confirmed "
               "opaque hit instead of searching for the closest one, which is cheaper. Visibility and lighting are the "
               "same. Only the hit distance of occluded samples changes, which only NRD reads, so this is ignored while "
               "NRD is the denoiser.");
    RTX_OPTION("rtx.shadowRays", bool, firstHitSubsurface, true,
               "The shadow rays of the diffusion-profile subsurface path (the profile sample, the transmission boundary "
               "term and the single-scattering sample in rtxcr_material.slangh) stop at the first confirmed opaque hit "
               "instead of searching for the closest one. Nothing reads their hit distance, so visibility, lighting and "
               "the image are the same; off only for comparing GPU time.");
    RTX_OPTION("rtx.shadowRays", bool, directLeanPath, true,
               "In the direct lighting shader that has diffusion-profile subsurface scattering compiled in, the pixels "
               "without it evaluate the material for all their lights before the first shadow ray, so the material does not "
               "stay in registers (or spill to memory) across the ray queries. The results are the same; off only for "
               "comparing GPU time, and it is what lets rtx.shadowRays.contributionCutoff act in that shader.");
    RTX_OPTION("rtx.shadowRays", bool, indirectCutoutOpacityFirst, true,
               "An indirect ray that hits a cutout tests the opacity first and goes on through a transparent texel without "
               "building the whole material. Only for materials whose opacity depends on nothing else. The image is the same.");
    RTX_OPTION("rtx.shadowRays", bool, pomAfterSceneRay, true,
               "On surfaces with height maps, the height-map self-shadow march of a shadow ray runs after the ray through "
               "the scene instead of before it, so a ray the scene already blocks (most of them) does not march. The two "
               "results only multiply, so visibility, lighting and the image are the same. Shell pixels keep the old "
               "order, because their march moves the ray's origin. Ignored while NRD is the denoiser.");
    RTX_OPTION("rtx.shadowRays", float, contributionCutoff, 3e-5f,
               "A shadow ray is not traced when even a fully visible light could add less than this to the pixel, on the "
               "display-linear scale after exposure (1 = white, so 1 / 255 / 12.92 = 0.0003 is one 8-bit step in the "
               "blacks), and a ray stops as soon as its accumulated transmission brings it under this. This bounds what one "
               "ray can lose, and a pixel takes a few of them (a local light, the sun, the bounces): at the default that is "
               "well under one 8-bit step in the darkest pixels and far under one elsewhere. 0 skips only rays whose light "
               "adds exactly nothing, which is exact. Negative turns it off. Ignored while NRD is the denoiser, in the "
               "denoiser's reference mode, and for the shaders of the slimmest direct lighting level, which trace in a "
               "pass of their own.");
    RTX_OPTION("rtx.shadowRays", float, contributionCutoffSunRelative, 0.004f,
               "On top of rtx.shadowRays.contributionCutoff, on a pixel the sun or moon reaches, a local light's shadow "
               "ray is not traced when in every colour channel it could add less than this fraction of what the sun or moon "
               "adds there. 0.004 is about a quarter of an 8-bit step at mid brightness. 0 turns it off.");
    RTX_OPTION("rtx.shadowRays", float, contributionCutoffIndirectScale, 1.0f,
               "Scale of rtx.shadowRays.contributionCutoff for the shadow rays of indirect bounces, with SHaRC only. Their "
               "contribution is taken after the path throughput in the query pass, and before it in the cache update, whose "
               "result is multiplied by a throughput of at most 1 on average later. Negative turns it off for indirect rays.");
    RTX_OPTION("rtx.shadowRays", bool, coarseAlphaMip, false,
               "Alpha-tested (cutout) texture reads on visibility rays (RTXDI visibility, shadow rays in direct and "
               "indirect lighting, volumetric and gradient visibility) sample a fixed coarse mip "
               "(rtx.shadowRays.alphaMipTargetResolution) instead of the ray cone's mip. Saves GPU time through "
               "foliage, and shadowed leaf and grass edges may soften slightly. Triangles resolved by opacity micromaps "
               "need no read.");
    RTX_OPTION("rtx.shadowRays", uint32_t, alphaMipTargetResolution, 128,
               "With rtx.shadowRays.coarseAlphaMip: visibility alpha tests read the mip whose larger side has this many "
               "texels (a 1024 texture reads mip 3, a 256 texture mip 1, and textures at or below it mip 0). A coarser "
               "ray-cone mip is kept. Lower values are cheaper and give softer cutout shadows. 4 to 4096.");

    static void setRaytraceArgs(RaytraceArgs& args, bool nrdDenoises);
    static void showImguiSettings();
  };

} // namespace dxvk
